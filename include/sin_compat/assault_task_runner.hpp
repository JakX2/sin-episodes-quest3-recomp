#pragma once
// Portable, opt-in interpreter of the VERIFIED DECLARATIVE task ordering.
// Host callbacks implement all engine effects. This is not reconstructed
// Source AI task behavior or a native SourceVR schedule registration.
#include "sin_compat/original_assault_tasks.hpp"
#include <cstddef>
#include <optional>
#include <string_view>

namespace sin_compat {
enum class AssaultTaskResult { Running, Complete, Failed };
enum class AssaultRunnerState { Idle, Running, Completed, Failed, Interrupted };
struct AssaultTaskInvocation {
    AssaultSchedule schedule;
    std::size_t position;
    std::string_view symbol;
    std::string_view parameter;
};
class AssaultTaskRunner {
public:
    void start(AssaultSchedule schedule) {
        schedule_ = schedule;
        position_ = 0;
        state_ = AssaultRunnerState::Running;
    }
    AssaultRunnerState state() const { return state_; }
    std::size_t position() const { return position_; }
    std::optional<AssaultSchedule> schedule() const { return schedule_; }

    // The caller supplies condition symbols observed by the actual engine.
    // An interrupt is a signal only; the host decides the next schedule.
    bool interrupt(std::string_view condition) {
        if (state_ != AssaultRunnerState::Running || !schedule_) return false;
        for (const auto& item : original_assault_interrupts) {
            if (item.schedule == *schedule_ && item.original_condition_symbol == condition) {
                state_ = AssaultRunnerState::Interrupted;
                return true;
            }
        }
        return false;
    }

    // At most ONE host task callback per tick, including synchronous tasks.
    // No guessed duration, pathing, activity, timeout or fail transition.
    template <typename Execute>
    AssaultRunnerState tick(Execute&& execute) {
        if (state_ != AssaultRunnerState::Running || !schedule_) return state_;
        for (const auto& task : original_assault_task_order) {
            if (task.schedule != *schedule_ || task.position != position_) continue;
            const auto outcome = execute(AssaultTaskInvocation{
                *schedule_, position_, task.original_task_symbol, task.original_parameter});
            if (outcome == AssaultTaskResult::Failed) state_ = AssaultRunnerState::Failed;
            else if (outcome == AssaultTaskResult::Complete) {
                ++position_;
                if (position_ == original_task_count_for(*schedule_))
                    state_ = AssaultRunnerState::Completed;
            }
            return state_;
        }
        state_ = AssaultRunnerState::Failed; // malformed metadata, not game AI failure
        return state_;
    }
private:
    std::optional<AssaultSchedule> schedule_;
    std::size_t position_ = 0;
    AssaultRunnerState state_ = AssaultRunnerState::Idle;
};
} // namespace sin_compat
