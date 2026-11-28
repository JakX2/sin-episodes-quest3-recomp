#pragma once
// Phase1730: the first 398 bytes of the SHA-pinned 2006 PE32 CWorld
// constructor, up to and NOT including the first dynamic vector insertion
// call at 0x240CCFBE -> 0x24061220. Inert host-owned byte image only.
// This composes source-verified deterministic prefix (Phase1728),
// 26+210-byte zeroed interpolator (Phase1729), and the two 85B lazy
// statics (Phase1727), with the original 32 post-subobject writes.
// It does not create valid vtables or an actual CWorld entity.
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_world_ctor_prefix_phase1728.hpp"
#include "sin_compat/original_client_world_interpolator_zeroed_phase1729.hpp"
#include "sin_compat/original_client_world_lazy_static_phase1727.hpp"

namespace sin_compat::original_client_world_pre_vector_stage_phase1730 {
namespace prefix=sin_compat::original_client_world_ctor_prefix_phase1728;
namespace embedded=sin_compat::original_client_world_interpolator_zeroed_phase1729;
namespace lazy=sin_compat::original_client_world_lazy_static_phase1727;
inline constexpr std::size_t kOwnerBytes=1200u;
inline constexpr std::uint32_t kSourceVectorInsertionCallerVA=0x240CCFBEu;
inline constexpr std::uint32_t kUnportedVectorInsertionVA=0x24061220u;
inline constexpr std::size_t kSourceConstructorPrefixBytes=398u;

using Store=prefix::SourceStore;
using Size=prefix::Size;
// Native write order before first original 85-byte lazy-static call.
inline constexpr std::array<Store,8> kBeforeFirstStatic{{
    {0x16Cu,Size::DWord,0x24288E3Cu},
    {0x1D8u,Size::DWord,0xFFFFFFFFu},
    {0x1E0u,Size::DWord,0xFFFFFFFFu},
    {0x22Cu,Size::DWord,0x24278694u},
    {0x258u,Size::DWord,0x24289524u},
    {0x230u,Size::DWord,0u},
    {0x244u,Size::DWord,1u},
    {0x254u,Size::DWord,0u},
}};
inline constexpr std::array<Store,13> kBetweenStatics{{
    {0x248u,Size::DWord,0u},
    {0x24Cu,Size::Byte,0u},
    {0x250u,Size::DWord,1u},
    {0x234u,Size::DWord,0u},
    {0x238u,Size::DWord,0u},
    {0x23Cu,Size::DWord,0u},
    {0x240u,Size::DWord,0u},
    {0x22Cu,Size::DWord,0x24278AA4u},
    {0x268u,Size::DWord,0x242893F0u},
    {0x294u,Size::DWord,0x24289510u},
    {0x26Cu,Size::DWord,0u},
    {0x280u,Size::DWord,1u},
    {0x290u,Size::DWord,0u},
}};
inline constexpr std::array<Store,11> kAfterSecondStatic{{
    {0x284u,Size::DWord,0u},
    {0x288u,Size::Byte,0u},
    {0x28Cu,Size::DWord,1u},
    {0x270u,Size::DWord,0u},
    {0x274u,Size::DWord,0u},
    {0x278u,Size::DWord,0u},
    {0x27Cu,Size::DWord,0u},
    {0x268u,Size::DWord,0x24289418u},
    {0x460u,Size::DWord,0xFFFFFFFFu},
    {0x464u,Size::DWord,0xFFFFFFFFu},
    {0x468u,Size::DWord,0xFFFFFFFFu},
}};

template<std::size_t N>
inline void ApplyStores(std::uint8_t* owner,
                        const std::array<Store,N>& sourceStores) noexcept {
    for(const auto& step:sourceStores) {
        if(step.size==Size::Byte)
            owner[step.nativeObjectOffset]=static_cast<std::uint8_t>(step.nativeValue);
        else prefix::StoreWordLE(owner+step.nativeObjectOffset,step.nativeValue);
    }
}

enum class Status:std::uint8_t {
    ProducedInertPreVectorSnapshot,
    RefusedNull,
    RefusedShort,
    RefusedNonzeroOriginalAllocation,
    RefusedNoHostRegistrar,
    RefusedUnexpectedDependency
};
struct Result final {
    Status status;
    std::uint32_t nextBlockedSourceCall;
    std::uint32_t nextBlockedTarget;
};
// Host registrar is a safe event-recorder for a host-owned cleanup ONLY;
// the source x86 cleanup VAs passed to it are never executable callbacks.
inline Result MakeInertPreVectorSnapshot(
    std::uint8_t* owner, std::size_t bytes,
    lazy::HostOwnedStatic& firstStatic,
    lazy::HostOwnedStatic& secondStatic,
    lazy::HostCleanupRegistrar hostRegistrar,
    void* hostContext,
    const embedded::BssTripleBits& sourceBssValues) noexcept {
    if(owner==nullptr) return {Status::RefusedNull,0u,0u};
    if(bytes<kOwnerBytes) return {Status::RefusedShort,0u,0u};
    // The real original 0x240C4FF0 allocator returns 1200 zeroed bytes.
    // Reject all other initial states to avoid pretending unsupported
    // conditional branches in nested constructors have been implemented.
    for(std::size_t i=0u;i<kOwnerBytes;++i)
        if(owner[i]!=0u) return {Status::RefusedNonzeroOriginalAllocation,0u,0u};
    if(hostRegistrar==nullptr &&
       (((firstStatic.nativeModeledGuard&1u)==0u) ||
        ((secondStatic.nativeModeledGuard&1u)==0u)))
        return {Status::RefusedNoHostRegistrar,0u,0u};
    if(prefix::ApplyInertSourceConstructorPrefix(owner,bytes)!=
       prefix::Status::ProducedInertPrefixSnapshot)
        return {Status::RefusedUnexpectedDependency,0u,0u};
    if(embedded::InitializeZeroedSourceEmbeddedSubobject(
           owner,bytes,sourceBssValues).status!=
       embedded::Status::InitializedInertZeroedSubobject)
        return {Status::RefusedUnexpectedDependency,0u,0u};
    ApplyStores(owner,kBeforeFirstStatic);
    if(lazy::InitOnce(lazy::Family::First,firstStatic,hostRegistrar,hostContext).code==
       lazy::ResultCode::RefusedNoHostRegistrar)
        return {Status::RefusedUnexpectedDependency,0u,0u};
    ApplyStores(owner,kBetweenStatics);
    if(lazy::InitOnce(lazy::Family::Second,secondStatic,hostRegistrar,hostContext).code==
       lazy::ResultCode::RefusedNoHostRegistrar)
        return {Status::RefusedUnexpectedDependency,0u,0u};
    ApplyStores(owner,kAfterSecondStatic);
    return {Status::ProducedInertPreVectorSnapshot,
            kSourceVectorInsertionCallerVA,kUnportedVectorInsertionVA};
}
constexpr bool AllSourcePostStoresInBounds() noexcept {
    for(const auto& row:kBeforeFirstStatic)
        if(static_cast<std::size_t>(row.nativeObjectOffset)+
           static_cast<std::size_t>(row.size)>kOwnerBytes)return false;
    for(const auto& row:kBetweenStatics)
        if(static_cast<std::size_t>(row.nativeObjectOffset)+
           static_cast<std::size_t>(row.size)>kOwnerBytes)return false;
    for(const auto& row:kAfterSecondStatic)
        if(static_cast<std::size_t>(row.nativeObjectOffset)+
           static_cast<std::size_t>(row.size)>kOwnerBytes)return false;
    return true;
}
inline constexpr bool CanPublishRetailWorld() noexcept { return false; }
static_assert(prefix::kNativeConstructorVA+kSourceConstructorPrefixBytes==
              kSourceVectorInsertionCallerVA);
static_assert(kBeforeFirstStatic.size()+kBetweenStatics.size()+
              kAfterSecondStatic.size()==32u);
static_assert(AllSourcePostStoresInBounds());
static_assert(!CanPublishRetailWorld());
} // namespace sin_compat::original_client_world_pre_vector_stage_phase1730
