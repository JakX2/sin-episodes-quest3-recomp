#include "sin_compat/assault_runtime_condition_adapter.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace sin_compat;

static_assert(!std::is_convertible_v<LegacyAssaultConditionId,
                                      RuntimeAssaultConditionId>);
static_assert(!std::is_convertible_v<RuntimeAssaultConditionId,
                                      LegacyAssaultConditionId>);
static_assert(!std::is_convertible_v<std::uint32_t, RuntimeAssaultConditionId>);
static_assert(!std::is_convertible_v<LegacyAssaultTaskId,
                                      RuntimeAssaultConditionId>);

static void test_runtime_registry_rejects_ambiguous_bindings() {
    AssaultRuntimeConditionBindings bindings;
    assert(!bindings.resolve(OriginalAssaultCondition::Provoked));
    assert(!bindings.bind(static_cast<OriginalAssaultCondition>(255),
                          RuntimeAssaultConditionId{100}));
    assert(!bindings.bind(OriginalAssaultCondition::Provoked,
                          RuntimeAssaultConditionId{
                              std::numeric_limits<std::uint32_t>::max()}));

    assert(bindings.bind(OriginalAssaultCondition::Provoked,
                         RuntimeAssaultConditionId{1201}));
    assert(bindings.bind(OriginalAssaultCondition::Provoked,
                         RuntimeAssaultConditionId{1201}));
    assert(!bindings.bind(OriginalAssaultCondition::Provoked,
                          RuntimeAssaultConditionId{1202}));
    assert(!bindings.bind(OriginalAssaultCondition::HearCombat,
                          RuntimeAssaultConditionId{1201}));
    assert(bindings.bind(OriginalAssaultCondition::HearCombat,
                         RuntimeAssaultConditionId{1202}));
    assert(bindings.resolve(OriginalAssaultCondition::Provoked)->value == 1201);
    assert(bindings.resolve(OriginalAssaultCondition::HearCombat)->value == 1202);
    assert(!bindings.resolve(OriginalAssaultCondition::ReceivedOrders));
}

static void test_begin_assault_fails_before_any_side_effect_without_binding() {
    AssaultRuntimeConditionBindings bindings;
    OriginalAssaultCueGate cue{1, 0};
    std::vector<OriginalGoalAssaultActor> actors{{17, &cue, true}};
    int reset_calls = 0, condition_calls = 0;
    auto result = begin_goal_assault_with_runtime_conditions(
        actors, bindings,
        [&](std::uint64_t) { ++reset_calls; },
        [&](std::uint64_t, RuntimeAssaultConditionId) { ++condition_calls; });

    assert(!result && cue.received_cue == 0);
    assert(reset_calls == 0 && condition_calls == 0);
}

static void test_begin_assault_translates_semantic_and_preserves_order() {
    AssaultRuntimeConditionBindings bindings;
    // Intentionally not using the original x86 ID 0x18 as the host ID.
    assert(bindings.bind(OriginalAssaultCondition::Provoked,
                         RuntimeAssaultConditionId{3011}));
    OriginalAssaultCueGate first{1, 0}, second{2, 0}, skipped{1, 0};
    std::vector<OriginalGoalAssaultActor> actors{
        {51, &first, true}, {52, nullptr, true},
        {53, &skipped, false}, {54, &second, true}
    };
    std::vector<std::pair<std::uint64_t, std::string_view>> events;
    auto reset = [&](std::uint64_t token) {
        events.emplace_back(token, "reset");
        if (token == 51) assert(first.received_cue == 0);
        if (token == 54) assert(second.received_cue == 0);
    };
    auto signal = [&](std::uint64_t token, RuntimeAssaultConditionId id) {
        events.emplace_back(token, "condition");
        assert(id.value == 3011);
        if (token == 51) assert(first.received_cue == 1);
        if (token == 54) assert(second.received_cue == 1);
    };
    auto result = begin_goal_assault_with_runtime_conditions(
        actors, bindings, reset, signal);
    assert(result && *result == 2);
    assert((events == std::vector<std::pair<std::uint64_t, std::string_view>>{
        {51, "reset"}, {51, "condition"}, {54, "reset"}, {54, "condition"}
    }));
    assert(skipped.received_cue == 0);

    // The historic evidence API still emits the historic condition number,
    // proving the native adapter does not mutate the decompilation witness.
    assert(original_assault_condition_id(OriginalAssaultCondition::Provoked).value
           == 0x18);
}

static void test_gunfire_cue_queries_only_the_runtime_hear_combat_id() {
    AssaultRuntimeConditionBindings bindings;
    OriginalAssaultCueGate gunfire{2, 0};
    int query_count = 0;
    auto query = [&](std::uint64_t actor, RuntimeAssaultConditionId id) {
        ++query_count;
        assert(actor == 96 && id.value == 4003);
        return true;
    };

    // No native registration: no unsafe query, no change of cue.
    assert(!check_assault_cue_with_runtime_conditions(
        gunfire, 96, bindings, query));
    assert(query_count == 0 && gunfire.received_cue == 0);
    assert(bindings.bind(OriginalAssaultCondition::HearCombat,
                         RuntimeAssaultConditionId{4003}));
    const auto passed = check_assault_cue_with_runtime_conditions(
        gunfire, 96, bindings, query);
    assert(passed && *passed && query_count == 1);
    assert(gunfire.received_cue == 0); // Original mode 2 does not write cue.

    // Input cue mode 1, don't-wait mode 3, and received_cue=4 override
    // do not invoke a native condition query at all.
    OriginalAssaultCueGate input{1, 1};
    OriginalAssaultCueGate bypass{3, 0};
    OriginalAssaultCueGate override{2, 4};
    assert(check_assault_cue_with_runtime_conditions(
        input, 96, bindings, query).value());
    assert(check_assault_cue_with_runtime_conditions(
        bypass, 96, bindings, query).value());
    assert(bypass.received_cue == 3);
    assert(check_assault_cue_with_runtime_conditions(
        override, 96, bindings, query).value());
    assert(query_count == 1);

    // An absent runtime gunfire condition cannot accidentally be taken as
    // the similarly named ReceivedOrders or Provoked condition.
    AssaultRuntimeConditionBindings wrong;
    assert(wrong.bind(OriginalAssaultCondition::ReceivedOrders,
                      RuntimeAssaultConditionId{4003}));
    assert(!check_assault_cue_with_runtime_conditions(
        gunfire, 96, wrong, query));
    assert(query_count == 1);
}

int main() {
    test_runtime_registry_rejects_ambiguous_bindings();
    test_begin_assault_fails_before_any_side_effect_without_binding();
    test_begin_assault_translates_semantic_and_preserves_order();
    test_gunfire_cue_queries_only_the_runtime_hear_combat_id();
    return 0;
}
