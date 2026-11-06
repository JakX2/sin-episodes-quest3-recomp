#include "sin_compat/original_assault_tasks.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string_view>

int main() {
    using namespace sin_compat;
    static_assert(original_assault_task_order.size() == 29);
    static_assert(original_assault_interrupts.size() == 35);
    static_assert(original_task_count_for(AssaultSchedule::MoveToRallyPoint) == 8);
    static_assert(original_task_count_for(AssaultSchedule::MoveToAssaultPoint) == 7);
    static_assert(original_task_count_for(AssaultSchedule::HoldRallyPoint) == 2);
    static_assert(original_task_count_for(AssaultSchedule::AssaultFailedToMove) == 1);
    static_assert(original_interrupt_count_for(AssaultSchedule::HoldAssaultPoint) == 11);
    static_assert(original_interrupt_count_for(AssaultSchedule::WaitAndClear) == 7);
    static_assert(original_interrupt_count_for(AssaultSchedule::HoldRallyPoint) == 9);
    static_assert(original_interrupt_count_for(AssaultSchedule::AssaultMoveAway) == 0);

    // Every original schedule begins at task 0 and increments sequentially.
    for (const auto& schedule : original_assault_schedules) {
        std::size_t expected_position = 0;
        for (const auto& task : original_assault_task_order) {
            if (task.schedule != schedule.semantic) continue;
            assert(task.position == expected_position++);
            assert(task.original_task_symbol.substr(0, 5) == "TASK_");
            assert(!task.original_parameter.empty());
        }
        assert(expected_position == original_task_count_for(schedule.semantic));
    }

    // Original 2006 script's rally arrival transitions to HoldRallyPoint.
    const auto& transition = original_assault_task_order.back();
    assert(transition.schedule == AssaultSchedule::MoveToRallyPoint);
    assert(transition.original_task_symbol == "TASK_SET_SCHEDULE");
    assert(transition.original_parameter == "SCHEDULE:SCHED_HOLD_RALLY_POINT");

    // Original assault approach ends in TASK_HIT_ASSAULT_POINT.
    const auto& hit = original_assault_task_order[14];
    assert(hit.schedule == AssaultSchedule::MoveToAssaultPoint);
    assert(hit.original_task_symbol == "TASK_HIT_ASSAULT_POINT");

    // Original time-clear schedule calls out the important timeout/announcement
    // tasks, but the native AI logic executing those tasks is not implemented.
    const auto& timeout = original_assault_task_order[6];
    assert(timeout.schedule == AssaultSchedule::WaitAndClear);
    assert(timeout.original_task_symbol == "TASK_AWAIT_ASSAULT_TIMEOUT");

    for (const auto& condition : original_assault_interrupts) {
        assert(condition.original_condition_symbol.substr(0, 5) == "COND_");
        assert(original_task_count_for(condition.schedule) != 0);
    }

    return 0;
}
