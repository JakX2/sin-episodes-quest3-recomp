#pragma once
// Phase1706 original-private SHA-pinned 2006 PE32 CLIENT source callback
// CALLSITE MAP. 1082 canonical RecvProp constructor routes (flags +8 are
// ZERO in these original factory argument sequences); 28 specialized
// constructors remain outside this canonical callback inventory.
//
// IMPORTANT: these VAs are labels in an original Windows executable,
// not host function pointers, not Android addresses and NOT SAFE to call.
// The independent client VClient011.slot5 remains guarded.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include "sin_compat/original_client_native32_recvtable_manifest.hpp"
namespace sin_compat::original_client_source_proxy_catalog {
namespace src=sin_compat::original_client_native32_recvtable_manifest;
inline constexpr std::size_t kCanonicalProperties=1082u;
inline constexpr std::size_t kSpecialProperties=28u;
inline constexpr std::size_t kSourceDefaultProxies=999u;
inline constexpr std::size_t kSourceCustomProxies=83u;
inline constexpr std::size_t kUniqueNativeSourceAddresses=44u;
// SHA256 of 1110 decoded index bytes:
// e74fe1d930d61ba70651fcb0a22dd6d13df222633d83a3812e2ddaffbff7b4f3
// SHA256 of 1110 original source proxy-VA DWORDs (unclassified=0):
// eb7f8427a064fe266cde69b8d587bc3b7835653da8c7b9f11f8fc9fa8ae9fc1f
// Index 0 is explicitly UNKNOWN, never inferred as a null source proxy.
inline constexpr std::array<std::uint32_t,kUniqueNativeSourceAddresses+1u>
kOriginalSourceCallbackVAs{{
    0x00000000u,
    0x2405fbd0u,
    0x2409c3e0u,
    0x2409c820u,
    0x2409ed70u,
    0x240c42d0u,
    0x240c4360u,
    0x240c4420u,
    0x240c61a0u,
    0x240c61e0u,
    0x240c7330u,
    0x240d2590u,
    0x240d2e80u,
    0x240d2f00u,
    0x240d2f50u,
    0x240d2fa0u,
    0x240da3e0u,
    0x240da430u,
    0x240eaa20u,
    0x240eaa60u,
    0x240ed640u,
    0x240f7240u,
    0x2411af10u,
    0x2411afb0u,
    0x2411b000u,
    0x2411b2e0u,
    0x2411b300u,
    0x2411b320u,
    0x2411b340u,
    0x24126c40u,
    0x24126c50u,
    0x24126c70u,
    0x24126c80u,
    0x24126c90u,
    0x24126ca0u,
    0x24126ce0u,
    0x241a8bf0u,
    0x241a8c20u,
    0x241a8cf0u,
    0x241a8d60u,
    0x241a8db0u,
    0x241a8df0u,
    0x241c2ec0u,
    0x24212e20u,
    0x242214f0u
}};
inline constexpr char kSourceIndexHex[]=
    "232323232323232323231e232323232323232323232323232323232323232323231f232823281f2328231f2121211e231e1d2121231e1e21211d1d1f21212c1d1d1d001d28231d23231f2223081f2325"
    "1d251d231e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1e1d23211d231f1e23231f22231f1f1d1d231d1d1e231f1f2121231f1d23231f2321231d211d1d2123231f1d"
    "1d1d23232323231d232322231e1e232b2323281f231d1d1d1d1d1d1e1d212328211d2a1d1d1d1d211f2321211e1e231e001e00281d211d1d1d23231e1e1e1f1d1d231d1e1e1f1d1d1d1d231d1d231d1d"
    "21212121231f232323222221212215002321241e1f21231d1d231e211d2128282020211414211d211f1d1d1e291f2123211d23211d23231f231d1d1f211d1d1f1f1f00000000000000231d1213212121"
    "2123231f231d23222222242121212121211e1e291d23232424221d1d1d1d1d1d2123231f2123231d231d1d1e21211d1d21212121212123231d2321211d1d1e1e1f2100231d1d1d21231d1d281f231d10"
    "1121212322001d001d211f231d23212328231d1d1d1d1d1d1d21231f1f1d211d1d1d231d1d1d23232328212121211d2121282800222300211e1d1d1d23061e1e210721212121211d1d2828291f0a0923"
    "21232626231d23232823232321212321211e2121211d001d001f1f21211f2823232823211f1f1f212121212128002100211d1d1d1d1d1d041d1d2121211d1e211e292321212103211d0721021e282121"
    "1f1d0023231d1d1f2808211d23232321212128211e1d1d1d29231e21231e1e211d212121231e211d212321211e231e211d21231e231e1e1d21231e2121231e1d1d1d1e2121231d1d1d231e1e1d212321"
    "212321231e1e211f231e211d1d21231e1e1e1e1d1d1d211f1f1f1f1f1f231e1e21211f232121212123211d21211e1f21211e1e1f2323231e212121211d1d1d231e1e212121231e1e211d211d231e1e21"
    "1d211d231e2121231e1d1d1d1e1e2121211d21231e21212121211e1e21212121212121211e00231e1d1d232121231e1e2321212321212321211e1e212121211d1d1d211d212121212121231e1e231d1d"
    "1d1d1d1d1d1f211f2121231d1d1d1d1d1d211d1d251e1e1d1e1e1d1e231d1e1e1d1d1d1d1d1d1d1d1d1f1f211d211d23211d231d1e1d1d1d1d1f231d1d1d1d1d231d1e1e1d1d1d1d1d1d1d1f211d1f1d"
    "231d1e1e1d1d1d1d1d1d1d1d1d1f1f211d211d23280021211d212121211f1f1e231d1d1d011e1e2323211e231e1e1d1d232121211f2323231f2121212121211d1d2121211d1d1d1d211d252521212127"
    "1f1d211d1d1d21001d0005271f1f00210c1d0b1f1f1f1d1d1d1d1e1e1e1f1f1f1d1f211e211f1f1e21211d1d1f1f1e21211d1d1d1e1e1e1e1e1e1e1e212128231d1d1d1d001f1f212128280d0e0f1e28"
    "00281e1d1d1d1d1d211d211b1c191a171816161d1d1d1d1d1d1e1e211d1d212021212121211f1d1d1d1d1d1d1e1d1d1d1d1d1e1e1e1e1e1d1f1d1d1d1f1f2121211f1f1d1d28";
constexpr std::uint8_t HexDigit(char c) noexcept {
    return c>='0'&&c<='9'?static_cast<std::uint8_t>(c-'0'):
        c>='a'&&c<='f'?static_cast<std::uint8_t>(c-'a'+10u):0xffu;
}
constexpr bool IsHexValid() noexcept {
    for(std::size_t i=0u;i<sizeof(kSourceIndexHex)-1u;++i)
        if(HexDigit(kSourceIndexHex[i])>=16u)return false;
    return true;
}
constexpr std::array<std::uint8_t,src::kStaticPropertySlots> Decode() noexcept {
    std::array<std::uint8_t,src::kStaticPropertySlots> out{};
    for(std::size_t i=0u;i<out.size();++i)
        out[i]=static_cast<std::uint8_t>(
            (HexDigit(kSourceIndexHex[2u*i])<<4u)|
              HexDigit(kSourceIndexHex[2u*i+1u]));
    return out;
}
inline constexpr auto kSourceIndices=Decode();
constexpr bool IsStandardOriginalSourceVA(std::uint32_t va) noexcept {
    return va==0x24126c40u || va==0x24126c50u ||
           va==0x24126c70u || va==0x24126c80u ||
           va==0x24126c90u || va==0x24126ca0u ||
           va==0x24126ce0u;
}
constexpr bool FullyClassifiedCanonicalRoutes() noexcept {
    std::size_t canonical=0u,special=0u,standard=0u,custom=0u;
    std::array<std::size_t,6> canonicalTypes{};
    for(std::size_t i=0u;i<kSourceIndices.size();++i) {
        const auto index=kSourceIndices[i];
        if(index>=kOriginalSourceCallbackVAs.size())return false;
        const auto type=src::kTypes[i];
        if(index==0u) {
            ++special;
            if(type!=4u && type!=5u)return false;
            continue;
        }
        ++canonical;
        ++canonicalTypes[type];
        const auto va=kOriginalSourceCallbackVAs[index];
        if(va<0x24001000u || va>=0x24276000u)return false;
        if(IsStandardOriginalSourceVA(va)) {
            ++standard;
            if((type==0u && va!=0x24126c70u &&
                  va!=0x24126c80u && va!=0x24126c90u) ||
               (type==1u && va!=0x24126c40u) ||
               (type==2u && va!=0x24126c50u) ||
               (type==3u && va!=0x24126ca0u) ||
               (type==5u && va!=0x24126ce0u))
                return false;
        }else ++custom;
    }
    return canonical==kCanonicalProperties && special==kSpecialProperties &&
        standard==kSourceDefaultProxies && custom==kSourceCustomProxies &&
        canonicalTypes[0]==414u && canonicalTypes[1]==300u &&
        canonicalTypes[2]==164u && canonicalTypes[3]==12u &&
        canonicalTypes[4]==0u && canonicalTypes[5]==192u;
}
inline constexpr std::uint32_t SourceVAForSlot(std::size_t sourceTableOrdinal,
                                                 std::size_t slot) {
    if(sourceTableOrdinal>=src::kStaticTables ||
       slot>=src::kCounts[sourceTableOrdinal])
        throw std::out_of_range("original native source callback slot");
    return kOriginalSourceCallbackVAs[
        kSourceIndices[src::kTypePrefixes[sourceTableOrdinal]+slot]];
}
inline constexpr bool CanExposeToRetailEngine() noexcept {return false;}
static_assert(sizeof(kSourceIndexHex)-1u==src::kStaticPropertySlots*2u);
static_assert(IsHexValid());
static_assert(FullyClassifiedCanonicalRoutes());
} // namespace sin_compat::original_client_source_proxy_catalog
