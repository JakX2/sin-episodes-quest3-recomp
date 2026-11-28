#pragma once
// Phase1716: SOURCE-ONLY original 2006 PE32 191 ClientClass creator
// function virtual addresses, in the authenticated VClient011.slot5
// head-prepend linked-list order. Owner original SHA256:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
// Full original 191 native constructor canonical SHA256:
// ded698d9f9b6c671e17a03a23a0e909b4acb5d8c6030f905af2899471d616c24
// Immutable CSV original-free LF SHA256:
// c1083d4cbb610c81e66dbea8d80c96085ac15c942ccdf4e022d35d167bc62f29
//
// THESE ORIGINAL WIN32 ADDRESSES ARE NOT POINTERS IN THE REPLACEMENT
// CLIENT.DLL OR ANDROID. Never reinterpret_cast or execute them.
// There is NO known portable creator ABI, entity allocation or full
// object lifetime here. The native32 owned image leaves +0/+4 NULL.
// Retail engine VClient011.slot5 remains explicitly guarded.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_client_creator_source_map {
inline constexpr std::size_t kOriginalClasses=191u;
inline constexpr std::size_t kOriginalEntityCreators=149u;
inline constexpr std::size_t kOriginalTempEventCreators=42u;
inline constexpr std::uint32_t kOriginalPECodeStart=0x24001000u;
inline constexpr std::uint32_t kOriginalPECodeEndExclusive=0x24275FBAu;
inline constexpr std::array<std::uint32_t,kOriginalClasses>
kOriginalSourceCreatorVAs{{
    0x2422b500u,0x2422afe0u,0x2422b3c0u,0x2422ad40u,0x2422ab70u,0x2422a9a0u,
    0x2422a7d0u,0x2422b620u,0x2422b240u,0x2422a4f0u,0x24229d30u,0x24229690u,
    0x242294c0u,0x242292f0u,0x24229bb0u,0x24229050u,0x24228e80u,0x24228cb0u,
    0x24228ae0u,0x24228910u,0x24228740u,0x24228570u,0x24229e50u,0x242282c0u,
    0x242280f0u,0x24227f20u,0x24227d50u,0x24227b80u,0x24229920u,0x242278e0u,
    0x242276e0u,0x24227060u,0x24226850u,0x242262e0u,0x24225c90u,0x24225800u,
    0x24222450u,0x24222100u,0x242213c0u,0x24220890u,0x242204c0u,0x242202a0u,
    0x2421f440u,0x2421ece0u,0x2421db90u,0x2421cdd0u,0x2421cb20u,0x2421bce0u,
    0x2421ba80u,0x2421aaa0u,0x242190e0u,0x24218d60u,0x242184f0u,0x24218250u,
    0x24217180u,0x242168a0u,0x24216540u,0x24216170u,0x24216250u,0x242161f0u,
    0x24215df0u,0x24215ba0u,0x24215600u,0x24213ab0u,0x24212920u,0x2420d5a0u,
    0x241cb710u,0x241c5d60u,0x241c2e80u,0x241a3b20u,0x241a49e0u,0x2413cfd0u,
    0x2412b030u,0x2412bf40u,0x241285e0u,0x240fc920u,0x240fc880u,0x240fa880u,
    0x240f9440u,0x240f8450u,0x240f8120u,0x240f78a0u,0x240f6e40u,0x240f6d30u,
    0x240f6050u,0x240f2260u,0x240f2030u,0x240ecdc0u,0x240ecb20u,0x240ecc70u,
    0x240ecc10u,0x240ecbb0u,0x240ec360u,0x240ebf80u,0x240eb0c0u,0x240e7140u,
    0x240e6c80u,0x240e6a60u,0x240e6790u,0x240e6540u,0x240e40b0u,0x240e3f40u,
    0x240e2770u,0x240e25a0u,0x240e21f0u,0x240e0f80u,0x240e1770u,0x240e0a50u,
    0x240dfcd0u,0x240dcc70u,0x240db940u,0x240db8e0u,0x240da0a0u,0x240d9d10u,
    0x240d9730u,0x240d82e0u,0x240d6d90u,0x240d6b50u,0x240d6160u,0x240d1570u,
    0x240cd470u,0x240c41b0u,0x240c39a0u,0x240c0c50u,0x240b3b50u,0x240bdbf0u,
    0x240b6b30u,0x240b1fc0u,0x2409edf0u,0x2409d530u,0x240993e0u,0x24099150u,
    0x24094110u,0x2407b1e0u,0x2407aec0u,0x2407acb0u,0x2407aa10u,0x2407a820u,
    0x2407a630u,0x2407a320u,0x2407a020u,0x24079bd0u,0x240799a0u,0x24079690u,
    0x24078da0u,0x24072660u,0x24072550u,0x24072350u,0x24072160u,0x240715f0u,
    0x24071300u,0x24071180u,0x24070520u,0x240703a0u,0x24070040u,0x2406fcd0u,
    0x2406fa00u,0x2406f800u,0x2406f5c0u,0x2406f2b0u,0x2406f020u,0x2406e7d0u,
    0x2406e3b0u,0x2406e240u,0x2406e080u,0x2406de30u,0x2406db90u,0x2406d950u,
    0x2406d6e0u,0x2406d410u,0x2406d070u,0x2406ce70u,0x2406cd90u,0x2406b6e0u,
    0x24069f40u,0x240687c0u,0x240685b0u,0x24067980u,0x24067870u,0x24068550u,
    0x240684f0u,0x24062930u,0x2405fc10u,0x2405dad0u,0x2405ce30u,0x240587c0u,
    0x240560d0u,0x24057880u,0x24055b70u,0x24057e20u,0x24053dc0u
}};
inline constexpr std::array<std::uint8_t,kOriginalClasses>
kSourceCreatorIsTempEvent{{
    0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,
    0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,1u,0u,0u,0u,
    0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,
    0u,0u,0u,1u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,
    0u,0u,1u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,
    0u,0u,0u,0u,0u,1u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,
    0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,1u,1u,1u,1u,1u,1u,
    1u,1u,1u,0u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,
    1u,1u,1u,1u,1u,1u,1u,1u,1u,1u,0u,1u,1u,0u,0u,0u,0u,0u,0u,0u,
    0u,0u,0u,0u,0u,1u,0u,0u,0u,0u,0u
}};
constexpr std::uint32_t SourceCreatorVAForOrdinal(std::size_t ordinal) noexcept {
    return ordinal<kOriginalClasses?kOriginalSourceCreatorVAs[ordinal]:0u;
}
constexpr bool SourceCreatorIsTemp(std::size_t ordinal) noexcept {
    return ordinal<kOriginalClasses && kSourceCreatorIsTempEvent[ordinal]!=0u;
}
constexpr bool SourceCreatorInventoryConsistent() noexcept {
    std::size_t temp=0u;
    for(std::size_t i=0u;i<kOriginalClasses;++i) {
        const auto va=kOriginalSourceCreatorVAs[i];
        if(va<kOriginalPECodeStart || va>=kOriginalPECodeEndExclusive ||
           kSourceCreatorIsTempEvent[i]>1u)return false;
        temp+=kSourceCreatorIsTempEvent[i];
        for(std::size_t j=0u;j<i;++j)
            if(kOriginalSourceCreatorVAs[j]==va)return false;
    }
    return temp==kOriginalTempEventCreators &&
           kOriginalClasses-temp==kOriginalEntityCreators;
}
inline constexpr bool CanPublishRetailCreatorPointers() noexcept {return false;}
static_assert(kOriginalSourceCreatorVAs.size()==191u);
static_assert(SourceCreatorInventoryConsistent());
static_assert(!CanPublishRetailCreatorPointers());
} // namespace sin_compat::original_client_creator_source_map
