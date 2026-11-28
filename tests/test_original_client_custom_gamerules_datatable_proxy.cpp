// Phase1714: original GameRules global-to-datatable source callback.
#include "sin_compat/original_client_custom_gamerules_datatable_proxy.hpp"
#include "sin_compat/original_client_standard_recvproxy_router.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace original=sin_compat::original_client_custom_gamerules_datatable_proxy;
namespace router=sin_compat::original_client_standard_recvproxy_router;
namespace source=sin_compat::original_client_source_proxy_catalog;

int main() {
    static_assert(original::SourceRoutesMatch());
    static_assert(original::kSourceCustomAssignments==1u);
    static_assert(!original::CanPublishRetailProxyPointers());
    static_assert(!router::CanPublishRetailProxyPointers());
    assert(source::SourceVAForSlot(64u,1u)==
           original::kNativeOriginalCallbackVA);

    std::uint32_t output=0xCAFEBABEu;
    const auto applyOriginal=[&](std::size_t table,std::size_t slot,
                                  std::uint32_t va,bool authenticated,
                                  std::uint32_t token) {
        return original::ApplySourceOriginalEffect(
            table,slot,va,authenticated,token,output);
    };
    assert(applyOriginal(64u,1u,original::kNativeOriginalCallbackVA,
                         false,0x10002000u)==
           original::Status::RefusedUnboundHostSingleton);
    assert(output==0xCAFEBABEu);
    for(const auto bad:std::array<std::uint32_t,7>{{
            original::kNativeSourceGlobalAddressVA,
            original::kNativeOriginalCallbackVA,0xFFFFFFFFu,
            0x10000001u,0x1FFFFFFFu,0x20000000u,0x0FFFFFFCu}}) {
        assert(applyOriginal(64u,1u,original::kNativeOriginalCallbackVA,
                             true,bad)==
               original::Status::RefusedUnsafeSyntheticToken);
        assert(output==0xCAFEBABEu);
    }
    assert(applyOriginal(64u,0u,original::kNativeOriginalCallbackVA,
                         true,0x10002000u)==
           original::Status::UnrecognizedSource);
    assert(applyOriginal(64u,1u,0x24126CE0u,
                         true,0x10002000u)==
           original::Status::UnrecognizedSource);
    assert(output==0xCAFEBABEu);

    // Explicit, caller-owned synthetic opaque address token, never the
    // old 0x2433F3F8 process GLOBAL or its original pointer value.
    assert(applyOriginal(64u,1u,original::kNativeOriginalCallbackVA,
                         true,0x10002000u)==original::Status::Applied);
    assert(output==0x10002000u);
    assert(applyOriginal(64u,1u,original::kNativeOriginalCallbackVA,
                         true,0u)==original::Status::Applied);
    assert(output==0u); // original source global could be null

    router::DecodedOwnedInput incoming{};
    std::vector<std::uint8_t> object(6000u,0xABu);
    const auto unchanged=object;
    output=0x10203040u;
    assert(router::ApplySourceStandardProperty(
        64u,1u,incoming,object,output)==
        router::Status::UnsupportedSourceCallback);
    assert(output==0x10203040u && object==unchanged);

    incoming.hasAuthenticatedGameRulesSingleton=true;
    incoming.ownedGameRulesSingletonToken=0x10005000u;
    assert(router::ApplySourceStandardProperty(
        64u,1u,incoming,object,output)==router::Status::Applied);
    assert(output==0x10005000u && object==unchanged);
    incoming.ownedGameRulesSingletonToken=0x2433F3F8u;
    assert(router::ApplySourceStandardProperty(
        64u,1u,incoming,object,output)==
        router::Status::RefusedBadOwnedMemory);
    assert(output==0x10005000u && object==unchanged);
    incoming.ownedGameRulesSingletonToken=0u;
    assert(router::ApplySourceStandardProperty(
        64u,1u,incoming,object,output)==router::Status::Applied);
    assert(output==0u && object==unchanged);

    std::puts("PASS Phase1714 original GameRules 13-byte global-to-output "
              "callback modeled with authenticated synthetic token, "
              "unknown original global and VClient011.slot5 blocked");
    return 0;
}
