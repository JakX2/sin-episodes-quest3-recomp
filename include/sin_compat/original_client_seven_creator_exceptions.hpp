#pragma once
// Phase1721: complete ORIGINAL 2006 PE32 creator source family census.
// The 142 common entity allocation prologues come from Phase1718.
// These seven ORIGINAL exceptional complete source functions are each
// SHA-pinned against the user's unchanged 2006 PE32 client.dll,
// but are NOT portable, callable entity creators or allocated objects.
// Original whole DLL SHA256:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
// Original seven-creator source CSV normalized-LF SHA256:
// 24b00c39fbf90a0af79448496cd4f1d91221dc752dc0021ae6a755c7f77f38e7
// Sources: 142 shared zeroing helper from native prologue; TWO other
// shared-zero-helper callers with different prologue; ONE direct
// allocator+manual zero; THREE raw allocator calls (constructors
// subsequently initialize objects); ONE CWorld global singleton.
// 148 source allocation request sizes recovered; CWorld has no
// allocation in this ClientClass creator. This does NOT mean 148
// native entities can be constructed in the replacement DLL.
// All retail VClient011.slot5 and native +0/+4 callbacks guarded.
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_entity_allocation_prologues.hpp"

namespace sin_compat::original_client_seven_creator_exceptions {
namespace common=sin_compat::original_client_entity_allocation_prologues;
namespace creators=sin_compat::original_client_creator_source_map;
inline constexpr std::size_t kOriginalEntityCreatorCount=149u;
inline constexpr std::size_t kExceptionalCreatorCount=7u;
inline constexpr std::size_t kSourceAllocationRequestCount=148u;
inline constexpr std::size_t kSourceExistingSingletonCreators=1u;
inline constexpr std::uint32_t kSharedZeroingHelperVA=0x240C4FF0u;
inline constexpr std::uint32_t kDirectNativeAllocatorVA=0x2416F8F0u;
inline constexpr std::uint32_t kOriginalRawAllocatorVA=0x2416FCF0u;
inline constexpr std::uint32_t kOriginalWorldSingletonGlobalVA=0x2433F3F4u;
enum class CreatorPath : std::uint8_t {
    NotAnEntity,
    SharedCommonZeroingHelper,
    SharedZeroingHelper,
    DirectAllocatorManualZero,
    RawAllocator,
    ExistingGlobalSingleton
};
struct ExceptionalCreatorSource final {
    std::uint16_t originalClassOrdinal;
    std::uint32_t originalCreatorVA;
    CreatorPath path;
    std::uint32_t nativeRequestedBytes;
    std::uint32_t originalAllocatorOrGlobalVA;
    std::uint16_t returnedNetworkableSubobjectOffset;
    std::uint16_t originalVirtualCallVtableByteOffset;
    std::uint16_t completeOriginalMethodBytes;
};
inline constexpr std::array<ExceptionalCreatorSource,kExceptionalCreatorCount>
kSevenNativeSourceExceptions{{
    {48u,0x2421ba80u,CreatorPath::SharedZeroingHelper,1168u,0x240c4ff0u,8u,148u,110u},
    {69u,0x241a3b20u,CreatorPath::SharedZeroingHelper,3408u,0x240c4ff0u,8u,148u,116u},
    {75u,0x240fc920u,CreatorPath::ExistingGlobalSingleton,0u,0x2433f3f4u,8u,148u,40u},
    {120u,0x240cd470u,CreatorPath::DirectAllocatorManualZero,1152u,0x2416f8f0u,8u,148u,76u},
    {143u,0x24079690u,CreatorPath::RawAllocator,28u,0x2416fcf0u,4u,48u,88u},
    {170u,0x2406d070u,CreatorPath::RawAllocator,76u,0x2416fcf0u,4u,48u,54u},
    {190u,0x24053dc0u,CreatorPath::RawAllocator,16u,0x2416fcf0u,4u,48u,82u}
}};
struct EntityCreatorSourcePlan final {
    CreatorPath path;
    std::uint32_t requestBytes;
    std::uint32_t originalCreatorVA;
    std::uint32_t originalAllocatorOrGlobalVA;
    std::uint32_t returnedNetworkableSubobjectOffset;
    std::uint32_t originalVirtualCallVtableByteOffset;
};
constexpr EntityCreatorSourcePlan SourcePlanForEntity(
    std::size_t originalClassOrdinal,std::uint32_t originalCreatorVA) noexcept {
    // C++17 constexpr must not depend on comparing a pointer
    // to the inside of a global std::array against nullptr.
    // Use value-based source identity comparison throughout.
    for(const auto& standard:common::kOriginalAllocationPrologues)
        if(standard.originalClassOrdinal==originalClassOrdinal &&
           standard.originalCreatorVA==originalCreatorVA)
            return {CreatorPath::SharedCommonZeroingHelper,
                    standard.sourceObjectByteCount,originalCreatorVA,
                    kSharedZeroingHelperVA,0u,0u}; // body NOT audited yet
    for(const auto& special:kSevenNativeSourceExceptions)
        if(special.originalClassOrdinal==originalClassOrdinal &&
           special.originalCreatorVA==originalCreatorVA)
            return {special.path,special.nativeRequestedBytes,
                    special.originalCreatorVA,
                    special.originalAllocatorOrGlobalVA,
                    special.returnedNetworkableSubobjectOffset,
                    special.originalVirtualCallVtableByteOffset};
    return {CreatorPath::NotAnEntity,0u,0u,0u,0u,0u};
}
constexpr bool SourceEntityCreatorFamilyComplete() noexcept {
    std::size_t exceptions=0u,allocated=0u,global=0u;
    for(const auto& special:kSevenNativeSourceExceptions) {
        const auto ord=std::size_t(special.originalClassOrdinal);
        if(ord>=creators::kOriginalClasses ||
           creators::SourceCreatorIsTemp(ord) ||
           creators::SourceCreatorVAForOrdinal(ord)!=special.originalCreatorVA)
            return false;
        // A seven-way exception must not appear in the 142
        // source common-allocator class/creator inventory.
        for(const auto& regular:common::kOriginalAllocationPrologues)
            if(regular.originalClassOrdinal==ord ||
               regular.originalCreatorVA==special.originalCreatorVA)
                return false;
        if(special.path==CreatorPath::ExistingGlobalSingleton) {
            if(special.nativeRequestedBytes!=0u ||
               special.originalAllocatorOrGlobalVA!=kOriginalWorldSingletonGlobalVA)
                return false;
            ++global;
        }else {
            if(special.nativeRequestedBytes==0u ||
               special.nativeRequestedBytes>common::kLargestAllocation ||
               (special.nativeRequestedBytes&3u)!=0u)
                return false;
            if(special.path==CreatorPath::SharedZeroingHelper &&
               special.originalAllocatorOrGlobalVA!=kSharedZeroingHelperVA)
                return false;
            if(special.path==CreatorPath::DirectAllocatorManualZero &&
               special.originalAllocatorOrGlobalVA!=kDirectNativeAllocatorVA)
                return false;
            if(special.path==CreatorPath::RawAllocator &&
               special.originalAllocatorOrGlobalVA!=kOriginalRawAllocatorVA)
                return false;
            ++allocated;
        }
        for(const auto& other:kSevenNativeSourceExceptions)
            if(&special!=&other &&
               (special.originalClassOrdinal==other.originalClassOrdinal ||
                special.originalCreatorVA==other.originalCreatorVA))
                return false;
        ++exceptions;
    }
    std::size_t allEntities=0u,sourceAllocations=0u;
    for(std::size_t i=0u;i<creators::kOriginalClasses;++i) {
        const auto va=creators::SourceCreatorVAForOrdinal(i);
        const auto plan=SourcePlanForEntity(i,va);
        if(creators::SourceCreatorIsTemp(i)) {
            if(plan.path!=CreatorPath::NotAnEntity)return false;
            continue;
        }
        if(plan.path==CreatorPath::NotAnEntity)return false;
        if(plan.path!=CreatorPath::ExistingGlobalSingleton)++sourceAllocations;
        ++allEntities;
    }
    return exceptions==kExceptionalCreatorCount && allocated==6u &&
           global==kSourceExistingSingletonCreators &&
           allEntities==kOriginalEntityCreatorCount &&
           sourceAllocations==kSourceAllocationRequestCount &&
           common::SourceAllocationInventoryConsistent();
}
inline constexpr bool CanCreateRetailEntity() noexcept {return false;}
static_assert(SourceEntityCreatorFamilyComplete());
static_assert(!CanCreateRetailEntity());
} // namespace sin_compat::original_client_seven_creator_exceptions
