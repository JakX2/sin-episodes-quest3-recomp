#pragma once
// Phase1714: original SiN Episodes (2006) PE32 custom GameRules
// RecvProxy at 0x24212E20 is exactly 13 executable bytes, SHA256
// 1b9fa5d21adeab9069a2d97339fed5cfa6e42055c079877dbe5fe7d6a2461043
// Its whole body is three MOV instructions and RET:
//   EAX = [ESP+8]                 // native data-table output DWORD*
//   ECX = [original 0x2433F3F8] // original global pointer DWORD
//   [EAX] = ECX                  // return source global's current pointer
//
// Neither the original global nor its original VALUE may be called,
// dereferenced, moved into a real engine table, or published to Android.
// Only a separately authenticated host-owned SYNTHETIC token is returned.
// This is an independently modeled host reference function, not an
// executable original 2006 PE32 network or ClientClass ABI.
// Original VClient011.slot5 remains deliberately guarded.
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_source_proxy_catalog.hpp"
#include "sin_compat/original_client_native32_recvtable_manifest.hpp"
#include "sin_compat/original_client_native_member_offsets.hpp"

namespace sin_compat::original_client_custom_gamerules_datatable_proxy {
namespace source=sin_compat::original_client_source_proxy_catalog;
namespace manifest=sin_compat::original_client_native32_recvtable_manifest;
namespace members=sin_compat::original_client_native_member_offsets;
inline constexpr std::uint32_t kNativeOriginalCallbackVA=0x24212E20u;
inline constexpr std::uint32_t kNativeSourceGlobalAddressVA=0x2433F3F8u;
inline constexpr std::size_t kNativeExecutableBytes=13u;
inline constexpr std::size_t kSourceCustomAssignments=1u;
inline constexpr std::size_t kOriginalTableOrdinal=64u;
inline constexpr std::size_t kOriginalPropertySlot=1u;
// This is an original-FREE host token convention, NOT the original
// 2006 Source client's native address range. Never reinterpret as ptr.
inline constexpr std::uint32_t kSyntheticGameObjectBase=0x10000000u;
inline constexpr std::uint32_t kSyntheticGameObjectLimit=0x20000000u;

constexpr bool IsOriginalGameRulesProxy(std::uint32_t va) noexcept {
    return va==kNativeOriginalCallbackVA;
}
constexpr bool IsSourceAssignment(std::size_t table,std::size_t slot,
                                  std::uint32_t va) noexcept {
    return table==kOriginalTableOrdinal && slot==kOriginalPropertySlot &&
           va==kNativeOriginalCallbackVA;
}
constexpr bool SourceRoutesMatch() noexcept {
    const auto index=manifest::kTypePrefixes[kOriginalTableOrdinal]+
                     kOriginalPropertySlot;
    return source::SourceVAForSlot(kOriginalTableOrdinal,kOriginalPropertySlot)
                    ==kNativeOriginalCallbackVA &&
           manifest::kTypes[index]==5u &&
           members::kSourceNativeMemberOffsets[index]==0u;
}
constexpr bool IsHostSyntheticToken(std::uint32_t token) noexcept {
    // A real source singleton may be null. Non-null values must be
    // deliberately host-owned tokens, not retail PE32 addresses.
    return token==0u || (token>=kSyntheticGameObjectBase &&
                          token<kSyntheticGameObjectLimit &&
                          (token&3u)==0u);
}
enum class Status : std::uint8_t {
    Applied, UnrecognizedSource, RefusedUnboundHostSingleton,
    RefusedUnsafeSyntheticToken
};
inline Status ApplySourceOriginalEffect(
    std::size_t table,std::size_t slot,std::uint32_t sourceVA,
    bool hasAuthenticatedHostSingleton,
    std::uint32_t hostSingletonToken,
    std::uint32_t& dataTableOutputToken) noexcept {
    if(!IsSourceAssignment(table,slot,sourceVA))
        return Status::UnrecognizedSource;
    if(!hasAuthenticatedHostSingleton)
        return Status::RefusedUnboundHostSingleton;
    if(!IsHostSyntheticToken(hostSingletonToken))
        return Status::RefusedUnsafeSyntheticToken;
    dataTableOutputToken=hostSingletonToken;
    return Status::Applied;
}
inline constexpr bool CanPublishRetailProxyPointers() noexcept {return false;}
static_assert(SourceRoutesMatch());
static_assert(IsSourceAssignment(64u,1u,kNativeOriginalCallbackVA));
static_assert(!IsSourceAssignment(64u,0u,kNativeOriginalCallbackVA));
static_assert(IsHostSyntheticToken(0u));
static_assert(IsHostSyntheticToken(0x10002000u));
static_assert(!IsHostSyntheticToken(kNativeSourceGlobalAddressVA));
static_assert(!IsHostSyntheticToken(kNativeOriginalCallbackVA));
static_assert(!IsHostSyntheticToken(0x10002001u));
static_assert(!CanPublishRetailProxyPointers());
} // namespace sin_compat::original_client_custom_gamerules_datatable_proxy
