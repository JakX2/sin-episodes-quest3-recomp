#include "sin_compat/original_grunt_schedule_selector.hpp"

namespace sin_compat::ai::grunt {
namespace {

std::optional<OriginalGruntScheduleId> extension_result(
    IGruntSelectorExtension& extension,
    UnrecoveredSelectorGap gap,
    GruntSelectorState& state) {
    return extension.select_unrecovered(gap, state);
}

} // namespace

std::optional<OriginalGruntScheduleId> select_verified_grunt_schedule(
    GruntSelectorState& state,
    IGruntSelectorExtension& extension) {
    const auto selected = [](GruntSchedule schedule) {
        return std::optional<OriginalGruntScheduleId>{
            original_schedule_id(schedule)};
    };

    // Original 0x2235E4E0 first-priority branch. The byte/field labels are
    // descriptive: the binary evidence establishes offsets, not C++ names.
    if ((state.npc_flags_b6 & 0x02U) != 0U &&
        !state.sitting_handled_cb0) {
        state.sitting_handled_cb0 = true;
        return selected(GruntSchedule::Sitting);
    }

    // Confirmed registration/selector pairs: 0x46 -> 0x68,
    // 0x4C -> 0x73, and 0x48 -> 0x75, in this priority order.
    if (state.conditions.has(GruntConditionId::OnFire)) {
        return selected(GruntSchedule::BurningStand);
    }
    if (state.conditions.has(GruntConditionId::KickableGrenade)) {
        return selected(GruntSchedule::KickGrenade);
    }
    if (state.conditions.has(GruntConditionId::ShotInHelmet)) {
        return selected(GruntSchedule::HelmetFlinch);
    }

    if (const auto result = extension_result(
            extension,
            UnrecoveredSelectorGap::BetweenEarlyPrioritiesAndDodge,
            state)) {
        return result;
    }

    // Original 0x2235E591 tests shared condition 0x37 and returns 0x6D.
    // Its condition name is intentionally left unresolved.
    if (state.has_unidentified_shared_condition_0x37) {
        return selected(GruntSchedule::Dodge);
    }

    if (const auto result = extension_result(
            extension,
            UnrecoveredSelectorGap::BetweenDodgeAndSquadHelp,
            state)) {
        return result;
    }

    // Original 0x2235E6FB: squadmate-needs-help plus a nonzero NPC field.
    if (state.conditions.has(GruntConditionId::SquadmateNeedsHelp) &&
        state.squadmate_help_guard_field_nonzero) {
        return selected(GruntSchedule::HelpStunned);
    }

    // Original 0x2235E719 clears both conditions before returning 0x71.
    if (state.conditions.has(GruntConditionId::HelpUp)) {
        state.conditions.clear(GruntConditionId::HelpUp);
        state.conditions.clear(GruntConditionId::ShotInKnee);
        return selected(GruntSchedule::GetUp);
    }

    // Without the help-up condition, a knee-shot condition returns 0x6B.
    if (state.conditions.has(GruntConditionId::ShotInKnee)) {
        return selected(GruntSchedule::Stunned);
    }

    if (state.conditions.has(GruntConditionId::HelperFoundPlayer)) {
        return selected(GruntSchedule::MoveToPlayer);
    }

    if (const auto result = extension_result(
            extension,
            UnrecoveredSelectorGap::BetweenHelperFoundPlayerAndPatrol,
            state)) {
        return result;
    }

    // The preserved trace identifies the patrol condition, but records an
    // additional guard. The host supplies that guard without guessing its
    // original private field name or offset.
    if (state.conditions.has(GruntConditionId::ShouldPatrol) &&
        state.patrol_guard_satisfied) {
        return selected(GruntSchedule::Patrol);
    }

    // Original 0x2235E8B0 tail-jumps to CAI_BaseHumanoid slot 269 at
    // 0x22089700. The caller performs that engine-specific parent fallback.
    return std::nullopt;
}

} // namespace sin_compat::ai::grunt
