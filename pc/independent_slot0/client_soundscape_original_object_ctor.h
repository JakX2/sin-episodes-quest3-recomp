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
// Original per-frame virtual slot10 enters its expensive update only
// when object BYTE +0xD0 !=0 and global DWORD 0x24338CF0 !=0.
inline constexpr std::uint32_t kOriginalSlot10VA=0x240F4D30u;
inline constexpr std::uint32_t kOriginalSlot10FlagOffset=0xD0u;
inline constexpr std::uint32_t kOriginalSlot10GlobalVA=0x24338CF0u;
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

// Compile-time ownership contract: no duplicated, overlapping, unaligned
// or out-of-range writes may silently enter the authenticated write model.
// This is a synthetic invariant, not an additional original-method claim.
constexpr bool ConstructorOffsetContract() noexcept {
    std::array<bool,kOwnedKnownDwords> occupied{};
    const auto claim=[&](std::uint32_t offset) constexpr -> bool {
        if(offset%4u || offset>=kOwnedKnownStorageBytes)return false;
        const auto index=offset/4u;
        if(occupied[index])return false;
        occupied[index]=true;
        return true;
    };
    if(!claim(0u) || !claim(4u) || !claim(0x30u) ||
       !claim(0x9Cu))return false;
    for(const auto offset:kOriginalEarlyZeroOffsets)
        if(!claim(offset))return false;
    for(const auto offset:kOriginalLaterZeroOffsets)
        if(!claim(offset))return false;
    std::size_t count=0;
    for(const bool is_written:occupied)
        if(is_written)++count;
    return count==24u && !occupied[0x34u/4u] &&
           !occupied[0x94u/4u] && !occupied[0x98u/4u];
}
static_assert(ConstructorOffsetContract(),
              "original soundscape constructor offsets must remain disjoint and bounded");
static_assert(kOriginalSlot10FlagOffset>=kOwnedKnownStorageBytes,
              "slot10 guard is outside the authenticated owned model");
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
enum class Slot10EarlyGate {
    SkippedObjectFlagZero,
    SkippedGlobalMissing,
    EnterUnreconstructedUpdate
};
// Mirrors only the two original early branches. No claim about what
// happens after entry. The caller supplies non-pointer independent
// booleans, never genuine original engine memory.
constexpr Slot10EarlyGate CheckOriginalSlot10EarlyGate(
    bool byte_d0_nonzero,bool global_pointer_nonnull) noexcept {
    if(!byte_d0_nonzero)return Slot10EarlyGate::SkippedObjectFlagZero;
    if(!global_pointer_nonnull)return Slot10EarlyGate::SkippedGlobalMissing;
    return Slot10EarlyGate::EnterUnreconstructedUpdate;
}
inline bool OriginalVtableSlotIsSingleRet(std::size_t slot) noexcept {
    return slot==4 || slot==6 || slot==8 || slot==9;
}
} // namespace sin_independent::soundscape_original_object
