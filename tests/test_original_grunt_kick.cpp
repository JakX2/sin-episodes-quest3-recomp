#include "sin_compat/original_grunt_kick.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
using namespace sin_compat::ai::grunt;
namespace {
OriginalGruntKickEligibilityInput eligible_input() {
    OriginalGruntKickEligibilityInput i{};
    i.enemy_resolves = true; i.tracked_grenade_resolves = true;
    i.candidate_is_base_grenade = true; i.grenade_life_state = 0;
    i.grunt_origin = {0.0F,0.0F,0.0F}; i.grenade_origin = {50.0F,0.0F,0.0F};
    i.fraggrenade_radius = 250.0F; i.horizontal_alignment_dot = 0.8F;
    i.npc_solid_trace_fraction = 1.0F; i.grenade_detonate_time = 1.0F; i.current_time = 0.0F;
    return i;
}
class FakeTaskHost final : public IOriginalGruntKickTaskHost {
public:
    std::uint32_t activity = 0;
    void set_activity(std::uint32_t v) noexcept override { activity = v; }
};
class FakeKickHost final : public IOriginalGruntKickEventHost {
public:
    OriginalGruntKickPhysicsResolution resolution = OriginalGruntKickPhysicsResolution::Ready;
    OriginalGruntKickVector3 grenade_origin{0.0F,0.0F,0.0F};
    OriginalGruntKickVector3 target_origin{3.0F,4.0F,0.0F};
    OriginalGruntPhysicsObjectToken physics_object = 77U;
    std::vector<std::string> calls;
    std::vector<float> minima, maxima, random_results{0.0F,90.0F};
    std::size_t random_index = 0;
    std::int32_t melee_damage = -1;
    std::string melee_sound;
    OriginalGruntKickVector3 linear{}, angular{};
    std::uint32_t handled_event = 0;
    OriginalGruntKickPhysicsResolution resolve_physics_object(
        OriginalGruntEntityToken entity, OriginalGruntPhysicsObjectToken& object,
        OriginalGruntKickVector3& grenade, OriginalGruntKickVector3& target) noexcept override {
        calls.push_back("resolve:" + std::to_string(entity)); object=physics_object;
        grenade=grenade_origin; target=target_origin; return resolution;
    }
    float random_float(float lo,float hi) noexcept override {
        minima.push_back(lo); maxima.push_back(hi); calls.push_back("random");
        return random_results.at(random_index++);
    }
    void add_velocity(OriginalGruntPhysicsObjectToken object,const OriginalGruntKickVector3& l,
        const OriginalGruntKickVector3& a) noexcept override {
        assert(object==physics_object); calls.push_back("add_velocity"); linear=l; angular=a;
    }
    void log_missing_physics_entity() noexcept override { calls.push_back("missing_entity"); }
    void log_missing_physics_object() noexcept override { calls.push_back("missing_physics_object"); }
    void perform_melee_fallback(std::int32_t d,const char* s) noexcept override {
        calls.push_back("melee_fallback"); melee_damage=d; melee_sound=s;
    }
    void clear_physics_entity_handle() noexcept override { calls.push_back("clear_handle"); }
    void record_last_anim_event_handled(std::uint32_t e) noexcept override {
        calls.push_back("record_event"); handled_event=e;
    }
};
void test_constants() {
    assert(original_grunt_task_kick_attack==0x9AU);
    assert(original_grunt_schedule_kick_grenade==0x73U);
    assert(original_grunt_kick_trace_mask_npc_solid==0x0202400BU);
    assert(original_grunt_kick_default_damage==10);
    assert(original_grunt_kick_default_fraggrenade_radius==250.0F);
}
void test_eligibility_default_passes() {
    const auto r=evaluate_original_grunt_kick_eligibility(eligible_input());
    assert(r.eligible() && r.failure==OriginalGruntKickEligibilityFailure::None);
    assert(r.measured_distance==50.0F && r.maximum_distance==125.0F && r.remaining_fuse==1.0F);
}
void test_enemy_and_grenade_required_in_order() {
    auto i=eligible_input(); i.enemy_resolves=false;
    assert(evaluate_original_grunt_kick_eligibility(i).failure==OriginalGruntKickEligibilityFailure::MissingEnemy);
    i.enemy_resolves=true; i.tracked_grenade_resolves=false;
    assert(evaluate_original_grunt_kick_eligibility(i).failure==OriginalGruntKickEligibilityFailure::MissingTrackedGrenade);
}
void test_vertical_limit_inclusive() {
    auto i=eligible_input(); i.grenade_origin.z=12.0F;
    assert(evaluate_original_grunt_kick_eligibility(i).eligible());
    i.grenade_origin.z=12.001F;
    assert(evaluate_original_grunt_kick_eligibility(i).failure==OriginalGruntKickEligibilityFailure::VerticalSeparation);
}
void test_distance_strictly_below_half_radius() {
    auto i=eligible_input(); i.grenade_origin={124.999F,0.0F,0.0F};
    assert(evaluate_original_grunt_kick_eligibility(i).eligible());
    i.grenade_origin.x=125.0F;
    assert(evaluate_original_grunt_kick_eligibility(i).failure==OriginalGruntKickEligibilityFailure::Distance);
    i.fraggrenade_radius=300.0F; assert(evaluate_original_grunt_kick_eligibility(i).eligible());
}
void test_alignment_dot_inclusive() {
    auto i=eligible_input(); i.horizontal_alignment_dot=0.8F;
    assert(evaluate_original_grunt_kick_eligibility(i).eligible());
    i.horizontal_alignment_dot=0.7999F;
    assert(evaluate_original_grunt_kick_eligibility(i).failure==OriginalGruntKickEligibilityFailure::DirectionAlignment);
}
void test_trace_fraction_exactly_one() {
    auto i=eligible_input(); i.npc_solid_trace_fraction=1.0F;
    assert(evaluate_original_grunt_kick_eligibility(i).eligible());
    i.npc_solid_trace_fraction=0.999F;
    assert(evaluate_original_grunt_kick_eligibility(i).failure==OriginalGruntKickEligibilityFailure::ObstructedTrace);
}
void test_class_and_life_state() {
    auto i=eligible_input(); i.candidate_is_base_grenade=false;
    assert(evaluate_original_grunt_kick_eligibility(i).failure==OriginalGruntKickEligibilityFailure::WrongGrenadeClass);
    i.candidate_is_base_grenade=true; i.grenade_life_state=1;
    assert(evaluate_original_grunt_kick_eligibility(i).failure==OriginalGruntKickEligibilityFailure::GrenadeNotAlive);
}
void test_fuse_strictly_greater_than_half_second() {
    auto i=eligible_input(); i.grenade_detonate_time=0.5F;
    assert(evaluate_original_grunt_kick_eligibility(i).failure==OriginalGruntKickEligibilityFailure::FuseTooShort);
    i.grenade_detonate_time=0.5001F; assert(evaluate_original_grunt_kick_eligibility(i).eligible());
}
void test_schedule_transfers_handle_only_after_eligibility() {
    OriginalGruntKickState s{}; s.current_grenade=42U; auto i=eligible_input();
    const auto id=prepare_original_grunt_kick_schedule(s,i);
    assert(id && *id==0x73U && !s.current_grenade && s.physics_entity && *s.physics_entity==42U);
    OriginalGruntKickState rejected{}; rejected.current_grenade=43U; i.npc_solid_trace_fraction=0.5F;
    assert(!prepare_original_grunt_kick_schedule(rejected,i));
    assert(rejected.current_grenade && *rejected.current_grenade==43U && !rejected.physics_entity);
}
void test_task_start_stores_time_and_registered_activity() {
    OriginalGruntKickState s{}; FakeTaskHost h;
    start_original_grunt_kick_task(s,12.25F,0x1234U,h);
    assert(s.last_kick_task_time==12.25F && h.activity==0x1234U);
}
void test_event_three_computes_velocity_spin_and_clears() {
    OriginalGruntKickState s{}; s.physics_entity=99U; FakeKickHost h;
    const auto r=handle_original_grunt_kick_animation_event(s,3U,h);
    assert(r==OriginalGruntKickEventResult::PhysicsVelocityApplied);
    assert(std::fabs(h.linear.x-480.0F)<0.001F && std::fabs(h.linear.y-640.0F)<0.001F);
    assert(std::fabs(h.linear.z-200.0F)<0.001F);
    assert(h.angular.x==0.0F && h.angular.y==90.0F && h.angular.z==20.0F);
    assert((h.minima==std::vector<float>{-360.0F,-180.0F}));
    assert((h.maxima==std::vector<float>{360.0F,180.0F}));
    assert((h.calls==std::vector<std::string>{"resolve:99","random","random","add_velocity","clear_handle","record_event"}));
    assert(!s.physics_entity && h.handled_event==3U);
}
void test_missing_entity_uses_melee_fallback() {
    OriginalGruntKickState s{}; s.physics_entity=99U; s.kick_damage=17; FakeKickHost h;
    h.resolution=OriginalGruntKickPhysicsResolution::MissingEntity;
    assert(handle_original_grunt_kick_animation_event(s,3U,h)==OriginalGruntKickEventResult::MeleeFallbackDispatched);
    assert(h.melee_damage==17 && h.melee_sound=="NPC_Grunt.WeaponBash" && h.handled_event==3U);
    assert(s.physics_entity && *s.physics_entity==99U); // cleanup on stale-handle path remains unverified
}
void test_absent_handle_uses_melee_fallback() {
    OriginalGruntKickState s{}; FakeKickHost h;
    assert(handle_original_grunt_kick_animation_event(s,3U,h)==OriginalGruntKickEventResult::MeleeFallbackDispatched);
    assert(h.melee_damage==10);
    assert((h.calls==std::vector<std::string>{"missing_entity","melee_fallback","record_event"}));
}
void test_missing_physics_object_remains_unresolved() {
    OriginalGruntKickState s{}; s.physics_entity=99U; FakeKickHost h;
    h.resolution=OriginalGruntKickPhysicsResolution::MissingPhysicsObject;
    assert(handle_original_grunt_kick_animation_event(s,3U,h)==OriginalGruntKickEventResult::MissingPhysicsObjectUnresolved);
    assert((h.calls==std::vector<std::string>{"resolve:99","missing_physics_object"}));
    assert(s.physics_entity);
}
void test_zero_length_direction_is_not_fabricated() {
    OriginalGruntKickState s{}; s.physics_entity=99U; FakeKickHost h; h.target_origin=h.grenade_origin;
    assert(handle_original_grunt_kick_animation_event(s,3U,h)==OriginalGruntKickEventResult::ZeroLengthDirectionUnresolved);
    assert(h.random_index==0U && h.calls==std::vector<std::string>{"resolve:99"} && s.physics_entity);
}
void test_other_events_delegate_to_parent() {
    OriginalGruntKickState s{}; s.physics_entity=99U; FakeKickHost h;
    assert(handle_original_grunt_kick_animation_event(s,4U,h)==OriginalGruntKickEventResult::NotHandled);
    assert(h.calls.empty() && s.physics_entity);
}
}
int main() {
    test_constants(); test_eligibility_default_passes(); test_enemy_and_grenade_required_in_order();
    test_vertical_limit_inclusive(); test_distance_strictly_below_half_radius(); test_alignment_dot_inclusive();
    test_trace_fraction_exactly_one(); test_class_and_life_state(); test_fuse_strictly_greater_than_half_second();
    test_schedule_transfers_handle_only_after_eligibility(); test_task_start_stores_time_and_registered_activity();
    test_event_three_computes_velocity_spin_and_clears(); test_missing_entity_uses_melee_fallback();
    test_absent_handle_uses_melee_fallback(); test_missing_physics_object_remains_unresolved();
    test_zero_length_direction_is_not_fabricated(); test_other_events_delegate_to_parent();
    std::puts("PASS: original SiN grunt grenade-kick eligibility, task start and event-3 host contract (mock host only)");
    return 0;
}
