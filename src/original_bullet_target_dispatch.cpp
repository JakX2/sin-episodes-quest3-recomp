#include "sin_compat/original_bullet_target_dispatch.hpp"

namespace sin_compat::combat {
namespace {

bool resolve_routes(BulletTargetClass target_class,
                    DamageFilterRoute& filter_route,
                    TraceAttackRoute& trace_route) noexcept {
    switch (target_class) {
    case BulletTargetClass::BaseEntity:
    case BulletTargetClass::BaseAnimating:
    case BulletTargetClass::BaseCombatCharacter:
        filter_route = DamageFilterRoute::BaseEntity;
        trace_route = TraceAttackRoute::BaseEntity;
        return true;
    case BulletTargetClass::BasePlayer:
        filter_route = DamageFilterRoute::BaseEntity;
        trace_route = TraceAttackRoute::BasePlayer;
        return true;
    case BulletTargetClass::Hl2Player:
        filter_route = DamageFilterRoute::Hl2PlayerOverride;
        trace_route = TraceAttackRoute::BasePlayer;
        return true;
    case BulletTargetClass::PhysicsProp:
    case BulletTargetClass::PropContainer:
        filter_route = DamageFilterRoute::BaseEntity;
        trace_route = TraceAttackRoute::PhysicsPropOrContainerCopyThenBase;
        return true;
    case BulletTargetClass::Unknown:
        return false;
    }
    return false;
}

} // namespace

BulletTargetDispatchResult dispatch_original_bullet_target_hit(
    IOriginalBulletTargetHost& host,
    EntityRef target,
    BulletTargetClass target_class,
    const DamageInfoView& damage) {
    BulletTargetDispatchResult result;
    if (!host.is_live(target)) {
        result.status = BulletTargetDispatchStatus::StaleTarget;
        return result;
    }

    if (!resolve_routes(target_class, result.filter_route, result.trace_route)) {
        result.status = BulletTargetDispatchStatus::UnsupportedTargetClass;
        return result;
    }

    // CHL2_Player has its own exact-value rejection before falling back to
    // CBaseEntity. This is equality, not a bit test: DUAG is 0x00400002.
    if (target_class == BulletTargetClass::Hl2Player &&
        damage.damage_type == kBaseDamageFilterRejectExactTypeA) {
        result.rejection = DamageFilterRejection::Hl2PlayerExactTypeA;
        result.status = BulletTargetDispatchStatus::RejectedByExactDamageType;
        return result;
    }

    // CBaseEntity::PassesDamageFilter rejects these two complete 32-bit values.
    // Do not turn either comparison into a mask test.
    if (damage.damage_type == kBaseDamageFilterRejectExactTypeA) {
        result.rejection = DamageFilterRejection::BaseEntityExactTypeA;
        result.status = BulletTargetDispatchStatus::RejectedByExactDamageType;
        return result;
    }
    if (damage.damage_type == kBaseDamageFilterRejectExactTypeB) {
        result.rejection = DamageFilterRejection::BaseEntityExactTypeB;
        result.status = BulletTargetDispatchStatus::RejectedByExactDamageType;
        return result;
    }

    EntityRef filter{};
    const DamageFilterLookup lookup = host.resolve_damage_filter(target, filter);
    if (lookup == DamageFilterLookup::ReadFailed) {
        result.status = BulletTargetDispatchStatus::FilterLookupFailed;
        return result;
    }

    if (lookup == DamageFilterLookup::LiveFilter && host.is_live(filter)) {
        result.filter_entity = filter;
        result.filter_invoked = true;
        bool passes = false;
        if (!host.invoke_damage_filter(filter, target, damage, passes)) {
            result.status = BulletTargetDispatchStatus::FilterInvocationFailed;
            return result;
        }
        if (!passes) {
            result.rejection = DamageFilterRejection::GameFilter;
            result.status = BulletTargetDispatchStatus::RejectedByDamageFilter;
            return result;
        }
    }
    // A handle that became stale between lookup and dispatch resolves as no
    // filter, matching the original handle-helper null path.

    result.trace_attack_invoked = true;
    if (!host.trace_attack(target, result.trace_route, damage)) {
        result.status = BulletTargetDispatchStatus::TraceAttackFailed;
        return result;
    }

    result.status = BulletTargetDispatchStatus::TraceAttackDispatched;
    return result;
}

} // namespace sin_compat::combat
