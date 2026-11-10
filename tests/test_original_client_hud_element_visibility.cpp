#include "sin_compat/original_client_hud_element_visibility.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <vector>
using namespace sin_compat::original_client_hud_element;

namespace {
void test_constructor_and_virtual_slots() {
    State s{};
    s.original_vtable_va=0xffffffffU;
    s.byte_04=42; s.mask_08=0xffffffffU;
    s.identity_0c=0x17; s.byte_10=0xff; s.byte_11=0xff;
    initialize(s,0xdeadbeefU);
    assert(s.original_vtable_va==kOriginalCHudElementVtable);
    assert(s.byte_04==0 && s.mask_08==0 && s.identity_0c==0xdeadbeefU);
    assert(s.byte_10==0 && s.byte_11==0);
    virtual_slot2_noop(s);
    assert(virtual_slot8_get_identity(s)==0xdeadbeefU);
    assert(virtual_slot10_get_byte04(s)==0);
    virtual_slot11_set_byte04(s,0xffU);
    virtual_slot12_set_mask08(s,0x20000010U);
    assert(virtual_slot10_get_byte04(s)==0xffU);
    assert(s.mask_08==0x20000010U);
    assert(s.byte_10==0 && s.byte_11==0);
}
void test_early_exits_never_call_player_virtual() {
    OriginalGateInputs in{};
    State s{};initialize(s,0);
    virtual_slot12_set_mask08(s,0x10);
    int calls=0;
    auto cb=[&] {++calls;return -1;};
    assert(original_global_hud_suppression(in,s.mask_08,cb));
    assert(!virtual_slot9_should_draw(s,in,cb));
    assert(calls==0);
    in.engine_flag_check_enabled=true;
    assert(original_global_hud_suppression(in,s.mask_08,cb));
    assert(calls==0);
    in.local_player_present=true;
    assert(original_global_hud_suppression(in,s.mask_08,cb));
    assert(calls==1);
}
void test_override_precedence_and_hard_bit4() {
    OriginalGateInputs in{};
    in.engine_flag_check_enabled=true;
    in.local_player_present=true;
    in.player_flags_d8c=4;
    int called=0;
    auto cb=[&]{++called;return 1;};
    assert(original_global_hud_suppression(in,0,cb));
    assert(original_global_hud_suppression(in,0x10,cb));
    assert(called==0);
    in.override_flags_2c=0x80;
    assert(!original_global_hud_suppression(in,0,cb));
    assert(original_global_hud_suppression(in,0x80,cb));
    assert(called==0);
    in.override_flags_2c=0;
    assert(original_global_hud_suppression(in,0,cb));
    assert(called==0);
}
void test_signed_player_virtual_and_byte_gate() {
    OriginalGateInputs in{};
    in.engine_flag_check_enabled=true;
    in.local_player_present=true;
    in.player_byte_e15=1;
    int called=0;
    auto negative=[&]{++called;return -1;};
    auto zero=[&]{++called;return 0;};
    auto positive=[&]{++called;return 1;};
    assert(original_global_hud_suppression(in,0x10,negative));
    assert(original_global_hud_suppression(in,0x10,zero));
    assert(!original_global_hud_suppression(in,0x10,positive));
    assert(called==3);
    assert(!original_global_hud_suppression(in,0x01,positive));
    assert(called==3);
    auto unreachable=[]{assert(false);return 1;};
    in.player_byte_e15=0;
    assert(original_global_hud_suppression(in,0x20,unreachable));
    in.player_byte_e15=7;
    assert(!original_global_hud_suppression(in,0x20,unreachable));
    in.player_flags_d8c=0x80;
    assert(original_global_hud_suppression(in,0x80,unreachable));
    assert(!original_global_hud_suppression(in,0x40,unreachable));
}
void test_mask_setter_and_should_draw_integration() {
    OriginalGateInputs in{};
    in.engine_flag_check_enabled=true;
    in.local_player_present=true;
    in.player_byte_e15=1;
    in.player_flags_d8c=0x80;
    State s{};initialize(s,0x99);
    int calls=0;
    auto cb=[&]{++calls;return 1;};
    assert(virtual_slot9_should_draw(s,in,cb));
    virtual_slot12_set_mask08(s,0x80);
    assert(!virtual_slot9_should_draw(s,in,cb));
    virtual_slot12_set_mask08(s,0x10);
    assert(virtual_slot9_should_draw(s,in,cb));
    assert(calls==1);
    virtual_slot12_set_mask08(s,0x20);
    assert(virtual_slot9_should_draw(s,in,cb));
    in.player_byte_e15=0;
    assert(!virtual_slot9_should_draw(s,in,cb));
}
void test_original_mask_and_virtual_branch_matrix() {
    constexpr std::array<std::uint32_t,8> masks{{0,1,4,0x10,0x20,0x30,0x80,0x90}};
    constexpr std::array<std::uint32_t,7> flags{{0,1,4,0x10,0x20,0x80,0xffffffffU}};
    State state{};initialize(state,1);
    for(auto player_flags:masks)for(auto override_flags:flags)
    for(auto mask:masks)for(auto virtual_return:{-1,0,1}) {
        OriginalGateInputs in{true,true,override_flags,player_flags,1};
        virtual_slot12_set_mask08(state,mask);
        int calls=0;
        auto cb=[&]{++calls;return virtual_return;};
        const auto active=override_flags?override_flags:player_flags;
        const bool expected=(active&4U)!=0U ||
          ((mask&0x10U)!=0U&&virtual_return<=0) ||
          ((active&mask)!=0U);
        assert(original_global_hud_suppression(in,mask,cb)==expected);
        assert(virtual_slot9_should_draw(state,in,cb)==!expected);
        const int expected_calls=(active&4U)==0U&&(mask&0x10U)!=0U?1:0;
        assert(calls==2*expected_calls);
    }
}
}
int main() {
    test_constructor_and_virtual_slots();
    test_early_exits_never_call_player_virtual();
    test_override_precedence_and_hard_bit4();
    test_signed_player_virtual_and_byte_gate();
    test_mask_setter_and_should_draw_integration();
    test_original_mask_and_virtual_branch_matrix();
}
