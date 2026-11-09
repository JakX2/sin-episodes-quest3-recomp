#include "sin_compat/original_alive_damage.hpp"
#include "sin_compat/original_game_rules.hpp"
#include "sin_compat/original_grunt_mobility_rules.hpp"

#include <cmath>
#include <limits>

namespace sin_compat::combat {
namespace {

bool damage_to_i32_truncating(float damage, std::int32_t& result) noexcept {
    if (!std::isfinite(damage)) return false;
    const double truncated = std::trunc(static_cast<double>(damage));
    if (truncated < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
        truncated > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
        return false;
    }
    result = static_cast<std::int32_t>(truncated);
    return true;
}

} // namespace

DamageRoute verified_damage_route(DamageClass damage_class) noexcept {
    DamageRoute route;
    const auto push = [&route](DamageStage stage) {
        if (route.count < route.stages.size()) route.stages[route.count++] = stage;
    };
    switch (damage_class) {
    case DamageClass::CombatCharacter:
        push(DamageStage::CombatCharacterTakeDamage);
        push(DamageStage::CommonHealthReduction);
        break;
    case DamageClass::BasePlayer:
        push(DamageStage::BasePlayerTakeDamage);
        push(DamageStage::CombatCharacterTakeDamage);
        push(DamageStage::BasePlayerAliveDamage);
        push(DamageStage::CommonHealthReduction);
        break;
    case DamageClass::Hl2Player:
        push(DamageStage::Hl2PlayerTakeDamage);
        push(DamageStage::BasePlayerTakeDamage);
        push(DamageStage::CombatCharacterTakeDamage);
        push(DamageStage::BasePlayerAliveDamage);
        push(DamageStage::CommonHealthReduction);
        break;
    case DamageClass::AiBaseNpc:
        push(DamageStage::CombatCharacterTakeDamage);
        push(DamageStage::AiBaseNpcAliveDamage);
        push(DamageStage::CommonHealthReduction);
        break;
    case DamageClass::RecombinantMajor:
        push(DamageStage::CombatCharacterTakeDamage);
        push(DamageStage::RecombinantMajorAliveDamage);
        push(DamageStage::MutantBaseAliveDamage);
        push(DamageStage::AiBaseNpcAliveDamage);
        push(DamageStage::CommonHealthReduction);
        break;
    case DamageClass::RecombinantMinor:
        push(DamageStage::CombatCharacterTakeDamage);
        push(DamageStage::RecombinantMinorAliveDamage);
        push(DamageStage::MutantBaseAliveDamage);
        push(DamageStage::AiBaseNpcAliveDamage);
        push(DamageStage::CommonHealthReduction);
        break;
    case DamageClass::Unknown:
        break;
    }
    return route;
}

DamageResult apply_common_alive_damage(IOriginalCombatDamageHost& host,
                                       EntityRef victim,
                                       float damage,
                                       DamageClass damage_class) {
    DamageResult result;
    result.route = verified_damage_route(damage_class);
    if (result.route.count == 0) {
        result.status = DamageStatus::UnsupportedClass;
        return result;
    }
    if (!host.is_live(victim)) {
        result.status = DamageStatus::StaleVictim;
        return result;
    }
    if (!damage_to_i32_truncating(damage, result.integer_damage)) {
        result.status = DamageStatus::InvalidDamage;
        return result;
    }

    std::uint8_t life_state = 0;
    if (!host.read_health_and_life_state(victim, result.old_health, life_state)) {
        result.status = DamageStatus::ReadFailed;
        return result;
    }
    if (life_state != 0) {
        result.new_health = result.old_health;
        result.status = DamageStatus::AlreadyDead;
        return result;
    }

    const auto next_health = static_cast<std::int64_t>(result.old_health) -
                             static_cast<std::int64_t>(result.integer_damage);
    if (next_health < std::numeric_limits<std::int32_t>::min() ||
        next_health > std::numeric_limits<std::int32_t>::max()) {
        result.status = DamageStatus::HealthOverflow;
        return result;
    }
    result.new_health = static_cast<std::int32_t>(next_health);
    if (!host.write_health(victim, result.new_health)) {
        result.status = DamageStatus::WriteFailed;
        result.new_health = result.old_health;
        return result;
    }

    // The original combat-character path tests the updated integer HP and
    // dispatches the subclass death virtual only when the result is <= 0.
    if (result.new_health <= 0) {
        result.killed_dispatch_attempted = true;
        result.killed_dispatch_succeeded = host.dispatch_killed(victim, damage_class);
        if (!result.killed_dispatch_succeeded) {
            result.status = DamageStatus::KilledDispatchFailed;
            return result;
        }
    }
    result.status = DamageStatus::Applied;
    return result;
}

} // namespace sin_compat::combat
