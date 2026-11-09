#include "sin_compat/original_alive_damage.hpp"
#include "sin_compat/original_game_rules.hpp"
#include "sin_compat/original_grunt_mobility_rules.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <vector>

using namespace sin_compat;
using namespace sin_compat::combat;
using namespace sin_compat::rules;

struct State {
    EntityRef ref{};
    std::int32_t health = 0;
    std::uint8_t life_state = 0;
};

struct DamageHost final : IOriginalCombatDamageHost {
    std::vector<State> states;
    std::vector<DamageClass> killed_classes;
    bool read_ok = true;
    bool write_ok = true;
    bool killed_ok = true;

    State* find(EntityRef ref) {
        for (auto& state : states) if (state.ref == ref) return &state;
        return nullptr;
    }
    const State* find(EntityRef ref) const {
        for (const auto& state : states) if (state.ref == ref) return &state;
        return nullptr;
    }
    bool is_live(EntityRef ref) const override { return find(ref) != nullptr; }
    bool read_health_and_life_state(EntityRef ref, std::int32_t& health,
                                    std::uint8_t& life_state) const override {
        if (!read_ok) return false;
        const auto* state = find(ref);
        if (!state) return false;
        health = state->health;
        life_state = state->life_state;
        return true;
    }
    bool write_health(EntityRef ref, std::int32_t health) override {
        if (!write_ok) return false;
        auto* state = find(ref);
        if (!state) return false;
        state->health = health;
        return true;
    }
    bool dispatch_killed(EntityRef ref, DamageClass damage_class) override {
        auto* state = find(ref);
        if (!state || !killed_ok) return false;
        killed_classes.push_back(damage_class);
        state->life_state = 1;
        return true;
    }
};

void test_verified_call_routes() {
    const auto player = verified_damage_route(DamageClass::Hl2Player);
    assert(player.count == 5);
    assert(player.stages[0] == DamageStage::Hl2PlayerTakeDamage);
    assert(player.stages[1] == DamageStage::BasePlayerTakeDamage);
    assert(player.stages[2] == DamageStage::CombatCharacterTakeDamage);
    assert(player.stages[3] == DamageStage::BasePlayerAliveDamage);
    assert(player.stages[4] == DamageStage::CommonHealthReduction);

    const auto major = verified_damage_route(DamageClass::RecombinantMajor);
    assert(major.count == 5);
    assert(major.stages[1] == DamageStage::RecombinantMajorAliveDamage);
    assert(major.stages[2] == DamageStage::MutantBaseAliveDamage);
    assert(major.stages[3] == DamageStage::AiBaseNpcAliveDamage);
    assert(major.stages[4] == DamageStage::CommonHealthReduction);

    const auto minor = verified_damage_route(DamageClass::RecombinantMinor);
    assert(minor.count == 5);
    assert(minor.stages[1] == DamageStage::RecombinantMinorAliveDamage);
    assert(minor.stages[2] == DamageStage::MutantBaseAliveDamage);
    assert(minor.stages[3] == DamageStage::AiBaseNpcAliveDamage);

    const auto npc = verified_damage_route(DamageClass::AiBaseNpc);
    assert(npc.count == 3);
    assert(npc.stages[1] == DamageStage::AiBaseNpcAliveDamage);
    assert(npc.stages[2] == DamageStage::CommonHealthReduction);
    assert(verified_damage_route(DamageClass::Unknown).count == 0);
}

void test_x87_truncate_toward_zero_and_integer_subtraction() {
    DamageHost host;
    const EntityRef victim{7, 3};
    host.states.push_back({victim, 100, 0});

    const auto fractional = apply_common_alive_damage(
        host, victim, 12.9f, DamageClass::Hl2Player);
    assert(fractional.status == DamageStatus::Applied);
    assert(fractional.old_health == 100 && fractional.integer_damage == 12);
    assert(fractional.new_health == 88 && host.find(victim)->health == 88);

    const auto sub_one = apply_common_alive_damage(
        host, victim, 0.99f, DamageClass::BasePlayer);
    assert(sub_one.status == DamageStatus::Applied);
    assert(sub_one.integer_damage == 0 && sub_one.new_health == 88);

    const auto negative = apply_common_alive_damage(
        host, victim, -2.9f, DamageClass::BasePlayer);
    assert(negative.status == DamageStatus::Applied);
    assert(negative.integer_damage == -2 && negative.new_health == 90);
}

void test_health_write_precedes_class_specific_death_dispatch() {
    DamageHost host;
    const EntityRef victim{10, 4};
    host.states.push_back({victim, 100, 0});
    const auto killed = apply_common_alive_damage(
        host, victim, 100.9f, DamageClass::RecombinantMajor);
    assert(killed.status == DamageStatus::Applied);
    assert(killed.integer_damage == 100 && killed.new_health == 0);
    assert(killed.killed_dispatch_attempted && killed.killed_dispatch_succeeded);
    assert(host.find(victim)->health == 0 && host.find(victim)->life_state == 1);
    assert(host.killed_classes.size() == 1);
    assert(host.killed_classes[0] == DamageClass::RecombinantMajor);

    const auto repeated = apply_common_alive_damage(
        host, victim, 5.0f, DamageClass::RecombinantMajor);
    assert(repeated.status == DamageStatus::AlreadyDead);
    assert(host.find(victim)->health == 0 && host.killed_classes.size() == 1);
}

void test_npc_death_virtual_is_class_specific() {
    DamageHost host;
    const EntityRef victim{11, 1};
    host.states.push_back({victim, 4, 0});
    const auto result = apply_common_alive_damage(
        host, victim, 9.0f, DamageClass::RecombinantMinor);
    assert(result.status == DamageStatus::Applied);
    assert(result.new_health == -5);
    assert(host.killed_classes.size() == 1);
    assert(host.killed_classes[0] == DamageClass::RecombinantMinor);
    assert(host.find(victim)->life_state == 1);
}

void test_invalid_damage_and_generation_safe_handles() {
    DamageHost host;
    const EntityRef victim{20, 2};
    const EntityRef recycled{20, 3};
    host.states.push_back({victim, 50, 0});
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    assert(apply_common_alive_damage(host, victim, nan, DamageClass::BasePlayer).status ==
           DamageStatus::InvalidDamage);
    assert(apply_common_alive_damage(host, victim, inf, DamageClass::BasePlayer).status ==
           DamageStatus::InvalidDamage);
    assert(apply_common_alive_damage(host, victim, 2147483648.0f, DamageClass::BasePlayer).status ==
           DamageStatus::InvalidDamage);
    assert(apply_common_alive_damage(host, victim, -2147483904.0f, DamageClass::BasePlayer).status ==
           DamageStatus::InvalidDamage);
    assert(apply_common_alive_damage(host, recycled, 1.0f, DamageClass::BasePlayer).status ==
           DamageStatus::StaleVictim);
    assert(host.find(victim)->health == 50);
}

void test_health_overflow_and_host_failures_are_transactional() {
    DamageHost host;
    const EntityRef victim{30, 1};
    host.states.push_back({victim, std::numeric_limits<std::int32_t>::min(), 0});
    const auto overflow = apply_common_alive_damage(
        host, victim, 1.0f, DamageClass::AiBaseNpc);
    assert(overflow.status == DamageStatus::HealthOverflow);
    assert(host.find(victim)->health == std::numeric_limits<std::int32_t>::min());

    host.states[0].health = 5;
    host.write_ok = false;
    const auto write_failure = apply_common_alive_damage(
        host, victim, 5.0f, DamageClass::AiBaseNpc);
    assert(write_failure.status == DamageStatus::WriteFailed);
    assert(host.find(victim)->health == 5 && host.killed_classes.empty());

    host.write_ok = true;
    host.killed_ok = false;
    const auto death_failure = apply_common_alive_damage(
        host, victim, 5.0f, DamageClass::AiBaseNpc);
    assert(death_failure.status == DamageStatus::KilledDispatchFailed);
    assert(death_failure.killed_dispatch_attempted && !death_failure.killed_dispatch_succeeded);
    assert(host.find(victim)->health == 0 && host.find(victim)->life_state == 0);

    host.read_ok = false;
    const auto read_failure = apply_common_alive_damage(
        host, victim, 1.0f, DamageClass::AiBaseNpc);
    assert(read_failure.status == DamageStatus::ReadFailed);
}

void test_unsupported_class_and_dead_state() {
    DamageHost host;
    const EntityRef victim{40, 1};
    host.states.push_back({victim, 100, 1});
    assert(apply_common_alive_damage(host, victim, 10.0f, DamageClass::Unknown).status ==
           DamageStatus::UnsupportedClass);
    assert(apply_common_alive_damage(host, victim, 10.0f, DamageClass::BasePlayer).status ==
           DamageStatus::AlreadyDead);
    assert(host.find(victim)->health == 100);
}

void test_cse1rules_damage_scaling_and_mask() {
    assert(apply_damage_done_scale(12.0f, 1.5f) == 18.0f);
    assert(apply_damage_done_scale(12.0f, 2.0f) == 24.0f);
    assert(apply_damage_taken_scale(12.0f, 0x00000002U, 0.5f) == 6.0f);
    assert(apply_damage_taken_scale(12.0f, 0x00000001U, 0.5f) == 12.0f);
    assert(apply_damage_taken_scale(12.0f, 0x00000020U, 0.5f) == 12.0f);
    assert(apply_damage_taken_scale(12.0f, 0x00020000U, 0.5f) == 12.0f);
    assert(apply_damage_taken_scale(12.0f, 0x80000000U, 0.5f) == 12.0f);
    assert(apply_damage_taken_scale(12.0f, 0x80024023U, 0.5f) == 12.0f);
}

void test_cse1rules_signed_spawn_count_adjustment() {
    assert(adjust_npc_spawn_count(10, 0.4f, 0.399f) == 11);
    assert(adjust_npc_spawn_count(10, 0.4f, 0.4f) == 10);
    assert(adjust_npc_spawn_count(10, -0.1f, 0.099f) == 9);
    assert(adjust_npc_spawn_count(10, -0.1f, 0.1f) == 10);
    assert(adjust_npc_spawn_count(10, 0.0f, 0.0f) == 10);
    assert(adjust_npc_spawn_count(10, 1.2f, 0.9f) == 11);
}

void test_cse1rules_spawn_count_uses_32_bit_add_sub() {
    assert(adjust_npc_spawn_count(
        std::numeric_limits<std::int32_t>::max(), 0.5f, 0.1f) ==
        std::numeric_limits<std::int32_t>::min());
    assert(adjust_npc_spawn_count(
        std::numeric_limits<std::int32_t>::min(), -0.5f, 0.1f) ==
        std::numeric_limits<std::int32_t>::max());
}

void test_cse1rules_npc_wait_modifier_is_additive() {
    assert(add_npc_wait_before_shoot(0.25f, 0.5f) == 0.75f);
    assert(add_npc_wait_before_shoot(0.1f, -0.5f) == -0.4f);
}


void test_cse1rules_grunt_mobility_probability() {
    assert(should_npc_be_mobile(0.5f, 1.0f, 0.1f));
    assert(!should_npc_be_mobile(0.5f, 1.0f, 0.1001f));
    assert(should_npc_be_mobile(0.5f, 4.0f, 0.4f));
    assert(!should_npc_be_mobile(0.5f, 4.0f, 0.4001f));
    assert(should_npc_be_mobile(-0.5f, 1.0f, 0.1f));
    assert(!should_npc_be_mobile(0.0f, 20.0f, 0.0f));
    // The original does not clamp the computed threshold to one.
    assert(should_npc_be_mobile(0.5f, 20.0f, 1.0f));
}

void test_cse1rules_grunt_callsite_factors() {
    assert(grunt_mobility_factor(GruntMobilityCallSite::GruntVtable292) == 1.0f);
    assert(grunt_mobility_factor(GruntMobilityCallSite::GruntVtable530InPlayerSights) == 4.0f);
    assert(grunt_mobility_factor(GruntMobilityCallSite::GruntVtable530Alternate) == 1.0f);
    assert(grunt_mobility_factor(GruntMobilityCallSite::GruntVtable271ScheduleTranslation) == 20.0f);

    assert(grunt_mobility_decision(
        0.5f, GruntMobilityCallSite::GruntVtable530Condition4D, 0.4f));
    assert(!grunt_mobility_decision(
        0.5f, GruntMobilityCallSite::GruntVtable530Alternate, 0.1001f));
    assert(grunt_mobility_decision(
        0.5f, GruntMobilityCallSite::GruntVtable271ScheduleTranslation, 1.0f));
}

int main() {
    test_verified_call_routes();
    test_x87_truncate_toward_zero_and_integer_subtraction();
    test_health_write_precedes_class_specific_death_dispatch();
    test_npc_death_virtual_is_class_specific();
    test_invalid_damage_and_generation_safe_handles();
    test_health_overflow_and_host_failures_are_transactional();
    test_unsupported_class_and_dead_state();
    test_cse1rules_damage_scaling_and_mask();
    test_cse1rules_signed_spawn_count_adjustment();
    test_cse1rules_spawn_count_uses_32_bit_add_sub();
    test_cse1rules_npc_wait_modifier_is_additive();
    test_cse1rules_grunt_mobility_probability();
    test_cse1rules_grunt_callsite_factors();
    std::puts("PASS: original SiN common alive-damage truncation, HP subtraction and death dispatch");
    std::puts("PASS: reconstructed CSE1Rules damage, spawn-count, wait and grunt mobility hooks");
    return 0;
}
