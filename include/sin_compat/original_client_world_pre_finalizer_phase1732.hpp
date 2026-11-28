#pragma once
// Phase1732: reconstruct the final 117 deterministic instruction bytes
// of the SHA-pinned CWorld constructor before original 414B finalizer.
// Completes a source-branch snapshot through 558 of the 573-byte native
// constructor's bytes: 398B pre-vector, two 12B vector appends, and 16
// final source field writes. This is NOT a complete constructor because
// native call 0x240CD05E -> 0x240C9F40 is still unported, including
// virtual +0xE8/+0xF0 and real engine owner state. Absolutely no real
// x86 vtables, callbacks, allocator or engine methods are executed here.
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_world_two_vector_entries_phase1731.hpp"

namespace sin_compat::original_client_world_pre_finalizer_phase1732 {
namespace vector=sin_compat::original_client_world_two_vector_entries_phase1731;
inline constexpr std::size_t kOwnerBytes=1200u;
inline constexpr std::size_t kPrefixBytes=558u;
inline constexpr std::uint32_t kConstructorVA=0x240CCE30u;
inline constexpr std::uint32_t kBlockedCallSiteVA=0x240CD05Eu;
inline constexpr std::uint32_t kBlockedNativeInitializerVA=0x240C9F40u;
inline constexpr std::size_t kBlockedNativeInitializerBytes=414u;

enum class Size:std::uint8_t {Byte=1u,Word=2u,DWord=4u};
struct Write final {
    std::uint16_t offset;
    Size size;
    std::uint32_t value;
};
inline constexpr std::array<Write,16> kOriginalWrites{{
    {0x45Cu,Size::DWord,0xFFFFFFFFu},
    {0x074u,Size::Word,0u},
    {0x150u,Size::Byte,0u},
    {0x058u,Size::DWord,0xFFu},
    {0x0C0u,Size::Byte,0u},
    {0x2E8u,Size::Byte,0u},
    {0x118u,Size::Byte,0u},
    {0x470u,Size::Byte,0u}, // original conditional branch on zero field
    {0x471u,Size::Byte,0u}, // original conditional branch on zero field
    {0x0BCu,Size::DWord,0u},
    {0x0C4u,Size::DWord,0xFFFFFFFFu},
    {0x0B0u,Size::Byte,1u},
    {0x0B1u,Size::Byte,0u},
    {0x138u,Size::DWord,0u},
    {0x459u,Size::Byte,0u},
    {0x0A8u,Size::DWord,0u},
}};

enum class Status:std::uint8_t {
    ProducedInertPreFinalizerSnapshot,
    RefusedNull,
    RefusedShort,
    RefusedVectorNotInitialized,
    RefusedUnexpectedVectorMetadata,
    RefusedAlreadyApplied
};
struct Result final {
    Status status;
    std::uint32_t nextBlockedSourceCall;
    std::uint32_t nextBlockedTarget;
};
inline void StoreLittleEndian(std::uint8_t* dst,std::uint32_t value,Size size) noexcept {
    for(std::size_t i=0u;i<static_cast<std::size_t>(size);++i)
        dst[i]=static_cast<std::uint8_t>((value>>(8u*i))&0xFFu);
}
inline Result ApplyInertPreFinalizerTail(
    std::uint8_t* owner,std::size_t bytes,
    const vector::HostOwnedSidecar& hostVector,
    std::uint32_t expectedAllocationToken) noexcept {
    const auto refusal=[](Status status) noexcept {
        return Result{status,0u,0u};
    };
    if(owner==nullptr)return refusal(Status::RefusedNull);
    if(bytes<kOwnerBytes)return refusal(Status::RefusedShort);
    if(hostVector.state!=vector::HostOwnedSidecar::State::Initialized)
        return refusal(Status::RefusedVectorNotInitialized);
    if(!vector::SafeSyntheticToken(expectedAllocationToken) ||
       vector::LoadWordLE(owner+0x10u)!=expectedAllocationToken ||
       vector::LoadWordLE(owner+0x14u)!=3u ||
       vector::LoadWordLE(owner+0x18u)!=0u ||
       vector::LoadWordLE(owner+0x1Cu)!=2u ||
       vector::LoadWordLE(owner+0x20u)!=expectedAllocationToken)
        return refusal(Status::RefusedUnexpectedVectorMetadata);
    // Original zeroing heap means +0x45C is zero before the tail stage,
    // and this sentinel becomes FFFFFFFF in the first source instruction.
    if(vector::LoadWordLE(owner+0x45Cu)!=0u)
        return refusal(Status::RefusedAlreadyApplied);
    for(const auto& write:kOriginalWrites)
        StoreLittleEndian(owner+write.offset,write.value,write.size);
    return {Status::ProducedInertPreFinalizerSnapshot,
            kBlockedCallSiteVA,kBlockedNativeInitializerVA};
}
inline constexpr bool CanPublishRetailWorld() noexcept {return false;}
constexpr bool WritesInBounds() noexcept {
    for(const auto& write:kOriginalWrites) {
        const auto n=static_cast<std::size_t>(write.size);
        if(n!=1u && n!=2u && n!=4u) return false;
        if(write.offset+n>kOwnerBytes) return false;
    }
    return true;
}
static_assert(kConstructorVA+kPrefixBytes==kBlockedCallSiteVA);
static_assert(WritesInBounds());
static_assert(!CanPublishRetailWorld());
} // namespace sin_compat::original_client_world_pre_finalizer_phase1732
