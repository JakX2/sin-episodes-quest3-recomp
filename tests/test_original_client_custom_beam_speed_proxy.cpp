// Phase1715: original 2006 PE32 Beam.m_fSpeed native x87 callback.
// 52 checked reference vectors were produced with x86-64 80-bit
// extended multiplication of the retail binary64 factor, FSTP float32.
// The production method itself uses only portable C++17 integer bits.
#include "sin_compat/original_client_custom_beam_speed_proxy.hpp"
#include "sin_compat/original_client_standard_recvproxy_router.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace beam=sin_compat::original_client_custom_beam_speed_proxy;
namespace router=sin_compat::original_client_standard_recvproxy_router;
namespace source=sin_compat::original_client_source_proxy_catalog;

static void SetReceived(std::array<std::uint8_t,16>& d,
                        std::uint32_t raw) {
    for(std::size_t k=0u;k<4u;++k)
        d[4u+k]=static_cast<std::uint8_t>(raw>>(k*8u));
}
struct Case final {std::uint32_t incoming;std::uint32_t output;};
static constexpr std::array<Case,52> kX87Golden{{
        {0x00000000u,0x00000000u},{0x80000000u,0x80000000u},{0x00000001u,0x00000000u},{0x80000001u,0x80000000u},
        {0x00000003u,0x00000000u},{0x007fffffu,0x000ccccdu},{0x00800000u,0x000ccccdu},{0x00800001u,0x000ccccdu},
        {0x3f800000u,0x3dcccccdu},{0xbf800000u,0xbdcccccdu},{0x3f000000u,0x3d4ccccdu},{0xbf000000u,0xbd4ccccdu},
        {0x7f7fffffu,0x7dccccccu},{0xff7fffffu,0xfdccccccu},{0x7f800000u,0x7f800000u},{0xff800000u,0xff800000u},
        {0x3eaaaaabu,0x3d088889u},{0x3dcccccdu,0x3c23d70au},{0x40000000u,0x3e4ccccdu},{0xc0000000u,0xbe4ccccdu},
        {0x1a0db987u,0x1862c272u},{0x6e59262au,0x6cadb822u},{0x3272c498u,0x30c236e0u},{0xe8fd8f88u,0xe74ad93au},
        {0x296fe076u,0x27bfe6c5u},{0xbe117366u,0xbc68b8a3u},{0x000ef8e9u,0x00017f4bu},{0xfcaf79a4u,0xfb0c6150u},
        {0x11ff3c27u,0x104c301fu},{0xda5d6edfu,0xd8b1257fu},{0x2d3c89a1u,0x2b96d481u},{0x520c18a2u,0x5060276au},
        {0x10da5b3fu,0x0f2eaf66u},{0xf0bb7615u,0xef15f811u},{0x416b64d9u,0x3fbc50aeu},{0xdf31e531u,0xdd8e50f4u},
        {0x703e8dfau,0x6e987195u},{0x5f68ad2au,0x5dba2422u},{0xee89ee7du,0xecdcb0c8u},{0x63934c68u,0x61ebad73u},
        {0x18872b66u,0x16d84570u},{0x4b4869f1u,0x49a054c1u},{0x93138aa8u,0x916c110du},{0x70fc954fu,0x6f4a110cu},
        {0xc7d78d75u,0xc62c712au},{0xe8cd4bd7u,0xe7243cacu},{0xba33e15au,0xb88fe77bu},{0xd35430bbu,0xd1a9c096u},
        {0x43ef9891u,0x423fad41u},{0x5bcc6b97u,0x5a238946u},{0x42635988u,0x40b5e13au},{0x14c130cfu,0x131a8d72u},
}};

int main() {
    static_assert(beam::kSourceAssignments==1u);
    static_assert(!beam::CanPublishRetailProxyPointers());
    static_assert(!router::CanPublishRetailProxyPointers());
    assert(source::SourceVAForSlot(128u,14u)==beam::kNativeSourceVA);
    std::array<std::uint8_t,16> originalProxyData{};
    router::DecodedOwnedInput input{};
    std::vector<std::uint8_t> obj(6000u,0xA5u);
    std::uint32_t dataTableToken=0x5A4B3C2Du;
    const auto at=beam::kNativeOwnerSpeedOffset;
    const auto applyDirect=[&](std::uint32_t raw) {
        SetReceived(originalProxyData,raw);
        return beam::ApplySourceEffect(
            128u,14u,beam::kNativeSourceVA,originalProxyData,obj);
    };
    const auto applyRoute=[&](std::uint32_t raw) {
        SetReceived(input.originalProxyData,raw);
        return router::ApplySourceStandardProperty(
            128u,14u,input,obj,dataTableToken);
    };
    for(const auto item:kX87Golden) {
        const auto calculated=
            beam::SourceScaleByOriginalBinary64Tenth(item.incoming);
        assert(calculated.supported && calculated.bits==item.output);
        // Pre-store 3.0f, ensuring the original x87 comparison branch
        // selects the FSTP path for every golden vector here.
        beam::WriteDword(obj,at,0x40400000u);
        const auto before=obj;
        assert(applyDirect(item.incoming)==beam::Status::Applied);
        assert(beam::ReadDword(obj.data()+at)==item.output);
        assert(obj[at-1u]==before[at-1u]);
        assert(obj[at+4u]==before[at+4u]);
        assert(dataTableToken==0x5A4B3C2Du);
        beam::WriteDword(obj,at,0x40400000u);
        assert(applyRoute(item.incoming)==router::Status::Applied);
        assert(beam::ReadDword(obj.data()+at)==item.output);
        assert(dataTableToken==0x5A4B3C2Du);
    }
    // x87 compares signed zero EQUALLY and does not store a new sign
    // when incoming +0 or -0 equals the object's previous zero.
    beam::WriteDword(obj,at,0x00000000u);
    assert(applyRoute(0x80000000u)==router::Status::Applied);
    assert(beam::ReadDword(obj.data()+at)==0x00000000u);
    beam::WriteDword(obj,at,0x80000000u);
    assert(applyRoute(0x00000000u)==router::Status::Applied);
    assert(beam::ReadDword(obj.data()+at)==0x80000000u);
    // Input +min subnormal is nonzero in x87 even if the final
    // float32 result underflows to zero. The FSTP DOES overwrite.
    beam::WriteDword(obj,at,0x80000000u);
    assert(applyDirect(0x00000001u)==beam::Status::Applied);
    assert(beam::ReadDword(obj.data()+at)==0x00000000u);

    // Non-default original x87 precision control, NaN exception/
    // signaling and payload quieting are not claimed here: refuse.
    for(const auto nan:std::array<std::uint32_t,4>{{
             0x7fc00001u,0x7fa00001u,0xffc00001u,0x7fffffffu}}) {
        const auto before=obj;
        assert(applyDirect(nan)==beam::Status::RefusedUnverifiedNaN);
        assert(obj==before);
        assert(applyRoute(nan)==router::Status::UnsupportedSourceCallback);
        assert(obj==before && dataTableToken==0x5A4B3C2Du);
    }
    // A valid but too short host-object buffer is never written.
    {
        std::vector<std::uint8_t> shortObject(at+3u,0x73u);
        const auto before=shortObject;
        SetReceived(originalProxyData,0x3f800000u);
        assert(beam::ApplySourceEffect(
            128u,14u,beam::kNativeSourceVA,originalProxyData,
            shortObject)==beam::Status::RefusedBadOwnedMemory);
        assert(shortObject==before);
    }
    // Wrong real source table, slot and callback identities must
    // NEVER borrow this primitive effect by coincidence.
    {
        const auto before=obj;
        SetReceived(originalProxyData,0x3f800000u);
        assert(beam::ApplySourceEffect(
            128u,13u,beam::kNativeSourceVA,originalProxyData,obj)
            ==beam::Status::UnrecognizedSource);
        assert(beam::ApplySourceEffect(
            127u,14u,beam::kNativeSourceVA,originalProxyData,obj)
            ==beam::Status::UnrecognizedSource);
        assert(beam::ApplySourceEffect(
            128u,14u,0x24126C40u,originalProxyData,obj)
            ==beam::Status::UnrecognizedSource);
        assert(obj==before);
    }
    assert(!beam::CanPublishRetailProxyPointers());
    std::puts("PASS Phase1715 52 source-original Beam.m_fSpeed x87 "
              "rounding vectors, NaN/bounds refusal and signed zeros; "
              "VClient011.slot5 still blocked");
    return 0;
}
