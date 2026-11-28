#pragma once
// Phase1722: 118 ORIGINAL 2006 PE32 fully authenticated canonical
// ClientClass entity creator method BODIES, source-ONLY C++17 recipes.
// 68 have exactly 60 executable bytes; 50 have exactly 86 bytes.
// Original complete concatenated function SHA256, in each family's
// ascending source ClientClass ordinal order:
// 60B x68: 401286913c256886a575d8d55985c7b3579143e9623b897fecb13a6cc548d10a
// 86B x50: ee7e1deea1a0608ca47aefc56bf8b2c6ca389ca19f35d86e641c1435cf27d84b
// 118 funcs concatenated family-60 then family-86 SHA256:
// a0fe29c6f806dd85c7f1e46ae18c5d167a5e50493faf48640425245ca2131cfa
// Original 60B and 86B x86 *normalized instruction-template* SHA256:
// 60B 7b4cd8eb8b67ef453cf3e8c586708b78fb34e6fff0bf5e4c5b02c36b25e1ca3d
// 86B f72e0856a850946a80e7332dd14ed37c15150062e8b154c940a81a9b4e8e0a06
// Whole owner original retail client.dll SHA256:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
//
// Source-factory success path: allocate original requested size via
// zeroing wrapper 0x240C4FF0, call its original constructor label,
// perform original virtual call at VTABLE byte offset +0x94, return
// an INTERIOR networkable subobject at +8. Failure returns NULL.
// The 86B branch also writes FOUR original vtable pointers to the
// constructed original entity. Original x86 executable method
// addresses/vtables are SOURCE LABELS, NEVER host callable pointers.
// The other 24 source generic allocation creators are structurally
// distinct and UNRESOLVED at complete-function level.
// All retail VClient011.slot5 and native +0/+4 callbacks guarded.
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_entity_allocation_prologues.hpp"

namespace sin_compat::original_client_canonical_creator_bodies {
namespace common=sin_compat::original_client_entity_allocation_prologues;
namespace creators=sin_compat::original_client_creator_source_map;
inline constexpr std::size_t kComplete60ByteMethods=68u;
inline constexpr std::size_t kComplete86ByteMethods=50u;
inline constexpr std::size_t kCompleteCanonicalBodies=118u;
inline constexpr std::size_t kUnresolvedCommonBodies=24u;
inline constexpr std::size_t kOriginal64BitVtablePointersPerInlineMethod=0u;
inline constexpr std::size_t kOriginal32BitVtableWritesPer86ByteMethod=4u;
inline constexpr std::uint32_t kOriginalAllocatorVA=0x240C4FF0u;
inline constexpr std::uint32_t kOriginalNetworkableOffset=8u;
inline constexpr std::uint32_t kOriginalNetworkInitializationVtableOffset=0x94u;
enum class Family : std::uint8_t {
    Unclassified, ConstructorReturnsObject60B, InlineFourVtables86B
};
struct OriginalConstructorLabel final {
    std::uint16_t originalClassOrdinal;
    std::uint32_t originalConstructorVA;
};
inline constexpr std::array<OriginalConstructorLabel,kComplete60ByteMethods>
k60ByteConstructors{{
    {9u, 0x2422a440u},
    {31u, 0x24226fd0u},
    {32u, 0x24226630u},
    {33u, 0x24225f90u},
    {34u, 0x24225a70u},
    {35u, 0x24225700u},
    {37u, 0x24221f70u},
    {38u, 0x24220c40u},
    {41u, 0x242201c0u},
    {43u, 0x2421eb90u},
    {44u, 0x2421dab0u},
    {45u, 0x2421c930u},
    {46u, 0x2421ca10u},
    {50u, 0x24219000u},
    {51u, 0x24218be0u},
    {55u, 0x242167c0u},
    {61u, 0x242159f0u},
    {62u, 0x24214870u},
    {67u, 0x241c5c10u},
    {68u, 0x241c2c30u},
    {70u, 0x241a3590u},
    {73u, 0x2412bdd0u},
    {77u, 0x240f9f60u},
    {78u, 0x240f93a0u},
    {80u, 0x240f8040u},
    {81u, 0x240f77b0u},
    {83u, 0x240f6a70u},
    {84u, 0x240f5f70u},
    {86u, 0x240f1d00u},
    {92u, 0x240ec2b0u},
    {93u, 0x240ebe00u},
    {94u, 0x240eaf80u},
    {95u, 0x240e7090u},
    {99u, 0x240e64a0u},
    {101u, 0x240e2f80u},
    {106u, 0x240e12f0u},
    {108u, 0x240df8b0u},
    {110u, 0x240db640u},
    {111u, 0x240db310u},
    {113u, 0x240d9910u},
    {114u, 0x240d9190u},
    {115u, 0x240d7c80u},
    {117u, 0x240d6aa0u},
    {118u, 0x240d5ed0u},
    {119u, 0x240d1400u},
    {122u, 0x240c3940u},
    {123u, 0x240c0b90u},
    {125u, 0x240bd7c0u},
    {128u, 0x2409e620u},
    {129u, 0x2409d360u},
    {130u, 0x24099380u},
    {131u, 0x24099030u},
    {132u, 0x24092bd0u},
    {133u, 0x2407b0f0u},
    {173u, 0x2406add0u},
    {174u, 0x24069de0u},
    {175u, 0x240681b0u},
    {176u, 0x24067b70u},
    {177u, 0x24065e00u},
    {178u, 0x24065850u},
    {179u, 0x24066aa0u},
    {180u, 0x24066790u},
    {181u, 0x240627d0u},
    {182u, 0x2405f250u},
    {183u, 0x2405d1a0u},
    {184u, 0x2405cc90u},
    {187u, 0x24057090u},
    {189u, 0x24057c10u}
}};
inline constexpr std::array<OriginalConstructorLabel,kComplete86ByteMethods>
k86ByteConstructors{{
    {0u, 0x24092bd0u},
    {1u, 0x24092bd0u},
    {2u, 0x24092bd0u},
    {3u, 0x24092bd0u},
    {4u, 0x24092bd0u},
    {5u, 0x24092bd0u},
    {6u, 0x24092bd0u},
    {7u, 0x24092bd0u},
    {8u, 0x24092bd0u},
    {10u, 0x24092bd0u},
    {11u, 0x24092bd0u},
    {12u, 0x24092bd0u},
    {13u, 0x24092bd0u},
    {14u, 0x24092bd0u},
    {15u, 0x24092bd0u},
    {16u, 0x24092bd0u},
    {17u, 0x24092bd0u},
    {18u, 0x24092bd0u},
    {19u, 0x24092bd0u},
    {20u, 0x24092bd0u},
    {21u, 0x24092bd0u},
    {22u, 0x24092bd0u},
    {23u, 0x24092bd0u},
    {24u, 0x24092bd0u},
    {25u, 0x24092bd0u},
    {26u, 0x24092bd0u},
    {27u, 0x24092bd0u},
    {28u, 0x24092bd0u},
    {29u, 0x24092bd0u},
    {39u, 0x240bd7c0u},
    {42u, 0x240b1ef0u},
    {56u, 0x240cce30u},
    {57u, 0x24092bd0u},
    {58u, 0x24092bd0u},
    {59u, 0x24092bd0u},
    {60u, 0x240b1ef0u},
    {64u, 0x2413cd50u},
    {65u, 0x24092bd0u},
    {72u, 0x240cce30u},
    {76u, 0x240cce30u},
    {85u, 0x240cce30u},
    {97u, 0x240cce30u},
    {100u, 0x240cce30u},
    {102u, 0x240cce30u},
    {103u, 0x240cce30u},
    {109u, 0x240cce30u},
    {112u, 0x240cce30u},
    {124u, 0x240cce30u},
    {127u, 0x240c3940u},
    {186u, 0x240cce30u}
}};
struct OriginalSourceCreatorPlan final {
    Family family;
    std::uint32_t originalConstructorVA;
    std::uint32_t originalRequestedAllocationBytes;
    std::uint32_t originalFullMethodBytes;
    std::uint32_t originalMethodPaddingBytes;
    std::uint32_t originalReturnSubobjectOffset;
    std::uint32_t originalVirtualCallVtableByteOffset;
    std::uint32_t originalVtableWrites;
};
constexpr OriginalSourceCreatorPlan SourceCanonicalBodyPlan(
    std::size_t ordinal,std::uint32_t originalSourceCreatorVA) noexcept {
    // Refuse source VAs not matching the authenticated class catalog.
    if(originalSourceCreatorVA==0u ||
       originalSourceCreatorVA!=creators::SourceCreatorVAForOrdinal(ordinal))
        return {Family::Unclassified,0u,0u,0u,0u,0u,0u,0u};
    for(const auto& record:common::kOriginalAllocationPrologues) {
        if(record.originalClassOrdinal!=ordinal ||
           record.originalCreatorVA!=originalSourceCreatorVA)continue;
        for(const auto& m:k60ByteConstructors)
            if(m.originalClassOrdinal==ordinal)
                return {Family::ConstructorReturnsObject60B,
                        m.originalConstructorVA,record.sourceObjectByteCount,
                        60u,4u,kOriginalNetworkableOffset,
                        kOriginalNetworkInitializationVtableOffset,0u};
        for(const auto& m:k86ByteConstructors)
            if(m.originalClassOrdinal==ordinal)
                return {Family::InlineFourVtables86B,
                        m.originalConstructorVA,record.sourceObjectByteCount,
                        86u,10u,kOriginalNetworkableOffset,
                        kOriginalNetworkInitializationVtableOffset,
                        kOriginal32BitVtableWritesPer86ByteMethod};
    }
    return {Family::Unclassified,0u,0u,0u,0u,0u,0u,0u};
}
constexpr bool SourceCanonicalInventoryConsistent() noexcept {
    if(kComplete60ByteMethods+kComplete86ByteMethods!=kCompleteCanonicalBodies ||
       kCompleteCanonicalBodies+kUnresolvedCommonBodies!=
           common::kSharedAllocatorPrologueCount)return false;
    std::size_t recognized=0u,unclassified=0u;
    for(std::size_t i=0u;i<creators::kOriginalClasses;++i) {
        const auto va=creators::SourceCreatorVAForOrdinal(i);
        const auto plan=SourceCanonicalBodyPlan(i,va);
        if(plan.family==Family::Unclassified) {
            if(plan.originalRequestedAllocationBytes!=0u)return false;
            for(const auto& commonRec:common::kOriginalAllocationPrologues)
                if(commonRec.originalClassOrdinal==i)++unclassified;
            continue;
        }
        if(creators::SourceCreatorIsTemp(i) ||
           plan.originalConstructorVA<0x24001000u ||
           plan.originalConstructorVA>=0x24275fbau ||
           plan.originalRequestedAllocationBytes<common::kSmallestAllocation ||
           plan.originalRequestedAllocationBytes>common::kLargestAllocation ||
           plan.originalReturnSubobjectOffset!=kOriginalNetworkableOffset ||
           plan.originalVirtualCallVtableByteOffset!=
               kOriginalNetworkInitializationVtableOffset)
            return false;
        ++recognized;
    }
    return recognized==kCompleteCanonicalBodies &&
           unclassified==kUnresolvedCommonBodies;
}
inline constexpr bool CanPublishRetailCreatorPointers() noexcept {return false;}
inline constexpr bool CanCreateRetailEntity() noexcept {return false;}
static_assert(SourceCanonicalInventoryConsistent());
static_assert(!CanPublishRetailCreatorPointers());
static_assert(!CanCreateRetailEntity());
} // namespace sin_compat::original_client_canonical_creator_bodies
