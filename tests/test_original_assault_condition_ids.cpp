#include "sin_compat/original_assault_condition_ids.hpp"
#include "sin_compat/original_assault_schedule.hpp"
#include "sin_compat/original_assault_task_ids.hpp"
#include "sin_compat/original_goal_assault_input.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <set>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace sin_compat;

static_assert(!std::is_convertible_v<LegacyAssaultConditionId, LegacyAssaultTaskId>);
static_assert(!std::is_convertible_v<LegacyAssaultConditionId, LegacyAssaultScheduleId>);
static_assert(!std::is_convertible_v<LegacyAssaultTaskId, LegacyAssaultConditionId>);
static_assert(!std::is_convertible_v<std::uint32_t, LegacyAssaultConditionId>);
static_assert(original_assault_condition_ids.size() == 9);
static_assert(original_assault_condition_id(OriginalAssaultCondition::Provoked).value == 0x18);
static_assert(original_assault_condition_id(OriginalAssaultCondition::HearCombat).value == 0x33);
static_assert(original_assault_condition_id(OriginalAssaultCondition::ReceivedOrders).value == 0x3B);
static_assert(original_assault_condition_id(OriginalAssaultCondition::SeeEnemy).value == 0x0A);
static_assert(original_assault_condition_id(OriginalAssaultCondition::NewEnemy).value == 0x19);
static_assert(original_assault_condition_name(OriginalAssaultCondition::HearCombat) ==
              std::string_view("COND_HEAR_COMBAT"));

static void test_register_mapping_and_separate_namespaces() {
    std::set<std::uint32_t> ids;
    std::set<std::string_view> names;
    for (const auto& row : original_assault_condition_ids) {
        assert(ids.insert(row.original_x86_id.value).second);
        assert(names.insert(row.symbol).second);
        auto resolved = decode_original_assault_condition(row.original_x86_id);
        assert(resolved && *resolved == row.semantic);
        assert(original_assault_condition_name(row.semantic) == row.symbol);
        assert(original_assault_condition_id(row.semantic).value == row.original_x86_id.value);
    }
    assert(!decode_original_assault_condition(LegacyAssaultConditionId{0xF0}));
    assert(original_assault_condition_id(static_cast<OriginalAssaultCondition>(255)).value
           == 0xFFFFFFFFu);

    // Original 0x18 is a condition, NOT the same number in TASK/SCHED namespace.
    assert(original_assault_condition_id(OriginalAssaultCondition::Provoked).value ==
           0x18);
}

static void test_goal_input_and_gunfire_cue_respect_distinct_condition_names() {
    OriginalAssaultCueGate input{1, 0};
    OriginalAssaultCueGate gunfire{2, 0};
    std::vector<OriginalGoalAssaultActor> actors{
        {1001, &input, true}, {1002, &gunfire, true}
    };
    std::vector<std::uint32_t> received_condition_ids;
    int reset_count = 0;

    const auto activated = begin_goal_assault(
        actors,
        [&](std::uint64_t) { ++reset_count; },
        [&](std::uint64_t, std::uint32_t condition_id) {
            received_condition_ids.push_back(condition_id);
        });
    assert(activated == 2);
    assert(reset_count == 2);
    assert((received_condition_ids == std::vector<std::uint32_t>{0x18, 0x18}));
    assert(input.received_cue == 1 && input.check(false));
    assert(gunfire.received_cue == 1);
    // Receiving order cue 1 does NOT satisfy mode-2 gunfire gate.
    assert(!gunfire.check(false));
    assert(gunfire.check(true)); // Host supplied COND_HEAR_COMBAT
}


static void test_goal_input_callback_order_and_repeated_delivery() {
    OriginalAssaultCueGate first{1, 0};
    OriginalAssaultCueGate second{2, 0};
    OriginalAssaultCueGate skipped{1, 0};
    std::vector<OriginalGoalAssaultActor> actors{
        {41, &first, true},
        {42, nullptr, true},
        {43, &skipped, false},
        {44, &second, true},
    };
    std::vector<std::uint64_t> resets;
    std::vector<std::uint64_t> conditions;
    std::vector<std::uint32_t> condition_ids;
    std::vector<std::string_view> events;
    const auto reset = [&](std::uint64_t actor) {
        resets.push_back(actor);
        events.push_back("reset");
        // No cue may have been delivered before the matching reset.
        if (actor == 41) assert(first.received_cue == 0);
        if (actor == 44) assert(second.received_cue == 0);
    };
    const auto signal = [&](std::uint64_t actor, std::uint32_t id) {
        conditions.push_back(actor);
        condition_ids.push_back(id);
        events.push_back("condition");
        // The original handler writes received cue 1 before signaling
        // COND_PROVOKED. It does not signal COND_RECEIVED_ORDERS.
        if (actor == 41) assert(first.received_cue == 1);
        if (actor == 44) assert(second.received_cue == 1);
        assert(id == original_assault_condition_id(
                         OriginalAssaultCondition::Provoked).value);
    };
    assert(begin_goal_assault(actors, reset, signal) == 2);
    assert((resets == std::vector<std::uint64_t>{41, 44}));
    assert((conditions == std::vector<std::uint64_t>{41, 44}));
    assert((condition_ids == std::vector<std::uint32_t>{0x18, 0x18}));
    assert((events == std::vector<std::string_view>{
        "reset", "condition", "reset", "condition"}));
    assert(skipped.received_cue == 0);

    // Repeated BeginAssault re-signals each valid actor even after a cue
    // was already received. Reset callbacks now observe the previous cue.
    resets.clear();
    conditions.clear();
    events.clear();
    assert(begin_goal_assault(actors,
        [&](std::uint64_t actor) {
            assert((actor == 41 ? first.received_cue : second.received_cue) == 1);
            resets.push_back(actor);
        },
        [&](std::uint64_t actor, std::uint32_t id) {
            assert(id == 0x18);
            conditions.push_back(actor);
        }) == 2);
    assert((resets == std::vector<std::uint64_t>{41, 44}));
    assert((conditions == std::vector<std::uint64_t>{41, 44}));
    assert(first.received_cue == 1 && second.received_cue == 1);
}

int main() {
    test_register_mapping_and_separate_namespaces();
    test_goal_input_and_gunfire_cue_respect_distinct_condition_names();
    test_goal_input_callback_order_and_repeated_delivery();
}
