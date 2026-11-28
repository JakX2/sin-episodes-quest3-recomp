#pragma once
// Phase1708: all twelve original 2006 PE32 static STRING RecvProp +12
// buffer sizes, from the actual original 0x24126AD0 constructor callsites.
// This is no runtime RecvProxy pointer or packet deserializer.
// 12 (table ordinal uint8, slot uint8, buffer-size LE uint16) tuples:
// SHA256 62b1a1a412724fbd51a2556545af0c28b8819ee1427ece1b6f7d1e4d5e5b5292
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_native32_recvtable_manifest.hpp"
#include "sin_compat/original_client_source_proxy_catalog.hpp"
namespace sin_compat::original_client_source_string_sizes {
namespace src=sin_compat::original_client_native32_recvtable_manifest;
namespace proxy=sin_compat::original_client_source_proxy_catalog;
struct SourceStringSize final {
    std::uint8_t tableOrdinal;
    std::uint8_t propertySlot;
    std::uint16_t size;
};
inline constexpr std::array<SourceStringSize,12u> kOriginalStrings{{
    {40u,2u,32u},
    {47u,2u,255u},
    {62u,1u,255u},
    {80u,1u,64u},
    {80u,2u,256u},
    {81u,2u,32u},
    {98u,1u,255u},
    {98u,2u,255u},
    {98u,3u,255u},
    {101u,3u,255u},
    {112u,1u,255u},
    {118u,14u,18u},
}};
constexpr std::uint16_t SourceCapacity(std::size_t table,std::size_t slot) noexcept {
    for(const auto& item:kOriginalStrings)
        if(item.tableOrdinal==table && item.propertySlot==slot)
            return item.size;
    return 0u;
}
constexpr bool EverySourceStringCapacityAuthenticated() noexcept {
    std::size_t sourceStringTypes=0u;
    for(std::size_t t=0u;t<src::kStaticTables;++t) {
        for(std::size_t p=0u;p<src::kCounts[t];++p) {
            if(src::kTypes[src::kTypePrefixes[t]+p]!=3u)continue;
            ++sourceStringTypes;
            unsigned matching=0u;
            for(const auto& item:kOriginalStrings)
                if(item.tableOrdinal==t && item.propertySlot==p &&
                   item.size>0u && item.size<=256u &&
                   proxy::SourceVAForSlot(t,p)==0x24126CA0u)
                    ++matching;
            if(matching!=1u)return false;
        }
    }
    return sourceStringTypes==kOriginalStrings.size();
}
inline constexpr bool CanExposeToRetailEngine() noexcept {return false;}
static_assert(EverySourceStringCapacityAuthenticated());
} // namespace sin_compat::original_client_source_string_sizes
