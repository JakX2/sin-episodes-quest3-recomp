#pragma once
// Phase1718 source-only: 142 original 2006 PE32 entity creator
// prologues begin PUSH ESI; PUSH original object byte SIZE; CALL
// original allocator helper at 0x240C4FF0. Source calls/size bytes
// are never compiled as executable original x86 instructions.
// Whole unchanged source client.dll SHA256:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
// SHA256 of 142 original 11-byte prologues in class-slot5 head order:
// 3562d9fedfa0db8a3e570ef023130e519cfcc3c48c2248c5b0c6ce0aa059c1ba
// LF-normalized source metadata CSV SHA256:
// 68fe5b23a33cd1b724fe9aa3761c667ef63333efcbe7c4c3afc32eb13dfa5c0e
// These are ALLOCATION REQUEST SIZES, NOT complete object layouts,
// reliable destructor sizes, native creator ABIs or portable factories.
// No original entity allocation function is called. No engine objects
// are constructed; original +0/+4 ClientClass callback fields stay NULL.
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_creator_source_map.hpp"

namespace sin_compat::original_client_entity_allocation_prologues {
namespace creators=sin_compat::original_client_creator_source_map;
inline constexpr std::size_t kOriginalEntityCreators=149u;
inline constexpr std::size_t kSharedAllocatorPrologueCount=142u;
inline constexpr std::size_t kOtherEntityCreatorCount=7u;
inline constexpr std::size_t kDistinctAllocationSizes=69u;
inline constexpr std::uint32_t kOriginalAllocatorVA=0x240C4FF0u;
inline constexpr std::size_t kOriginalPrologueExecutableBytes=11u;
inline constexpr std::uint32_t kSmallestAllocation=1152u;
inline constexpr std::uint32_t kLargestAllocation=10048u;
struct OriginalSourceAllocation final {
    std::uint16_t originalClassOrdinal;
    std::uint16_t sourceObjectByteCount;
    std::uint32_t originalCreatorVA;
};
inline constexpr std::array<OriginalSourceAllocation,kSharedAllocatorPrologueCount>
kOriginalAllocationPrologues{{
    {0u,2096u,0x2422b500u},
    {1u,2096u,0x2422afe0u},
    {2u,2096u,0x2422b3c0u},
    {3u,2096u,0x2422ad40u},
    {4u,2096u,0x2422ab70u},
    {5u,2096u,0x2422a9a0u},
    {6u,2096u,0x2422a7d0u},
    {7u,2096u,0x2422b620u},
    {8u,2096u,0x2422b240u},
    {9u,3464u,0x2422a4f0u},
    {10u,2096u,0x24229d30u},
    {11u,2096u,0x24229690u},
    {12u,2096u,0x242294c0u},
    {13u,2096u,0x242292f0u},
    {14u,2096u,0x24229bb0u},
    {15u,2096u,0x24229050u},
    {16u,2096u,0x24228e80u},
    {17u,2096u,0x24228cb0u},
    {18u,2096u,0x24228ae0u},
    {19u,2096u,0x24228910u},
    {20u,2096u,0x24228740u},
    {21u,2096u,0x24228570u},
    {22u,2096u,0x24229e50u},
    {23u,2080u,0x242282c0u},
    {24u,2096u,0x242280f0u},
    {25u,2096u,0x24227f20u},
    {26u,2096u,0x24227d50u},
    {27u,2096u,0x24227b80u},
    {28u,2096u,0x24229920u},
    {29u,2080u,0x242278e0u},
    {30u,1944u,0x242276e0u},
    {31u,2136u,0x24227060u},
    {32u,1968u,0x24226850u},
    {33u,1960u,0x242262e0u},
    {34u,1952u,0x24225c90u},
    {35u,2808u,0x24225800u},
    {37u,1328u,0x24222100u},
    {38u,1160u,0x242213c0u},
    {39u,1928u,0x24220890u},
    {40u,1336u,0x242204c0u},
    {41u,1160u,0x242202a0u},
    {42u,3448u,0x2421f440u},
    {43u,3920u,0x2421ece0u},
    {44u,3464u,0x2421db90u},
    {45u,3464u,0x2421cdd0u},
    {46u,3496u,0x2421cb20u},
    {47u,1552u,0x2421bce0u},
    {49u,1192u,0x2421aaa0u},
    {50u,1192u,0x242190e0u},
    {51u,1168u,0x24218d60u},
    {52u,2048u,0x242184f0u},
    {53u,1936u,0x24218250u},
    {54u,1184u,0x24217180u},
    {55u,4688u,0x242168a0u},
    {56u,1152u,0x24216540u},
    {57u,2096u,0x24216170u},
    {58u,2096u,0x24216250u},
    {59u,2096u,0x242161f0u},
    {60u,3424u,0x24215df0u},
    {61u,1384u,0x24215ba0u},
    {62u,1640u,0x24215600u},
    {64u,1152u,0x24212920u},
    {65u,2096u,0x2420d5a0u},
    {66u,1160u,0x241cb710u},
    {67u,2872u,0x241c5d60u},
    {68u,1240u,0x241c2e80u},
    {70u,3352u,0x241a49e0u},
    {71u,1152u,0x2413cfd0u},
    {72u,1152u,0x2412b030u},
    {73u,1216u,0x2412bf40u},
    {74u,1160u,0x241285e0u},
    {76u,1160u,0x240fc880u},
    {77u,1296u,0x240fa880u},
    {78u,2192u,0x240f9440u},
    {79u,1160u,0x240f8450u},
    {80u,1512u,0x240f8120u},
    {81u,1224u,0x240f78a0u},
    {83u,1496u,0x240f6d30u},
    {84u,1192u,0x240f6050u},
    {85u,1176u,0x240f2260u},
    {86u,2032u,0x240f2030u},
    {87u,1160u,0x240ecdc0u},
    {88u,1968u,0x240ecb20u},
    {89u,1176u,0x240ecc70u},
    {90u,1976u,0x240ecc10u},
    {91u,1976u,0x240ecbb0u},
    {92u,1184u,0x240ec360u},
    {93u,3072u,0x240ebf80u},
    {94u,10048u,0x240eb0c0u},
    {95u,1968u,0x240e7140u},
    {96u,1952u,0x240e6c80u},
    {97u,1160u,0x240e6a60u},
    {98u,1928u,0x240e6790u},
    {99u,1376u,0x240e6540u},
    {100u,1168u,0x240e40b0u},
    {101u,1752u,0x240e3f40u},
    {102u,1160u,0x240e2770u},
    {103u,1152u,0x240e25a0u},
    {104u,1168u,0x240e21f0u},
    {106u,1448u,0x240e1770u},
    {107u,1160u,0x240e0a50u},
    {108u,2224u,0x240dfcd0u},
    {109u,1168u,0x240dcc70u},
    {110u,1424u,0x240db940u},
    {111u,3768u,0x240db8e0u},
    {112u,3768u,0x240da0a0u},
    {113u,2160u,0x240d9d10u},
    {114u,1408u,0x240d9730u},
    {115u,1208u,0x240d82e0u},
    {116u,1184u,0x240d6d90u},
    {117u,1944u,0x240d6b50u},
    {118u,4552u,0x240d6160u},
    {119u,3064u,0x240d1570u},
    {121u,1160u,0x240c41b0u},
    {122u,3400u,0x240c39a0u},
    {123u,2056u,0x240c0c50u},
    {124u,1160u,0x240b3b50u},
    {125u,1928u,0x240bdbf0u},
    {126u,1160u,0x240b6b30u},
    {127u,3416u,0x240b1fc0u},
    {128u,1320u,0x2409edf0u},
    {129u,2008u,0x2409d530u},
    {130u,1368u,0x240993e0u},
    {131u,3440u,0x24099150u},
    {132u,2080u,0x24094110u},
    {133u,1160u,0x2407b1e0u},
    {173u,1528u,0x2406b6e0u},
    {174u,1616u,0x24069f40u},
    {175u,1496u,0x240687c0u},
    {176u,1464u,0x240685b0u},
    {177u,1520u,0x24067980u},
    {178u,1416u,0x24067870u},
    {179u,1496u,0x24068550u},
    {180u,1496u,0x240684f0u},
    {181u,2096u,0x24062930u},
    {182u,9040u,0x2405fc10u},
    {183u,5000u,0x2405dad0u},
    {184u,3192u,0x2405ce30u},
    {186u,1184u,0x240560d0u},
    {187u,1184u,0x24057880u},
    {188u,1600u,0x24055b70u},
    {189u,1256u,0x24057e20u}
}};
constexpr const OriginalSourceAllocation* SourceAllocationForClass(
    std::size_t ordinal,std::uint32_t originalCreatorVA) noexcept {
    for(const auto& record:kOriginalAllocationPrologues)
        if(record.originalClassOrdinal==ordinal &&
           record.originalCreatorVA==originalCreatorVA)
            return &record;
    return nullptr;
}
// Original-source metadata records the actual first 11 x86 bytes of
// each routine. A source-size is NOT permission to create a
// partially initialized entity or to invoke original 0x240C4FF0.
constexpr bool SourceAllocationInventoryConsistent() noexcept {
    std::uint32_t minSize=0xffffffffu,maxSize=0u;
    std::size_t uniqueSizes=0u;
    for(std::size_t i=0u;i<kOriginalAllocationPrologues.size();++i) {
        const auto& rec=kOriginalAllocationPrologues[i];
        if(rec.originalClassOrdinal>=creators::kOriginalClasses ||
           creators::SourceCreatorIsTemp(rec.originalClassOrdinal) ||
           rec.originalCreatorVA!=creators::SourceCreatorVAForOrdinal(
                rec.originalClassOrdinal) ||
           rec.sourceObjectByteCount<kSmallestAllocation ||
           rec.sourceObjectByteCount>kLargestAllocation ||
           (rec.sourceObjectByteCount&3u)!=0u)
            return false;
        minSize=rec.sourceObjectByteCount<minSize?
            rec.sourceObjectByteCount:minSize;
        maxSize=rec.sourceObjectByteCount>maxSize?
            rec.sourceObjectByteCount:maxSize;
        bool first=true;
        for(std::size_t j=0u;j<i;++j) {
            const auto& previous=kOriginalAllocationPrologues[j];
            if(previous.originalClassOrdinal==rec.originalClassOrdinal ||
               previous.originalCreatorVA==rec.originalCreatorVA)
                return false;
            if(previous.sourceObjectByteCount==rec.sourceObjectByteCount)
                first=false;
        }
        if(first)++uniqueSizes;
    }
    std::size_t exceptionCount=0u;
    for(std::size_t i=0u;i<creators::kOriginalClasses;++i) {
        if(creators::SourceCreatorIsTemp(i))continue;
        unsigned matches=0u;
        for(const auto& rec:kOriginalAllocationPrologues)
            if(rec.originalClassOrdinal==i)++matches;
        if(matches==0u)++exceptionCount;
        if(matches>1u)return false;
    }
    return minSize==kSmallestAllocation &&
           maxSize==kLargestAllocation &&
           uniqueSizes==kDistinctAllocationSizes &&
           exceptionCount==kOtherEntityCreatorCount &&
           creators::kOriginalEntityCreators==kOriginalEntityCreators;
}
inline constexpr bool CanCreateRetailEntity() noexcept {return false;}
static_assert(SourceAllocationInventoryConsistent());
static_assert(!CanCreateRetailEntity());
} // namespace sin_compat::original_client_entity_allocation_prologues
