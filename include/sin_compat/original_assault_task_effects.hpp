#pragma once

// A deliberately small, engine-neutral adapter for original SiN 2006
// CAI_AssaultBehavior task-side field changes recovered at these x86 sites:
// TASK_HIT_RALLY_POINT: 0x220CF539 / 0x220CF556
// TASK_HIT_ASSAULT_POINT: 0x220CF44A / 0x220CF467
// TASK_WAIT_ASSAULT_DELAY: 0x220CF617 / 0x220CF621
// TASK_AWAIT_ASSAULT_TIMEOUT: 0x220CF663 / 0x220CF66D
// TASK_ASSAULT_DEFER_SCHEDULE_SELECTION: 0x220CF582..0x220CF593
// These are portable event/field *effects*, not original full task handlers.
// Do not register them as native SourceVR tasks or copy x86 object offsets.

#include <cmath>
#include <optional>
#include <utility>

namespace sin_compat {

enum class OriginalAssaultArrivalOutput {
    RallyOnArrival,
    AssaultOnArrival,
};

struct OriginalAssaultTaskEffectState {
    bool hit_rally_point = false;
    bool hit_assault_point = false;
    std::optional<float> defer_schedule_selection_until;

    // Host must verify the real Source entity handle. Missing original
    // entity paths are not yet reconstructed; return false rather than
    // fire a speculative output or falsely declare a native task complete.
    template <typename FireOutput>
    bool mark_rally_arrived_if_resolved(bool rallypoint_resolved,
                                       FireOutput&& fire_output) {
        if (!rallypoint_resolved) return false;
        hit_rally_point = true;
        std::forward<FireOutput>(fire_output)(
            OriginalAssaultArrivalOutput::RallyOnArrival);
        return true;
    }

    template <typename FireOutput>
    bool mark_assault_arrived_if_resolved(bool assaultpoint_resolved,
                                         FireOutput&& fire_output) {
        if (!assaultpoint_resolved) return false;
        hit_assault_point = true;
        std::forward<FireOutput>(fire_output)(
            OriginalAssaultArrivalOutput::AssaultOnArrival);
        return true;
    }

    // Source x86 copies configured fields m_flAssaultDelay +0x39C and
    // m_flAssaultTimeout +0x3B4 into its NPC wait routine, rather than
    // substituting the unrelated hard-coded 4-second diversion threshold.
    // Host must actually start/advance its engine wait state.
    static std::optional<float> rally_delay_for_host(
        bool rallypoint_resolved, float configured_seconds) {
        if (!rallypoint_resolved || !std::isfinite(configured_seconds))
            return std::nullopt;
        return configured_seconds;
    }

    static std::optional<float> assault_timeout_for_host(
        bool assaultpoint_resolved, float configured_seconds) {
        if (!assaultpoint_resolved || !std::isfinite(configured_seconds))
            return std::nullopt;
        return configured_seconds;
    }

    // Native StartTask assembly adds the original float task argument to
    // global curtime and stores the result in behavior+0x2C.
    // Finite-input guarding is a NEW port-side safety choice, not a
    // recovered original Source x87 corner-case rule.
    bool defer_schedule_selection(float curtime, float task_argument) {
        if (!std::isfinite(curtime) || !std::isfinite(task_argument))
            return false;
        const float deadline = curtime + task_argument;
        if (!std::isfinite(deadline)) return false;
        defer_schedule_selection_until = deadline;
        return true;
    }
};

} // namespace sin_compat
