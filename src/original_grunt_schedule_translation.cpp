#include "sin_compat/original_grunt_schedule_translation.hpp"

#include "sin_compat/original_grunt_mobility_rules.hpp"

namespace sin_compat::ai::grunt {
namespace {

constexpr GruntTranslationResult translated(std::uint8_t id) noexcept {
    return {GruntTranslationDisposition::Translated,
            OriginalGruntScheduleId{id}};
}

constexpr GruntTranslationResult delegate_to_humanoid() noexcept {
    return {GruntTranslationDisposition::DelegateToHumanoid, {}};
}

constexpr GruntTranslationResult unresolved_special() noexcept {
    return {GruntTranslationDisposition::UnresolvedSpecialCase, {}};
}

} // namespace

bool is_grunt_special_translation_input(
    OriginalGruntScheduleId input) noexcept {
    switch (input.value) {
    case 0x12:
    case 0x19:
    case 0x1A:
    case 0x21:
    case 0x24:
    case 0x29:
    case 0x2A:
    case 0x30:
    case 0x4F:
    case 0x50:
    case 0x57:
    case 0x62:
    case 0x64:
    case 0x6B:
        return true;
    default:
        return false;
    }
}

GruntTranslationResult translate_verified_grunt_schedule(
    OriginalGruntScheduleId input,
    GruntScheduleTranslationState& state) noexcept {
    switch (input.value) {
    case 0x12:
        // SCHED_GRUNT_VICTORY_DANCE.
        return translated(0x52);

    case 0x1A:
        // SCHED_GRUNT_TAKE_COVER_FROM_BEST_SOUND.
        return translated(0x5E);

    case 0x4F:
        // The source virtual's semantic name is unknown. Its nonzero result
        // selects SCHED_GRUNT_COMBAT_FAIL; zero preserves the shared input.
        return translated(state.combat_failure_virtual_result_nonzero
                              ? 0x51
                              : 0x4F);

    case 0x50:
        // SCHED_GRUNT_SUPPRESS can be promoted to SIGNAL_SUPPRESS. The
        // original clears +0xC67 only when every observed guard passes.
        if (state.suppression_enemy_resolved &&
            state.suppression_enemy_secondary_guard_passed &&
            state.suppression_latch_set &&
            state.suppression_distance_guard_passed) {
            state.suppression_latch_set = false;
            return translated(0x56);
        }
        return translated(0x50);

    case 0x30:
        // The shared input ID's symbolic name is unresolved. Its confirmed
        // outcomes are grenade+reload (0x63), mobility-controlled hide+reload
        // (0x54), or the still-unnamed shared schedule ID (0x31).
        if (state.grenade_reload_guards_passed) {
            return translated(0x63);
        }
        if (rules::grunt_mobility_decision(
                state.npc_mobility_modifier,
                rules::GruntMobilityCallSite::GruntVtable271ScheduleTranslation,
                state.mobility_random_sample)) {
            return translated(0x54);
        }
        return translated(0x31);

    case 0x64:
        // SCHED_GRUNT_PATROL becomes SCHED_GRUNT_PATROL_ENEMY only when the
        // enemy pointer resolves and its observed +0x8C field equals zero.
        if (state.patrol_enemy_resolved &&
            state.patrol_enemy_field_8c_is_zero) {
            return translated(0x67);
        }
        return translated(0x64);

    default:
        if (is_grunt_special_translation_input(input)) {
            // Other custom cases have additional guards, virtual calls, or
            // local side effects that are not yet reconstructed. In
            // particular 0x6B delegates to the parent only after side effects.
            return unresolved_special();
        }
        return delegate_to_humanoid();
    }
}

} // namespace sin_compat::ai::grunt
