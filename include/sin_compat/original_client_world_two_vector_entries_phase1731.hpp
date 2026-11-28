#pragma once
// Phase1731: reconstruct the TWO original CWorld constructor insertions
// through source 121-byte 0x24061220, for the zero-allocated owner path.
// This models the 115B native growth helper at 0x24060870 and its initial
// source 0->3 element-capacity allocation request, 3 * 12 == 36 bytes.
//
// A real 2006 allocation (0x2416F8F0) is NOT invoked. The caller-supplied
// host allocator callback only authorizes/records a 36-byte owned backing
// allocation, and a typed sidecar owns all three synthetic element records.
// Source pointers in the 2006 owner are represented by distinct opaque
// 32-bit test tokens, NEVER physical host/native executable pointers.
// First source index = 0, second source index = 1; neither insert shifts
// elements; native +0x10/+0x20 vector pointer slots are token labels.
// No full CWorld, networking, retail slot5 or ClientClass callbacks.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_client_world_two_vector_entries_phase1731 {
inline constexpr std::size_t kOwnerBytes = 1200u;
inline constexpr std::uint32_t kNativeConstructorVA=0x240CCE30u;
inline constexpr std::uint32_t kFirstOriginalCallVA=0x240CCFBEu;
inline constexpr std::uint32_t kSecondOriginalCallVA=0x240CCFE4u;
inline constexpr std::uint32_t kOriginalInsertVA=0x24061220u;
inline constexpr std::uint32_t kOriginalGrowVA=0x24060870u;
inline constexpr std::uint32_t kOriginalCRTAllocateVA=0x2416F8F0u;
inline constexpr std::uint32_t kOriginalCRTRellocateVA=0x2416F960u;
inline constexpr std::uint32_t kOriginalCRTMovedVA=0x24248B50u;
inline constexpr std::size_t kVectorOwnerOffset=0x10u;
inline constexpr std::size_t kVectorSlotBytes=12u;
inline constexpr std::size_t kInitialCapacity=3u;
inline constexpr std::size_t kInitialAllocationBytes=kVectorSlotBytes*kInitialCapacity;
inline constexpr std::size_t kInsertedEntries=2u;

struct HostOwnedSidecar final {
    enum class State : std::uint8_t { Empty, Allocating, Initialized };
    std::array<std::array<std::uint32_t,3>,kInitialCapacity> words{};
    State state=State::Empty;
};
using HostAllocationRequest=bool(*)(std::size_t bytes,void* context) noexcept;
enum class Status:std::uint8_t {
    ProducedTwoInsertSnapshot,
    RefusedNull,
    RefusedShort,
    RefusedInvalidOwnerToken,
    RefusedInvalidAllocationToken,
    RefusedOverlappingTokenRanges,
    RefusedUninitializedVectorExpected,
    RefusedSidecarNotEmpty,
    RefusedNoHostAllocator,
    RefusedHostAllocation
};
struct Result final {
    Status status;
    std::size_t firstSourceInsertionIndex;
    std::size_t secondSourceInsertionIndex;
    std::size_t sourceCapacity;
    std::size_t sourceAllocationBytes;
};
inline constexpr std::uint32_t kTokenMin=0x10001000u;
inline constexpr std::uint32_t kTokenMax=0x1FFF0000u;
constexpr bool SafeSyntheticToken(std::uint32_t token) noexcept {
    return token>=kTokenMin && token<=kTokenMax &&
           (token & 3u)==0u;
}
inline std::uint32_t LoadWordLE(const std::uint8_t* src) noexcept {
    return std::uint32_t(src[0]) | (std::uint32_t(src[1]) << 8u) |
           (std::uint32_t(src[2]) << 16u) | (std::uint32_t(src[3]) << 24u);
}
inline void StoreWordLE(std::uint8_t* dst,std::uint32_t value) noexcept {
    for(std::size_t i=0u;i<4u;++i)
        dst[i]=static_cast<std::uint8_t>((value>>(8u*i))&0xFFu);
}
inline Result MakeOriginalTwoInsertions(
    std::uint8_t* owner,std::size_t bytes,HostOwnedSidecar& backing,
    std::uint32_t syntheticOwnerToken,
    std::uint32_t syntheticAllocationToken,
    HostAllocationRequest hostAllocator,void* hostContext=nullptr) noexcept {
    const auto refusal=[](Status status) noexcept {
        return Result{status,0u,0u,0u,0u};
    };
    if(owner==nullptr)return refusal(Status::RefusedNull);
    if(bytes<kOwnerBytes)return refusal(Status::RefusedShort);
    if(!SafeSyntheticToken(syntheticOwnerToken))
        return refusal(Status::RefusedInvalidOwnerToken);
    if(!SafeSyntheticToken(syntheticAllocationToken))
        return refusal(Status::RefusedInvalidAllocationToken);
    // Tokens are labels only, but must not ambiguously identify the same
    // source owner and vector storage or overflow source pointer offsets.
    if((syntheticOwnerToken<=syntheticAllocationToken &&
        syntheticAllocationToken<syntheticOwnerToken+kOwnerBytes) ||
       (syntheticAllocationToken<syntheticOwnerToken &&
        syntheticOwnerToken<syntheticAllocationToken+kInitialAllocationBytes))
        return refusal(Status::RefusedOverlappingTokenRanges);
    for(std::size_t i=0u;i<5u;++i)
        if(LoadWordLE(owner+kVectorOwnerOffset+i*4u)!=0u)
            return refusal(Status::RefusedUninitializedVectorExpected);
    if(backing.state!=HostOwnedSidecar::State::Empty)
        return refusal(Status::RefusedSidecarNotEmpty);
    if(hostAllocator==nullptr)return refusal(Status::RefusedNoHostAllocator);
    // For exactly the two append operations, the original 115B growth
    // routine chooses capacity=3 from capacity=0 and calls malloc(36).
    // A denied host allocation changes neither owner nor backing storage.
    backing.state=HostOwnedSidecar::State::Allocating;
    if(!hostAllocator(kInitialAllocationBytes,hostContext)) {
        backing.state=HostOwnedSidecar::State::Empty;
        return refusal(Status::RefusedHostAllocation);
    }
    backing.words[0]={{2u,syntheticOwnerToken+0x220u,
                        syntheticOwnerToken+0x22Cu}};
    backing.words[1]={{2u,syntheticOwnerToken+0x25Cu,
                        syntheticOwnerToken+0x268u}};
    // The third source capacity slot is uninitialized malloc storage in
    // the original game. We make NO claim about its original contents.
    backing.state=HostOwnedSidecar::State::Initialized;
    StoreWordLE(owner+kVectorOwnerOffset+0u,syntheticAllocationToken);
    StoreWordLE(owner+kVectorOwnerOffset+4u,3u); // capacity
    // +8 = growth override, originally zero, remains zero.
    StoreWordLE(owner+kVectorOwnerOffset+12u,2u); // element count
    StoreWordLE(owner+kVectorOwnerOffset+16u,syntheticAllocationToken);
    return {Status::ProducedTwoInsertSnapshot,0u,1u,kInitialCapacity,
            kInitialAllocationBytes};
}
inline constexpr bool CanPublishRetailWorld() noexcept { return false; }
static_assert(kInitialAllocationBytes==36u && kInsertedEntries==2u);
static_assert(kVectorOwnerOffset+20u<kOwnerBytes);
static_assert(kTokenMax+kOwnerBytes<=0x20000000u);
static_assert(!CanPublishRetailWorld());
} // namespace sin_compat::original_client_world_two_vector_entries_phase1731
