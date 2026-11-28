#include "sin_compat/original_client_custom_color_integer_proxies.hpp"
#include "sin_compat/original_client_source_proxy_catalog.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace host=sin_compat::original_client_custom_color_integer_proxies;
namespace source=sin_compat::original_client_source_proxy_catalog;

int main() {
    static_assert(host::kNativeColorExecutableBytes==38u);
    static_assert(host::kNativeDecrementExecutableBytes==17u);
    static_assert(host::kOriginalDecodedProxyValueOffset==4u);
    static_assert(host::kSourceColorProperties==4u);
    static_assert(host::kSourceDecrementProperties==5u);
    static_assert(!host::CanPublishRetailProxyPointers());
    assert(host::IsOriginalCustomColorOrDecrement(0x241A8BF0u));
    assert(host::IsOriginalCustomColorOrDecrement(0x241A8C20u));
    assert(!host::IsOriginalCustomColorOrDecrement(0x24126C90u));
    assert(!host::IsOriginalCustomColorOrDecrement(0u));

    // Source original 38B routine writes AL after EDX SHR24/16/8:
    // output bytes therefore read high-to-low from the input DWORD.
    for(const auto raw:std::array<std::uint32_t,7>{{
        0u,1u,0x01000000u,0x00010203u,0x12345678u,
        0x80000000u,0xFFFFFFFFu}}) {
        const auto written=host::SourceColorBytes(raw);
        assert(written[0]==static_cast<std::uint8_t>(raw>>24u));
        assert(written[1]==static_cast<std::uint8_t>(raw>>16u));
        assert(written[2]==static_cast<std::uint8_t>(raw>>8u));
        assert(written[3]==static_cast<std::uint8_t>(raw));
        const auto reversed=
            (std::uint32_t(written[0])<<24u)|
            (std::uint32_t(written[1])<<16u)|
            (std::uint32_t(written[2])<<8u)|written[3];
        assert(reversed==raw);
        assert(host::SourceDecrementDword(raw)==raw-1u);
    }
    assert(host::SourceDecrementDword(0u)==0xFFFFFFFFu);
    assert(host::SourceDecrementDword(1u)==0u);

    // Every original native callback identity is source-attested from
    // real constructor callsites. Custom ones are not default fallbacks.
    std::size_t nativeColor=0u,nativeDecrement=0u;
    for(std::size_t table=0u;table<211u;++table)
        for(std::size_t slot=0u;
            slot<sin_compat::original_client_native32_recvtable_manifest::kCounts[table];
            ++slot) {
            const auto va=source::SourceVAForSlot(table,slot);
            if(va==host::kNativeColorByteReverseVA)++nativeColor;
            if(va==host::kNativeDecrementDwordVA)++nativeDecrement;
        }
    assert(nativeColor==4u && nativeDecrement==5u);
    assert(!host::CanPublishRetailProxyPointers());
    std::puts("PASS Phase1710 38-byte original source color-byte reordering "
              "+ 17-byte decrement modulo32; exact four/five callback routes");
    return 0;
}
