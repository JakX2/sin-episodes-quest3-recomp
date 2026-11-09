#include "sin_compat/original_grunt_schedule_translation.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <initializer_list>

using namespace sin_compat::ai::grunt;

namespace {

GruntTranslationResult translate(std::uint8_t input,
                                 GruntScheduleTranslationState& state) {
    return translate_verified_grunt_schedule(
        OriginalGruntScheduleId{input}, state);
}

void expect_schedule(const GruntTranslationResult& result,
                     std::uint8_t schedule) {
    assert(result.disposition == GruntTranslationDisposition::Translated);
    assert(result.schedule.value == schedule);
}

void test_direct_registered_schedule_cases() {
    GruntScheduleTranslationState state;
    expect_schedule(translate(0x12, state), 0x52);
    expect_schedule(translate(0x1A, state), 0x5E);
}

void test_suppression_promotion_requires_every_observed_guard() {
    GruntScheduleTranslationState state;
    state.suppression_enemy_resolved = true;
    state.suppression_enemy_secondary_guard_passed = true;
    state.suppression_latch_set = true;
    state.suppression_distance_guard_passed = true;

    expect_schedule(translate(0x50, state), 0x56);
    assert(!state.suppression_latch_set);

    state.suppression_latch_set = true;
    state.suppression_distance_guard_passed = false;
    expect_schedule(translate(0x50, state), 0x50);
    assert(state.suppression_latch_set);

    state.suppression_distance_guard_passed = true;
    state.suppression_enemy_secondary_guard_passed = false;
    expect_schedule(translate(0x50, state), 0x50);
    assert(state.suppression_latch_set);

    state.suppression_enemy_secondary_guard_passed = true;
    state.suppression_enemy_resolved = false;
    expect_schedule(translate(0x50, state), 0x50);
    assert(state.suppression_latch_set);
}

void test_grenade_reload_preempts_difficulty_decision() {
    GruntScheduleTranslationState state;
    state.grenade_reload_guards_passed = true;
    state.npc_mobility_modifier = 0.5f;
    state.mobility_random_sample = 1.0f;
    expect_schedule(translate(0x30, state), 0x63);
}

void test_schedule_30_uses_factor_20_mobility_rule() {
    GruntScheduleTranslationState state;
    state.npc_mobility_modifier = 0.5f;
    state.mobility_random_sample = 1.0f;
    expect_schedule(translate(0x30, state), 0x54);

    state.npc_mobility_modifier = 0.0f;
    expect_schedule(translate(0x30, state), 0x31);
}

void test_patrol_becomes_patrol_enemy_only_for_resolved_enemy_and_zero_field() {
    GruntScheduleTranslationState state;
    state.patrol_enemy_resolved = true;
    state.patrol_enemy_field_8c_is_zero = true;
    expect_schedule(translate(0x64, state), 0x67);

    state.patrol_enemy_field_8c_is_zero = false;
    expect_schedule(translate(0x64, state), 0x64);

    state.patrol_enemy_resolved = false;
    state.patrol_enemy_field_8c_is_zero = true;
    expect_schedule(translate(0x64, state), 0x64);
}

void test_combat_failure_virtual_result_selects_fallback() {
    GruntScheduleTranslationState state;
    expect_schedule(translate(0x4F, state), 0x4F);
    state.combat_failure_virtual_result_nonzero = true;
    expect_schedule(translate(0x4F, state), 0x51);
}

void test_unrecovered_special_cases_are_not_misrouted_to_parent() {
    constexpr std::uint8_t special[] = {
        0x12, 0x19, 0x1A, 0x21, 0x24, 0x29, 0x2A,
        0x30, 0x4F, 0x50, 0x57, 0x62, 0x64, 0x6B
    };
    for (const auto id : special) {
        assert(is_grunt_special_translation_input(
            OriginalGruntScheduleId{id}));
    }

    GruntScheduleTranslationState state;
    for (const auto id : {0x19, 0x21, 0x24, 0x29, 0x2A, 0x57, 0x62, 0x6B}) {
        const auto result = translate(static_cast<std::uint8_t>(id), state);
        assert(result.disposition ==
               GruntTranslationDisposition::UnresolvedSpecialCase);
    }
}

void test_ordinary_inputs_delegate_to_humanoid_translator() {
    GruntScheduleTranslationState state;
    for (const auto id : {0x00, 0x11, 0x13, 0x55, 0x6A, 0x70, 0xFF}) {
        const auto result = translate(static_cast<std::uint8_t>(id), state);
        assert(result.disposition ==
               GruntTranslationDisposition::DelegateToHumanoid);
    }
}

} // namespace

int main() {
    test_direct_registered_schedule_cases();
    test_suppression_promotion_requires_every_observed_guard();
    test_grenade_reload_preempts_difficulty_decision();
    test_schedule_30_uses_factor_20_mobility_rule();
    test_patrol_becomes_patrol_enemy_only_for_resolved_enemy_and_zero_field();
    test_combat_failure_virtual_result_selects_fallback();
    test_unrecovered_special_cases_are_not_misrouted_to_parent();
    test_ordinary_inputs_delegate_to_humanoid_translator();
    return 0;
}
