#pragma once
// Phase161: 2006 original client.dll CHudBreathTimer/CHudPoisonTimer
// constructor *observations*, not complete engine-side constructors.
// Exact original vtable and member offsets were verified on owner binaries.
// Neither original C++ ABI nor VGUI/global registration is reproduced.
#include "sin_compat/original_client_hud_element_visibility.hpp"
#include <array>
#include <cstdint>

namespace sin_compat::original_client_timer_constructors {
namespace base = sin_compat::original_client_hud_element;

inline constexpr std::uint32_t kOriginalBreathPrimaryVtable = 0x242C78CCu;
inline constexpr std::uint32_t kOriginalBreathPanelVtable = 0x242C769Cu;
inline constexpr std::uint32_t kOriginalPoisonPrimaryVtable = 0x242C8DECu;
inline constexpr std::uint32_t kOriginalPoisonPanelVtable = 0x242C8B24u;
inline constexpr std::uint32_t kOriginalBreathVisibilityMask = 0x10u;
inline constexpr std::uint32_t kOriginalPoisonVisibilityMask = 0x18u;
inline constexpr std::uint32_t kOriginalPanelSubobjectOffset = 0x14u;
inline constexpr std::uint32_t kOriginalPanelFirstByteOffset = 0xA3u;
inline constexpr std::uint32_t kOriginalPanelFirstResourceOffset = 0xA8u;
inline constexpr std::uint32_t kOriginalCompleteFirstByteOffset = 0xB7u;
inline constexpr std::uint32_t kOriginalCompleteFirstResourceOffset = 0xBCu;
static_assert(kOriginalPanelSubobjectOffset+kOriginalPanelFirstByteOffset==
              kOriginalCompleteFirstByteOffset,"original subobject field offset");
static_assert(kOriginalPanelSubobjectOffset+kOriginalPanelFirstResourceOffset==
              kOriginalCompleteFirstResourceOffset,"original resource field offset");

// 0x242309F0 / 0x24236030 and 0x24230A10 / 0x24236050:
// native helper takes a raw 32-bit *original* address, returns nullptr
// unchanged, otherwise adds an immediate offset modulo 2^32.
inline std::uint32_t original_pointer_adjustment(
    std::uint32_t original_pointer,std::uint32_t field_offset) noexcept {
    return original_pointer==0u?0u:original_pointer+field_offset;
}
inline std::uint32_t original_first_panel_byte_pointer(
    std::uint32_t original_pointer) noexcept {
    return original_pointer_adjustment(
        original_pointer,kOriginalPanelFirstByteOffset);
}
inline std::uint32_t original_first_resource_pointer(
    std::uint32_t original_pointer) noexcept {
    return original_pointer_adjustment(
        original_pointer,kOriginalPanelFirstResourceOffset);
}
// These original immediate-return helpers exist separately in BOTH
// native timer classes: 0x24230A00/0x24236040 and
// 0x24230A20/0x24236060.
inline constexpr std::uint32_t original_first_panel_byte_offset() noexcept {
    return kOriginalCompleteFirstByteOffset;
}
inline constexpr std::uint32_t original_first_resource_offset() noexcept {
    return kOriginalCompleteFirstResourceOffset;
}

// Independently observed *direct stores* from two original constructors.
// Native engine-side Panel constructors, property registries, singleton
// calls, and lifetime effects are deliberately NOT reconstructed.
struct ConstructorObservations {
    base::State element{};
    std::uint32_t panel_vtable = 0;
    // Original CHudBreathTimer explicitly zeroes +0xB7..+0xBA.
// CHudPoisonTimer does not directly write these bytes in its body.
    std::array<std::uint8_t,4> complete_bytes_b7_to_ba{};
    // Original poison constructor zeroes +0x2FC/+0x300/+0x304.
    std::array<std::uint32_t,3> poison_words_2fc_to_304{};
};
inline void apply_verified_breath_constructor_stores(
    ConstructorObservations& output,std::uint32_t original_identity) noexcept {
    base::initialize(output.element,original_identity);
    output.element.original_vtable_va=kOriginalBreathPrimaryVtable;
    output.panel_vtable=kOriginalBreathPanelVtable;
    output.complete_bytes_b7_to_ba.fill(0);
    base::virtual_slot12_set_mask08(output.element,
                                   kOriginalBreathVisibilityMask);
}
inline void apply_verified_poison_constructor_stores(
    ConstructorObservations& output,std::uint32_t original_identity) noexcept {
    base::initialize(output.element,original_identity);
    output.element.original_vtable_va=kOriginalPoisonPrimaryVtable;
    output.panel_vtable=kOriginalPoisonPanelVtable;
    output.poison_words_2fc_to_304.fill(0);
    base::virtual_slot12_set_mask08(output.element,
                                   kOriginalPoisonVisibilityMask);
}
} // namespace sin_compat::original_client_timer_constructors
