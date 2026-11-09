#pragma once

// Partial, independently authored reconstruction of the original SiN
// CNPC_GruntBase slot-271 schedule translator. The numeric IDs are local to
// the pinned 2006 DLL and are not native Source/SourceVR schedule values.

#include "sin_compat/original_grunt_schedule_selector.hpp"

#include <cstdint>

namespace sin_compat::ai::grunt {

enum class GruntTranslationDisposition : std::uint8_t {
    Translated,
    DelegateToHumanoid,
    UnresolvedSpecialCase
};

struct GruntTranslationResult {
    GruntTranslationDisposition disposition =
        GruntTranslationDisposition::UnresolvedSpecialCase;
    OriginalGruntScheduleId schedule{};
};

struct GruntScheduleTranslationState {
    // Host-resolved facts for input 0x50 / SCHED_GRUNT_SUPPRESS.
    bool suppression_enemy_resolved = false;
    bool suppression_enemy_secondary_guard_passed = false;
    bool suppression_latch_set = false; // original byte at grunt +0xC67
    bool suppression_distance_guard_passed = false;

    // Host-resolved eligibility checks for shared input 0x30.
    bool grenade_reload_guards_passed = false;
    float npc_mobility_modifier = 0.0f;
    float mobility_random_sample = 0.0f;

    // Host-resolved facts for input 0x64 / SCHED_GRUNT_PATROL.
    bool patrol_enemy_resolved = false;
    bool patrol_enemy_field_8c_is_zero = false;

    // Result of the original NPC virtual decision for input 0x4F.
    bool combat_failure_virtual_result_nonzero = false;
};

bool is_grunt_special_translation_input(
    OriginalGruntScheduleId input) noexcept;

// Reconstructs only cases supported by the preserved Phase 8 report.
// Unhandled IDs in the 14-entry grunt switch are reported as
// UnresolvedSpecialCase, not silently sent to the parent. Ordinary IDs use
// DelegateToHumanoid, matching the original translator's default path.
GruntTranslationResult translate_verified_grunt_schedule(
    OriginalGruntScheduleId input,
    GruntScheduleTranslationState& state) noexcept;

} // namespace sin_compat::ai::grunt
