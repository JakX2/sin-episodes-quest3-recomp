#pragma once
// Phase1724: original SOURCE-ONLY complete executable-body fingerprint
// coverage for ALL 191 ClientClass creator functions, no retail code.
// Complete original 191 x86 creator bodies in linked-list ordinal order
// concatenate to 14,868 bytes, SHA256:
// c62a23ee2494ee996a00fb77745f734a821993f86546774b6d43669095767d5b
// 42 original temp event static getters = 6 bytes each (Phase1717).
// 118 original canonical entity factories = 68x60B +50x86B (Phase1722).
// 7 fully source-audited exceptional entity factory bodies (Phase1721).
// 24 noncanonical common-allocator factory methods, individually
// SHA256 authenticated and bounded, are recorded HERE (source CSV):
// CSV LF SHA256:
// 7d11b2d9eb4ad1c22d399cbad38fe8d9ce8c65e44a55c0d670f0ff732914a2d2
// Concatenated original 24 whole body SHA256:
// 9633b2d099c8882f69e517be7700b5d1d7a323655f1954ea8cdfc4790238f6c9
// Original client.dll whole SHA256:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
//
// WHOLE ORIGINAL METHOD HASH COVERAGE != RECOMPILATION! The 24 bodies
// contain unique construction and side-effect paths not modeled here.
// This is an immutable integrity inventory, NOT live 191 creator ABIs.
// Native32 +0/+4 executable callbacks remain NULL; retail slot5 guarded.
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_inline_factory_vtable_init.hpp"
#include "sin_compat/original_client_seven_creator_exceptions.hpp"
#include "sin_compat/original_client_temp_event_static_creators.hpp"

namespace sin_compat::original_client_all_creator_body_fingerprints {
namespace canonical=sin_compat::original_client_canonical_creator_bodies;
namespace special=sin_compat::original_client_seven_creator_exceptions;
namespace creators=sin_compat::original_client_creator_source_map;
namespace common=sin_compat::original_client_entity_allocation_prologues;
inline constexpr std::size_t kOriginalCreatorFunctions=191u;
inline constexpr std::size_t kOriginalTempGetterBodies=42u;
inline constexpr std::size_t kOriginalCanonical60Bodies=68u;
inline constexpr std::size_t kOriginalCanonical86Bodies=50u;
inline constexpr std::size_t kOriginalNoncanonicalCommonBodies=24u;
inline constexpr std::size_t kOriginalExceptionalBodies=7u;
inline constexpr std::size_t kOriginalConcatenatedExecutableBytes=14868u;
struct OriginalNoncanonicalSourceMethod final {
    std::uint16_t originalClassOrdinal;
    std::uint32_t originalCreatorVA;
    std::uint16_t originalRequestedBytes;
    std::uint16_t completeOriginalMethodBytes;
    std::uint8_t originalInt3PaddingBytes;
};
inline constexpr std::array<OriginalNoncanonicalSourceMethod,
                            kOriginalNoncanonicalCommonBodies>
kOriginal24NoncanonicalMethods{{
    {30u,0x242276e0u,1944u,295u,9u},
    {40u,0x242204c0u,1336u,774u,10u},
    {47u,0x2421bce0u,1552u,774u,10u},
    {49u,0x2421aaa0u,1192u,117u,11u},
    {52u,0x242184f0u,2048u,97u,15u},
    {53u,0x24218250u,1936u,358u,10u},
    {54u,0x24217180u,1184u,107u,5u},
    {66u,0x241cb710u,1160u,105u,7u},
    {71u,0x2413cfd0u,1152u,92u,4u},
    {74u,0x241285e0u,1160u,92u,4u},
    {79u,0x240f8450u,1160u,92u,4u},
    {87u,0x240ecdc0u,1160u,164u,12u},
    {88u,0x240ecb20u,1968u,106u,6u},
    {89u,0x240ecc70u,1176u,106u,6u},
    {90u,0x240ecc10u,1976u,202u,6u},
    {91u,0x240ecbb0u,1976u,298u,6u},
    {96u,0x240e6c80u,1952u,113u,15u},
    {98u,0x240e6790u,1928u,137u,7u},
    {104u,0x240e21f0u,1168u,113u,15u},
    {107u,0x240e0a50u,1160u,156u,4u},
    {116u,0x240d6d90u,1184u,106u,6u},
    {121u,0x240c41b0u,1160u,169u,7u},
    {126u,0x240b6b30u,1160u,1000u,8u},
    {188u,0x24055b70u,1600u,97u,15u}
}};
enum class WholeSourceBody : std::uint8_t {
    Unrecognized,
    OriginalStaticTempGetter6B,
    OriginalCanonicalEntity60B,
    OriginalCanonicalEntity86B,
    OriginalNoncanonicalEntity,
    OriginalExceptionalEntity
};
struct OriginalBodyIdentity final {
    WholeSourceBody kind;
    std::uint32_t completeMethodBytes;
    std::uint32_t trailingInt3Bytes;
    std::uint32_t originalSourceCreatorVA;
};
constexpr OriginalBodyIdentity WholeSourceBodyForCreator(
    std::size_t ordinal,std::uint32_t originalVA) noexcept {
    if(originalVA==0u || originalVA!=creators::SourceCreatorVAForOrdinal(ordinal))
        return {WholeSourceBody::Unrecognized,0u,0u,0u};
    if(creators::SourceCreatorIsTemp(ordinal))
        return {WholeSourceBody::OriginalStaticTempGetter6B,6u,10u,originalVA};
    const auto canonicalPlan=canonical::SourceCanonicalBodyPlan(
        ordinal,originalVA);
    if(canonicalPlan.family==canonical::Family::ConstructorReturnsObject60B)
        return {WholeSourceBody::OriginalCanonicalEntity60B,60u,4u,originalVA};
    if(canonicalPlan.family==canonical::Family::InlineFourVtables86B)
        return {WholeSourceBody::OriginalCanonicalEntity86B,86u,10u,originalVA};
    for(const auto& record:kOriginal24NoncanonicalMethods)
        if(record.originalClassOrdinal==ordinal &&
           record.originalCreatorVA==originalVA)
            return {WholeSourceBody::OriginalNoncanonicalEntity,
                    record.completeOriginalMethodBytes,
                    record.originalInt3PaddingBytes,originalVA};
    for(const auto& record:special::kSevenNativeSourceExceptions)
        if(record.originalClassOrdinal==ordinal &&
           record.originalCreatorVA==originalVA)
            return {WholeSourceBody::OriginalExceptionalEntity,
                    record.completeOriginalMethodBytes,0u,originalVA};
    return {WholeSourceBody::Unrecognized,0u,0u,0u};
}
constexpr bool All191OriginalSourceBodiesIdentified() noexcept {
    std::array<std::size_t,7> counts{};
    for(std::size_t i=0u;i<creators::kOriginalClasses;++i) {
        const auto va=creators::SourceCreatorVAForOrdinal(i);
        const auto id=WholeSourceBodyForCreator(i,va);
        const auto code=static_cast<std::size_t>(id.kind);
        if(code==0u || code>=counts.size() ||
           id.completeMethodBytes<4u || id.completeMethodBytes>1000u ||
           id.originalSourceCreatorVA!=va)return false;
        ++counts[code];
    }
    for(std::size_t i=0u;i<kOriginal24NoncanonicalMethods.size();++i) {
        const auto& item=kOriginal24NoncanonicalMethods[i];
        if(item.originalClassOrdinal>=creators::kOriginalClasses ||
           creators::SourceCreatorIsTemp(item.originalClassOrdinal) ||
           creators::SourceCreatorVAForOrdinal(item.originalClassOrdinal)!=
                item.originalCreatorVA ||
           item.originalRequestedBytes<common::kSmallestAllocation ||
           item.originalRequestedBytes>common::kLargestAllocation ||
           item.originalInt3PaddingBytes<4u ||
           item.completeOriginalMethodBytes<=86u)
            return false;
        bool found=false;
        for(const auto& rec:common::kOriginalAllocationPrologues)
            if(rec.originalClassOrdinal==item.originalClassOrdinal &&
               rec.originalCreatorVA==item.originalCreatorVA &&
               rec.sourceObjectByteCount==item.originalRequestedBytes)
                found=true;
        if(!found)return false;
        for(std::size_t j=0u;j<i;++j)
            if(kOriginal24NoncanonicalMethods[j].originalClassOrdinal==
                item.originalClassOrdinal)return false;
    }
    return counts[1]==kOriginalTempGetterBodies &&
           counts[2]==kOriginalCanonical60Bodies &&
           counts[3]==kOriginalCanonical86Bodies &&
           counts[4]==kOriginalNoncanonicalCommonBodies &&
           counts[5]==kOriginalExceptionalBodies &&
           counts[6]==0u;
}
inline constexpr bool CanPublishRetailCreatorPointers() noexcept {return false;}
static_assert(All191OriginalSourceBodiesIdentified());
static_assert(!CanPublishRetailCreatorPointers());
} // namespace sin_compat::original_client_all_creator_body_fingerprints
