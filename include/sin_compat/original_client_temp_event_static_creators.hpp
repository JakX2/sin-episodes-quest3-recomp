#pragma once
// Phase1717: 42 real original 2006 PE32 temporary-event creator
// functions are exactly 6 executable bytes each:
//   MOV EAX, <constant address of static 2006 client object>; RET
// Each original method is followed by 10 INT3 padding bytes.
// SHA256 of 42 ORIGINAL 6-byte method bodies concatenated in genuine
// native ClientClass slot5 head order:
// 266efe94cd2323cd097a9ba2af229d5f57ca547889e4dc0fe244a04374a4db30
// Owner-original whole client.dll SHA256:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
// Source-only CSV LF SHA256:
// b40182b4b5257a02bcce50b85da6123665b551b2f6615b3f759e0dccc23f4b50
//
// These original Win32 source labels / static data addresses are NOT
// callable or dereferenceable in a replacement DLL, Linux or Android.
// Actual original static-object construction/VTABLE/lifetime is unknown.
// This code models only a validated host-OWNED opaque token return.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "sin_compat/original_client_creator_source_map.hpp"

namespace sin_compat::original_client_temp_event_static_creators {
namespace creators=sin_compat::original_client_creator_source_map;
inline constexpr std::size_t kOriginalTempEventGetters=42u;
inline constexpr std::uint32_t kNativeStaticDataMin=0x242F3000u;
inline constexpr std::uint32_t kNativeStaticDataEnd=0x2438F320u;
inline constexpr std::uint32_t kSyntheticHostBase=0x10000000u;
inline constexpr std::uint32_t kSyntheticHostMinOffset=0x1400u;
struct OriginalTempGetter final {
    std::uint16_t classOrdinal;
    std::uint32_t originalCreatorVA;
    std::uint32_t originalStaticObjectVA;
};
inline constexpr std::array<OriginalTempGetter,kOriginalTempEventGetters>
kSourceStaticGetters{{
    {36u,0x24222450u,0x2437d160u},
    {63u,0x24213ab0u,0x2437a4e0u},
    {82u,0x240f6e40u,0x2433e9bcu},
    {105u,0x240e0f80u,0x2433bd34u},
    {134u,0x2407aec0u,0x2432af38u},
    {135u,0x2407acb0u,0x2432add4u},
    {136u,0x2407aa10u,0x2432ab58u},
    {137u,0x2407a820u,0x2432a98cu},
    {138u,0x2407a630u,0x2432a800u},
    {139u,0x2407a320u,0x2432a63cu},
    {140u,0x2407a020u,0x2432a520u},
    {141u,0x24079bd0u,0x2432a35cu},
    {142u,0x240799a0u,0x2432a1bcu},
    {144u,0x24078da0u,0x24329df0u},
    {145u,0x24072660u,0x24329b98u},
    {146u,0x24072550u,0x24329a5cu},
    {147u,0x24072350u,0x24329940u},
    {148u,0x24072160u,0x24329774u},
    {149u,0x240715f0u,0x2432953cu},
    {150u,0x24071300u,0x243290fcu},
    {151u,0x24071180u,0x24328f00u},
    {152u,0x24070520u,0x24328d14u},
    {153u,0x240703a0u,0x24328a50u},
    {154u,0x24070040u,0x243287fcu},
    {155u,0x2406fcd0u,0x24328788u},
    {156u,0x2406fa00u,0x243284c4u},
    {157u,0x2406f800u,0x243282b0u},
    {158u,0x2406f5c0u,0x24328060u},
    {159u,0x2406f2b0u,0x24327e24u},
    {160u,0x2406f020u,0x24327c6cu},
    {161u,0x2406e7d0u,0x243278f0u},
    {162u,0x2406e3b0u,0x24327694u},
    {163u,0x2406e240u,0x2432732cu},
    {164u,0x2406e080u,0x243271a4u},
    {165u,0x2406de30u,0x24326fecu},
    {166u,0x2406db90u,0x24326e5cu},
    {167u,0x2406d950u,0x24326cdcu},
    {168u,0x2406d6e0u,0x24326b5cu},
    {169u,0x2406d410u,0x243269c4u},
    {171u,0x2406ce70u,0x24326414u},
    {172u,0x2406cd90u,0x243263b8u},
    {185u,0x240587c0u,0x24323f94u}
}};
constexpr const OriginalTempGetter* SourceGetterForClass(
    std::size_t ordinal,std::uint32_t originalVA) noexcept {
    for(const auto& item:kSourceStaticGetters)
        if(item.classOrdinal==ordinal && item.originalCreatorVA==originalVA)
            return &item;
    return nullptr;
}
constexpr bool SourceGettersConsistent() noexcept {
    for(std::size_t i=0u;i<kSourceStaticGetters.size();++i) {
        const auto& g=kSourceStaticGetters[i];
        if(g.classOrdinal>=creators::kOriginalClasses ||
           !creators::SourceCreatorIsTemp(g.classOrdinal) ||
           creators::SourceCreatorVAForOrdinal(g.classOrdinal)!=g.originalCreatorVA ||
           g.originalStaticObjectVA<kNativeStaticDataMin ||
           g.originalStaticObjectVA>=kNativeStaticDataEnd)
            return false;
        for(std::size_t j=0u;j<i;++j)
            if(kSourceStaticGetters[j].classOrdinal==g.classOrdinal ||
               kSourceStaticGetters[j].originalCreatorVA==g.originalCreatorVA ||
               kSourceStaticGetters[j].originalStaticObjectVA==g.originalStaticObjectVA)
                return false;
    }
    for(std::size_t i=0u;i<creators::kOriginalClasses;++i) {
        if(creators::SourceCreatorIsTemp(i)) {
            unsigned matches=0u;
            for(const auto& g:kSourceStaticGetters)
                if(g.classOrdinal==i)++matches;
            if(matches!=1u)return false;
        }
    }
    return true;
}
enum class Status : std::uint8_t {
    Applied, UnrecognizedSource, RefusedUnboundHostObject,
    RefusedBadOwnedMemory
};
// Return only a supplied, explicit synthetic host reference for a source
// exact static temp event getter. Verify the token is non-null, aligned,
// inside the owned arena, and not an original PE32 address. Do not copy
// the original 2006 address into an actual ClientClass creator slot.
inline Status ApplySourceOriginalGetter(
    std::size_t classOrdinal,std::uint32_t originalCreatorVA,
    bool hasAuthenticatedOwnedObject,std::uint32_t ownedObjectToken,
    const std::vector<std::uint8_t>& ownedObjectArena,
    std::uint32_t& resultToken) noexcept {
    if(SourceGetterForClass(classOrdinal,originalCreatorVA)==nullptr)
        return Status::UnrecognizedSource;
    if(!hasAuthenticatedOwnedObject)
        return Status::RefusedUnboundHostObject;
    if(ownedObjectToken<kSyntheticHostBase+kSyntheticHostMinOffset ||
       (ownedObjectToken&3u)!=0u)
        return Status::RefusedBadOwnedMemory;
    const auto offset=static_cast<std::size_t>(
        ownedObjectToken-kSyntheticHostBase);
    if(offset>=ownedObjectArena.size())
        return Status::RefusedBadOwnedMemory;
    resultToken=ownedObjectToken;
    return Status::Applied;
}
inline constexpr bool CanPublishRetailCreatorPointers() noexcept {return false;}
static_assert(kSourceStaticGetters.size()==42u);
static_assert(SourceGettersConsistent());
static_assert(!CanPublishRetailCreatorPointers());
} // namespace sin_compat::original_client_temp_event_static_creators
