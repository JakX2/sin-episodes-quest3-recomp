#include "sin_compat/original_client_timer_constructor_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
using namespace sin_compat::original_client_timer_constructors;

namespace {
void test_rtti_vtables_and_actual_native_masks() {
    static_assert(kOriginalBreathPrimaryVtable==0x242C78CCu,"breath");
    static_assert(kOriginalPoisonPrimaryVtable==0x242C91C4u,"poison");
    static_assert(kOriginalHealthPrimaryVtable==0x242C8DECu,"health");
    static_assert(kOriginalPoisonPanelVtable==0x242C8F94u,"poison panel");
    static_assert(kOriginalHealthPanelVtable==0x242C8B24u,"health panel");
    static_assert(kOriginalBreathVisibilityMask==0x10u,"breath mask");
    static_assert(kOriginalPoisonVisibilityMask==0x10u,"poison mask");
    static_assert(kOriginalHealthVisibilityMask==0x18u,"health mask");
    static_assert(kOriginalPanelFirstByteOffset+3u==0xA6u,"panel 4th byte");
    static_assert(kOriginalCompleteFirstByteOffset+3u==0xBAu,"full 4th byte");
    assert(kOriginalBreathVisibilityMask==kOriginalPoisonVisibilityMask);
    assert(kOriginalPoisonVisibilityMask!=kOriginalHealthVisibilityMask);
}
void test_nullable_offset_helpers_in_two_original_classes() {
    assert(original_first_panel_byte_pointer(0u)==0u);
    assert(original_first_resource_pointer(0u)==0u);
    assert(original_first_panel_byte_pointer(0x1000u)==0x10A3u);
    assert(original_first_resource_pointer(0x1000u)==0x10A8u);
    assert(original_first_panel_byte_offset()==0xB7u);
    assert(original_first_resource_offset()==0xBCu);
    assert(original_first_panel_byte_pointer(0xFFFFFFF0u)==0x93u);
    assert(original_first_resource_pointer(0xFFFFFFF0u)==0x98u);
}
void test_breath_and_poison_explicit_color_zeroes() {
    for(bool poison_kind:{false,true}) {
        ConstructorObservations s{};
        s.complete_bytes_b7_to_ba={{10,20,30,40}};
        s.health_words_2fc_to_304={{9u,8u,7u}};
        if(poison_kind)apply_verified_poison_constructor_stores(s,101u);
        else apply_verified_breath_constructor_stores(s,101u);
        assert(s.element.identity_0c==101u);
        assert(s.element.mask_08==0x10u);
        assert(s.element.original_vtable_va==(poison_kind
            ?kOriginalPoisonPrimaryVtable:kOriginalBreathPrimaryVtable));
        assert(s.panel_vtable==(poison_kind
            ?kOriginalPoisonPanelVtable:kOriginalBreathPanelVtable));
        assert((s.complete_bytes_b7_to_ba==
                std::array<std::uint8_t,4>{{0,0,0,0}}));
        // Neither timer constructor directly zeroes CHudHealth tail fields.
        assert((s.health_words_2fc_to_304==
                std::array<std::uint32_t,3>{{9u,8u,7u}}));
    }
}
void test_health_constructor_has_its_own_mask_and_tail_fields() {
    ConstructorObservations s{};
    s.complete_bytes_b7_to_ba={{0x11,0x22,0x33,0x44}};
    s.health_words_2fc_to_304={{7u,8u,9u}};
    apply_verified_health_constructor_stores(s,0xF0E0D0C0u);
    assert(s.element.original_vtable_va==kOriginalHealthPrimaryVtable);
    assert(s.panel_vtable==kOriginalHealthPanelVtable);
    assert(s.element.mask_08==0x18u);
    assert(s.element.identity_0c==0xF0E0D0C0u);
    assert((s.health_words_2fc_to_304==
            std::array<std::uint32_t,3>{{0u,0u,0u}}));
    // Original health ctor does not directly touch these panel bytes.
    assert((s.complete_bytes_b7_to_ba==
            std::array<std::uint8_t,4>{{0x11,0x22,0x33,0x44}}));
}
void test_three_rtti_verified_masks_with_original_visibility_gate() {
    ConstructorObservations breath{},poison{},health{};
    apply_verified_breath_constructor_stores(breath,10u);
    apply_verified_poison_constructor_stores(poison,20u);
    apply_verified_health_constructor_stores(health,30u);
    base::OriginalGateInputs in{};
    in.engine_flag_check_enabled=true;
    in.local_player_present=true;
    in.player_flags_d8c=0x08u;
    in.player_byte_e15=1;
    int invoked=0;
    auto player_signed=[&]{++invoked;return 1;};
    // Previous Phase161 incorrectly claimed poison is hidden by 0x08.
    // RTTI proves that was a CHudHealth constructor, NOT poison.
    assert(base::virtual_slot9_should_draw(breath.element,in,player_signed));
    assert(base::virtual_slot9_should_draw(poison.element,in,player_signed));
    assert(!base::virtual_slot9_should_draw(health.element,in,player_signed));
    assert(invoked==3);
    auto zero=[] {return 0;};
    assert(!base::virtual_slot9_should_draw(breath.element,in,zero));
    assert(!base::virtual_slot9_should_draw(poison.element,in,zero));
    assert(!base::virtual_slot9_should_draw(health.element,in,zero));
    in.player_flags_d8c=0x04u;
    auto never=[]()->int {assert(false);return 1;};
    assert(!base::virtual_slot9_should_draw(breath.element,in,never));
    assert(!base::virtual_slot9_should_draw(poison.element,in,never));
    assert(!base::virtual_slot9_should_draw(health.element,in,never));
    in.player_flags_d8c=0u;
    in.override_flags_2c=0x08u;
    assert(base::virtual_slot9_should_draw(breath.element,in,player_signed));
    assert(base::virtual_slot9_should_draw(poison.element,in,player_signed));
    assert(!base::virtual_slot9_should_draw(health.element,in,player_signed));
}
}
int main() {
    test_rtti_vtables_and_actual_native_masks();
    test_nullable_offset_helpers_in_two_original_classes();
    test_breath_and_poison_explicit_color_zeroes();
    test_health_constructor_has_its_own_mask_and_tail_fields();
    test_three_rtti_verified_masks_with_original_visibility_gate();
}
