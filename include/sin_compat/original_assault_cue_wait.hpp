#pragma once

// Source-grounded, *engine-neutral* original SiN 2006 assault cue and fixed
// NPC wait reference. These two routines are decompiled from original
// server.dll x86, audited by audit_sin_assault_cue_wait.py. No SourceVR ABI.
// Original cue helper VA 0x220CEA90; wait setup/completion 0x220325C0/
// 0x220326D0. Original Windows member offsets are NOT used in class layout.

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

namespace sin_compat {

struct OriginalAssaultCueGate {
    // Original FGD: 1=Entity System Input, 2=Gunfire, 3=Don't wait for cue.
    // Values 0 and 4 appear in original code but are not named FGD choices.
    std::int32_t configured_mode = 1;
    std::int32_t received_cue = 0;

    // The host supplies the actual Source NPC numeric condition 0x33.
    // This does not prove the native C++ enum symbol corresponding to 0x33.
    // Mode 3 has a verified side effect: sets m_ReceivedAssaultCue to 3.
    bool check(bool npc_condition_0x33) {
        if (received_cue == 4) return true;
        switch (configured_mode) {
            case 1: return received_cue == 1;
            case 2: return npc_condition_0x33;
            case 3:
                received_cue = 3;
                return true;
            default:
                return false;
        }
    }
};

// Original original NPC wait setup converts a float duration to tenth-second
// ticks with x87 rounding toward positive infinity, scales by double 0.1,
// adds game curtime, and stores an absolute float NPC deadline at x86 +0x9DC.
// This reference models only the deterministic zero-spread path used by
// original AssaultBehavior tasks. It omits the unrelated random-spread path
// and the subtle original x87 extended precision / NaN corner cases.
//
// Invalid numeric values/overflow are refused as a NEW port-side choice,
// rather than guessing how an original 2006 x87 indefinite integer behaved.
inline std::optional<float> original_fixed_npc_wait_deadline(
    float curtime, float requested_seconds) {
    if (!std::isfinite(curtime) || !std::isfinite(requested_seconds))
        return std::nullopt;
    const float scaled = requested_seconds * 10.0f; // original fmul float
    if (!std::isfinite(scaled))
        return std::nullopt;
    const double tenth_ticks = std::ceil(static_cast<double>(scaled));
    if (tenth_ticks < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
        tenth_ticks > static_cast<double>(std::numeric_limits<std::int32_t>::max()))
        return std::nullopt;
    const double deadline = static_cast<double>(curtime) + tenth_ticks * 0.1;
    if (!std::isfinite(deadline) ||
        deadline > static_cast<double>(std::numeric_limits<float>::max()) ||
        deadline < -static_cast<double>(std::numeric_limits<float>::max()))
        return std::nullopt;
    return static_cast<float>(deadline);
}

inline std::optional<bool> original_fixed_npc_wait_finished(
    float curtime, float stored_deadline) {
    if (!std::isfinite(curtime) || !std::isfinite(stored_deadline))
        return std::nullopt;
    return curtime >= stored_deadline;
}

} // namespace sin_compat
