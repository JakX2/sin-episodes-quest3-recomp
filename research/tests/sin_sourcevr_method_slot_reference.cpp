#include "sin_compat/sourcevr_method_slot_compat.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <array>
using namespace sin_compat::phase97;
int main(){
    static_assert(emitter_requirements.size()==2);
    static_assert(vgui_requirements.size()==6);
    assert(emitter_requirements[0].original_x86_vtable_byte_offset==0x0c);
    assert(emitter_requirements[0].native_arm64_vtable_byte_offset_for_same_index==0x18);
    assert(emitter_requirements[1].original_x86_vtable_byte_offset==0x18);
    assert(emitter_requirements[1].native_arm64_vtable_byte_offset_for_same_index==0x30);
    assert(native_emitter_slot3_function==0x12970);
    assert(native_emitter_slot6_function==0x13380);
    for(const auto& item:emitter_requirements){
        assert(item.verdict==SlotVerdict::ProvenSemanticMismatch);
        assert(!native_method_call_permitted(item.verdict));
    }
    for(const auto& item:vgui_requirements){
        assert(item.arm64_vtable_byte_offset_for_same_index==2*item.original_x86_vtable_byte_offset);
        assert(item.verdict==SlotVerdict::MethodSemanticsUnverified);
    }
    assert(!all_native_methods_verified(emitter_requirements));
    assert(!all_native_methods_verified(vgui_requirements));
    constexpr std::array<OriginalMethodRequirement,1> verified{{
       {"mock","verified",0,0,SlotVerdict::ProvenMethodCompatible}}};
    static_assert(all_native_methods_verified(verified));
    assert(native_method_call_permitted(SlotVerdict::ProvenMethodCompatible));
    assert(!native_method_call_permitted(SlotVerdict::MethodSemanticsUnverified));
}
