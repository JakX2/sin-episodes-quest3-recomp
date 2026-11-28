// Phase1713: original 2006 PE32 Team array-index receiver, owned tokens.
#include "sin_compat/original_client_custom_team_array_element_proxy.hpp"
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

namespace team=sin_compat::original_client_custom_team_array_element_proxy;
namespace router=sin_compat::original_client_standard_recvproxy_router;
namespace source=sin_compat::original_client_source_proxy_catalog;

static void SetValue(std::array<std::uint8_t,16>& bytes,
                     std::uint32_t raw) {
    for(std::size_t k=0u;k<4u;++k)
        bytes[4u+k]=static_cast<std::uint8_t>(raw>>(k*8u));
}
static std::uint32_t Read(const std::vector<std::uint8_t>& bytes,
                          std::size_t offset) {
    assert(offset<=bytes.size() && bytes.size()-offset>=4u);
    return team::ReadDword(bytes,offset);
}

int main() {
    static_assert(team::SourceRoutesMatch());
    static_assert(team::kSourceCustomAssignments==1u);
    static_assert(!team::CanPublishRetailProxyPointers());
    static_assert(!router::CanPublishRetailProxyPointers());
    assert(source::SourceVAForSlot(81u,3u)==team::kNativeTeamArrayElementVA);
    assert(source::SourceVAForSlot(81u,4u)==0u); // type4 still unresolved

    constexpr auto token=team::kSyntheticVirtualArenaBase+
                         team::kMinSyntheticArrayOffset;
    constexpr auto first=std::size_t(team::kMinSyntheticArrayOffset);
    std::vector<std::uint8_t> owner(6000u,0xA5u);
    team::WriteDword(owner,team::kOwnerArrayPointerOffset,token);
    std::array<std::uint8_t,16> originalProxyData{};
    router::DecodedOwnedInput incoming{};
    std::uint32_t datatableToken=0x12345678u;
    const auto applyDirect=[&](std::uint32_t index,std::uint32_t raw) {
        SetValue(originalProxyData,raw);
        return team::ApplySourceOriginalEffect(
            81u,3u,team::kNativeTeamArrayElementVA,
            originalProxyData,index,owner);
    };
    const auto applyRouter=[&](std::uint32_t index,std::uint32_t raw) {
        SetValue(incoming.originalProxyData,raw);
        incoming.originalSourceElementIndex=index;
        return router::ApplySourceStandardProperty(
            81u,3u,incoming,owner,datatableToken);
    };

    // The original x86 CRecvProxyData +0x14 array index is decoded
    // separately; its source value remains a DWORD at original +4.
    auto before=owner;
    assert(applyDirect(0u,0xFEEDFACEu)==team::Status::Applied);
    assert(Read(owner,first)==0xFEEDFACEu);
    assert(owner[first+4u]==before[first+4u]);
    assert(owner[first-1u]==before[first-1u]);
    assert(owner[first+128u]==before[first+128u]);

    // Last legal source array element 31, then intermediate index17.
    before=owner;
    assert(applyRouter(31u,0xDEADBEEFu)==router::Status::Applied);
    assert(Read(owner,first+31u*4u)==0xDEADBEEFu);
    assert(owner[first+31u*4u-1u]==before[first+31u*4u-1u]);
    assert(owner[first+128u]==before[first+128u]);
    assert(Read(owner,team::kOwnerArrayPointerOffset)==token);
    assert(datatableToken==0x12345678u);
    before=owner;
    assert(applyRouter(17u,0x80000001u)==router::Status::Applied);
    assert(Read(owner,first+17u*4u)==0x80000001u);
    assert(owner[first+17u*4u-1u]==before[first+17u*4u-1u]);
    assert(owner[first+17u*4u+4u]==before[first+17u*4u+4u]);

    // Bad original array indexes and wrong original source identity
    // are both refused before any write, even if addresses are owned.
    for(const auto idx:std::array<std::uint32_t,3>{{
            32u,33u,0xFFFFFFFFu}}) {
        before=owner;
        assert(applyDirect(idx,0x9ABCDEF0u)==
               team::Status::RefusedBadOwnedMemory);
        assert(owner==before);
        assert(applyRouter(idx,0x9ABCDEF0u)==
               router::Status::RefusedBadOwnedMemory);
        assert(owner==before);
    }
    {
        SetValue(originalProxyData,0x1234ABCDu);
        before=owner;
        assert(team::ApplySourceOriginalEffect(
            81u,4u,team::kNativeTeamArrayElementVA,
            originalProxyData,0u,owner)==team::Status::UnrecognizedSource);
        assert(team::ApplySourceOriginalEffect(
            81u,3u,0x240F76B0u,
            originalProxyData,0u,owner)==team::Status::UnrecognizedSource);
        assert(owner==before);
    }

    // Reject stale original PE32 pointers, object-address aliasing,
    // short team arrays, invalid alignment, and pointer overflow.
    for(const auto bad:std::array<std::uint32_t,6>{{
            0u,0x240F7240u,
            team::kSyntheticVirtualArenaBase+0x480u,
            team::kSyntheticVirtualArenaBase+0x13FCu,
            team::kSyntheticVirtualArenaBase+0x1401u,
            team::kSyntheticVirtualArenaBase+0x2000u}}) {
        team::WriteDword(owner,team::kOwnerArrayPointerOffset,bad);
        before=owner;
        assert(applyDirect(17u,0x87654321u)==
               team::Status::RefusedBadOwnedMemory);
        assert(owner==before);
    }
    team::WriteDword(owner,team::kOwnerArrayPointerOffset,token);
    {
        std::vector<std::uint8_t> tooShort(
            first+team::kSyntheticArrayBytes-1u,0xA5u);
        team::WriteDword(tooShort,team::kOwnerArrayPointerOffset,token);
        const auto saved=tooShort;
        SetValue(originalProxyData,0x01020304u);
        assert(team::ApplySourceOriginalEffect(
            81u,3u,team::kNativeTeamArrayElementVA,
            originalProxyData,31u,tooShort)==
            team::Status::RefusedBadOwnedMemory);
        assert(tooShort==saved);
    }
    {
        std::vector<std::uint8_t> tiny(10u,0x11u);
        const auto saved=tiny;
        assert(team::ApplySourceOriginalEffect(
            81u,3u,team::kNativeTeamArrayElementVA,
            originalProxyData,0u,tiny)==
            team::Status::RefusedBadOwnedMemory);
        assert(tiny==saved);
    }

    // The original type4 outer array remains unimplemented: its
    // callback is NOT equated with the independently reconstructed
    // type0 element callback.
    before=owner;
    assert(router::ApplySourceStandardProperty(
        81u,4u,incoming,owner,datatableToken)==
        router::Status::UnsupportedSourceCallback);
    assert(owner==before && datatableToken==0x12345678u);
    assert(!team::CanPublishRetailProxyPointers());
    std::puts("PASS Phase1713 original Team player_array_element: "
              "32 indexed source elements via bounded synthetic token; "
              "native type4 outer callback and retail slot5 blocked");
    return 0;
}
