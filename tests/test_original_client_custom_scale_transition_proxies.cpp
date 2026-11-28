// Phase1712: original 2006 PE32 scale proxies, x87 ordered comparisons.
#include "sin_compat/original_client_custom_scale_transition_proxies.hpp"
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

namespace scale=sin_compat::original_client_custom_scale_transition_proxies;
namespace router=sin_compat::original_client_standard_recvproxy_router;
namespace source=sin_compat::original_client_source_proxy_catalog;

static void SetRaw(std::array<std::uint8_t,16>& data,std::uint32_t raw) {
    for(std::size_t k=0u;k<4u;++k)
        data[4u+k]=static_cast<std::uint8_t>(raw>>(k*8u));
}
static std::uint32_t Get(const std::vector<std::uint8_t>& bytes,
                         std::size_t offset) {
    assert(offset<=bytes.size() && bytes.size()-offset>=4u);
    return scale::ReadOwnedDword(bytes,offset);
}
static void Put(std::vector<std::uint8_t>& bytes,std::size_t off,
                std::uint32_t value) {
    scale::WriteOwnedDword(bytes,off,value);
}

int main() {
    static_assert(scale::SourceRoutesMatch());
    static_assert(scale::kSourceCustomAssignments==2u);
    static_assert(!router::CanPublishRetailProxyPointers());
    static_assert(!scale::CanPublishRetailProxyPointers());
    assert(source::SourceVAForSlot(94u,2u)==scale::kNativePlasmaScaleVA);
    assert(source::SourceVAForSlot(111u,2u)==scale::kNativeFireSmokeScaleVA);

    std::array<std::uint8_t,16> originalProxyData{};
    router::DecodedOwnedInput incoming{};
    std::vector<std::uint8_t> object(6000u,0xa5u);
    std::uint32_t token=0x7A9BCAEEu;
    const auto applyDirect=[&](std::size_t table,std::size_t slot,
                                std::uint32_t va,std::uint32_t raw) {
        SetRaw(originalProxyData,raw);
        return scale::ApplySourceOriginalEffect(
            table,slot,va,originalProxyData,object);
    };
    const auto applyRouter=[&](std::size_t table,std::size_t slot,
                                std::uint32_t raw) {
        SetRaw(incoming.originalProxyData,raw);
        return router::ApplySourceStandardProperty(
            table,slot,incoming,object,token);
    };

    // Plasma equal: no change, including the +0/-0 equivalence,
    // target-copy side effect must not occur without a value change.
    Put(object,0x484u,0x3f800000u); // 1.0
    Put(object,0x49cu,0x40400000u); // 3.0 (target)
    Put(object,0x4a0u,0xDEADBEEFu);
    Put(object,0x4a4u,0xAABBCCDDu);
    auto before=object;
    assert(applyDirect(94u,2u,scale::kNativePlasmaScaleVA,0x3f800000u)
           ==scale::Status::Applied);
    assert(object==before);
    assert(applyRouter(94u,2u,0x40000000u)==router::Status::Applied);
    assert(Get(object,0x484u)==0x40000000u);
    assert(Get(object,0x4a4u)==0x40000000u);
    assert(Get(object,0x4a0u)==0x40400000u);
    assert(Get(object,0x49cu)==0x40400000u);
    assert(object[0x483u]==before[0x483u]);
    assert(object[0x4a8u]==before[0x4a8u]);
    assert(token==0x7A9BCAEEu);

    // Ordered signed zero comparison must skip Plasma's transition and
    // preserve the object's original sign bit, not bitwise-compare.
    Put(object,0x484u,0x80000000u); // negative zero
    before=object;
    assert(applyDirect(94u,2u,scale::kNativePlasmaScaleVA,0x00000000u)
           ==scale::Status::Applied);
    assert(object==before);

    // FireSmoke changes both current and target if source value differs
    // from both, remembering original +0x494 in the +0x498 field.
    Put(object,0x484u,0x3f800000u);
    Put(object,0x49cu,0x40400000u);
    Put(object,0x494u,0x40900000u);
    Put(object,0x498u,0xBEEFBEAFu);
    before=object;
    assert(applyRouter(111u,2u,0x40000000u)==router::Status::Applied);
    assert(Get(object,0x484u)==0x40000000u);
    assert(Get(object,0x49cu)==0x40000000u);
    assert(Get(object,0x498u)==0x40900000u);
    assert(Get(object,0x494u)==0x40900000u);
    assert(object[0x483u]==before[0x483u]);
    assert(object[0x4a0u]==before[0x4a0u]);

    // FireSmoke new != current, but new == target: no target-history
    // movement; final current store still occurs.
    Put(object,0x484u,0x3f800000u);
    Put(object,0x49cu,0x40000000u);
    Put(object,0x498u,0xA1B2C3D4u);
    before=object;
    assert(applyDirect(111u,2u,scale::kNativeFireSmokeScaleVA,0x40000000u)
           ==scale::Status::Applied);
    assert(Get(object,0x484u)==0x40000000u);
    assert(Get(object,0x49cu)==0x40000000u);
    assert(Get(object,0x498u)==0xA1B2C3D4u);
    assert(object[0x483u]==before[0x483u]);
    assert(object[0x488u]==before[0x488u]);

    // Equal FireSmoke compares floats, but ALWAYS stores the incoming
    // raw value: changing +0.0 to -0.0 changes the sign bit at +0x484.
    Put(object,0x484u,0x00000000u);
    Put(object,0x49cu,0xA0000000u);
    Put(object,0x498u,0xC0FFEE00u);
    before=object;
    assert(applyDirect(111u,2u,scale::kNativeFireSmokeScaleVA,0x80000000u)
           ==scale::Status::Applied);
    assert(Get(object,0x484u)==0x80000000u);
    assert(Get(object,0x49cu)==Get(before,0x49cu));
    assert(Get(object,0x498u)==Get(before,0x498u));

    // NaN source/owner values remain unproven for original x87 FSTP
    // payload quieting. We refuse without modifying any owned bytes.
    for(const auto nan:std::array<std::uint32_t,4>{{
            0x7fc00001u,0x7fa00001u,0xffc00001u,0x7fffffffu}}) {
        Put(object,0x484u,0x3f800000u);
        before=object;
        assert(applyDirect(94u,2u,scale::kNativePlasmaScaleVA,nan)
               ==scale::Status::RefusedUnverifiedNaNBehavior);
        assert(object==before);
        assert(applyRouter(111u,2u,nan)==
               router::Status::UnsupportedSourceCallback);
        assert(object==before);
        Put(object,0x484u,nan);
        before=object;
        assert(applyDirect(111u,2u,scale::kNativeFireSmokeScaleVA,
                           0x40000000u)
               ==scale::Status::RefusedUnverifiedNaNBehavior);
        assert(object==before);
    }
    // NaN target only matters in FireSmoke's second comparison path.
    Put(object,0x484u,0x3f800000u);
    Put(object,0x49cu,0x7fc00001u);
    before=object;
    assert(applyDirect(111u,2u,scale::kNativeFireSmokeScaleVA,0x40000000u)
           ==scale::Status::RefusedUnverifiedNaNBehavior);
    assert(object==before);

    // Original PE32 native width checks before ANY write, including
    // non-4-byte buffer sizes and a just-one-byte-short +0x4A8 endpoint.
    for(const auto sz:std::array<std::size_t,5>{{
            0u,0x484u,0x49fu,0x4a0u,0x4a7u}}) {
        std::vector<std::uint8_t> shortObject(sz,0x3cu);
        const auto saved=shortObject;
        SetRaw(originalProxyData,0x40000000u);
        if(sz<0x4a8u)
            assert(scale::ApplySourceOriginalEffect(
                94u,2u,scale::kNativePlasmaScaleVA,originalProxyData,
                shortObject)==scale::Status::RefusedBadOwnedMemory);
        if(sz<0x4a0u)
            assert(scale::ApplySourceOriginalEffect(
                111u,2u,scale::kNativeFireSmokeScaleVA,originalProxyData,
                shortObject)==scale::Status::RefusedBadOwnedMemory);
        assert(shortObject==saved);
    }
    before=object;
    assert(applyDirect(94u,3u,scale::kNativePlasmaScaleVA,0x40000000u)
           ==scale::Status::UnrecognizedSource);
    assert(applyDirect(111u,2u,scale::kNativePlasmaScaleVA,0x40000000u)
           ==scale::Status::UnrecognizedSource);
    assert(object==before);

    // Infinity remains well-defined for float equality, unlike NaNs.
    Put(object,0x484u,0x3f800000u);
    Put(object,0x49cu,0x40000000u);
    assert(applyRouter(94u,2u,0x7f800000u)==router::Status::Applied);
    assert(Get(object,0x484u)==0x7f800000u);

    assert(token==0x7A9BCAEEu);
    std::puts("PASS Phase1712 original Plasma/FireSmoke x87-ordered scale "
              "callbacks, signed-zero, NaN refusal, atomic bounds; "
              "no executable retail slot5");
    return 0;
}
