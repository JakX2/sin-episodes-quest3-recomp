#include "sin_compat/original_grunt_schedule_selector.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <optional>
#include <vector>

using namespace sin_compat::ai::grunt;

class Extension final : public IGruntSelectorExtension {
public:
    std::vector<UnrecoveredSelectorGap> calls;
    std::optional<OriginalGruntScheduleId> answer;
    std::optional<UnrecoveredSelectorGap> answer_gap;

    std::optional<OriginalGruntScheduleId> select_unrecovered(
        UnrecoveredSelectorGap gap,
        GruntSelectorState&) override {
        calls.push_back(gap);
        if (answer && answer_gap && gap == *answer_gap) return answer;
        return std::nullopt;
    }
};

void test_sitting_is_one_shot_and_preempts_conditions() {
    GruntSelectorState state;
    Extension extension;
    state.npc_flags_b6 = 0x02;
    state.conditions.set(GruntConditionId::OnFire);
    state.conditions.set(GruntConditionId::KickableGrenade);
    state.conditions.set(GruntConditionId::ShotInHelmet);

    auto result = select_verified_grunt_schedule(state, extension);
    assert(result && *result == original_schedule_id(GruntSchedule::Sitting));
    assert(state.sitting_handled_cb0);
    assert(extension.calls.empty());

    result = select_verified_grunt_schedule(state, extension);
    assert(result && *result == original_schedule_id(GruntSchedule::BurningStand));
}

void test_early_condition_priority_order() {
    GruntSelectorState state;
    Extension extension;
    state.conditions.set(GruntConditionId::OnFire);
    state.conditions.set(GruntConditionId::KickableGrenade);
    state.conditions.set(GruntConditionId::ShotInHelmet);

    auto result = select_verified_grunt_schedule(state, extension);
    assert(result && *result == original_schedule_id(GruntSchedule::BurningStand));

    state.conditions.clear(GruntConditionId::OnFire);
    result = select_verified_grunt_schedule(state, extension);
    assert(result && *result == original_schedule_id(GruntSchedule::KickGrenade));

    state.conditions.clear(GruntConditionId::KickableGrenade);
    result = select_verified_grunt_schedule(state, extension);
    assert(result && *result == original_schedule_id(GruntSchedule::HelmetFlinch));
}

void test_unrecovered_gap_can_return_original_schedule_id() {
    GruntSelectorState state;
    Extension extension;
    extension.answer = OriginalGruntScheduleId{0x55};
    extension.answer_gap =
        UnrecoveredSelectorGap::BetweenEarlyPrioritiesAndDodge;

    const auto result = select_verified_grunt_schedule(state, extension);
    assert(result && result->value == 0x55);
    assert(extension.calls.size() == 1);
    assert(extension.calls[0] ==
           UnrecoveredSelectorGap::BetweenEarlyPrioritiesAndDodge);
}

void test_shared_condition_37_selects_dodge_without_renaming_it() {
    GruntSelectorState state;
    Extension extension;
    state.has_unidentified_shared_condition_0x37 = true;

    const auto result = select_verified_grunt_schedule(state, extension);
    assert(result && *result == original_schedule_id(GruntSchedule::Dodge));
    assert(extension.calls.size() == 1);
    assert(extension.calls[0] ==
           UnrecoveredSelectorGap::BetweenEarlyPrioritiesAndDodge);
}

void test_squadmate_help_requires_both_condition_and_guard() {
    GruntSelectorState state;
    Extension extension;
    state.conditions.set(GruntConditionId::SquadmateNeedsHelp);

    auto result = select_verified_grunt_schedule(state, extension);
    assert(!result);
    assert(state.conditions.has(GruntConditionId::SquadmateNeedsHelp));

    state.squadmate_help_guard_field_nonzero = true;
    result = select_verified_grunt_schedule(state, extension);
    assert(result && *result == original_schedule_id(GruntSchedule::HelpStunned));
}

void test_help_up_clears_helpup_and_knee_before_getup() {
    GruntSelectorState state;
    Extension extension;
    state.conditions.set(GruntConditionId::HelpUp);
    state.conditions.set(GruntConditionId::ShotInKnee);

    const auto result = select_verified_grunt_schedule(state, extension);
    assert(result && *result == original_schedule_id(GruntSchedule::GetUp));
    assert(!state.conditions.has(GruntConditionId::HelpUp));
    assert(!state.conditions.has(GruntConditionId::ShotInKnee));
}

void test_knee_shot_without_helpup_selects_stunned() {
    GruntSelectorState state;
    Extension extension;
    state.conditions.set(GruntConditionId::ShotInKnee);

    const auto result = select_verified_grunt_schedule(state, extension);
    assert(result && *result == original_schedule_id(GruntSchedule::Stunned));
}

void test_helper_found_player_selects_move_to_player() {
    GruntSelectorState state;
    Extension extension;
    state.conditions.set(GruntConditionId::HelperFoundPlayer);

    const auto result = select_verified_grunt_schedule(state, extension);
    assert(result && *result == original_schedule_id(GruntSchedule::MoveToPlayer));
}

void test_patrol_requires_registered_condition_and_unrecovered_guard() {
    GruntSelectorState state;
    Extension extension;
    state.conditions.set(GruntConditionId::ShouldPatrol);

    auto result = select_verified_grunt_schedule(state, extension);
    assert(!result);
    assert(extension.calls.size() == 3);

    state.patrol_guard_satisfied = true;
    extension.calls.clear();
    result = select_verified_grunt_schedule(state, extension);
    assert(result && *result == original_schedule_id(GruntSchedule::Patrol));
}

void test_unrecovered_regions_are_visited_in_control_flow_order() {
    GruntSelectorState state;
    Extension extension;
    const auto result = select_verified_grunt_schedule(state, extension);
    assert(!result);
    assert(extension.calls.size() == 3);
    assert(extension.calls[0] ==
           UnrecoveredSelectorGap::BetweenEarlyPrioritiesAndDodge);
    assert(extension.calls[1] ==
           UnrecoveredSelectorGap::BetweenDodgeAndSquadHelp);
    assert(extension.calls[2] ==
           UnrecoveredSelectorGap::BetweenHelperFoundPlayerAndPatrol);
}

int main() {
    test_sitting_is_one_shot_and_preempts_conditions();
    test_early_condition_priority_order();
    test_unrecovered_gap_can_return_original_schedule_id();
    test_shared_condition_37_selects_dodge_without_renaming_it();
    test_squadmate_help_requires_both_condition_and_guard();
    test_help_up_clears_helpup_and_knee_before_getup();
    test_knee_shot_without_helpup_selects_stunned();
    test_helper_found_player_selects_move_to_player();
    test_patrol_requires_registered_condition_and_unrecovered_guard();
    test_unrecovered_regions_are_visited_in_control_flow_order();
    return 0;
}
