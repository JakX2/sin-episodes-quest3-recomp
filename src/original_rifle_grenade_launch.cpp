#include "sin_compat/original_rifle_grenade_launch.hpp"
#include "sin_compat/original_assault_rifle_profile.hpp"

#include <cmath>
#include <limits>

namespace sin_compat::ar_grenade_launch {

namespace {
struct ResetBusy final {
    bool& flag;
    ~ResetBusy() { flag = false; }
};

bool valid_ref(EntityRef value) noexcept {
    return value.index != std::numeric_limits<std::uint32_t>::max() &&
           value.generation != std::numeric_limits<std::uint32_t>::max();
}

bool valid_vector(const ar_grenade::Vec3& v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

bool scaled_velocity(const ar_grenade::Vec3& direction,
                     float multiplier,
                     ar_grenade::Vec3& out) noexcept {
    if (!valid_vector(direction)) return false;
    out = {
        direction.x * multiplier,
        direction.y * multiplier,
        direction.z * multiplier
    };
    return valid_vector(out);
}

LaunchResult failure(LaunchCode code) noexcept {
    return {code, {}, 0.0f, 0};
}
} // namespace

LaunchResult Launcher::fire_player(WeaponSession& session,
                                   ILaunchHost& host,
                                   const LaunchInput& input,
                                   ar_grenade::Runtime& out) {
    if (busy_) return failure(LaunchCode::RecursiveLaunch);
    busy_ = true;
    const ResetBusy reset{busy_};

    if (out.spawned()) return failure(LaunchCode::ProjectileAlreadyBound);
    if (session.runtime().kind() != WeaponKind::AssaultRifle ||
        session.owner_ref() != input.owner ||
        session.weapon_ref() != input.weapon ||
        !valid_ref(input.owner) || !valid_ref(input.weapon))
        return failure(LaunchCode::WrongWeaponOrOwner);

    if (!session.can_launch_rifle_grenade())
        return failure(LaunchCode::WeaponNotSelectedOrReady);

    // This additional host-side validation protects against accidental
    // mismatch between an input's identity and the selected session.
    if (!host.is_live(input.owner) || !host.is_live(input.weapon) ||
        host.owner_of(input.weapon) != input.owner ||
        host.owner_kind(input.owner) != OwnerKind::Player)
        return failure(LaunchCode::OwnerNotAuthorized);

    const AmmoSnapshot ammo =
        host.player_secondary_ammo(input.owner, original_secondary_ammo_name);
    if (ammo.kind != AmmoKind::Finite)
        return failure(LaunchCode::AmmoSemanticsUnverified);
    if (ammo.remaining < original_player_secondary_debit)
        return failure(LaunchCode::NoSecondaryAmmo);

    return commit(host, input, out, original_player_direction_multiplier,
                  original_player_secondary_debit);
}

LaunchResult Launcher::fire_grunt(ILaunchHost& host,
                                  const LaunchInput& input,
                                  ar_grenade::Runtime& out) {
    if (busy_) return failure(LaunchCode::RecursiveLaunch);
    busy_ = true;
    const ResetBusy reset{busy_};

    if (out.spawned()) return failure(LaunchCode::ProjectileAlreadyBound);
    if (!valid_ref(input.owner) || !valid_ref(input.weapon) ||
        !host.is_live(input.owner) || !host.is_live(input.weapon) ||
        host.owner_of(input.weapon) != input.owner ||
        host.owner_kind(input.owner) != OwnerKind::GruntBase)
        return failure(LaunchCode::WrongWeaponOrOwner);

    if (!host.approve_grunt_launch(input.owner, input.weapon))
        return failure(LaunchCode::OwnerNotAuthorized);

    // Original NPC path bypasses this PARTICULAR player-ammo subtraction.
    // Any remaining NPC AI ammo/cooldown checks are controlled by the host;
    // this is not evidence of universally infinite NPC grenade supply.
    return commit(host, input, out, original_grunt_direction_multiplier, 0);
}

LaunchResult Launcher::commit(ILaunchHost& host, const LaunchInput& input,
                              ar_grenade::Runtime& out, float multiplier,
                              std::int32_t ammo_debit) {
    ar_grenade::Vec3 velocity{};
    if (!valid_vector(input.origin) ||
        !scaled_velocity(input.direction, multiplier, velocity))
        return failure(LaunchCode::InvalidPhysicsInput);

    // Make the eventual projectile's first think schedule and event parameters
    // valid BEFORE asking the engine to allocate a real entity.
    ar_grenade::Runtime candidate{};
    if (!candidate.spawn(input.origin, velocity, input.game_time_seconds))
        return failure(LaunchCode::InvalidPhysicsInput);

    const EntityRef staged = host.prepare_projectile(
        input.owner, OriginalAssaultRifle::secondary_projectile_classname,
        OriginalAssaultRifle::secondary_projectile_model,
        input.origin, velocity);

    if (!valid_ref(staged))
        return failure(LaunchCode::EntityPreparationFailed);

    // The normal original player ammo path subtracts exactly one *after*
    // entity creation. This atomic host operation avoids publishing an entity
    // without charging ammo, or charging ammo for a failed spawn.
    if (!host.commit_projectile(staged, input.owner, ammo_debit)) {
        host.abort_prepared_projectile(staged);
        return failure(LaunchCode::TransactionFailed);
    }

    out = candidate;
    return {LaunchCode::Launched, staged, multiplier, ammo_debit};
}

} // namespace sin_compat::ar_grenade_launch
