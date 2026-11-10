#include "sin_compat/original_client_timer_constructor_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
using namespace sin_compat::original_client_timer_constructors;

namespace {
void test_native_rtti_vtable_and_mask_invariants() {
    static_assert(kOriginalBreathPrimaryVtable==0x242C78CCu,"original breath RTTI");
    static_assert(kOriginalPoisonPrimaryVtable==0x242C8DECu,"original poison RTTI");
    static_assert(kOriginalPanelFirstByteOffset+3u==0xA6u,"original panel fourth byte");
    static_assert(kOriginalCompleteFirstByteOffset+3u==0xBAu,"breath ctor last store");
    static_assert(kOriginalBreathVisibilityMask==0x10u,"original breath mask");
    static_assert(kOriginalPoisonVisibilityMask==0x18u,"original poison mask");
    assert(kOriginalBreathVisibilityMask!=kOriginalPoisonVisibilityMask);
}
void test_exact_original_nullable_pointer_thunks() {
    assert(original_first_panel_byte_pointer(0u)==0u);
    assert(original_first_resource_pointer(0u)==0u);
    assert(original_first_panel_byte_pointer(0x1000u)==0x10A3u);
    assert(original_first_resource_pointer(0x1000u)==0x10A8u);
    assert(original_first_panel_byte_offset()==0xB7u);
    assert(original_first_resource_offset()==0xBCu);
    // The original 32-bit LEA/ADD uses modulo-32-bit arithmetic.
    assert(original_first_panel_byte_pointer(0xFFFFFFF0u)==0x93u);
    assert(original_first_resource_pointer(0xFFFFFFF0u)==0x98u);
    assert(original_first_panel_byte_pointer(1u)==0xA4u);
}
void test_breath_ctor_explicit_panel_byte_initialization() {
    ConstructorObservations s{};
    s.complete_bytes_b7_to_ba={{10,20,30,40}};
    s.poison_words_2fc_to_304={{9u,8u,7u}};
    apply_verified_breath_constructor_stores(s,0xABCDEF01u);
    assert(s.element.original_vtable_va==kOriginalBreathPrimaryVtable);
    assert(s.panel_vtable==kOriginalBreathPanelVtable);
    assert(s.element.mask_08==0x10u && s.element.identity_0c==0xABCDEF01u);
    assert((s.complete_bytes_b7_to_ba==std::array<std::uint8_t,4>{{0,0,0,0}}));
    // The breath constructor has no corresponding direct stores to the
    // poison-only +0x2FC..+0x304 members.
    assert((s.poison_words_2fc_to_304==std::array<std::uint32_t,3>{{9u,8u,7u}}));
}
void test_poison_ctor_direct_stores_and_unknown_color_init() {
    ConstructorObservations s{};
    s.complete_bytes_b7_to_ba={{0x11,0x22,0x33,0x44}};
    s.poison_words_2fc_to_304={{7u,8u,9u}};
    apply_verified_poison_constructor_stores(s,0xF0E0D0C0u);
    assert(s.element.original_vtable_va==kOriginalPoisonPrimaryVtable);
    assert(s.panel_vtable==kOriginalPoisonPanelVtable);
    assert(s.element.mask_08==0x18u && s.element.identity_0c==0xF0E0D0C0u);
    assert((s.poison_words_2fc_to_304==std::array<std::uint32_t,3>{{0u,0u,0u}}));
    // No explicit color-byte store in poison ctor. Do not silently
    // manufacture zero-init that may instead be caused by VGUI base.
    assert((s.complete_bytes_b7_to_ba==std::array<std::uint8_t,4>{{
        0x11,0x22,0x33,0x44}}));
}
void test_original_mask_difference_changes_visibility() {
    ConstructorObservations breath{};
    ConstructorObservations poison{};
    apply_verified_breath_constructor_stores(breath,10);
    apply_verified_poison_constructor_stores(poison,20);
    base::OriginalGateInputs input{};
    input.engine_flag_check_enabled=true;
    input.local_player_present=true;
    input.player_flags_d8c=0x08u;
    input.player_byte_e15=1;
    int calls=0;
    auto signed_player_virtual=[&](){++calls;return 1;};
    assert(base::virtual_slot9_should_draw(
        breath.element,input,signed_player_virtual));
    assert(!base::virtual_slot9_should_draw(
        poison.element,input,signed_player_virtual));
    assert(calls==2);
    // Both original masks include 0x10, so signed <=0 suppresses both.
    auto blocked=[](){return 0;};
    assert(!base::virtual_slot9_should_draw(breath.element,input,blocked));
    assert(!base::virtual_slot9_should_draw(poison.element,input,blocked));
    // Hard suppression bit4 from original CHudElement global gate.
    input.player_flags_d8c=0x04u;
    auto must_not_run=[]()->int{assert(false);return 1;};
    assert(!base::virtual_slot9_should_draw(breath.element,input,must_not_run));
    assert(!base::virtual_slot9_should_draw(poison.element,input,must_not_run));
    input.player_flags_d8c=0;
    input.override_flags_2c=0x08;
    assert(base::virtual_slot9_should_draw(
        breath.element,input,signed_player_virtual));
    assert(!base::virtual_slot9_should_draw(
        poison.element,input,signed_player_virtual));
}
}
int main() {
    test_native_rtti_vtable_and_mask_invariants();
    test_exact_original_nullable_pointer_thunks();
    test_breath_ctor_explicit_panel_byte_initialization();
    test_poison_ctor_direct_stores_and_unknown_color_init();
    test_original_mask_difference_changes_visibility();
}
