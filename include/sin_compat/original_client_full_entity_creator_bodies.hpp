#pragma once
// Phase1722: COMPLETE original x86 executable method bodies for the
// 142 COMMON entity ClientClass creators, not just 11-byte prologues.
// Original source client.dll SHA256:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
// Source 142 native COMPLETE creator methods concatenated in genuine
// VClient011.slot5 original class order SHA256:
// 0badb693b0f8c3977a74b182c1fa696bbd6f620e40c0acbbd33746933a9ac902
// Original-free source CSV LF SHA256:
// 3393a7ef2ddaefa2ff0c4a3d0cb6539ae165d764f3a97a42f4b177e5483f2530
// Source 142 body methods encode TWO distinct constructor patterns:
// 68 constructed-first 60-byte methods; 74 vtable-stamped methods
// with native x86 method lengths 86,92,96,97,106,107,113,117 bytes.
// Every one invokes an ORIGINAL additional native constructor helper,
// the virtual method at vtable byte offset +0x94, and returns
// original networkable subobject base +8 on success.
// 12 original source functions end with no INT3 padding and
// directly abut the following executable method. Full SHA checks
// and actual RET/failure path witnesses avoid false boundaries.
//
// NOTE: these source identities and constructor TARGET addresses
// are NOT executable in the independent Win32/Android runtime.
// Constructor helper bodies, real entity layouts, objects/vtables,
// failure cleanup, global engine side effects and lifetime remain
// UNIMPLEMENTED. ClientClass +0/+4 callbacks remain NULL and
// VClient011.slot5 stays guarded.
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_entity_allocation_prologues.hpp"
#include "sin_compat/original_client_seven_creator_exceptions.hpp"

namespace sin_compat::original_client_full_entity_creator_bodies {
namespace common=sin_compat::original_client_entity_allocation_prologues;
namespace family=sin_compat::original_client_seven_creator_exceptions;
namespace creators=sin_compat::original_client_creator_source_map;
inline constexpr std::size_t kCommonCompleteSourceBodies=142u;
inline constexpr std::size_t kConstructThenVirtualCount=68u;
inline constexpr std::size_t kVtableStampThenVirtualCount=74u;
inline constexpr std::size_t kSourceNoPaddingBodies=12u;
inline constexpr std::size_t kDistinctSourceConstructorTargetFunctions=73u;
inline constexpr std::uint32_t kNativeVirtualCallByteOffset=0x94u;
inline constexpr std::uint32_t kOriginalReturnedSubobjectOffset=8u;
enum class CreatorBodyFamily : std::uint8_t {
    ConstructThenVirtual,
    VtableStampThenVirtual
};
struct CreatorBodySource final {
    std::uint16_t originalClassOrdinal;
    std::uint32_t originalCreatorVA;
    std::uint16_t originalRequestedBytes;
    std::uint16_t completeExecutableBytes;
    std::uint32_t originalConstructorTargetVA;
    std::uint8_t originalTrailingInt3Bytes;
    CreatorBodyFamily methodFamily;
};
inline constexpr std::array<CreatorBodySource,kCommonCompleteSourceBodies>
kSourceCompleteCreatorBodies{{
    {0u,0x2422b500u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {1u,0x2422afe0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {2u,0x2422b3c0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {3u,0x2422ad40u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {4u,0x2422ab70u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {5u,0x2422a9a0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {6u,0x2422a7d0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {7u,0x2422b620u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {8u,0x2422b240u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {9u,0x2422a4f0u,3464u,60u,0x2422a440u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {10u,0x24229d30u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {11u,0x24229690u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {12u,0x242294c0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {13u,0x242292f0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {14u,0x24229bb0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {15u,0x24229050u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {16u,0x24228e80u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {17u,0x24228cb0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {18u,0x24228ae0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {19u,0x24228910u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {20u,0x24228740u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {21u,0x24228570u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {22u,0x24229e50u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {23u,0x242282c0u,2080u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {24u,0x242280f0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {25u,0x24227f20u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {26u,0x24227d50u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {27u,0x24227b80u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {28u,0x24229920u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {29u,0x242278e0u,2080u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {30u,0x242276e0u,1944u,96u,0x240bd7c0u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {31u,0x24227060u,2136u,60u,0x24226fd0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {32u,0x24226850u,1968u,60u,0x24226630u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {33u,0x242262e0u,1960u,60u,0x24225f90u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {34u,0x24225c90u,1952u,60u,0x24225a70u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {35u,0x24225800u,2808u,60u,0x24225700u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {37u,0x24222100u,1328u,60u,0x24221f70u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {38u,0x242213c0u,1160u,60u,0x24220c40u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {39u,0x24220890u,1928u,86u,0x240bd7c0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {40u,0x242204c0u,1336u,96u,0x240f9f60u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {41u,0x242202a0u,1160u,60u,0x242201c0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {42u,0x2421f440u,3448u,86u,0x240b1ef0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {43u,0x2421ece0u,3920u,60u,0x2421eb90u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {44u,0x2421db90u,3464u,60u,0x2421dab0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {45u,0x2421cdd0u,3464u,60u,0x2421c930u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {46u,0x2421cb20u,3496u,60u,0x2421ca10u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {47u,0x2421bce0u,1552u,96u,0x240f9f60u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {49u,0x2421aaa0u,1192u,117u,0x240cce30u,11u,CreatorBodyFamily::VtableStampThenVirtual},
    {50u,0x242190e0u,1192u,60u,0x24219000u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {51u,0x24218d60u,1168u,60u,0x24218be0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {52u,0x242184f0u,2048u,97u,0x240bd7c0u,15u,CreatorBodyFamily::VtableStampThenVirtual},
    {53u,0x24218250u,1936u,96u,0x240bd7c0u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {54u,0x24217180u,1184u,107u,0x240cce30u,5u,CreatorBodyFamily::VtableStampThenVirtual},
    {55u,0x242168a0u,4688u,60u,0x242167c0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {56u,0x24216540u,1152u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {57u,0x24216170u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {58u,0x24216250u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {59u,0x242161f0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {60u,0x24215df0u,3424u,86u,0x240b1ef0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {61u,0x24215ba0u,1384u,60u,0x242159f0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {62u,0x24215600u,1640u,60u,0x24214870u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {64u,0x24212920u,1152u,86u,0x2413cd50u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {65u,0x2420d5a0u,2096u,86u,0x24092bd0u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {66u,0x241cb710u,1160u,96u,0x240cce30u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {67u,0x241c5d60u,2872u,60u,0x241c5c10u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {68u,0x241c2e80u,1240u,60u,0x241c2c30u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {70u,0x241a49e0u,3352u,60u,0x241a3590u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {71u,0x2413cfd0u,1152u,92u,0x240cce30u,4u,CreatorBodyFamily::VtableStampThenVirtual},
    {72u,0x2412b030u,1152u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {73u,0x2412bf40u,1216u,60u,0x2412bdd0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {74u,0x241285e0u,1160u,92u,0x240cce30u,4u,CreatorBodyFamily::VtableStampThenVirtual},
    {76u,0x240fc880u,1160u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {77u,0x240fa880u,1296u,60u,0x240f9f60u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {78u,0x240f9440u,2192u,60u,0x240f93a0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {79u,0x240f8450u,1160u,92u,0x240cce30u,4u,CreatorBodyFamily::VtableStampThenVirtual},
    {80u,0x240f8120u,1512u,60u,0x240f8040u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {81u,0x240f78a0u,1224u,60u,0x240f77b0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {83u,0x240f6d30u,1496u,60u,0x240f6a70u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {84u,0x240f6050u,1192u,60u,0x240f5f70u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {85u,0x240f2260u,1176u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {86u,0x240f2030u,2032u,60u,0x240f1d00u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {87u,0x240ecdc0u,1160u,96u,0x240cce30u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {88u,0x240ecb20u,1968u,106u,0x240e6bf0u,6u,CreatorBodyFamily::VtableStampThenVirtual},
    {89u,0x240ecc70u,1176u,106u,0x240e6a10u,6u,CreatorBodyFamily::VtableStampThenVirtual},
    {90u,0x240ecc10u,1976u,96u,0x240d6aa0u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {91u,0x240ecbb0u,1976u,96u,0x240d6aa0u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {92u,0x240ec360u,1184u,60u,0x240ec2b0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {93u,0x240ebf80u,3072u,60u,0x240ebe00u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {94u,0x240eb0c0u,10048u,60u,0x240eaf80u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {95u,0x240e7140u,1968u,60u,0x240e7090u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {96u,0x240e6c80u,1952u,113u,0x240d6aa0u,15u,CreatorBodyFamily::VtableStampThenVirtual},
    {97u,0x240e6a60u,1160u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {98u,0x240e6790u,1928u,96u,0x240cce30u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {99u,0x240e6540u,1376u,60u,0x240e64a0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {100u,0x240e40b0u,1168u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {101u,0x240e3f40u,1752u,60u,0x240e2f80u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {102u,0x240e2770u,1160u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {103u,0x240e25a0u,1152u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {104u,0x240e21f0u,1168u,113u,0x240cce30u,15u,CreatorBodyFamily::VtableStampThenVirtual},
    {106u,0x240e1770u,1448u,60u,0x240e12f0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {107u,0x240e0a50u,1160u,96u,0x240cce30u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {108u,0x240dfcd0u,2224u,60u,0x240df8b0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {109u,0x240dcc70u,1168u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {110u,0x240db940u,1424u,60u,0x240db640u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {111u,0x240db8e0u,3768u,60u,0x240db310u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {112u,0x240da0a0u,3768u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {113u,0x240d9d10u,2160u,60u,0x240d9910u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {114u,0x240d9730u,1408u,60u,0x240d9190u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {115u,0x240d82e0u,1208u,60u,0x240d7c80u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {116u,0x240d6d90u,1184u,106u,0x240cce30u,6u,CreatorBodyFamily::VtableStampThenVirtual},
    {117u,0x240d6b50u,1944u,60u,0x240d6aa0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {118u,0x240d6160u,4552u,60u,0x240d5ed0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {119u,0x240d1570u,3064u,60u,0x240d1400u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {121u,0x240c41b0u,1160u,96u,0x240cce30u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {122u,0x240c39a0u,3400u,60u,0x240c3940u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {123u,0x240c0c50u,2056u,60u,0x240c0b90u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {124u,0x240b3b50u,1160u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {125u,0x240bdbf0u,1928u,60u,0x240bd7c0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {126u,0x240b6b30u,1160u,96u,0x240cce30u,0u,CreatorBodyFamily::VtableStampThenVirtual},
    {127u,0x240b1fc0u,3416u,86u,0x240c3940u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {128u,0x2409edf0u,1320u,60u,0x2409e620u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {129u,0x2409d530u,2008u,60u,0x2409d360u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {130u,0x240993e0u,1368u,60u,0x24099380u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {131u,0x24099150u,3440u,60u,0x24099030u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {132u,0x24094110u,2080u,60u,0x24092bd0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {133u,0x2407b1e0u,1160u,60u,0x2407b0f0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {173u,0x2406b6e0u,1528u,60u,0x2406add0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {174u,0x24069f40u,1616u,60u,0x24069de0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {175u,0x240687c0u,1496u,60u,0x240681b0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {176u,0x240685b0u,1464u,60u,0x24067b70u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {177u,0x24067980u,1520u,60u,0x24065e00u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {178u,0x24067870u,1416u,60u,0x24065850u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {179u,0x24068550u,1496u,60u,0x24066aa0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {180u,0x240684f0u,1496u,60u,0x24066790u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {181u,0x24062930u,2096u,60u,0x240627d0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {182u,0x2405fc10u,9040u,60u,0x2405f250u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {183u,0x2405dad0u,5000u,60u,0x2405d1a0u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {184u,0x2405ce30u,3192u,60u,0x2405cc90u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {186u,0x240560d0u,1184u,86u,0x240cce30u,10u,CreatorBodyFamily::VtableStampThenVirtual},
    {187u,0x24057880u,1184u,60u,0x24057090u,4u,CreatorBodyFamily::ConstructThenVirtual},
    {188u,0x24055b70u,1600u,97u,0x240cce30u,15u,CreatorBodyFamily::VtableStampThenVirtual},
    {189u,0x24057e20u,1256u,60u,0x24057c10u,4u,CreatorBodyFamily::ConstructThenVirtual}
}};
constexpr bool SourceFullBodyInventoryConsistent() noexcept {
    std::size_t constructed=0u,stamped=0u,noPadding=0u,uniqueCtors=0u;
    std::size_t totalBytes=0u;
    for(std::size_t i=0u;i<kSourceCompleteCreatorBodies.size();++i) {
        const auto& row=kSourceCompleteCreatorBodies[i];
        if(row.originalClassOrdinal>=creators::kOriginalClasses ||
           creators::SourceCreatorIsTemp(row.originalClassOrdinal) ||
           creators::SourceCreatorVAForOrdinal(row.originalClassOrdinal)!=row.originalCreatorVA ||
           row.completeExecutableBytes<60u || row.completeExecutableBytes>117u ||
           row.originalConstructorTargetVA<0x24001000u ||
           row.originalConstructorTargetVA>=0x24275FBAu ||
           row.originalTrailingInt3Bytes>15u)
            return false;
        // Exact by-value source matching, NEVER constexpr pointer
        // comparisons to elements of another inline std::array.
        unsigned sourceMatches=0u;
        for(const auto& previous:common::kOriginalAllocationPrologues)
            if(previous.originalClassOrdinal==row.originalClassOrdinal &&
               previous.originalCreatorVA==row.originalCreatorVA &&
               previous.sourceObjectByteCount==row.originalRequestedBytes)
                ++sourceMatches;
        if(sourceMatches!=1u)return false;
        if(row.methodFamily==CreatorBodyFamily::ConstructThenVirtual) {
            if(row.completeExecutableBytes!=60u ||
               row.originalTrailingInt3Bytes!=4u)return false;
            ++constructed;
        }else if(row.methodFamily==CreatorBodyFamily::VtableStampThenVirtual) {
            if(row.completeExecutableBytes==60u)return false;
            ++stamped;
        }else return false;
        if(row.originalTrailingInt3Bytes==0u) {
            if(row.completeExecutableBytes!=96u ||
               row.methodFamily!=CreatorBodyFamily::VtableStampThenVirtual)
                return false;
            ++noPadding;
        }
        bool distinct=true;
        for(std::size_t j=0u;j<i;++j) {
            const auto& other=kSourceCompleteCreatorBodies[j];
            if(row.originalClassOrdinal==other.originalClassOrdinal ||
               row.originalCreatorVA==other.originalCreatorVA)
                return false;
            if(row.originalConstructorTargetVA==other.originalConstructorTargetVA)
                distinct=false;
        }
        if(distinct)++uniqueCtors;
        totalBytes+=row.completeExecutableBytes;
    }
    return constructed==kConstructThenVirtualCount &&
           stamped==kVtableStampThenVirtualCount &&
           noPadding==kSourceNoPaddingBodies &&
           uniqueCtors==kDistinctSourceConstructorTargetFunctions &&
           totalBytes==10770u &&
           family::SourceEntityCreatorFamilyComplete();
}
inline constexpr bool CanPublishRetailCreatorPointers() noexcept {return false;}
inline constexpr bool CanCreateRetailEntity() noexcept {return false;}
static_assert(SourceFullBodyInventoryConsistent());
static_assert(!CanCreateRetailEntity());
static_assert(!CanPublishRetailCreatorPointers());
} // namespace sin_compat::original_client_full_entity_creator_bodies
