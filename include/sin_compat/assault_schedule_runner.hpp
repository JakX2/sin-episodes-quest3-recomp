#pragma once

// Minimal host-driven executor for the verified original 2006 SiN assault
// SCHED_* declarations. This does NOT implement Source AI TASK_* handlers:
// the host must provide every game's movement, timing, cue and combat result.
// Source server.dll task text/order and interrupt declarations are verified by
// audit_sin_assault_schedule_scripts.py, but this callback runner is NEW code.
// No original x86 task IDs, Source EHANDLE pointers, RTTI, or engine ABI used.

#include "sin_compat/original_assault_tasks.hpp"

#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string_view>

namespace sin_compat {

enum class AssaultHostTaskResult { Completed, Pending, Failed };

enum class AssaultTickResult {
    Inactive,
    TaskCompleted,
    TaskPending,
    ScheduleFinished,
    ExplicitScheduleChange,
    FallbackScheduleSelected,
    TaskFailedNoFallback,
    Interrupted,
    UnknownScheduleReference,
};

struct AssaultTickReport {
    AssaultTickResult result = AssaultTickResult::Inactive;
    AssaultSchedule schedule = AssaultSchedule::MoveToRallyPoint;
    std::size_t original_task_position = 0;
    std::string_view task_symbol;
    std::string_view interrupt_condition;
    std::optional<AssaultSchedule> new_schedule;
};

// Pure metadata resolution, independent of the Windows 2006 numeric schedule
// and task namespaces. Never interpret an arbitrary numeric ID as an enum.
inline std::optional<AssaultSchedule> find_assault_schedule_by_name(
    std::string_view symbol) {
    for (const auto& entry : original_assault_schedules) {
        if (entry.original_symbol_name == symbol) return entry.semantic;
    }
    return std::nullopt;
}

// A deterministic execution harness, not the original scheduler. One tick
// resolves at most one declared original task. Host callbacks are responsible
// for asynchronous task state; Pending retries the SAME task next tick.
// A declared interrupt is returned to the host without guessing which
// non-assault combat schedule the original engine would select instead.
class AssaultScheduleRunner {
public:
    explicit AssaultScheduleRunner(AssaultSchedule initial)
        : schedule_(initial) {}

    bool start(AssaultSchedule schedule) {
        if (original_task_count_for(schedule) == 0) return false;
        schedule_ = schedule;
        position_ = 0;
        fallback_.reset();
        active_ = true;
        return true;
    }

    bool active() const { return active_; }
    AssaultSchedule current_schedule() const { return schedule_; }
    std::size_t next_task_position() const { return position_; }
    std::optional<AssaultSchedule> fallback_schedule() const { return fallback_; }

    void stop() {
        active_ = false;
        fallback_.reset();
    }

    template <typename HostTask>
    AssaultTickReport tick(std::initializer_list<std::string_view> active_conditions,
                           HostTask&& host_task) {
        AssaultTickReport report;
        report.schedule = schedule_;
        report.original_task_position = position_;
        if (!active_) return report;

        // The interrupt order is the original schedule declaration order.
        for (const auto& declared : original_assault_interrupts) {
            if (declared.schedule != schedule_) continue;
            for (const auto& current : active_conditions) {
                if (declared.original_condition_symbol == current) {
                    report.result = AssaultTickResult::Interrupted;
                    report.interrupt_condition = current;
                    return report;
                }
            }
        }

        const AssaultTaskWitness* task = nullptr;
        for (const auto& witness : original_assault_task_order) {
            if (witness.schedule == schedule_ &&
                witness.position == position_) {
                task = &witness;
                break;
            }
        }
        if (task == nullptr) {
            // Exhaustion is an explicit signal. Native engine decides the
            // next combat/assault schedule, not this offline reference.
            active_ = false;
            report.result = AssaultTickResult::ScheduleFinished;
            return report;
        }

        report.task_symbol = task->original_task_symbol;

        // Interpret only the two declarative schedule-control symbols.
        // This is a tested portable harness interpretation, not the recovered
        // native Source TASK_SET_SCHEDULE task implementation.
        if (task->original_task_symbol == "TASK_SET_FAIL_SCHEDULE") {
            const auto target = resolve_parameter(*task);
            if (!target) {
                stop();
                report.result = AssaultTickResult::UnknownScheduleReference;
                return report;
            }
            fallback_ = target;
            ++position_;
            report.result = AssaultTickResult::TaskCompleted;
            return report;
        }

        if (task->original_task_symbol == "TASK_SET_SCHEDULE") {
            const auto target = resolve_parameter(*task);
            if (!target || !start(*target)) {
                stop();
                report.result = AssaultTickResult::UnknownScheduleReference;
                return report;
            }
            report.result = AssaultTickResult::ExplicitScheduleChange;
            report.new_schedule = target;
            return report;
        }

        const auto host_result = host_task(*task);
        switch (host_result) {
            case AssaultHostTaskResult::Pending:
                report.result = AssaultTickResult::TaskPending;
                return report;
            case AssaultHostTaskResult::Failed:
                if (fallback_) {
                    const auto next = *fallback_;
                    if (!start(next)) {
                        stop();
                        report.result = AssaultTickResult::UnknownScheduleReference;
                        return report;
                    }
                    report.result = AssaultTickResult::FallbackScheduleSelected;
                    report.new_schedule = next;
                    return report;
                }
                stop();
                report.result = AssaultTickResult::TaskFailedNoFallback;
                return report;
            case AssaultHostTaskResult::Completed:
                ++position_;
                report.result = AssaultTickResult::TaskCompleted;
                return report;
        }
        // Defensive invalid-enum fallback. Never advance an unknown result.
        stop();
        report.result = AssaultTickResult::TaskFailedNoFallback;
        return report;
    }

private:
    static std::optional<AssaultSchedule> resolve_parameter(
        const AssaultTaskWitness& task) {
        constexpr std::string_view prefix = "SCHEDULE:";
        const auto parameter = task.original_parameter;
        if (parameter.substr(0, prefix.size()) != prefix) return std::nullopt;
        return find_assault_schedule_by_name(parameter.substr(prefix.size()));
    }

    AssaultSchedule schedule_;
    std::size_t position_ = 0;
    std::optional<AssaultSchedule> fallback_;
    bool active_ = false;
};

} // namespace sin_compat
