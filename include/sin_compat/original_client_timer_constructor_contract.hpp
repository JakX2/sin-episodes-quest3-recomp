#pragma once
// Phase161 corrected: exact owner-held 2006 SiN Episodes client.dll
// RTTI-backed CHudBreathTimer / CHudPoisonTimer / CHudHealth constructor
// *direct stores*. Full VGUI engine constructors and registrars NOT ported.
// Original Windows x86 addresses are evidence, not portable C++ ABI.
#include "sin_compat/original_client_hud_element_visibility.hpp"
#include <array>
#include <cstdint>

namespace sin_compat::original_client_timer_constructors {
namespace base = sin_compat::original_client_hud_element;

inline constexpr std::uint32_t kOriginalBreathPrimaryVtable = 0x242C78CCu;
inline constexpr std::uint32_t kOriginalBreathPanelVtable = 0x242C769Cu;
inline constexpr std::uint32_t kOriginalPoisonPrimaryVtable = 0x242C91C4u;
inline constexpr std::uint32_t kOriginalPoisonPanelVtable = 0x242C8F94u;
inline constexpr std::uint32_t kOriginalHealthPrimaryVtable = 0x242C8DECu;
inline constexpr std::uint32_t kOriginalHealthPanelVtable = 0x242C8B24u;
inline constexpr std::uint32_t kOriginalBreathVisibilityMask = 0x10u;
inline constexpr std::uint32_t kOriginalPoisonVisibilityMask = 0x10u;
inline constexpr std::uint32_t kOriginalHealthVisibilityMask = 0x18u;
inline constexpr std::uint32_t kOriginalPanelSubobjectOffset = 0x14u;
inline constexpr std::uint32_t kOriginalPanelFirstByteOffset = 0xA3u;
inline constexpr std::uint32_t kOriginalPanelFirstResourceOffset = 0xA8u;
inline constexpr std::uint32_t kOriginalCompleteFirstByteOffset = 0xB7u;
inline constexpr std::uint32_t kOriginalCompleteFirstResourceOffset = 0xBCu;
static_assert(kOriginalPanelSubobjectOffset+kOriginalPanelFirstByteOffset==
              kOriginalCompleteFirstByteOffset,"original subobject field offset");
static_assert(kOriginalPanelSubobjectOffset+kOriginalPanelFirstResourceOffset==
              kOriginalCompleteFirstResourceOffset,"original resource field offset");

// The original breath/poison 16-byte helpers return input+panel offset
// when nonnull, else zero, using native 32-bit wrapping arithmetic.
inline std::uint32_t original_pointer_adjustment(
    std::uint32_t pointer,std::uint32_t field_offset) noexcept {
    return pointer==0u?0u:pointer+field_offset;
}
inline std::uint32_t original_first_panel_byte_pointer(
    std::uint32_t pointer) noexcept {
    return original_pointer_adjustment(pointer,kOriginalPanelFirstByteOffset);
}
inline std::uint32_t original_first_resource_pointer(
    std::uint32_t pointer) noexcept {
    return original_pointer_adjustment(pointer,kOriginalPanelFirstResourceOffset);
}
inline constexpr std::uint32_t original_first_panel_byte_offset() noexcept {
    return kOriginalCompleteFirstByteOffset;
}
inline constexpr std::uint32_t original_first_resource_offset() noexcept {
    return kOriginalCompleteFirstResourceOffset;
}

// Only explicitly observed direct constructor writes. We do not infer
// missing writes by conflating the three classes' native constructors.
struct ConstructorObservations {
    base::State element{};
    std::uint32_t panel_vtable = 0;
    // Real Breath +0xB7..0xBA and real Poison +0xB7..0xBA initialize to zero.
    // Health constructor does not directly write these four bytes.
    std::array<std::uint8_t,4> complete_bytes_b7_to_ba{};
    // Original CHudHealth constructor zeroes +0x2FC, +0x300, +0x304.
    std::array<std::uint32_t,3> health_words_2fc_to_304{};
};
inline void apply_verified_breath_constructor_stores(
    ConstructorObservations& output,std::uint32_t identity) noexcept {
    base::initialize(output.element,identity);
    output.element.original_vtable_va=kOriginalBreathPrimaryVtable;
    output.panel_vtable=kOriginalBreathPanelVtable;
    output.complete_bytes_b7_to_ba.fill(0);
    base::virtual_slot12_set_mask08(output.element,kOriginalBreathVisibilityMask);
}
inline void apply_verified_poison_constructor_stores(
    ConstructorObservations& output,std::uint32_t identity) noexcept {
    base::initialize(output.element,identity);
    output.element.original_vtable_va=kOriginalPoisonPrimaryVtable;
    output.panel_vtable=kOriginalPoisonPanelVtable;
    output.complete_bytes_b7_to_ba.fill(0);
    base::virtual_slot12_set_mask08(output.element,kOriginalPoisonVisibilityMask);
}
inline void apply_verified_health_constructor_stores(
    ConstructorObservations& output,std::uint32_t identity) noexcept {
    base::initialize(output.element,identity);
    output.element.original_vtable_va=kOriginalHealthPrimaryVtable;
    output.panel_vtable=kOriginalHealthPanelVtable;
    output.health_words_2fc_to_304.fill(0);
    base::virtual_slot12_set_mask08(output.element,kOriginalHealthVisibilityMask);
}
} // namespace sin_compat::original_client_timer_constructors
