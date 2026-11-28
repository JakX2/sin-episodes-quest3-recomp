#include "sin_compat/original_client_standard_recvproxy_router.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace router=sin_compat::original_client_standard_recvproxy_router;
namespace source=sin_compat::original_client_source_proxy_catalog;
namespace strings=sin_compat::original_client_source_string_sizes;
namespace schema=sin_compat::original_client_native32_recvtable_manifest;

int main() {
    static_assert(source::FullyClassifiedCanonicalRoutes());
    static_assert(strings::EverySourceStringCapacityAuthenticated());
    static_assert(!router::CanPublishRetailProxyPointers());
    router::DecodedOwnedInput incoming{};
    for(std::size_t i=0u;i<12u;++i)
        incoming.originalProxyData[4u+i]=
            static_cast<std::uint8_t>(0x10u+i);
    const std::string sourceString("ABC\0",4u);
    incoming.decodedStringBytes=std::string_view(
        sourceString.data(),sourceString.size());
    incoming.dataTableObjectToken=0x9ABCDEF0u;
    std::vector<std::uint8_t> object(6000u,0xA5u);
    std::uint32_t tableToken=0x55667788u;

    // Original PE32 native receiver identity => genuine source +44 offset.
    // The host object is fabricated, owned bytes, never game process state.
    struct Example final {
        std::size_t table;
        std::size_t slot;
        std::size_t offset;
        std::size_t width;
    };
    constexpr std::array<Example,5> examples{{ 
        {31u,1u,2132u,1u}, // byte: DT_PropSinVehicle.m_bHeadlightIsOn
        {86u,3u,1884u,2u}, // word: DT_RopeKeyframe.m_iStartAttachment
        {35u,2u,2180u,4u}, // DWORD: DT_PropAirboat.m_nAmmoCount
        {36u,2u,40u,4u},   // float: DT_TEConcussiveExplosion.m_flScale
        {36u,1u,28u,12u}   // vector: DT_TEConcussiveExplosion.m_vecNormal
    }};
    for(const auto& e:examples) {
        const auto left=object[e.offset-1u];
        const auto right=object[e.offset+e.width];
        assert(router::ApplySourceStandardProperty(
            e.table,e.slot,incoming,object,tableToken)==router::Status::Applied);
        for(std::size_t i=0u;i<e.width;++i)
            assert(object[e.offset+i]==0x10u+i);
        assert(object[e.offset-1u]==left);
        assert(object[e.offset+e.width]==right);
    }
    assert(tableToken==0x55667788u);

    // All 12 original type3 RecvProp native +12 sizes are known from
    // real original constructor arguments, not guessed modern SDK data.
    constexpr auto originalStringOffset=1297u;
    assert(strings::SourceCapacity(40u,2u)==32u);
    assert(strings::SourceCapacity(80u,2u)==256u);
    assert(strings::SourceCapacity(118u,14u)==18u);
    assert(strings::SourceCapacity(0u,0u)==0u);
    assert(router::ApplySourceStandardProperty(
        40u,2u,incoming,object,tableToken)==router::Status::Applied);
    assert(object[originalStringOffset]=='A');
    assert(object[originalStringOffset+1u]=='B');
    assert(object[originalStringOffset+2u]=='C');
    assert(object[originalStringOffset+3u]==0u);
    assert(object[originalStringOffset+4u]==0xA5u);
    assert(object[originalStringOffset+31u]==0u);

    // Short, non-NUL-terminated source must refuse before any bytes move.
    {
        const auto old=object;
        incoming.decodedStringBytes="XY";
        assert(router::ApplySourceStandardProperty(
            40u,2u,incoming,object,tableToken)
            ==router::Status::RefusedBadOwnedMemory);
        assert(object==old);
        incoming.decodedStringBytes=std::string_view(
            sourceString.data(),sourceString.size());
    }
    // Ordinary type5 source datatable proxy moves an OWNED TOKEN only.
    {
        const auto before=object;
        assert(router::ApplySourceStandardProperty(
            0u,0u,incoming,object,tableToken)==router::Status::Applied);
        assert(tableToken==0x9ABCDEF0u);
        assert(object==before);
    }
    // The original 83 custom receiver callbacks and 28 special factory
    // routes MUST NOT silently be replaced with raw memory copying.
    {
        const auto old=object;
        const auto savedToken=tableToken;
        assert(router::ApplySourceStandardProperty(
            32u,1u,incoming,object,tableToken)
            ==router::Status::UnsupportedSourceCallback);
        assert(router::ApplySourceStandardProperty(
            81u,4u,incoming,object,tableToken)
            ==router::Status::UnsupportedSourceCallback);
        assert(router::ApplySourceStandardProperty(
            197u,0u,incoming,object,tableToken)
            ==router::Status::UnsupportedSourceCallback);
        assert(object==old && tableToken==savedToken);
    }
    {
        const auto old=object;
        assert(router::ApplySourceStandardProperty(
            211u,0u,incoming,object,tableToken)
            ==router::Status::RefusedBadOwnedMemory);
        assert(router::ApplySourceStandardProperty(
            0u,999u,incoming,object,tableToken)
            ==router::Status::RefusedBadOwnedMemory);
        assert(object==old);
    }
    // Prove the whole REAL source 211-table/1110-property classification
    // rather than exercising only hand-picked primitive proxy examples.
    std::size_t applied=0u,custom=0u,special=0u;
    for(std::size_t t=0u;t<schema::kStaticTables;++t)
        for(std::size_t p=0u;p<schema::kCounts[t];++p) {
            const auto outcome=router::ApplySourceStandardProperty(
                t,p,incoming,object,tableToken);
            const auto va=source::SourceVAForSlot(t,p);
            if(va==0u) {
                ++special;
                assert(outcome==router::Status::UnsupportedSourceCallback);
            }else if(!source::IsStandardOriginalSourceVA(va)) {
                ++custom;
                assert(outcome==router::Status::UnsupportedSourceCallback);
            }else {
                ++applied;
                assert(outcome==router::Status::Applied);
            }
        }
    assert(applied==999u && custom==83u && special==28u);

    // Small buffers must never allow an otherwise valid source +44
    // member offset to write beyond the owned host object.
    {
        std::vector<std::uint8_t> tiny(10u,0x33u);
        const auto old=tiny;
        const auto token=tableToken;
        assert(router::ApplySourceStandardProperty(
            31u,1u,incoming,tiny,tableToken)
            ==router::Status::RefusedBadOwnedMemory);
        assert(tiny==old && tableToken==token);
    }
    assert(!router::CanPublishRetailProxyPointers());
    std::puts("PASS Phase1708 source-dispatched 999 original standard "
              "callbacks on bounded owned objects; 83 custom and 28 "
              "special deliberately blocked, no retail slot5");
    return 0;
}
