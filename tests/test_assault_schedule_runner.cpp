#include "sin_compat/assault_schedule_runner.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

using sin_compat::AssaultHostTaskResult;
using sin_compat::AssaultSchedule;
using sin_compat::AssaultScheduleRunner;
using sin_compat::AssaultTickResult;
using sin_compat::AssaultTaskWitness;
using sin_compat::find_assault_schedule_by_name;

static void test_lookup_and_inactive() {
    for (const auto& entry : sin_compat::original_assault_schedules) {
        const auto resolved = find_assault_schedule_by_name(entry.original_symbol_name);
        assert(resolved && *resolved == entry.semantic);
    }
    assert(!find_assault_schedule_by_name("SCHED_SIN_NOT_A_REAL_SCHEDULE"));
    assert(!find_assault_schedule_by_name("TASK_MOVE_TO_RALLY_POINT"));
    AssaultScheduleRunner runner(AssaultSchedule::MoveToRallyPoint);
    int calls = 0;
    const auto report = runner.tick({}, [&](const AssaultTaskWitness&) {
        ++calls;
        return AssaultHostTaskResult::Completed;
    });
    assert(report.result == AssaultTickResult::Inactive);
    assert(calls == 0);
    assert(!runner.active());
    assert(!runner.start(static_cast<AssaultSchedule>(0xFF)));
    assert(!runner.active());
}

static void test_original_rally_path_pending_interrupt_and_transition() {
    AssaultScheduleRunner runner(AssaultSchedule::MoveToRallyPoint);
    assert(runner.start(AssaultSchedule::MoveToRallyPoint));
    assert(runner.active());
    std::vector<std::string_view> host_tasks;
    int callback_calls = 0;
    auto host = [&](const AssaultTaskWitness& task) {
        ++callback_calls;
        host_tasks.push_back(task.original_task_symbol);
        if (task.original_task_symbol == "TASK_GET_PATH_TO_RALLY_POINT" &&
            callback_calls == 1) return AssaultHostTaskResult::Pending;
        return AssaultHostTaskResult::Completed;
    };

    // Control metadata is interpreted without dispatching a Source task.
    auto r = runner.tick({}, host);
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(r.task_symbol == "TASK_SET_FAIL_SCHEDULE");
    assert(runner.fallback_schedule() == AssaultSchedule::AssaultFailedToMove);
    assert(callback_calls == 0);
    assert(runner.next_task_position() == 1);

    // Original rally schedule has no COND_NEW_ENEMY interrupt.
    r = runner.tick({"COND_NEW_ENEMY"}, host);
    assert(r.result == AssaultTickResult::TaskPending);
    assert(r.task_symbol == "TASK_GET_PATH_TO_RALLY_POINT");
    assert(runner.next_task_position() == 1);

    // Danger interrupts this schedule. No task is run or position lost.
    r = runner.tick({"COND_NO_PRIMARY_AMMO", "COND_HEAR_DANGER"}, host);
    assert(r.result == AssaultTickResult::Interrupted);
    assert(r.interrupt_condition == "COND_HEAR_DANGER"); // declaration order
    assert(callback_calls == 1);
    assert(runner.next_task_position() == 1);

    r = runner.tick({}, host);
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(runner.next_task_position() == 2);

    // Follow original task order until explicit hold-rally transition.
    const char* expected[] = {
        "TASK_RUN_PATH", "TASK_WAIT_FOR_MOVEMENT", "TASK_STOP_MOVING",
        "TASK_FACE_RALLY_POINT", "TASK_HIT_RALLY_POINT"
    };
    for (auto* name : expected) {
        r = runner.tick({}, host);
        assert(r.result == AssaultTickResult::TaskCompleted);
        assert(r.task_symbol == name);
    }
    assert(runner.next_task_position() == 7);
    r = runner.tick({}, host);
    assert(r.result == AssaultTickResult::ExplicitScheduleChange);
    assert(r.new_schedule == AssaultSchedule::HoldRallyPoint);
    assert(runner.current_schedule() == AssaultSchedule::HoldRallyPoint);
    assert(runner.next_task_position() == 0);
    assert(!runner.fallback_schedule());
    assert(callback_calls == 7);

    // Now hold until cue and delay. Pending must keep the same original task.
    r = runner.tick({}, [](const AssaultTaskWitness& task) {
        assert(task.original_task_symbol == "TASK_AWAIT_CUE");
        return AssaultHostTaskResult::Pending;
    });
    assert(r.result == AssaultTickResult::TaskPending);
    assert(runner.next_task_position() == 0);
    r = runner.tick({}, [](const AssaultTaskWitness&) {
        return AssaultHostTaskResult::Completed;
    });
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(runner.next_task_position() == 1);
    r = runner.tick({}, [](const AssaultTaskWitness& task) {
        assert(task.original_task_symbol == "TASK_WAIT_ASSAULT_DELAY");
        return AssaultHostTaskResult::Completed;
    });
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(runner.active()); // terminal signal is explicit on following tick
    r = runner.tick({}, [](const AssaultTaskWitness&) {
        assert(false && "unexpected callback on completed schedule");
        return AssaultHostTaskResult::Failed;
    });
    assert(r.result == AssaultTickResult::ScheduleFinished);
    assert(!runner.active());
}

static void test_path_failure_selects_original_fallback() {
    AssaultScheduleRunner runner(AssaultSchedule::MoveToAssaultPoint);
    assert(runner.start(AssaultSchedule::MoveToAssaultPoint));
    auto r = runner.tick({}, [](const AssaultTaskWitness&) {
        assert(false && "first task is metadata");
        return AssaultHostTaskResult::Failed;
    });
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(runner.fallback_schedule() == AssaultSchedule::AssaultFailedToMove);
    r = runner.tick({}, [](const AssaultTaskWitness& task) {
        assert(task.original_task_symbol == "TASK_GATHER_CONDITIONS");
        return AssaultHostTaskResult::Completed;
    });
    assert(r.result == AssaultTickResult::TaskCompleted);
    r = runner.tick({}, [](const AssaultTaskWitness& task) {
        assert(task.original_task_symbol == "TASK_GET_PATH_TO_ASSAULT_POINT");
        return AssaultHostTaskResult::Failed;
    });
    assert(r.result == AssaultTickResult::FallbackScheduleSelected);
    assert(r.new_schedule == AssaultSchedule::AssaultFailedToMove);
    assert(runner.current_schedule() == AssaultSchedule::AssaultFailedToMove);
    assert(runner.next_task_position() == 0);
    assert(!runner.fallback_schedule()); // no retry loops through stale fallback
    r = runner.tick({}, [](const AssaultTaskWitness& task) {
        assert(task.original_task_symbol == "TASK_ASSAULT_DEFER_SCHEDULE_SELECTION");
        assert(task.original_parameter == "1");
        return AssaultHostTaskResult::Completed;
    });
    assert(r.result == AssaultTickResult::TaskCompleted);
    r = runner.tick({}, [](const AssaultTaskWitness&) {
        assert(false && "completed schedule has no further host task");
        return AssaultHostTaskResult::Failed;
    });
    assert(r.result == AssaultTickResult::ScheduleFinished);
}

static void test_interrupts_are_exact_and_do_not_advance() {
    AssaultScheduleRunner runner(AssaultSchedule::WaitAndClear);
    assert(runner.start(AssaultSchedule::WaitAndClear));
    int callback_calls = 0;
    auto host = [&](const AssaultTaskWitness&) {
        ++callback_calls;
        return AssaultHostTaskResult::Completed;
    };
    // Unknown, unrelated conditions do not interrupt. Exact declared tokens do.
    auto r = runner.tick({"COND_PLAYER_PUSHING"}, host);
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(callback_calls == 1);
    r = runner.tick({"COND_HEAR_DANGER", "COND_NEW_ENEMY"}, host);
    assert(r.result == AssaultTickResult::Interrupted);
    assert(r.interrupt_condition == "COND_NEW_ENEMY");
    assert(callback_calls == 1);
    assert(runner.next_task_position() == 1);
    r = runner.tick({}, host);
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(runner.next_task_position() == 2);
    r = runner.tick({}, [](const AssaultTaskWitness& task) {
        assert(task.original_task_symbol == "TASK_AWAIT_ASSAULT_TIMEOUT");
        return AssaultHostTaskResult::Pending;
    });
    assert(r.result == AssaultTickResult::TaskPending);
    assert(runner.next_task_position() == 2);
    runner.stop();
    assert(!runner.fallback_schedule());
    r = runner.tick({}, host);
    assert(r.result == AssaultTickResult::Inactive);
    assert(callback_calls == 2);
}

static void test_unhandled_host_failure_and_no_implicit_next_schedule() {
    AssaultScheduleRunner runner(AssaultSchedule::ClearAssaultPoint);
    assert(runner.start(AssaultSchedule::ClearAssaultPoint));
    auto r = runner.tick({}, [](const AssaultTaskWitness& task) {
        assert(task.original_task_symbol == "TASK_ANNOUNCE_CLEAR");
        return AssaultHostTaskResult::Failed;
    });
    assert(r.result == AssaultTickResult::TaskFailedNoFallback);
    assert(!runner.active());

    assert(runner.start(AssaultSchedule::AssaultMoveAway));
    for (int i = 0; i < 3; ++i) {
        r = runner.tick({}, [](const AssaultTaskWitness&) {
            return AssaultHostTaskResult::Completed;
        });
        assert(r.result == AssaultTickResult::TaskCompleted);
    }
    r = runner.tick({}, [](const AssaultTaskWitness&) {
        assert(false && "no host task after last scripted step");
        return AssaultHostTaskResult::Completed;
    });
    assert(r.result == AssaultTickResult::ScheduleFinished);
    assert(!r.new_schedule); // original engine chooses next task/schedule
}

static void test_every_registered_original_schedule_has_callable_tasks() {
    for (const auto& entry : sin_compat::original_assault_schedules) {
        AssaultScheduleRunner runner(entry.semantic);
        assert(runner.start(entry.semantic));
        std::size_t calls = 0;
        std::size_t result_steps = 0;
        while (runner.active()) {
            auto r = runner.tick({}, [&](const AssaultTaskWitness& task) {
                ++calls;
                assert(task.schedule == runner.current_schedule());
                return AssaultHostTaskResult::Completed;
            });
            ++result_steps;
            assert(r.result != AssaultTickResult::UnknownScheduleReference);
            assert(r.result != AssaultTickResult::TaskFailedNoFallback);
            assert(result_steps <= 16); // stops even with explicit rally->hold
        }
        assert(calls <= 16);
    }
}

static void test_invalid_restart_fails_closed_mid_task() {
    AssaultScheduleRunner runner(AssaultSchedule::MoveToRallyPoint);
    assert(runner.start(AssaultSchedule::MoveToRallyPoint));
    auto r = runner.tick({}, [](const AssaultTaskWitness&) {
        assert(false && "metadata must not reach host");
        return AssaultHostTaskResult::Failed;
    });
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(runner.fallback_schedule() == AssaultSchedule::AssaultFailedToMove);
    r = runner.tick({}, [](const AssaultTaskWitness&) {
        return AssaultHostTaskResult::Pending;
    });
    assert(r.result == AssaultTickResult::TaskPending);
    assert(runner.next_task_position() == 1);

    assert(!runner.start(static_cast<AssaultSchedule>(0xFF)));
    assert(!runner.active());
    assert(!runner.fallback_schedule());
    assert(runner.next_task_position() == 0);
    int callbacks = 0;
    r = runner.tick({}, [&](const AssaultTaskWitness&) {
        ++callbacks;
        return AssaultHostTaskResult::Completed;
    });
    assert(r.result == AssaultTickResult::Inactive);
    assert(callbacks == 0);

    // A subsequent valid restart must begin cleanly at task zero.
    assert(runner.start(AssaultSchedule::HoldRallyPoint));
    assert(runner.next_task_position() == 0);
    assert(!runner.fallback_schedule());
    r = runner.tick({}, [](const AssaultTaskWitness& task) {
        assert(task.original_task_symbol == "TASK_AWAIT_CUE");
        return AssaultHostTaskResult::Pending;
    });
    assert(r.result == AssaultTickResult::TaskPending);
}

static void test_explicit_stop_clears_position_and_fallback() {
    AssaultScheduleRunner runner(AssaultSchedule::MoveToRallyPoint);
    assert(runner.start(AssaultSchedule::MoveToRallyPoint));
    auto r = runner.tick({}, [](const AssaultTaskWitness&) {
        assert(false && "declarative fallback must not dispatch to host");
        return AssaultHostTaskResult::Failed;
    });
    assert(r.result == AssaultTickResult::TaskCompleted);
    assert(runner.next_task_position() == 1);
    assert(runner.fallback_schedule() == AssaultSchedule::AssaultFailedToMove);

    runner.stop();
    assert(!runner.active());
    assert(runner.next_task_position() == 0);
    assert(!runner.fallback_schedule());
    int calls = 0;
    r = runner.tick({}, [&](const AssaultTaskWitness&) {
        ++calls;
        return AssaultHostTaskResult::Completed;
    });
    assert(r.result == AssaultTickResult::Inactive);
    assert(r.original_task_position == 0);
    assert(calls == 0);

    // Stop is idempotent and a new schedule starts from its first task.
    runner.stop();
    assert(runner.next_task_position() == 0);
    assert(runner.start(AssaultSchedule::HoldRallyPoint));
    r = runner.tick({}, [](const AssaultTaskWitness& task) {
        assert(task.original_task_symbol == "TASK_AWAIT_CUE");
        return AssaultHostTaskResult::Pending;
    });
    assert(r.result == AssaultTickResult::TaskPending);
    assert(runner.next_task_position() == 0);
}

int main() {
    test_explicit_stop_clears_position_and_fallback();
    test_invalid_restart_fails_closed_mid_task();
    test_lookup_and_inactive();
    test_original_rally_path_pending_interrupt_and_transition();
    test_path_failure_selects_original_fallback();
    test_interrupts_are_exact_and_do_not_advance();
    test_unhandled_host_failure_and_no_implicit_next_schedule();
    test_every_registered_original_schedule_has_callable_tasks();
}
