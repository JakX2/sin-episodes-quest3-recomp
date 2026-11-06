#include "sin_compat/original_assault_task_ids.hpp"
#include "sin_compat/original_assault_tasks.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <set>
#include <string_view>
#include <type_traits>

using namespace sin_compat;

static_assert(!std::is_convertible_v<LegacyAssaultTaskId, LegacyAssaultScheduleId>);
static_assert(!std::is_convertible_v<LegacyAssaultScheduleId, LegacyAssaultTaskId>);
static_assert(!std::is_convertible_v<std::uint32_t, LegacyAssaultTaskId>);
static_assert(original_assault_task_ids.size() == 11);

static_assert(original_assault_task_id_for("TASK_AWAIT_CUE")->value == 0x186A6);
static_assert(original_assault_task_id_for("TASK_HIT_RALLY_POINT")->value == 0x186A5);
static_assert(original_assault_task_id_for("TASK_ASSAULT_DEFER_SCHEDULE_SELECTION")->value
              == 0x186AB);
static_assert(!original_assault_task_id_for("TASK_RUN_PATH"));
static_assert(!original_assault_task_id_for("TASK_WAIT_FOR_MOVEMENT"));
static_assert(!original_assault_task_id_for("TASK_NOT_ORIGINAL"));

int main() {
    std::set<std::uint32_t> registered_ids;
    std::set<std::string_view> registered_names;
    for (const auto& entry : original_assault_task_ids) {
        assert(registered_ids.insert(entry.original_task_id.value).second);
        assert(registered_names.insert(entry.name).second);
        auto reverse = original_assault_task_name_for(entry.original_task_id);
        assert(reverse && *reverse == entry.name);
        auto forward = original_assault_task_id_for(entry.name);
        assert(forward && forward->value == entry.original_task_id.value);
    }
    assert(!registered_ids.count(0x186AA)); // not registered in audited block
    assert(!original_assault_task_name_for(LegacyAssaultTaskId{0x186AA}));

    // The integer 0x186A6 is both a TASK_AWAIT_CUE task ID and a
    // SCHED_WAIT_AND_CLEAR schedule ID in two distinct original registries.
    assert(original_assault_task_name_for(LegacyAssaultTaskId{0x186A6}) ==
           std::string_view("TASK_AWAIT_CUE"));
    assert(decode_original_assault_schedule(LegacyAssaultScheduleId{0x186A6})
           == AssaultSchedule::WaitAndClear);

    int original_task_count = 0;
    int shared_tasks = 0;
    for (const auto& task : original_assault_task_order) {
        const auto id = original_assault_task_id_for(task.original_task_symbol);
        if (id) {
            ++original_task_count;
            assert(registered_ids.count(id->value) == 1);
        } else {
            // TASK_SET_ACTIVITY, TASK_RUN_PATH, TASK_WAIT and similar
            // are inherited/shared Source tasks rather than these
            // 11 assault-specific registered symbols.
            ++shared_tasks;
        }
    }
    assert(original_task_count > 0);
    assert(shared_tasks > 0);
    return 0;
}
