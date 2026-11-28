#pragma once
// Phase1709: both original 2006 PE32 handle RecvProxy callbacks share
// EXACT 49-byte executable bodies, SHA256
// a67004960a3f69d9cfbf0662c7a7f2cc5016d0f14d8f277fbd748507314a8067.
// Original VAs 0x241A8DB0 and 0x241A8DF0 are SOURCE IDENTITIES ONLY:
// neither is executable or exposed to the independent client/Android.
// The original [CRecvProxyData+4] DWORD value transforms as:
//   if raw == 0x001FFFFF: destination DWORD = 0xFFFFFFFF
//   else: ((int32(raw) SAR 11) SHL 12) | (raw & 0x7FF)
// The equivalent unsigned, modulo-32-bit expression is verified against
// original signed SAR/SHL shifts for boundary and representative values.
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_client_custom_handle_proxy {
inline constexpr std::uint32_t kNativeHandleProxyVA=0x241A8DB0u;
inline constexpr std::uint32_t kNativeMoveParentProxyVA=0x241A8DF0u;
inline constexpr std::size_t kNativeOriginalMethodBytes=49u;
inline constexpr std::uint32_t kOriginalInvalidRawHandle=0x001FFFFFu;
inline constexpr std::uint32_t kNativeInvalidHandle=0xFFFFFFFFu;
inline constexpr std::uint32_t kHandleIndexMask=0x7FFu;
inline constexpr std::uint32_t kOriginalSourceValueOffset=4u;
inline constexpr std::size_t kSourceHandleAssignments=30u;
inline constexpr std::size_t kSourceMoveParentAssignments=5u;

constexpr bool IsOriginalHandleProxy(std::uint32_t originalSourceVA) noexcept {
    return originalSourceVA==kNativeHandleProxyVA ||
           originalSourceVA==kNativeMoveParentProxyVA;
}
constexpr std::uint32_t DecodeSourceHandle(std::uint32_t originalValue) noexcept {
    if(originalValue==kOriginalInvalidRawHandle)
        return kNativeInvalidHandle;
    const auto sourceSerial=(originalValue<<1u)&0xFFFFF000u;
    return sourceSerial | (originalValue&kHandleIndexMask);
}
constexpr bool CanPublishRetailProxyPointers() noexcept {return false;}
static_assert(DecodeSourceHandle(0u)==0u);
static_assert(DecodeSourceHandle(0x7FFu)==0x7FFu);
static_assert(DecodeSourceHandle(0x800u)==0x1000u);
static_assert(DecodeSourceHandle(0x801u)==0x1001u);
static_assert(DecodeSourceHandle(kOriginalInvalidRawHandle)==kNativeInvalidHandle);
static_assert(DecodeSourceHandle(0x80000000u)==0u);
static_assert(DecodeSourceHandle(0xFFFFFFFFu)==0xFFFFF7FFu);
} // namespace sin_compat::original_client_custom_handle_proxy
