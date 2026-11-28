#pragma once
// Phase1698: real 2006 PE32 client.dll auxiliary static RecvTable metadata.
// A complete source-original SHA-pinned audit identifies 20 table objects,
// their constructor order-independent VA, property arrays and total
// 171 native 60-byte RecvProp slots. These are META labels only,
// not valid Windows runtime pointers or a safe network decoder.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_client_aux_recvtable_metadata {
inline constexpr std::size_t kOriginalClassAttachedTableCount=191u;
inline constexpr std::size_t kOriginalClassAttachedPropSlots=939u;
inline constexpr std::size_t kOriginalAuxiliaryTableCount=20u;
inline constexpr std::size_t kOriginalAuxiliaryPropSlots=171u;
inline constexpr std::size_t kOriginalStaticTableCount=211u;
inline constexpr std::size_t kOriginalStaticPropSlots=1110u;
inline constexpr std::size_t kOriginalRecvPropStride=60u;
inline constexpr std::size_t kOriginalTypedIntegerAux=75u;
inline constexpr std::size_t kOriginalTypedFloatAux=62u;
inline constexpr std::size_t kOriginalTypedVectorAux=28u;
inline constexpr std::size_t kOriginalTypedArrayAux=1u;
inline constexpr std::size_t kOriginalTypedNestedAux=5u;
inline constexpr std::size_t kOriginalOrdinaryPropertyConstructors=166u;
// Original type-5 static links: Phase1697 191 class-attached and
// Phase1698 the sole ordinary auxiliary link m_Local -> DT_Local.
inline constexpr std::size_t kOriginalClassOrdinaryNestedLinks=191u;
inline constexpr std::size_t kOriginalAuxOrdinaryNestedLinks=1u;
inline constexpr std::size_t kOriginalTotalOrdinaryNestedLinks=192u;
inline constexpr std::uint32_t kOriginalLocalPlayerTableVA=0x24338e8cu;
inline constexpr std::uint32_t kOriginalLocalChildTableVA=0x24338e74u;
inline constexpr std::uint32_t kOriginalLocalNativeMemberOffset=0x0d68u;
inline constexpr std::size_t kOriginalAlternativePropertyConstructors=5u;

struct Table final {
    std::string_view networkTableName;
    std::uint32_t originalTableVA;
    std::uint32_t originalPropertyArrayVA;
    std::uint32_t declaredPropertyCount;
};

// SOURCE 2006 client VA labels are sorted by VA, not by runtime
// ClientClass head order: aux tables aren't themselves ClientClasses.
inline constexpr std::array<Table,kOriginalAuxiliaryTableCount>
 kOriginalAuxiliaryTables{{
    {"DT_EnvWindShared",0x24323ddcu,0x243232d4u,11},
    {"DT_LocalActiveWeaponData",0x2432b964u,0x2432b4fcu,4},
    {"DT_LocalWeaponData",0x2432b97cu,0x2432b62cu,5},
    {"DT_BeamPredictableId",0x2432d124u,0x2432ca14u,2},
    {"DT_ServerAnimationData",0x24336278u,0x2433652cu,1},
    {"DT_Animationlayer",0x24337464u,0x2433719cu,5},
    {"DT_OverlayVars",0x2433747cu,0x24337424u,1},
    {"DT_BCCLocalPlayerExclusive",0x24337674u,0x24337504u,2},
    {"DT_AnimTimeMustBeFirst",0x24337944u,0x24337a74u,1},
    {"DT_PredictableId",0x2433795cu,0x24337af4u,2},
    {"DT_PlayerState",0x24338e5cu,0x24338f24u,1},
    {"DT_Local",0x24338e74u,0x2433936cu,49},
    {"DT_LocalPlayerExclusive",0x24338e8cu,0x24339f2cu,27},
    {"DT_ProxyToggle_ProxiedData",0x2433ece0u,0x2433ed8cu,1},
    {"DT_CollisionProperty",0x2435878cu,0x24358584u,8},
    {"DT_EffectData",0x24360f28u,0x24360a74u,20},
    {"DT_EntityParticleTrailInfo",0x24361044u,0x24360f8cu,3},
    {"DT_EnvHeadcrabCanisterShared",0x24379860u,0x243794b4u,15},
    {"DT_SE1GameRules",0x2437a398u,0x24379af0u,0},
    {"DT_HL2Local",0x2437b7b4u,0x2437b4a4u,13},
}};

constexpr std::size_t CountOriginalAuxiliarySlots() noexcept {
    std::size_t total=0;
    for(const Table& t:kOriginalAuxiliaryTables)total+=t.declaredPropertyCount;
    return total;
}
constexpr bool RealSourceTableNamesUnique() noexcept {
    for(std::size_t i=0;i<kOriginalAuxiliaryTables.size();++i)
        for(std::size_t j=i+1;j<kOriginalAuxiliaryTables.size();++j)
            if(kOriginalAuxiliaryTables[i].networkTableName==
                 kOriginalAuxiliaryTables[j].networkTableName ||
               kOriginalAuxiliaryTables[i].originalTableVA==
                 kOriginalAuxiliaryTables[j].originalTableVA)return false;
    return true;
}
inline const Table* Find(std::string_view name) noexcept {
    for(const Table& t:kOriginalAuxiliaryTables)
        if(t.networkTableName==name)return &t;
    return nullptr;
}
inline constexpr bool CanExposeToRetailEngine() noexcept {return false;}
static_assert(kOriginalAuxiliaryTables.size()==20u);
static_assert(kOriginalClassOrdinaryNestedLinks+
              kOriginalAuxOrdinaryNestedLinks==
              kOriginalTotalOrdinaryNestedLinks);
static_assert(CountOriginalAuxiliarySlots()==171u);
static_assert(RealSourceTableNamesUnique());
static_assert(kOriginalClassAttachedPropSlots+kOriginalAuxiliaryPropSlots==
              kOriginalStaticPropSlots);
static_assert(kOriginalTypedIntegerAux+kOriginalTypedFloatAux+
              kOriginalTypedVectorAux+kOriginalTypedArrayAux+
              kOriginalTypedNestedAux==kOriginalAuxiliaryPropSlots);
static_assert(kOriginalOrdinaryPropertyConstructors+
              kOriginalAlternativePropertyConstructors==kOriginalAuxiliaryPropSlots);
} // namespace sin_compat::original_client_aux_recvtable_metadata
