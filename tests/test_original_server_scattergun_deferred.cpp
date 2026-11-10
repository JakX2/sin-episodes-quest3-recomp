#include "sin_compat/original_server_scattergun_deferred.hpp"
#include "sin_compat/original_server_scattergun_ammo.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace d = sin_compat::original_server_scattergun_deferred;
namespace a = sin_compat::original_server_scattergun_ammo;
namespace {
struct FakeHost {
    d::ScatterGunState& state;
    d::OwnerState* owner=nullptr;
    std::vector<std::string> calls{};
    float first_clock=10.0f;
    float second_clock=12.0f;
    float interval=2.0f;
    std::uint32_t selector=0x4545u;
    unsigned clock_reads=0;
    bool switch_to_tracking=false;

    float read_game_time() {
        calls.emplace_back("time");
        return clock_reads++==0?first_clock:second_clock;
    }
    d::OwnerState* find_owner() {
        calls.emplace_back("owner");
        return owner;
    }
    void dispatch_state_event(std::uint32_t code,std::uint32_t arg) {
        calls.emplace_back("event");
        assert(code==13u && arg==0u);
        assert(state.pending_byte_589==0u);
        if(switch_to_tracking)state.secondary_track_578=1u;
    }
    std::uint32_t tracked_selection(std::uint32_t address) {
        calls.emplace_back("lookup");
        assert(address==d::kTrackedSelectorDataVA);
        return selector;
    }
    void dispatch_selection(std::uint32_t value) {
        calls.emplace_back("selection");
        assert(value==(state.secondary_track_578!=0u
            ?selector:d::kDefaultUntrackedSelection));
    }
    float timing_interval(std::uint32_t original_word) {
        calls.emplace_back("interval");
        assert(original_word==state.timing_arg_3ec);
        return interval;
    }
};
void test_float_setters_equal_values_and_sign_of_zero() {
    float value=0.0f;
    std::uint32_t flags=0x10u;
    d::ChangeNotification n{};
    n.external_flags_16c=&flags;
    assert(!d::original_tracked_float_assign(value,-0.0f,n));
    assert(flags==0x10u && !std::signbit(value));
    assert(d::original_tracked_float_assign(value,3.5f,n));
    assert(value==3.5f && flags==0x11u);
    n.mode_byte_1f0=9;
    n.dirty_byte_200=0;
    flags=0x10;
    assert(d::original_tracked_float_assign(value,-7.0f,n));
    assert(value==-7.0f && n.dirty_byte_200==1);
    assert(flags==0x10);
    n.mode_byte_1f0=0;n.dirty_byte_200=0;
    n.external_flags_16c=nullptr;
    assert(d::original_tracked_float_assign(value,-8.0f,n));
    assert(n.dirty_byte_200==0 && value==-8.0f);
    const float nan=std::numeric_limits<float>::quiet_NaN();
    value=nan;
    n.mode_byte_1f0=1;
    assert(d::original_tracked_float_assign(value,nan,n));
    assert(n.dirty_byte_200==1 && std::isnan(value));
}
void test_no_pending_never_queries_external_state() {
    d::ScatterGunState s{};
    FakeHost host{s};
    assert(d::original_scattergun_process_deferred(s,host)==
           d::DeferredOutcome::NoPending);
    assert(host.calls.empty());
}
void test_gate_due_check_and_original_signed_clip() {
    d::ScatterGunState s{};
    s.pending_byte_589=1;
    s.primary_clip_4ec=5;
    s.gate_timer_49c=11.0f;
    FakeHost host{s};host.first_clock=10.0f;
    assert(d::original_scattergun_process_deferred(s,host)==
           d::DeferredOutcome::ScheduledForLater);
    assert((host.calls==std::vector<std::string>{"time"}));
    assert(s.pending_byte_589==1u);
    for(auto clip:{0u,0x80000000u,0xffffffffu}) {
        s.primary_clip_4ec=clip;
        s.gate_timer_49c=10.0f;
        FakeHost next{s};
        assert(d::original_scattergun_process_deferred(s,next)==
               d::DeferredOutcome::NonpositivePrimaryClip);
        assert((next.calls==std::vector<std::string>{"time"}));
        assert(s.pending_byte_589==1u);
    }
    s.primary_clip_4ec=1;s.blocking_byte_4ac=2;
    FakeHost blocked{s};
    assert(d::original_scattergun_process_deferred(s,blocked)==
           d::DeferredOutcome::Blocked);
    assert((blocked.calls==std::vector<std::string>{"time"}));
    s.blocking_byte_4ac=0;
    FakeHost missing{s};
    assert(d::original_scattergun_process_deferred(s,missing)==
           d::DeferredOutcome::MissingOwner);
    assert((missing.calls==std::vector<std::string>{"time","owner"}));
    assert(s.pending_byte_589==1);
}
void test_complete_untracked_event_timer_and_notifications() {
    d::ScatterGunState s{};
    s.pending_byte_589=1;
    s.primary_clip_4ec=5;
    s.gate_timer_49c=4.0f;
    s.mirrored_timer_4a0=3.0f;
    s.timing_arg_3ec=0xBBu;
    std::uint32_t weapon_flags=0x20u;
    s.notify.external_flags_16c=&weapon_flags;
    d::OwnerState owner{};
    owner.float_timer_60c=2.0f;
    std::uint32_t owner_flags=0x80u;
    owner.notify.external_flags_16c=&owner_flags;
    FakeHost host{s};host.owner=&owner;
    host.first_clock=9.0f;host.second_clock=15.0f;
    host.interval=2.25f;
    assert(d::original_scattergun_process_deferred(s,host)==
           d::DeferredOutcome::Dispatched);
    assert((host.calls==std::vector<std::string>{
        "time","owner","event","selection","interval","time"}));
    assert(host.clock_reads==2u);
    assert(s.pending_byte_589==0u);
    assert(owner.float_timer_60c==17.25f);
    assert(s.gate_timer_49c==17.25f && s.mirrored_timer_4a0==17.25f);
    assert(owner_flags==0x81u && weapon_flags==0x21u);
}
void test_tracking_branch_uses_original_opaque_looked_up_id() {
    d::ScatterGunState s{};
    s.pending_byte_589=1;
    s.primary_clip_4ec=0x7fffffffu;
    s.timing_arg_3ec=7;
    d::OwnerState owner{};
    owner.float_timer_60c=4.0f;
    s.notify.mode_byte_1f0=5;
    owner.notify.mode_byte_1f0=5;
    std::uint32_t unrelated_flags=0x90u;
    s.notify.external_flags_16c=&unrelated_flags;
    FakeHost host{s};
    host.owner=&owner;
    host.switch_to_tracking=true; // code checks byte AFTER virtual event
    host.selector=0xABCDEF12u;
    host.interval=0.5f;
    assert(d::original_scattergun_process_deferred(s,host)==
           d::DeferredOutcome::Dispatched);
    assert((host.calls==std::vector<std::string>{
        "time","owner","event","lookup","selection","interval","time"}));
    assert(s.notify.dirty_byte_200==1u && owner.notify.dirty_byte_200==1u);
    assert(unrelated_flags==0x90u);
    assert(owner.float_timer_60c==12.5f);
}
void test_equal_owner_deadline_preserves_negative_zero_when_copied() {
    d::ScatterGunState s{};
    s.primary_clip_4ec=1;s.pending_byte_589=1;
    s.gate_timer_49c=-0.0f;
    s.mirrored_timer_4a0=10.0f;
    d::OwnerState owner{};
    owner.float_timer_60c=-0.0f;
    FakeHost host{s};host.owner=&owner;
    host.first_clock=0.0f;host.second_clock=0.0f;
    host.interval=0.0f;
    assert(d::original_scattergun_process_deferred(s,host)==
           d::DeferredOutcome::Dispatched);
    // +0 and -0 compare equal in x87 FUCOMPP, so the original owner
    // does not overwrite its -0 bits with the incoming +0 bits.
    assert(std::signbit(owner.float_timer_60c));
    assert(std::signbit(s.gate_timer_49c));
    assert(!std::signbit(s.mirrored_timer_4a0));
    assert(owner.notify.dirty_byte_200==0);
}
void test_x87_unordered_deadline_does_not_suppress_dispatch() {
    d::ScatterGunState s{};
    s.primary_clip_4ec=1;s.pending_byte_589=1;
    s.gate_timer_49c=std::numeric_limits<float>::quiet_NaN();
    d::OwnerState owner{};
    FakeHost host{s};host.owner=&owner;
    assert(d::original_scattergun_process_deferred(s,host)==
           d::DeferredOutcome::Dispatched);
    assert(s.pending_byte_589==0);
    // +infinity, in contrast, suppresses when clock is finite.
    s.pending_byte_589=1;
    s.gate_timer_49c=std::numeric_limits<float>::infinity();
    FakeHost next{s};next.owner=&owner;
    assert(d::original_scattergun_process_deferred(s,next)==
           d::DeferredOutcome::ScheduledForLater);
}
void test_phase165_snapshot_tail_dispatch_integration() {
    a::ObservedAmmoState old{};
    old.clip_4ec=5;old.prior_clip_57c=6;
    d::ScatterGunState s{};
    s.primary_clip_4ec=old.clip_4ec;
    s.gate_timer_49c=0.0f;
    d::OwnerState owner{};
    FakeHost host{s};host.owner=&owner;
    int base_calls=0,continuations=0;
    a::original_scattergun_update_ammo(old,
        [&]{++base_calls;},
        [&]{
            ++continuations;
            s.pending_byte_589=old.change_pending_589;
            assert(d::original_scattergun_process_deferred(s,host)==
                   d::DeferredOutcome::Dispatched);
            old.change_pending_589=s.pending_byte_589;
        });
    assert(base_calls==1 && continuations==1);
    assert(s.pending_byte_589==0 && old.change_pending_589==0);
    assert(owner.float_timer_60c==14.0f);
}
}
int main(){
    test_float_setters_equal_values_and_sign_of_zero();
    test_no_pending_never_queries_external_state();
    test_gate_due_check_and_original_signed_clip();
    test_complete_untracked_event_timer_and_notifications();
    test_tracking_branch_uses_original_opaque_looked_up_id();
    test_equal_owner_deadline_preserves_negative_zero_when_copied();
    test_x87_unordered_deadline_does_not_suppress_dispatch();
    test_phase165_snapshot_tail_dispatch_integration();
}
