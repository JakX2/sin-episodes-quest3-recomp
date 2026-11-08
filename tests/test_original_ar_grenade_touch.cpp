#include "sin_compat/original_ar_grenade_touch.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>

using namespace sin_compat;
using namespace sin_compat::ar_grenade_touch;

struct TestHost final : ar_grenade::IHost {
    int trace=0,shake=0,damage=0,combat=0,removed=0,flight=0;
    void on_flight_ai_sound(const ar_grenade::AiSoundEvent&) override {++flight;}
    void on_original_trace_effect(ar_grenade::Decal) override {++trace;}
    void on_screen_shake_feedback(const ar_grenade::ScreenShakeEvent&) override {++shake;}
    void on_radius_damage(const ar_grenade::BlastDamageEvent&) override {++damage;}
    void on_explosion_ai_sound(const ar_grenade::AiSoundEvent&) override {++combat;}
    void on_remove_projectile() override {++removed;}
};

int main() {
    ar_grenade::Runtime projectile{};
    TestHost host{};
    assert(projectile.spawn({10,20,30},{1,2,3},1.0f));

    OriginalTouchObservation o{};
    auto decision=evaluate(o);
    assert(decision.gate==Gate::BlockedNoOtherEntity);
    assert(!decision.original_flag_795_set_requested);
    assert(dispatch(o,projectile,ar_grenade::Decal::Scorch,host).projectile_step==
           ar_grenade::StepResult::TriggerRejected);

    o.other_entity_present=true;
    decision=evaluate(o);
    assert(decision.gate==Gate::BlockedOtherField11cZero);
    o.other_field_11c=1u; // raw original field, not an inferred entity type
    o.other_flags_120=4u;
    decision=evaluate(o);
    assert(decision.gate==Gate::BlockedOtherFlag120Bit4);
    o.other_flags_120=0u;
    decision=evaluate(o);
    assert(decision.gate==Gate::BlockedOriginalFlag795Zero);

    // Non-null other vtable+0xFC reference equals original local helper:
    // do not set +0x795. With the spawn flag clear, no detonation follows.
    o.other_virtual_fc_result_nonnull=true;
    o.other_virtual_fc_matches_local_helper=true;
    decision=evaluate(o);
    assert(decision.gate==Gate::BlockedOriginalFlag795Zero);
    assert(!decision.original_flag_795_set_requested);

    // Distinct, non-null references invoke the original byte flag setter.
    o.other_virtual_fc_matches_local_helper=false;
    decision=evaluate(o);
    assert(decision.should_dispatch_detonation());
    assert(decision.original_flag_795_set_requested);
    assert(decision.original_flag_795_after_contact);
    const auto result=dispatch(o,projectile,ar_grenade::Decal::Scorch,host);
    assert(result.projectile_step==ar_grenade::StepResult::Applied);
    assert(result.evaluation.original_flag_795_set_requested);
    assert(!projectile.armed() && host.damage==1 && host.removed==1);
    assert(host.trace==1 && host.shake==1 && host.combat==1);
    assert(dispatch(o,projectile,ar_grenade::Decal::Scorch,host).projectile_step==
           ar_grenade::StepResult::Disarmed);
    assert(host.damage==1 && host.removed==1);

    // Original flag can already be nonzero after another original path,
    // so a null / equal other reference does NOT inherently block detonation.
    assert(projectile.spawn({0,0,0},{0,0,0},2.0f));
    o.other_virtual_fc_result_nonnull=false;
    o.other_virtual_fc_matches_local_helper=true;
    o.grenade_original_field_795=true;
    decision=evaluate(o);
    assert(decision.should_dispatch_detonation());
    assert(!decision.original_flag_795_set_requested);
    assert(dispatch(o,projectile,ar_grenade::Decal::None,host).projectile_step==
           ar_grenade::StepResult::Applied);
    assert(host.damage==2 && host.removed==2);

    // Pre-filter is decisive even when original +0x795 is set already.
    o.other_flags_120=4u;
    assert(evaluate(o).gate==Gate::BlockedOtherFlag120Bit4);
    assert(!evaluate(o).should_dispatch_detonation());

    // The old +0x795 byte is NOT the same as Runtime::armed(): a new
    // portable projectile is eligible for a future shot, but original
    // spawn clears +0x795 until source-observed conditions set it.
    assert(projectile.spawn({0,0,0},{0,0,0},3.0f));
    o.grenade_original_field_795=false;
    o.other_flags_120=0u;
    assert(projectile.armed());
    assert(evaluate(o).gate==Gate::BlockedOriginalFlag795Zero);
    assert(dispatch(o,projectile,ar_grenade::Decal::None,host).projectile_step==
           ar_grenade::StepResult::TriggerRejected);
    assert(host.damage==2 && projectile.armed());

    std::puts("PASS: original SiN grenade touch contact/flag gates and one-shot blast (mock host only)");
    return 0;
}
