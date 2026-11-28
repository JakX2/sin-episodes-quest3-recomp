#pragma once
// Phase1728: deterministic, source-authenticated Windows PE32 CWorld
// constructor prefix before its FIRST side-effectful embedded object ctor.
// This is an inert snapshot of native x86 object bytes, not a real CWorld.
// All x86 vtable values below are SOURCE LABELS, never native host pointers.
// Constructor stops BEFORE original call at 0x240CCEDE -> 0x2411AEB0,
// which requires an unported virtual callback and engine-owned state.
// Native +0/+4 ClientClass callback pointers and retail slot5 stay NULL.
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_world_embedded_word_phase1726.hpp"

namespace sin_compat::original_client_world_ctor_prefix_phase1728 {
namespace leaf = sin_compat::original_client_world_embedded_word_phase1726;
inline constexpr std::size_t kOwnerBytes = leaf::kOwnerAllocationBytes;
inline constexpr std::uint32_t kNativeConstructorVA = 0x240CCE30u;
inline constexpr std::uint32_t kPrefixEndsBeforeCallVA = 0x240CCEDEu;
inline constexpr std::uint32_t kBlockedEmbeddedCtorVA = 0x2411AEB0u;
inline constexpr std::size_t kPrefixBytecodeLength = 174u;

enum class Size : std::uint8_t { Byte=1u, DWord=4u };
struct SourceStore final {
    std::uint16_t nativeObjectOffset;
    Size size;
    std::uint32_t nativeValue;
};
// Ordered original source writes. Earlier three vtable labels at offsets
// +4/+8/+C are deliberately OVERWRITTEN by later CWorld RTTI vtables;
// preserving their order lets the audit compare source instruction order.
// One zero store (+0xA4) executes inside the already reconstructed 9B leaf.
inline constexpr std::array<SourceStore, 27> kOrderedOriginalWrites{{
    {0x004u,Size::DWord,0x2427DEBCu},
    {0x008u,Size::DWord,0x242762F4u},
    {0x00Cu,Size::DWord,0x24288E90u},
    {0x000u,Size::DWord,0x242895ECu},
    {0x004u,Size::DWord,0x24289584u},
    {0x008u,Size::DWord,0x24289550u},
    {0x00Cu,Size::DWord,0x24289538u},
    {0x010u,Size::DWord,0u},
    {0x014u,Size::DWord,0u},
    {0x018u,Size::DWord,0u},
    {0x01Cu,Size::DWord,0u},
    {0x020u,Size::DWord,0u},
    {0x024u,Size::Byte,0u},
    {0x028u,Size::DWord,0u},
    {0x0A4u,Size::DWord,0u},
    {0x0B4u,Size::DWord,0xFFFFFFFFu},
    {0x0C8u,Size::DWord,0u},
    {0x0CCu,Size::DWord,0u},
    {0x0D0u,Size::DWord,0u},
    {0x0D4u,Size::DWord,0u},
    {0x0D8u,Size::DWord,0u},
    {0x14Cu,Size::DWord,0xFFFFFFFFu},
    {0x154u,Size::DWord,0xFFFFFFFFu},
    {0x158u,Size::DWord,0xFFFFFFFFu},
    {0x15Cu,Size::DWord,0xFFFFFFFFu},
    {0x160u,Size::DWord,0xFFFFFFFFu},
    {0x164u,Size::DWord,0xFFFFFFFFu},
}};

enum class Status : std::uint8_t {
    ProducedInertPrefixSnapshot,
    RefusedNullBuffer,
    RefusedTruncatedAllocation
};
inline void StoreWordLE(std::uint8_t* const bytes,
                        std::uint32_t value) noexcept {
    for (unsigned i=0u;i<4u;++i)
        bytes[i]=static_cast<std::uint8_t>((value>>(8u*i))&0xFFu);
}
inline Status ApplyInertSourceConstructorPrefix(
    std::uint8_t* const owner, std::size_t bytes) noexcept {
    if(owner==nullptr) return Status::RefusedNullBuffer;
    if(bytes<kOwnerBytes) return Status::RefusedTruncatedAllocation;
    for(const auto& write:kOrderedOriginalWrites) {
        if(write.nativeObjectOffset==leaf::kEmbeddedWordOffset) {
            // Exactly reproduces the source 0x24195260 leaf's write; it
            // cannot be used to claim that its caller is reconstructed.
            const auto leafResult=leaf::ResetVerifiedEmbeddedWord(owner,bytes);
            if(leafResult.status!=leaf::ResetStatus::Applied)
                return Status::RefusedTruncatedAllocation;
        } else if(write.size==Size::Byte) {
            owner[write.nativeObjectOffset]=static_cast<std::uint8_t>(write.nativeValue);
        } else {
            StoreWordLE(owner+write.nativeObjectOffset,write.nativeValue);
        }
    }
    return Status::ProducedInertPrefixSnapshot;
}
constexpr bool AllStoresWithinOriginalOwner() noexcept {
    for(const auto& write:kOrderedOriginalWrites) {
        const auto n=static_cast<std::uint8_t>(write.size);
        if(n!=1u && n!=4u) return false;
        if(write.nativeObjectOffset+n>kOwnerBytes) return false;
    }
    return true;
}
inline constexpr bool CanPublishRetailCWorld() noexcept { return false; }
static_assert(kNativeConstructorVA+kPrefixBytecodeLength==kPrefixEndsBeforeCallVA);
static_assert(kOrderedOriginalWrites.size()==27u);
static_assert(AllStoresWithinOriginalOwner());
static_assert(!CanPublishRetailCWorld());
} // namespace sin_compat::original_client_world_ctor_prefix_phase1728
