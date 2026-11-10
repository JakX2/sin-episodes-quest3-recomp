#include "sin_compat/original_server_scattergun_ammo.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <vector>
namespace a=sin_compat::original_server_scattergun_ammo;
namespace {
void test_initial_ammo_snapshot_no_spurious_action() {
    a::ObservedAmmoState s{};
    s.clip_4ec=7;
    std::vector<int> calls;
    a::original_scattergun_update_ammo(s,
        [&]{calls.push_back(1);},[&]{calls.push_back(2);});
    assert((calls==std::vector<int>{1}));
    assert(s.prior_clip_57c==7 && s.change_pending_589==0);
    s.clip_4ec=6;
    a::original_scattergun_update_ammo(s,
        [&]{calls.push_back(1);},[&]{calls.push_back(2);});
    assert((calls==std::vector<int>{1,1,2}));
    assert(s.prior_clip_57c==6 && s.change_pending_589==1);
}
void test_base_callback_precedes_unchanged_ammo_comparisons() {
    a::ObservedAmmoState s{};
    s.clip_4ec=100;s.prior_clip_57c=10;
    std::vector<char> calls;
    a::original_scattergun_update_ammo(s,
       [&]{calls.push_back('B');s.clip_4ec=11;},
       [&]{calls.push_back('D');assert(s.prior_clip_57c==11);});
    assert((calls==std::vector<char>{'B','D'}));
}
void test_secondary_tracking_disabled_does_not_rewrite_snapshot() {
    a::ObservedAmmoState s{};
    s.clip_4ec=2;s.prior_clip_57c=2;
    s.prior_secondary_580=5;s.secondary_584=6;
    int deferred=0;
    a::original_scattergun_update_ammo(s,[]{},[&]{++deferred;});
    assert(s.prior_secondary_580==5 && deferred==0);
    s.secondary_tracking_578=1;
    a::original_scattergun_update_ammo(s,[]{},[&]{++deferred;});
    assert(s.prior_secondary_580==6 && deferred==1);
    assert(s.change_pending_589==1);
}
void test_first_secondary_snapshot_does_not_latch() {
    a::ObservedAmmoState s{};
    s.clip_4ec=5;s.prior_clip_57c=5;
    s.secondary_tracking_578=1;s.secondary_584=10;
    a::original_scattergun_update_ammo(s,[]{},[]{assert(false);});
    assert(s.prior_secondary_580==10 && s.change_pending_589==0);
}
void test_pending_latch_is_not_cleared() {
    a::ObservedAmmoState s{};
    s.prior_clip_57c=2;s.clip_4ec=2;s.change_pending_589=42;
    int n=0;
    a::original_scattergun_update_ammo(s,[]{},[&]{++n;});
    a::original_scattergun_update_ammo(s,[]{},[&]{++n;});
    assert(n==2 && s.change_pending_589==42);
}
void test_two_notification_modes_and_absent_pointer() {
    a::SecondaryAmmoConsumption s{};
    s.secondary_clip_4f0=4;s.mode_byte_1f0=1;
    std::uint32_t flags=0x14;s.event_flags_at_pointer_16c=&flags;
    a::original_scattergun_consume_secondary_ammo(s);
    assert(s.secondary_clip_4f0==3 && s.signal_byte_200==1);
    assert(flags==0x14); // original mode byte diverts notification
    s.mode_byte_1f0=0;s.signal_byte_200=0;
    a::original_scattergun_consume_secondary_ammo(s);
    assert(s.secondary_clip_4f0==2 && s.signal_byte_200==0);
    assert(flags==0x15);
    s.event_flags_at_pointer_16c=nullptr;
    a::original_scattergun_consume_secondary_ammo(s);
    assert(s.secondary_clip_4f0==1 && flags==0x15);
}
void test_no_original_zero_ammo_guard_and_dword_wrap() {
    a::SecondaryAmmoConsumption s{};
    s.secondary_clip_4f0=0;
    a::original_scattergun_consume_secondary_ammo(s);
    assert(s.secondary_clip_4f0==0xffffffffu);
    a::original_scattergun_consume_secondary_ammo(s);
    assert(s.secondary_clip_4f0==0xfffffffeu);
}
void test_two_simultaneous_ammo_changes_dispatch_once() {
    a::ObservedAmmoState s{};
    s.clip_4ec=8;s.prior_clip_57c=9;
    s.secondary_tracking_578=2;
    s.secondary_584=3;s.prior_secondary_580=4;
    int dispatched=0;
    a::original_scattergun_update_ammo(s,[]{},[&]{++dispatched;});
    assert(dispatched==1 && s.change_pending_589==1);
    assert(s.prior_clip_57c==8 && s.prior_secondary_580==3);
}
}
int main(){
    test_initial_ammo_snapshot_no_spurious_action();
    test_base_callback_precedes_unchanged_ammo_comparisons();
    test_secondary_tracking_disabled_does_not_rewrite_snapshot();
    test_first_secondary_snapshot_does_not_latch();
    test_pending_latch_is_not_cleared();
    test_two_notification_modes_and_absent_pointer();
    test_no_original_zero_ammo_guard_and_dword_wrap();
    test_two_simultaneous_ammo_changes_dispatch_once();
}
