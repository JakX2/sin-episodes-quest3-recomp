#pragma once
// SiN Episodes (2006) x86 vs SourceVR 0.5.5 ARM64 original method-slot ABI.
// Inert host-side metadata. This is NOT a callable native adapter.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace sin_compat::phase97 {
enum class SlotVerdict : std::uint8_t {
    ProvenSemanticMismatch, MethodSemanticsUnverified, ProvenMethodCompatible
};
struct OriginalMethodRequirement {
    std::string_view original_interface;
    std::string_view required_operation;
    std::uint32_t original_x86_vtable_byte_offset;
    std::uint32_t native_arm64_vtable_byte_offset_for_same_index;
    SlotVerdict verdict;
};
inline constexpr std::array<OriginalMethodRequirement,2> emitter_requirements{{
    {"VSoundEmitter001","name_to_token",0x0cu,0x18u,SlotVerdict::ProvenSemanticMismatch},
    {"VSoundEmitter001","token_to_name",0x18u,0x30u,SlotVerdict::ProvenSemanticMismatch}
}};
struct OriginalVGuiRequirement {
    std::string_view original_interface;
    std::uint32_t original_x86_vtable_byte_offset;
    std::uint32_t arm64_vtable_byte_offset_for_same_index;
    SlotVerdict verdict;
};
// Genuine client.dll callsite indexes; native vtable entries exist, but
// actual parameter/signature compatibility remains UNPROVEN.
inline constexpr std::array<OriginalVGuiRequirement,6> vgui_requirements{{
    {"VGUI_InputInternal001",0xdcu,0x1b8u,SlotVerdict::MethodSemanticsUnverified},
    {"VGUI_InputInternal001",0xe0u,0x1c0u,SlotVerdict::MethodSemanticsUnverified},
    {"VGUI_InputInternal001",0xe8u,0x1d0u,SlotVerdict::MethodSemanticsUnverified},
    {"VGUI_InputInternal001",0xf4u,0x1e8u,SlotVerdict::MethodSemanticsUnverified},
    {"VGUI_InputInternal001",0xf8u,0x1f0u,SlotVerdict::MethodSemanticsUnverified},
    {"VGUI_System010",0x60u,0xc0u,SlotVerdict::MethodSemanticsUnverified}
}};
constexpr bool native_method_call_permitted(SlotVerdict s) noexcept {
    return s==SlotVerdict::ProvenMethodCompatible;
}
template<class C>
constexpr bool all_native_methods_verified(const C& items) noexcept {
    for(const auto& item:items)if(!native_method_call_permitted(item.verdict))return false;
    return true;
}
inline constexpr std::uint32_t native_emitter_vtable=0x41f98u;
inline constexpr std::uint32_t native_vgui_input_vtable=0x670c8u;
inline constexpr std::uint32_t native_vgui_system_vtable=0x67cb0u;
inline constexpr std::uint32_t native_emitter_slot3_function=0x12970u;
inline constexpr std::uint32_t native_emitter_slot6_function=0x13380u;
static_assert(emitter_requirements[0].native_arm64_vtable_byte_offset_for_same_index==2*0x0c);
static_assert(vgui_requirements[4].arm64_vtable_byte_offset_for_same_index==2*0xf8);
static_assert(!all_native_methods_verified(emitter_requirements));
static_assert(!all_native_methods_verified(vgui_requirements));
} // namespace sin_compat::phase97
