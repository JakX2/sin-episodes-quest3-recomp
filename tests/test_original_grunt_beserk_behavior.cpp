#include "sin_compat/original_grunt_beserk_behavior.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

using namespace sin_compat::ai::grunt;

namespace {
class FakeBeserkHost final : public IOriginalGruntBeserkHost {
public:
    std::vector<std::string> calls;
    std::int32_t unresolved_argument = -1;

    void set_schedule_done_condition() noexcept override {
        calls.push_back("condition:COND_SCHEDULE_DONE");
    }
    void invoke_unresolved_npc_operation(
        std::int32_t argument) noexcept override {
        unresolved_argument = argument;
        calls.push_back("unresolved_npc_operation");
    }
    void apply_unresolved_npc_state_change_if_required() noexcept override {
        calls.push_back("unresolved_npc_state_change_if_required");
    }
};

class FakeRelationshipHost final : public IOriginalGruntRelationshipHost {
public:
    bool npc = false;
    bool player = false;
    std::vector<OriginalSharedAiCondition> conditions;
    std::size_t parent_gather_calls = 0;

    std::int32_t parent_relation_type(
        OriginalGruntEntityToken) const noexcept override { return 2; }
    bool target_is_npc(
        OriginalGruntEntityToken) const noexcept override { return npc; }
    bool target_is_player(
        OriginalGruntEntityToken) const noexcept override { return player; }
    std::vector<OriginalGruntEntityToken>
    relationship_targets_in_original_order() const override { return {1U}; }
    OriginalGruntEntityToken current_enemy() const noexcept override { return 1U; }
    std::int32_t relation_priority(
        OriginalGruntEntityToken) const noexcept override { return 11; }
    void set_ai_condition(
        OriginalSharedAiCondition condition) noexcept override {
        conditions.push_back(condition);
    }
    void delegate_parent_condition_gather() noexcept override {
        ++parent_gather_calls;
    }
    void defer_unrecovered_target_condition_tail(
        OriginalGruntEntityToken) noexcept override {}
};

void test_constructor_initializes_inactive_and_deadline_from_current_time() {
    const OriginalGruntBeserkBehavior behavior(3.25F);
    assert(!behavior.active());
    assert(behavior.deadline() == 3.25F);
}

void test_first_activation_sets_ten_second_deadline_and_callbacks_in_order() {
    OriginalGruntBeserkBehavior behavior(0.0F);
    FakeBeserkHost host;

    behavior.activate(12.5F, host);

    assert(behavior.active());
    assert(behavior.deadline() == 22.5F);
    assert(host.unresolved_argument == 3);
    assert((host.calls == std::vector<std::string>{
        "condition:COND_SCHEDULE_DONE",
        "unresolved_npc_operation",
        "unresolved_npc_state_change_if_required"}));
}

void test_reactivation_refreshes_deadline_without_repeating_first_activation_calls() {
    OriginalGruntBeserkBehavior behavior(0.0F);
    FakeBeserkHost host;

    behavior.activate(1.0F, host);
    const auto first_calls = host.calls;
    behavior.activate(4.0F, host);

    assert(behavior.active());
    assert(behavior.deadline() == 14.0F);
    assert(host.calls == first_calls);
}

void test_expiration_is_strictly_greater_than_deadline() {
    OriginalGruntBeserkBehavior behavior(0.0F);
    FakeBeserkHost host;
    behavior.activate(5.0F, host);

    behavior.update_expiration(15.0F);
    assert(behavior.active());

    behavior.update_expiration(15.0001F);
    assert(!behavior.active());
}

void test_inactive_expiration_is_a_noop() {
    OriginalGruntBeserkBehavior behavior(8.0F);
    behavior.update_expiration(100.0F);
    assert(!behavior.active());
    assert(behavior.deadline() == 8.0F);
}

void test_schedule_request_byte_is_distinct_from_active_state() {
    OriginalGruntBeserkBehavior inactive(0.0F);
    assert(!inactive.active());
    assert(!original_grunt_beserk_schedule(false).has_value());
    const auto selected = original_grunt_beserk_schedule(true);
    assert(selected.has_value() && *selected == 0x171U);

    FakeBeserkHost host;
    inactive.activate(0.0F, host);
    assert(inactive.active());
    assert(!original_grunt_beserk_schedule(false).has_value());
}

void test_relationship_adapter_uses_behavior_active_byte() {
    OriginalGruntBeserkBehavior behavior(0.0F);
    FakeBeserkHost beserk_host;
    FakeRelationshipHost relation_host;

    assert(original_grunt_relation_type(relation_host, behavior, 1U) == 2);
    relation_host.npc = true;
    behavior.activate(1.0F, beserk_host);
    assert(original_grunt_relation_type(relation_host, behavior, 1U) ==
           disposition_hate);
    relation_host.npc = false;
    assert(original_grunt_relation_type(relation_host, behavior, 1U) ==
           disposition_neutral);
}

void test_condition_gather_adapter_delegates_until_behavior_is_active() {
    OriginalGruntBeserkBehavior behavior(0.0F);
    FakeBeserkHost beserk_host;
    FakeRelationshipHost relation_host;

    auto result = gather_original_grunt_conditions(relation_host, behavior);
    assert(result.disposition ==
           OriginalGruntConditionGatherDisposition::ParentDelegated);
    assert(relation_host.parent_gather_calls == 1U);

    behavior.activate(1.0F, beserk_host);
    result = gather_original_grunt_conditions(relation_host, behavior);
    assert(result.disposition ==
           OriginalGruntConditionGatherDisposition::PartialHostilityMode);
    assert(relation_host.parent_gather_calls == 1U);
}

} // namespace

int main() {
    test_constructor_initializes_inactive_and_deadline_from_current_time();
    test_first_activation_sets_ten_second_deadline_and_callbacks_in_order();
    test_reactivation_refreshes_deadline_without_repeating_first_activation_calls();
    test_expiration_is_strictly_greater_than_deadline();
    test_inactive_expiration_is_a_noop();
    test_schedule_request_byte_is_distinct_from_active_state();
    test_relationship_adapter_uses_behavior_active_byte();
    test_condition_gather_adapter_delegates_until_behavior_is_active();
    std::puts("PASS: original SiN CAI_BeserkBehavior timer, schedule gate and relationship integration (mock host only)");
    return 0;
}
