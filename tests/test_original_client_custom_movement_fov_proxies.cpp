// Phase1711: source-exact 2006 PE32 3 custom callback test; host only.
#include "sin_compat/original_client_custom_movement_fov_proxies.hpp"
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

namespace original=sin_compat::original_client_custom_movement_fov_proxies;
namespace router=sin_compat::original_client_standard_recvproxy_router;
namespace source=sin_compat::original_client_source_proxy_catalog;

static void WriteInput(std::array<std::uint8_t,16>& input,
                       std::uint32_t raw) {
    for(std::size_t k=0u;k<4u;++k)
        input[4u+k]=static_cast<std::uint8_t>(raw>>(k*8u));
}
static std::uint32_t At(const std::vector<std::uint8_t>& bytes,
                        std::size_t offset) {
    assert(offset<=bytes.size() && bytes.size()-offset>=4u);
    return original::ReadOwnedDword(bytes,offset);
}

int main() {
    static_assert(original::SourceRoutesMatch());
    static_assert(original::kSourceCustomAssignments==3u);
    static_assert(!original::CanPublishRetailProxyPointers());
    static_assert(!router::CanPublishRetailProxyPointers());
    assert(source::SourceVAForSlot(120u,17u)==original::kNativeMoveTypeVA);
    assert(source::SourceVAForSlot(120u,18u)==original::kNativeMoveCollideVA);
    assert(source::SourceVAForSlot(202u,4u)==original::kNativeDefaultFovVA);
    assert(!original::IsOriginalMovementOrFovProxy(0x241a8cf0u));

    std::array<std::uint8_t,16> originalProxyData{};
    router::DecodedOwnedInput incoming{};
    std::vector<std::uint8_t> object(6000u,0xa5u);
    std::uint32_t datatableToken=0x8394A5B6u;
    const auto apply=[&](std::size_t table,std::size_t slot,
                         std::uint32_t raw) {
        WriteInput(incoming.originalProxyData,raw);
        return router::ApplySourceStandardProperty(
            table,slot,incoming,object,datatableToken);
    };

    // The original movetype writes +0x144 and explicitly clears +0x148.
    // It does NOT write its RecvProp +44=0 field.
    original::WriteOwnedDword(object,0x148u,0xAABBCCDDu);
    const auto beforeType=object;
    assert(apply(120u,17u,0x12345678u)==router::Status::Applied);
    assert(At(object,0x144u)==0x12345678u);
    assert(At(object,0x148u)==0u);
    assert(At(object,0u)==At(beforeType,0u));
    assert(object[0x143u]==beforeType[0x143u]);
    assert(object[0x14cu]==beforeType[0x14cu]);
    assert(datatableToken==0x8394A5B6u);

    // Independent movecollide changes only +0x148.
    const auto beforeCollide=object;
    assert(apply(120u,18u,0xF1234567u)==router::Status::Applied);
    assert(At(object,0x148u)==0xF1234567u);
    assert(At(object,0x144u)==At(beforeCollide,0x144u));
    assert(object[0x14cu]==beforeCollide[0x14cu]);
    assert(object[0x147u]==beforeCollide[0x147u]);

    // Default-FOV change invalidates the transition cache at +0x2C
    // and writes +0x38. Equal FOV leaves +0x2C UNCHANGED.
    original::WriteOwnedDword(object,0x2cu,0xCAFEBABEu);
    original::WriteOwnedDword(object,0x38u,72u);
    const auto priorEqual=object;
    assert(apply(202u,4u,72u)==router::Status::Applied);
    assert(object==priorEqual);
    assert(At(object,0x2cu)==0xCAFEBABEu);
    assert(apply(202u,4u,90u)==router::Status::Applied);
    assert(At(object,0x2cu)==0u);
    assert(At(object,0x38u)==90u);
    assert(object[0x2bu]==priorEqual[0x2bu]);
    assert(object[0x3cu]==priorEqual[0x3cu]);
    assert(datatableToken==0x8394A5B6u);

    // Strict all-or-nothing refusal at the original hardcoded endpoints.
    for(const std::size_t shortSize:
            std::array<std::size_t,5>{{0u,0x2cu,0x3bu,0x148u,0x14bu}}) {
        std::vector<std::uint8_t> shortObject(shortSize,0x73u);
        const auto saved=shortObject;
        WriteInput(originalProxyData,0xffffffffu);
        if(shortSize<0x14cu) {
            assert(original::ApplySourceOriginalEffect(
                120u,17u,original::kNativeMoveTypeVA,
                originalProxyData,shortObject)==
                original::Status::RefusedBadOwnedMemory);
            assert(original::ApplySourceOriginalEffect(
                120u,18u,original::kNativeMoveCollideVA,
                originalProxyData,shortObject)==
                original::Status::RefusedBadOwnedMemory);
        }
        if(shortSize<0x3cu)
            assert(original::ApplySourceOriginalEffect(
                202u,4u,original::kNativeDefaultFovVA,
                originalProxyData,shortObject)==
                original::Status::RefusedBadOwnedMemory);
        assert(shortObject==saved);
    }
    {
        const auto saved=object;
        WriteInput(originalProxyData,0xBADC0FFEu);
        assert(original::ApplySourceOriginalEffect(
            120u,17u,original::kNativeMoveCollideVA,
            originalProxyData,object)==original::Status::UnrecognizedSource);
        assert(original::ApplySourceOriginalEffect(
            202u,99u,original::kNativeDefaultFovVA,
            originalProxyData,object)==original::Status::UnrecognizedSource);
        assert(original::ApplySourceOriginalEffect(
            120u,17u,0x241a8cf0u,
            originalProxyData,object)==original::Status::UnrecognizedSource);
        assert(object==saved);
    }

    // Exhaustive checks across representative raw DWORD boundaries,
    // including signed bit 31, serial-like masks and exact equality.
    const std::array<std::uint32_t,12> values{{
        0u,1u,72u,90u,0x7ffu,0x800u,0xffffu,0x10000u,
        0x001fffffu,0x7fffffffu,0x80000000u,0xffffffffu}};
    for(const auto raw:values) {
        assert(apply(120u,17u,raw)==router::Status::Applied);
        assert(At(object,0x144u)==raw && At(object,0x148u)==0u);
        assert(apply(120u,18u,raw)==router::Status::Applied);
        assert(At(object,0x148u)==raw);
        original::WriteOwnedDword(object,0x2cu,0xFEEDFACEu);
        const auto existing=At(object,0x38u);
        assert(apply(202u,4u,raw)==router::Status::Applied);
        assert(At(object,0x38u)==raw);
        assert(At(object,0x2cu)==(existing==raw?0xFEEDFACEu:0u));
    }
    assert(datatableToken==0x8394A5B6u);
    std::puts("PASS Phase1711 three exact original movetype/movecollide/"
              "default-FOV callbacks in bounded owned host memory; "
              "original engine slot5 remains blocked");
    return 0;
}
