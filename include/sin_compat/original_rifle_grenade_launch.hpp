#pragma once

// Independent, engine-neutral secondary-fire integration from the original
// 2006 SiN Episodes server.dll CWeaponAssaultRifle to CGrenadeAR.
//
// Evidence is preserved in research/phases/sin-original-ar-grenade-lifecycle-
// and-ammo.md and sin-original-2006-assault-rifle-grenade-creation.md:
//   * secondary grenade creation calls CreateEntityByName("grenade_ar")
//   * the player path consumes ONE finite/valid "ARNADE" secondary ammo
//     after entering entity creation
//   * CBasePlayer and CNPC_GruntBase launch branches use direction-vector
//     scalar multipliers 2000.0f and 1000.0f, respectively
//   * the grunt branch bypasses this particular player ammo debit
// The owner-derived transform, complete collision/velocity setup, NPC ammo
// economy, action cooldowns and Source engine-side effects are UNVERIFIED.
// The caller must supply an original-verified direction and authorization.
//
// This module is not an IServerGameDLL, SourceVR engine entity, VR APK,
// networked grenade or proof of an original x86-to-AArch64 ABI conversion.
#include "sin_compat/original_ar_grenade_runtime.hpp"
#include "sin_compat/weapon_session.hpp"

#include <cstdint>
#include <string_view>

namespace sin_compat::ar_grenade_launch {

enum class OwnerKind : std::uint8_t { Unknown, Player, GruntBase };
enum class AmmoKind : std::uint8_t { InvalidIndex, Finite, InfiniteOrUnknown };

struct AmmoSnapshot {
    AmmoKind kind = AmmoKind::InvalidIndex;
    std::int32_t remaining = 0;
};

struct LaunchInput {
    EntityRef owner{};
    EntityRef weapon{};
    ar_grenade::Vec3 origin{};
    // Already calculated host-space direction from a validated original
    // firing/aiming path. Do not invent muzzle offsets or normalization.
    ar_grenade::Vec3 direction{};
    float game_time_seconds = 0.0f;
};

enum class LaunchCode : std::uint8_t {
    Launched,
    RecursiveLaunch,
    ProjectileAlreadyBound,
    WrongWeaponOrOwner,
    WeaponNotSelectedOrReady,
    OwnerNotAuthorized,
    AmmoSemanticsUnverified,
    NoSecondaryAmmo,
    InvalidPhysicsInput,
    EntityPreparationFailed,
    TransactionFailed
};

struct LaunchResult {
    LaunchCode code = LaunchCode::WrongWeaponOrOwner;
    EntityRef projectile{};
    float direction_multiplier = 0.0f;
    std::int32_t player_ammo_debit = 0;
    bool ok() const noexcept { return code == LaunchCode::Launched; }
};

inline constexpr float original_player_direction_multiplier = 2000.0f;
inline constexpr float original_grunt_direction_multiplier = 1000.0f;
inline constexpr std::int32_t original_player_secondary_debit = 1;
inline constexpr std::string_view original_secondary_ammo_name = "ARNADE";

// A real game server supplies generation-safe entity references and owns the
// authoritative ammo ledger. A prepared projectile must be UNPUBLISHED until
// commit succeeds, and commit must atomically publish it and subtract any
// requested ammo. On failed commit no ammo is spent and abort removes the
// prepared entity. This all-or-nothing requirement is an integration-safety
// policy, NOT an assertion of identical 2006 transaction internals.
class ILaunchHost {
public:
    virtual ~ILaunchHost() = default;
    virtual bool is_live(EntityRef) const noexcept = 0;
    virtual EntityRef owner_of(EntityRef weapon) const noexcept = 0;
    virtual OwnerKind owner_kind(EntityRef) const noexcept = 0;

    // Only authoritative finite/valid player-side ammo is supported here.
    // We do NOT guess what the old invalid/infinite index paths did.
    virtual AmmoSnapshot player_secondary_ammo(EntityRef,
                                               std::string_view) const noexcept = 0;

    // NPC ammo rules are not known. Only a host whose real NPC game rules
    // authorize this specific shot may return true. No player ammo debited.
    virtual bool approve_grunt_launch(EntityRef, EntityRef) const noexcept = 0;

    virtual EntityRef prepare_projectile(EntityRef owner,
                                         std::string_view classname,
                                         std::string_view model,
                                         const ar_grenade::Vec3& origin,
                                         const ar_grenade::Vec3& velocity) noexcept = 0;

    // The host must treat projectile creation and optional ammo debit as ONE
    // atomic publication transaction. False must mean no ammo was consumed.
    virtual bool commit_projectile(EntityRef projectile, EntityRef owner,
                                   std::int32_t player_ammo_debit) noexcept = 0;
    virtual void abort_prepared_projectile(EntityRef) noexcept = 0;
};

// One per weapon. Single-threaded like the existing host C ABI: callback
// reentry is rejected, including recursive attempts during entity creation.
class Launcher final {
public:
    // Reuses the existing generation-aware WeaponSession selection, holster,
    // and reload guards. Player ammo lives on the host's player entity,
    // NOT in the weapon's primary/clip2 bookkeeping.
    LaunchResult fire_player(WeaponSession& session, ILaunchHost& host,
                             const LaunchInput& input,
                             ar_grenade::Runtime& projectile_runtime);

    // Does not route NPCs through a player-owned WeaponSession. The host
    // must independently verify actual CNPC_GruntBase classification,
    // ownership and this shot's NPC-side authorization.
    LaunchResult fire_grunt(ILaunchHost& host, const LaunchInput& input,
                            ar_grenade::Runtime& projectile_runtime);

private:
    LaunchResult commit(ILaunchHost& host, const LaunchInput& input,
                        ar_grenade::Runtime& projectile_runtime,
                        float multiplier, std::int32_t ammo_debit);
    bool busy_ = false;
};

} // namespace sin_compat::ar_grenade_launch
