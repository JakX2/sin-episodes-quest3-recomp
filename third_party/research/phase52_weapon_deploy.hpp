#pragma once
// Phase 52 narrow behavioral model. NOT Ritual source code.
// Assumptions and guard inputs are explicit; this is not a drop-in Source SDK implementation.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sin_phase52 {
constexpr int ACT_VM_DRAW = 0x9B;
constexpr int ACT_VM_HOLSTER = 0x9C;
constexpr int ACT_VM_IDLE = 0x9D;
constexpr int ACT_VM_ZOOMED_IDLE = 0xC0;

// The real selector at 0x222CB7F0 additionally evaluates owner-specific state,
// timers, a random check, and a runtime activity-name lookup. The boolean here
// represents the outcome of those not-fully-reconstructed checks; the alternate
// activity number is a caller-provided runtime lookup result, NOT a fixed ID.
inline int select_draw_activity(bool draw2_conditions_met, std::optional<int> draw2_id) {
    if (draw2_conditions_met && draw2_id && *draw2_id >= 0)
        return *draw2_id;
    return ACT_VM_DRAW;
}

enum class WeaponKind { Magnum, AssaultRifle, Scattergun };
struct DeployFields { int field_57c=0, field_584=0, field_4ec=0; };
inline void apply_slot209_prelude(WeaponKind kind, DeployFields& fields) {
    if (kind==WeaponKind::AssaultRifle) { fields.field_584=0; fields.field_57c=0; }
    else if (kind==WeaponKind::Scattergun) fields.field_57c=fields.field_4ec;
    // Magnum's original virtual slot209 points directly to the common routine.
}

struct Trace {
    std::vector<std::string> events;
    void add(const char *what) { events.emplace_back(what); }
};

struct WeaponState {
    bool zoomed = false;
    bool holster_success_marker = false; // models field at weapon +0x56C
    float last_holster_success_time = 0.0f; // models +0x574
    float next_primary_time = 0.0f; // representative slot207 timing field +0x49C
    float next_secondary_time = 0.0f; // representative slot207 timing field +0x4A0
};

// Demonstrates only the verified, guarded player path at 0x222AEC60-0x222AED60.
// Some other branches in the DLL skip this path but still perform other updates.
inline bool deploy_with_valid_player_path(WeaponState& state, Trace& trace,
                                          int activity, float now, float sequence_duration,
                                          bool owner_is_valid, bool player_busy) {
    if (!owner_is_valid || player_busy || !std::isfinite(now) ||
        !std::isfinite(sequence_duration) || sequence_duration < 0.0f) {
        trace.add("guard_stopped_player_path");
        return false;
    }
    trace.add("player_context");             // 0x222AEC67
    trace.add("configure_viewmodel");        // weapon virtual slot 199
    trace.add("request_weapon_activity");    // weapon virtual slot 197
    trace.add("read_sequence_duration");     // 0x220EEDE0
    trace.add("set_player_action_time");     // 0x22040B10
    const float until = now + sequence_duration;
    // Original updates are max-like and are subject to its own network-state logic.
    state.next_primary_time = std::max(state.next_primary_time, until);
    state.next_secondary_time = std::max(state.next_secondary_time, until);
    trace.add("update_weapon_action_times");
    trace.add("deployment_done");
    (void)activity; // used by the real virtual call; trace concerns ordering
    return true;
}

// Actual slot210 at 0x222CBDB0 calls zoom-out before base holster.
// It writes +0x56C/+0x574 only when the base function returns nonzero.
inline bool holster_wrapper(WeaponState& state, Trace& trace,
                            bool base_holster_success, float current_time) {
    trace.add("zoom_exit_called");
    state.zoomed = false;
    trace.add("request_holster_activity"); // base 0x222AED80 requests 0x9C
    trace.add("base_holster_result");
    if (!base_holster_success) return false;
    state.holster_success_marker = true;
    state.last_holster_success_time = current_time;
    trace.add("record_successful_holster");
    return true;
}
} // namespace sin_phase52
