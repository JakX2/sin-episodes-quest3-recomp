#pragma once
// Phase 499: authenticated 2006 C_SoundscapeSystem base storage
// and 120-byte original constructor at VA 0x240F3BC0.
//
// Original source SHA256 ctor:
//   0d9e47028fda189671b406f07549c147f3bfebebd24291e4460221c210ee96f0
// Original 12-entry vtable VA 0x24293C34 SHA256:
//   049c45c9002710aedfaef967c4c4aaebbc81bbc645528e4a5dea7c2735ed02b8
//
// This models ONLY authenticated DWORD writes. The true C++ class
// has more state than 0xC8 bytes (e.g. original +0xD0/+0xD4).
// Never reinterpret this as an original game object or call its
// original absolute virtual-address fields as function pointers.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent::soundscape_original_object {
inline constexpr std::uint32_t kOriginalCtorVA=0x240F3BC0u;
inline constexpr std::uint32_t kOriginalCtorBytes=120u;
inline constexpr std::uint32_t kOriginalVtableVA=0x24293C34u;
inline constexpr std::uint32_t kOriginalSubobjectVtableVA=0x2428A9B0u;
inline constexpr std::uint32_t kOriginalSlot7VA=0x240F3CA0u;
inline constexpr std::uint32_t kOriginalSlot7Bytes=12u;
inline constexpr std::uint32_t kOriginalSlot7GlobalVA=0x242FA458u;
inline constexpr std::size_t kOwnedKnownStorageBytes=0xC8u;
inline constexpr std::size_t kOwnedKnownDwords=kOwnedKnownStorageBytes/4u;
inline constexpr std::uint32_t kUninitializedSentinel=0xFFFFFFFFu;
inline constexpr std::array<std::uint32_t,12> kOriginalVtableSlots{{
    0x240F4160u, // 0 original Init, 343 bytes
    0x240F3EE0u, // 1 original Shutdown, 267 bytes
    0x240F3DF0u, // 2 source virtual method, not assigned guessed name
    0x240F3C40u, // 3 original two-ConVar FindVar cache, 61 bytes
    0x240F3C80u, // 4 exactly RET
    0x240F3E10u, // 5 direct tail jump to 0x240F3D10
    0x240F3C90u, // 6 exactly RET
    0x240F3CA0u, // 7 copy source global dword at pointer+4 to this+4
    0x240F3CB0u, // 8 exactly RET
    0x240F3CC0u, // 9 exactly RET
    0x240F4D30u, //10 complex per-frame style update, not implemented
    0x240F3E20u, //11 original cleanup path, not implemented
}};
inline constexpr std::array<std::uint32_t,10> kOriginalEarlyZeroOffsets{{
    0x08u,0x0Cu,0x10u,0x14u,0x18u,
    0x1Cu,0x20u,0x24u,0x28u,0x2Cu,
}};
inline constexpr std::array<std::uint32_t,10> kOriginalLaterZeroOffsets{{
    0xA0u,0xA4u,0xA8u,0xACu,0xB0u,
    0xB4u,0xB8u,0xBCu,0xC0u,0xC4u,
}};
static_assert(kOriginalVtableSlots[0]==0x240F4160u);
static_assert(kOriginalVtableSlots[1]==0x240F3EE0u);
static_assert(kOriginalVtableSlots[3]==0x240F3C40u);
static_assert(kOriginalVtableSlots[7]==kOriginalSlot7VA);
static_assert(kOriginalEarlyZeroOffsets.size()==10);
static_assert(kOriginalLaterZeroOffsets.size()==10);
struct KnownDwordModel final {
    // An independently owned DWORD array. It is NOT the original
    // class layout, only an exact-offset model for known DWORD writes.
    std::array<std::uint32_t,kOwnedKnownDwords> words{};
    std::uint64_t revision=0;
    std::uint64_t slot7_invocations=0;
    constexpr std::uint32_t At(std::uint32_t offset) const noexcept {
        return offset<kOwnedKnownStorageBytes &&
               (offset%4u)==0?
               words[offset/4u]:0u;
    }
};
inline void ApplyOriginalConstructor(KnownDwordModel& owner) noexcept {
    // Original method writes precisely these observed offsets.
    // The model preserves other fields, instead of inventing
    // zero-initialization of the opaque base/embedded objects.
    auto& bytes=owner.words;
    bytes[0]=kOriginalVtableVA;
    bytes[0x04u/4u]=kUninitializedSentinel;
    for(const auto offset:kOriginalEarlyZeroOffsets)
        bytes[offset/4u]=0u;
    bytes[0x30u/4u]=kOriginalSubobjectVtableVA;
    bytes[0x9Cu/4u]=kUninitializedSentinel;
    for(const auto offset:kOriginalLaterZeroOffsets)
        bytes[offset/4u]=0u;
    ++owner.revision;
    owner.slot7_invocations=0;
}
// Original slot7 at 0x240F3CA0 is EXACTLY a 12-byte field copy:
//   eax = *(void**)0x242FA458;
//   edx = *(uint32_t*)(eax+4);
//   *(uint32_t*)(this+4) = edx;
//   ret
// Caller extracts a trusted owned DWORD from its independent global
// provider. No retail pointer is ever dereferenced in this function.
inline void ApplyOriginalSlot7GlobalPlus4(KnownDwordModel& owner,
                                         std::uint32_t original_value) noexcept {
    owner.words[0x04u/4u]=original_value;
    ++owner.slot7_invocations;
    ++owner.revision;
}
inline bool OriginalVtableSlotIsSingleRet(std::size_t slot) noexcept {
    return slot==4 || slot==6 || slot==8 || slot==9;
}
} // namespace sin_independent::soundscape_original_object
