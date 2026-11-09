#include "sin_compat/original_bullet_target_dispatch.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

using namespace sin_compat;
using namespace sin_compat::combat;

namespace {

class Host final : public IOriginalBulletTargetHost {
public:
    EntityRef target{10, 1};
    std::optional<EntityRef> filter{};
    bool target_live = true;
    bool filter_live = true;
    bool lookup_ok = true;
    bool filter_call_ok = true;
    bool filter_passes = true;
    bool trace_ok = true;
    std::vector<std::string> calls;
    TraceAttackRoute last_route = TraceAttackRoute::BaseEntity;
    const void* last_payload = nullptr;

    bool is_live(EntityRef entity) const override {
        if (entity == target) return target_live;
        if (filter.has_value() && entity == *filter) return filter_live;
        return false;
    }

    DamageFilterLookup resolve_damage_filter(
        EntityRef entity, EntityRef& out) const override {
        assert(entity == target);
        if (!lookup_ok) return DamageFilterLookup::ReadFailed;
        if (!filter.has_value()) return DamageFilterLookup::NoFilter;
        out = *filter;
        return DamageFilterLookup::LiveFilter;
    }

    bool invoke_damage_filter(EntityRef filter_entity,
                              EntityRef target_entity,
                              const DamageInfoView& damage,
                              bool& passes) override {
        calls.emplace_back("filter");
        assert(filter.has_value() && filter_entity == *filter);
        assert(target_entity == target);
        last_payload = damage.host_payload;
        passes = filter_passes;
        return filter_call_ok;
    }

    bool trace_attack(EntityRef target_entity,
                      TraceAttackRoute route,
                      const DamageInfoView& damage) override {
        calls.emplace_back("trace");
        assert(target_entity == target);
        last_route = route;
        last_payload = damage.host_payload;
        return trace_ok;
    }
};

void test_no_filter_defaults_to_allow_and_dispatches_base_trace() {
    Host host;
    int payload = 7;
    const auto result = dispatch_original_bullet_target_hit(
        host, host.target, BulletTargetClass::BaseEntity, {0x00000002U, &payload});
    assert(result.status == BulletTargetDispatchStatus::TraceAttackDispatched);
    assert(!result.filter_invoked && result.trace_attack_invoked);
    assert(host.calls == std::vector<std::string>{"trace"});
    assert(host.last_route == TraceAttackRoute::BaseEntity);
    assert(host.last_payload == &payload);
}

void test_base_filter_rejects_exact_values_not_masks() {
    for (const std::uint32_t type : {0x00400000U, 0x00010000U}) {
        Host host;
        const auto result = dispatch_original_bullet_target_hit(
            host, host.target, BulletTargetClass::BaseAnimating, {type, nullptr});
        assert(result.status == BulletTargetDispatchStatus::RejectedByExactDamageType);
        assert(!result.filter_invoked && !result.trace_attack_invoked);
        assert(host.calls.empty());
    }
}

void test_duag_combined_damage_bits_are_not_rejected_by_exact_comparison() {
    Host host;
    host.filter = EntityRef{20, 4};
    const auto result = dispatch_original_bullet_target_hit(
        host, host.target, BulletTargetClass::BaseCombatCharacter,
        {0x00400002U, nullptr});
    assert(result.status == BulletTargetDispatchStatus::TraceAttackDispatched);
    assert(result.filter_invoked && result.trace_attack_invoked);
    assert(host.calls == (std::vector<std::string>{"filter", "trace"}));
}

void test_filter_rejection_suppresses_trace_attack() {
    Host host;
    host.filter = EntityRef{20, 4};
    host.filter_passes = false;
    const auto result = dispatch_original_bullet_target_hit(
        host, host.target, BulletTargetClass::BaseEntity, {0x00000002U, nullptr});
    assert(result.status == BulletTargetDispatchStatus::RejectedByDamageFilter);
    assert(result.rejection == DamageFilterRejection::GameFilter);
    assert(result.filter_invoked && !result.trace_attack_invoked);
    assert(host.calls == std::vector<std::string>{"filter"});
}

void test_hl2_player_exact_override_precedes_base_filter_lookup() {
    Host host;
    host.filter = EntityRef{20, 4};
    const auto result = dispatch_original_bullet_target_hit(
        host, host.target, BulletTargetClass::Hl2Player,
        {0x00400000U, nullptr});
    assert(result.status == BulletTargetDispatchStatus::RejectedByExactDamageType);
    assert(result.rejection == DamageFilterRejection::Hl2PlayerExactTypeA);
    assert(host.calls.empty());
}

void test_hl2_player_combined_duag_bits_fall_through_to_base_filter() {
    Host host;
    const auto result = dispatch_original_bullet_target_hit(
        host, host.target, BulletTargetClass::Hl2Player,
        {0x00400002U, nullptr});
    assert(result.status == BulletTargetDispatchStatus::TraceAttackDispatched);
    assert(result.filter_route == DamageFilterRoute::Hl2PlayerOverride);
    assert(result.trace_route == TraceAttackRoute::BasePlayer);
    assert(host.last_route == TraceAttackRoute::BasePlayer);
}

void test_base_player_uses_its_trace_attack_override() {
    Host host;
    const auto result = dispatch_original_bullet_target_hit(
        host, host.target, BulletTargetClass::BasePlayer, {0x00000002U, nullptr});
    assert(result.status == BulletTargetDispatchStatus::TraceAttackDispatched);
    assert(result.trace_route == TraceAttackRoute::BasePlayer);
}

void test_physics_prop_and_container_copy_damage_info_then_call_base() {
    for (const auto target_class : {BulletTargetClass::PhysicsProp,
                                    BulletTargetClass::PropContainer}) {
        Host host;
        const auto result = dispatch_original_bullet_target_hit(
            host, host.target, target_class, {0x00000002U, nullptr});
        assert(result.status == BulletTargetDispatchStatus::TraceAttackDispatched);
        assert(result.trace_route ==
               TraceAttackRoute::PhysicsPropOrContainerCopyThenBase);
    }
}

void test_stale_filter_handle_resolves_to_no_filter() {
    Host host;
    host.filter = EntityRef{20, 4};
    host.filter_live = false;
    const auto result = dispatch_original_bullet_target_hit(
        host, host.target, BulletTargetClass::BaseEntity, {0x00000002U, nullptr});
    assert(result.status == BulletTargetDispatchStatus::TraceAttackDispatched);
    assert(!result.filter_invoked && result.trace_attack_invoked);
    assert(host.calls == std::vector<std::string>{"trace"});
}

void test_lookup_and_filter_host_failures_do_not_dispatch_trace() {
    Host lookup_host;
    lookup_host.lookup_ok = false;
    assert(dispatch_original_bullet_target_hit(
        lookup_host, lookup_host.target, BulletTargetClass::BaseEntity,
        {0x00000002U, nullptr}).status ==
        BulletTargetDispatchStatus::FilterLookupFailed);

    Host call_host;
    call_host.filter = EntityRef{20, 4};
    call_host.filter_call_ok = false;
    assert(dispatch_original_bullet_target_hit(
        call_host, call_host.target, BulletTargetClass::BaseEntity,
        {0x00000002U, nullptr}).status ==
        BulletTargetDispatchStatus::FilterInvocationFailed);
    assert(call_host.calls == std::vector<std::string>{"filter"});
}

void test_stale_target_and_unknown_class_are_rejected() {
    Host stale;
    stale.target_live = false;
    assert(dispatch_original_bullet_target_hit(
        stale, stale.target, BulletTargetClass::BaseEntity,
        {0x00000002U, nullptr}).status == BulletTargetDispatchStatus::StaleTarget);

    Host unknown;
    assert(dispatch_original_bullet_target_hit(
        unknown, unknown.target, BulletTargetClass::Unknown,
        {0x00000002U, nullptr}).status ==
        BulletTargetDispatchStatus::UnsupportedTargetClass);
}

void test_trace_attack_failure_is_reported() {
    Host host;
    host.trace_ok = false;
    const auto result = dispatch_original_bullet_target_hit(
        host, host.target, BulletTargetClass::BaseEntity, {0x00000002U, nullptr});
    assert(result.status == BulletTargetDispatchStatus::TraceAttackFailed);
    assert(result.trace_attack_invoked);
}

} // namespace

int main() {
    test_no_filter_defaults_to_allow_and_dispatches_base_trace();
    test_base_filter_rejects_exact_values_not_masks();
    test_duag_combined_damage_bits_are_not_rejected_by_exact_comparison();
    test_filter_rejection_suppresses_trace_attack();
    test_hl2_player_exact_override_precedes_base_filter_lookup();
    test_hl2_player_combined_duag_bits_fall_through_to_base_filter();
    test_base_player_uses_its_trace_attack_override();
    test_physics_prop_and_container_copy_damage_info_then_call_base();
    test_stale_filter_handle_resolves_to_no_filter();
    test_lookup_and_filter_host_failures_do_not_dispatch_trace();
    test_stale_target_and_unknown_class_are_rejected();
    test_trace_attack_failure_is_reported();
    std::puts("PASS: original SiN bullet target filter and TraceAttack routing");
}
