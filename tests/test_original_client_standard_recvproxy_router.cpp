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
namespace handle=sin_compat::original_client_custom_handle_proxy;
namespace colorInt=sin_compat::original_client_custom_color_integer_proxies;
namespace moveFov=sin_compat::original_client_custom_movement_fov_proxies;
namespace scale=sin_compat::original_client_custom_scale_transition_proxies;
namespace team=sin_compat::original_client_custom_team_array_element_proxy;

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
    // Original code at 0x241A8DB0 and 0x241A8DF0 has byte-identical
    // 49-byte bodies. 30 entity handles and 5 moveparent properties
    // now have exact packed handle native source behavior.
    static_assert(handle::kSourceHandleAssignments==30u);
    static_assert(handle::kSourceMoveParentAssignments==5u);
    static_assert(!handle::CanPublishRetailProxyPointers());
    const auto setRawHandle=[&](std::uint32_t raw) {
        for(std::size_t i=0u;i<4u;++i)
            incoming.originalProxyData[4u+i]=
                static_cast<std::uint8_t>(raw>>(i*8u));
    };
    const auto readSourceDword=[&](std::size_t offset) {
        std::uint32_t value=0u;
        for(std::size_t i=0u;i<4u;++i)
            value|=std::uint32_t(object[offset+i])<<(i*8u);
        return value;
    };
    for(const auto raw:std::array<std::uint32_t,7>{{
            0u,0x7ffu,0x800u,0x801u,0x001fffffu,
            0x80000000u,0xffffffffu}}) {
        setRawHandle(raw);
        for(const auto& item:std::array<std::array<std::size_t,3>,2>{{
            {{32u,1u,1956u}},{{86u,15u,352u}}}}) {
            const auto before=object;
            const auto tokenBefore=tableToken;
            assert(router::ApplySourceStandardProperty(
                item[0],item[1],incoming,object,tableToken)
                ==router::Status::Applied);
            assert(readSourceDword(item[2])==
                   handle::DecodeSourceHandle(raw));
            assert(object[item[2]-1u]==before[item[2]-1u]);
            assert(object[item[2]+4u]==before[item[2]+4u]);
            assert(tableToken==tokenBefore);
        }
    }
    // Phase1710 source x86 custom callback 0x241A8BF0 reverses the
    // input DWORD byte order (38-byte method), while 0x241A8C20
    // decrements that raw DWORD (17-byte method), modulo native32.
    static_assert(colorInt::kSourceColorProperties==4u);
    static_assert(colorInt::kSourceDecrementProperties==5u);
    static_assert(!colorInt::CanPublishRetailProxyPointers());
    for(const auto raw:std::array<std::uint32_t,4>{{
             0x11223344u,0x00000000u,0xFFFFFFFFu,0x80000001u}}) {
        setRawHandle(raw);
        {
            constexpr std::size_t kColorOffset=92u;
            const auto before=object;
            const auto beforeToken=tableToken;
            assert(router::ApplySourceStandardProperty(
                83u,0u,incoming,object,tableToken)
                ==router::Status::Applied);
            const auto expected=colorInt::SourceColorBytes(raw);
            for(std::size_t i=0u;i<4u;++i)
                assert(object[kColorOffset+i]==expected[i]);
            assert(object[kColorOffset-1u]==before[kColorOffset-1u]);
            assert(object[kColorOffset+4u]==before[kColorOffset+4u]);
            assert(tableToken==beforeToken);
        }
        {
            constexpr std::size_t kDecOffset=3428u;
            const auto before=object;
            const auto beforeToken=tableToken;
            assert(router::ApplySourceStandardProperty(
                42u,1u,incoming,object,tableToken)
                ==router::Status::Applied);
            assert(readSourceDword(kDecOffset)==
                   colorInt::SourceDecrementDword(raw));
            assert(object[kDecOffset-1u]==before[kDecOffset-1u]);
            assert(object[kDecOffset+4u]==before[kDecOffset+4u]);
            assert(tableToken==beforeToken);
        }
    }

    // Phase1711: source custom movetype and movecollide use ORIGINAL
    // object hardcoded +0x144/+0x148, not their RecvProp +44 (=0).
    // Default-FOV conditionally resets +0x2C only when +0x38 changes.
    static_assert(moveFov::kSourceCustomAssignments==3u);
    static_assert(moveFov::SourceRoutesMatch());
    static_assert(!moveFov::CanPublishRetailProxyPointers());
    setRawHandle(0x10203040u);
    {
        const auto before=object;
        const auto tokenBefore=tableToken;
        assert(router::ApplySourceStandardProperty(
            120u,17u,incoming,object,tableToken)==router::Status::Applied);
        assert(readSourceDword(0x144u)==0x10203040u);
        assert(readSourceDword(0x148u)==0u);
        assert(object[0x143u]==before[0x143u]);
        assert(object[0x14cu]==before[0x14cu]);
        assert(tableToken==tokenBefore);
    }
    {
        const auto before=object;
        const auto tokenBefore=tableToken;
        assert(router::ApplySourceStandardProperty(
            120u,18u,incoming,object,tableToken)==router::Status::Applied);
        assert(readSourceDword(0x148u)==0x10203040u);
        assert(readSourceDword(0x144u)==0x10203040u);
        assert(object[0x14cu]==before[0x14cu]);
        assert(tableToken==tokenBefore);
    }
    {
        // Prepopulate native m_iDefaultFOV at +0x38 and the reset DWORD
        // at +0x2C with nonzero values: equal values must NOT reset.
        for(std::size_t k=0u;k<4u;++k) {
            object[0x38u+k]=static_cast<std::uint8_t>(
                0x10203040u>>(k*8u));
            object[0x2Cu+k]=static_cast<std::uint8_t>(
                0x11223344u>>(k*8u));
        }
        const auto beforeEqual=object;
        assert(router::ApplySourceStandardProperty(
            202u,4u,incoming,object,tableToken)==router::Status::Applied);
        assert(object==beforeEqual);
        setRawHandle(0x55667788u);
        assert(router::ApplySourceStandardProperty(
            202u,4u,incoming,object,tableToken)==router::Status::Applied);
        assert(readSourceDword(0x38u)==0x55667788u);
        assert(readSourceDword(0x2Cu)==0u);
        assert(object[0x2Bu]==beforeEqual[0x2Bu]);
        assert(object[0x3Cu]==beforeEqual[0x3Cu]);
    }

    // A genuinely custom, as-yet-unported FOV callback, native type4
    // special array, and animation overlay must all remain unsupported.
    {
        const auto old=object;
        const auto savedToken=tableToken;
        assert(router::ApplySourceStandardProperty(
            37u,10u,incoming,object,tableToken)
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
    static_assert(scale::SourceRoutesMatch());
    static_assert(scale::kSourceCustomAssignments==2u);
    static_assert(!scale::CanPublishRetailProxyPointers());
    // Use a clean, entirely zero-initialized owned object to ensure
    // original x87 comparisons do not receive unverified NaN payloads
    // from unrelated synthetic property writes in earlier examples.
    std::vector<std::uint8_t> censusObject(6000u,0u);
    static_assert(team::SourceRoutesMatch());
    static_assert(team::kSourceCustomAssignments==1u);
    static_assert(!team::CanPublishRetailProxyPointers());
    std::size_t applied=0u,custom=0u,special=0u,
                standard=0u,handleAssignments=0u,moveparentAssignments=0u,
                colorAssignments=0u,decrementAssignments=0u,
                movementFovAssignments=0u,scaleAssignments=0u,
                teamArrayAssignments=0u;
    for(std::size_t t=0u;t<schema::kStaticTables;++t)
        for(std::size_t p=0u;p<schema::kCounts[t];++p) {
            if(t==team::kSourceTableOrdinal && p==team::kSourceSlot) {
                // Source Team+0x480 must hold a bounded synthetic
                // array token, not another original PE32 pointer or
                // an accidental copied integer from an earlier prop.
                team::WriteDword(censusObject,team::kOwnerArrayPointerOffset,
                    team::kSyntheticVirtualArenaBase+
                    team::kMinSyntheticArrayOffset);
                incoming.originalSourceElementIndex=7u;
            }
            const auto outcome=router::ApplySourceStandardProperty(
                t,p,incoming,censusObject,tableToken);
            const auto va=source::SourceVAForSlot(t,p);
            if(va==0u) {
                ++special;
                assert(outcome==router::Status::UnsupportedSourceCallback);
            }else if(handle::IsOriginalHandleProxy(va)) {
                ++applied;
                ++handleAssignments;
                if(va==handle::kNativeMoveParentProxyVA)
                    ++moveparentAssignments;
                assert(outcome==router::Status::Applied);
            }else if(colorInt::IsOriginalCustomColorOrDecrement(va)) {
                ++applied;
                if(va==colorInt::kNativeColorByteReverseVA)
                    ++colorAssignments;
                else ++decrementAssignments;
                assert(outcome==router::Status::Applied);
            }else if(moveFov::IsOriginalMovementOrFovProxy(va)) {
                ++applied;
                ++movementFovAssignments;
                assert(outcome==router::Status::Applied);
            }else if(scale::IsSourceScaleProxy(va)) {
                ++applied;
                ++scaleAssignments;
                assert(outcome==router::Status::Applied);
            }else if(team::IsOriginalTeamElementProxy(va)) {
                ++applied;
                ++teamArrayAssignments;
                assert(outcome==router::Status::Applied);
            }else if(!source::IsStandardOriginalSourceVA(va)) {
                ++custom;
                assert(outcome==router::Status::UnsupportedSourceCallback);
            }else {
                ++applied;
                ++standard;
                assert(outcome==router::Status::Applied);
            }
        }
    assert(standard==999u && handleAssignments==35u &&
           moveparentAssignments==5u);
    assert(colorAssignments==4u && decrementAssignments==5u);
    assert(movementFovAssignments==3u && scaleAssignments==2u);
    assert(teamArrayAssignments==1u);
    assert(applied==1049u && custom==33u && special==28u);

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
    std::puts("PASS Phase1713 999 standard + 35 handles + 4 color + 5 "
              "decrement + 3 movement/FOV + 2 x87 scale + 1 Team element "
              "receiver routes; 33 custom / 28 special blocked, NO slot5");
    return 0;
}
