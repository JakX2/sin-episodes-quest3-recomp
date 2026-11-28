#pragma once
// Phase1720: original 2006 PE32 allocator interface DISPATCH metadata.
// Original source 0x2416F8F0 is a complete 79 executable byte method:
// SHA256 e4682f7b52bf956d9769e11f7390b3f6b17076d25e36f35951211b5a6e1fe82a
// It lazy-initializes an original allocation context via original
// 0x2416F7F0 (complete 90 bytes, SHA256
// c2a27c407aa1fa127dc525282abb96d283e1026826f428c0855df9480123cce0).
// Source original global:
// 0x2437290C lazy-initialization bit0 flag
// 0x24372908 cached allocation context pointer
// 0x242761CC engine allocator interface slot (pointer to pointer)
// No context -> original engine interface vtable+4 (one size arg).
// Context -> original engine interface vtable+0 (size,context,flags=0).
//
// THIS HEADER IS NOT A MEMORY ALLOCATOR. It chooses only a source-
// authenticated host-side DISPATCH PLAN. Old PE32 globals, vtables,
// virtual methods, CRT Windows imports, native entity constructors,
// game objects, and allocator callback pointers are never dereferenced.
// No engine interface is fabricated. Retail VClient011.slot5 guarded.
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_entity_allocation_prologues.hpp"

namespace sin_compat::original_client_source_allocator_dispatch {
namespace source=sin_compat::original_client_entity_allocation_prologues;
inline constexpr std::uint32_t kNativeOriginalDispatcherVA=0x2416F8F0u;
inline constexpr std::size_t kNativeDispatcherExecutableBytes=79u;
inline constexpr std::uint32_t kNativeOriginalLazyResolverVA=0x2416F7F0u;
inline constexpr std::size_t kNativeResolverExecutableBytes=90u;
inline constexpr std::uint32_t kOriginalLazyFlagVA=0x2437290Cu;
inline constexpr std::uint32_t kOriginalCachedContextVA=0x24372908u;
inline constexpr std::uint32_t kOriginalEngineAllocatorInterfaceVA=0x242761CCu;
inline constexpr std::uint32_t kOriginalDirectVirtualByteOffset=4u;
inline constexpr std::uint32_t kOriginalContextVirtualByteOffset=0u;
inline constexpr std::uint32_t kSyntheticContextBase=0x10000000u;
inline constexpr std::uint32_t kSyntheticContextEndExclusive=0x20000000u;

enum class Route : std::uint8_t {
    RefusedUnrecognizedSource,
    RefusedUnboundEngineAllocator,
    RequiresOriginalContextResolver,
    RefusedOriginalOrUnsafeContextPointer,
    DirectVirtualSlot4,
    ContextVirtualSlot0
};
struct AllocationDispatchPlan final {
    Route selected;
    std::uint32_t originalSourceRequestedBytes;
    std::uint32_t originalVtableByteOffset;
    std::uint32_t syntheticHostContextToken;
};
struct HostOriginalAllocatorState final {
    bool hasAuthenticatedEngineInterfaceBinding=false;
    bool originalLazyFlagBit0=false;
    // The old Windows 2006 allocator context POINTER is not available
    // on other platforms. Only explicit, opaque synthetic host tokens
    // within the test-owned range are accepted here. 0 => source fallback.
    std::uint32_t syntheticContextToken=0u;
};
constexpr bool IsSyntheticBoundContext(std::uint32_t token) noexcept {
    return token>=kSyntheticContextBase &&
           token<kSyntheticContextEndExclusive &&
           (token&3u)==0u;
}

// Decision-only source ABI classification, with NO state changes,
// pointer reads or original vtable calls. If the lazy flag is absent,
// the original client CALLS 0x2416F7F0 and writes its pointer result
// before deciding the virtual dispatch. We refuse to guess that result.
inline AllocationDispatchPlan PlanOriginalSourceAllocation(
    std::size_t classOrdinal,std::uint32_t sourceCreatorVA,
    HostOriginalAllocatorState state) noexcept {
    const auto* allocation=source::SourceAllocationForClass(
        classOrdinal,sourceCreatorVA);
    if(allocation==nullptr)
        return {Route::RefusedUnrecognizedSource,0u,0u,0u};
    const auto bytes=static_cast<std::uint32_t>(
        allocation->sourceObjectByteCount);
    if(!state.hasAuthenticatedEngineInterfaceBinding)
        return {Route::RefusedUnboundEngineAllocator,bytes,0u,0u};
    if(!state.originalLazyFlagBit0)
        return {Route::RequiresOriginalContextResolver,bytes,0u,0u};
    if(state.syntheticContextToken==0u)
        return {Route::DirectVirtualSlot4,bytes,
                kOriginalDirectVirtualByteOffset,0u};
    if(!IsSyntheticBoundContext(state.syntheticContextToken))
        return {Route::RefusedOriginalOrUnsafeContextPointer,bytes,0u,0u};
    return {Route::ContextVirtualSlot0,bytes,
            kOriginalContextVirtualByteOffset,state.syntheticContextToken};
}
inline constexpr bool CanDispatchRetailAllocator() noexcept {return false;}
inline constexpr bool CanPublishRetailCreatorPointers() noexcept {return false;}
static_assert(kNativeOriginalDispatcherVA==0x2416F8F0u);
static_assert(kOriginalLazyFlagVA==kOriginalCachedContextVA+4u);
static_assert(!IsSyntheticBoundContext(0x24372908u));
static_assert(!IsSyntheticBoundContext(0x2416F8F0u));
static_assert(!CanDispatchRetailAllocator());
static_assert(!CanPublishRetailCreatorPointers());
} // namespace sin_compat::original_client_source_allocator_dispatch
