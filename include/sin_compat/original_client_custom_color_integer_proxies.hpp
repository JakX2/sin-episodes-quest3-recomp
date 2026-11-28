#pragma once
// Phase1710 source reconstructed custom RecvProxy methods from the
// owner's original SiN Episodes (2006) PE32 client.dll:
// 0x241A8BF0, 38 executable bytes, SHA256
// d60b249c0c261174c934cd38641d46656fd299741af2b43b3bc4cd7e944a5a1b
// 0x241A8C20, 17 executable bytes, SHA256
// bbea8c9b7a7fce6219d595fb3305ce6b6137f5186e171df2ab93d3fe23323582
// Original x86 pointer VAs are SOURCE LABELS, never exposed as callbacks.
// These functions operate solely on host-owned decoded 32-bit values.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_client_custom_color_integer_proxies {
inline constexpr std::uint32_t kNativeColorByteReverseVA=0x241A8BF0u;
inline constexpr std::uint32_t kNativeDecrementDwordVA=0x241A8C20u;
inline constexpr std::size_t kNativeColorExecutableBytes=38u;
inline constexpr std::size_t kNativeDecrementExecutableBytes=17u;
inline constexpr std::size_t kSourceColorProperties=4u;
inline constexpr std::size_t kSourceDecrementProperties=5u;
inline constexpr std::uint32_t kOriginalDecodedProxyValueOffset=4u;

constexpr bool IsOriginalCustomColorOrDecrement(
    std::uint32_t originalSourceVA) noexcept {
    return originalSourceVA==kNativeColorByteReverseVA ||
           originalSourceVA==kNativeDecrementDwordVA;
}
constexpr std::array<std::uint8_t,4> SourceColorBytes(
    std::uint32_t raw) noexcept {
    // x86 SHR EDX,24 / 16 / 8, write byte to output+0/+1/+2;
    // output+3 is native AL of the unchanged input DWORD.
    return {{
        static_cast<std::uint8_t>(raw>>24u),
        static_cast<std::uint8_t>(raw>>16u),
        static_cast<std::uint8_t>(raw>>8u),
        static_cast<std::uint8_t>(raw)
    }};
}
constexpr std::uint32_t SourceDecrementDword(std::uint32_t raw) noexcept {
    // Original x86 SUB ECX,1 wraps in 32 bits.
    return raw-1u;
}
inline constexpr bool CanPublishRetailProxyPointers() noexcept {return false;}
static_assert(SourceColorBytes(0x11223344u)[0]==0x11u);
static_assert(SourceColorBytes(0x11223344u)[1]==0x22u);
static_assert(SourceColorBytes(0x11223344u)[2]==0x33u);
static_assert(SourceColorBytes(0x11223344u)[3]==0x44u);
static_assert(SourceDecrementDword(0u)==0xFFFFFFFFu);
static_assert(SourceDecrementDword(1u)==0u);
static_assert(SourceDecrementDword(0xFFFFFFFFu)==0xFFFFFFFEu);
} // namespace sin_compat::original_client_custom_color_integer_proxies
