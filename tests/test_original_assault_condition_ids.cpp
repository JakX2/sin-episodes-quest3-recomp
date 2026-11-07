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

int main() {
    test_register_mapping_and_separate_namespaces();
    test_goal_input_and_gunfire_cue_respect_distinct_condition_names();
}
