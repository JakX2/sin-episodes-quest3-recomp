#pragma once
// Phase1713: original 2006 PE32 Team player_array_element RecvProxy.
// The original 0x240F7240 is 24 executable bytes, SHA256
// 491da928d45037adb4f7934e27130a1e5924a1013c447c2a12b30b22ad8ea859
// Original x86 reads DWORD [CRecvProxyData+0x14] as the array index,
// owner third argument +0x480 as a PTR TO a 32-element array and
// [CRecvProxyData+4] as the network DWORD. It writes [array+index*4].
//
// This portable model never exposes, reuses or dereferences original
// 2006 PE32 addresses. The pointer stored in host object+0x480 is an
// explicitly SYNTHETIC bounded token into a host-owned vector, with
// a private 0x10000000 virtual base and a reserved object/array split.
// The specialized outer type4 player_array constructor/proxy and
// native Team owner allocation remain deliberately UNIMPLEMENTED.
// VClient011.slot5 and every retail pointer publication guard are closed.
#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include "sin_compat/original_client_source_proxy_catalog.hpp"
#include "sin_compat/original_client_native32_recvtable_manifest.hpp"
#include "sin_compat/original_client_native_member_offsets.hpp"
#include "sin_compat/original_client_native_type4_array_fields.hpp"

namespace sin_compat::original_client_custom_team_array_element_proxy {
namespace source=sin_compat::original_client_source_proxy_catalog;
namespace schema=sin_compat::original_client_native32_recvtable_manifest;
namespace members=sin_compat::original_client_native_member_offsets;
namespace arrays=sin_compat::original_client_native_type4_array_fields;

inline constexpr std::uint32_t kNativeTeamArrayElementVA=0x240F7240u;
inline constexpr std::size_t kNativeExecutableBytes=24u;
inline constexpr std::size_t kSourceCustomAssignments=1u;
inline constexpr std::size_t kSourceTableOrdinal=81u;
inline constexpr std::size_t kSourceSlot=3u;
inline constexpr std::size_t kSourceOuterArraySlot=4u;
inline constexpr std::size_t kArrayElements=32u;
inline constexpr std::size_t kArrayElementBytes=4u;
inline constexpr std::size_t kSourceDecodedDwordOffset=4u;
inline constexpr std::size_t kOriginalElementIndexSourceOffset=0x14u;
inline constexpr std::size_t kOwnerArrayPointerOffset=0x480u;
// The original PE32 pointer is NEVER a valid host address here.
inline constexpr std::uint32_t kSyntheticVirtualArenaBase=0x10000000u;
inline constexpr std::uint32_t kMinSyntheticArrayOffset=0x1400u;
inline constexpr std::size_t kSyntheticArrayBytes=
    kArrayElements*kArrayElementBytes;

constexpr bool SourceRoutesMatch() noexcept {
    if(source::SourceVAForSlot(kSourceTableOrdinal,kSourceSlot)!=
           kNativeTeamArrayElementVA ||
       schema::kTypes[
           schema::kTypePrefixes[kSourceTableOrdinal]+kSourceSlot]!=0u ||
       members::kSourceNativeMemberOffsets[
           schema::kTypePrefixes[kSourceTableOrdinal]+kSourceSlot]!=0u)
        return false;
    bool foundOuter=false;
    for(const auto& shape:arrays::kSourceArrays)
        if(shape.tableOrdinal==kSourceTableOrdinal &&
           shape.propertySlot==kSourceOuterArraySlot) {
            if(foundOuter || shape.elementCount!=kArrayElements ||
               !shape.nativeCallbackNotReconstructed)
                return false;
            foundOuter=true;
        }
    return foundOuter;
}
constexpr bool IsOriginalTeamElementProxy(std::uint32_t va) noexcept {
    return va==kNativeTeamArrayElementVA;
}
constexpr bool IsAttestedAssignment(std::size_t table,std::size_t slot,
                                     std::uint32_t va) noexcept {
    return table==kSourceTableOrdinal && slot==kSourceSlot &&
           va==kNativeTeamArrayElementVA;
}

enum class Status : std::uint8_t {
    Applied, UnrecognizedSource, RefusedBadOwnedMemory
};
inline std::uint32_t ReadDword(const std::vector<std::uint8_t>& bytes,
                                std::size_t at) noexcept {
    return std::uint32_t(bytes[at]) |
           (std::uint32_t(bytes[at+1u])<<8u) |
           (std::uint32_t(bytes[at+2u])<<16u) |
           (std::uint32_t(bytes[at+3u])<<24u);
}
inline std::uint32_t ReadSourceValue(
    const std::array<std::uint8_t,16>& originalProxyData) noexcept {
    const auto at=kSourceDecodedDwordOffset;
    return std::uint32_t(originalProxyData[at]) |
           (std::uint32_t(originalProxyData[at+1u])<<8u) |
           (std::uint32_t(originalProxyData[at+2u])<<16u) |
           (std::uint32_t(originalProxyData[at+3u])<<24u);
}
inline void WriteDword(std::vector<std::uint8_t>& bytes,
                       std::size_t offset,std::uint32_t value) noexcept {
    for(std::size_t i=0u;i<4u;++i)
        bytes[offset+i]=static_cast<std::uint8_t>(value>>(i*8u));
}

// Source's CRecvProxyData +0x14 DWORD is supplied as a separate decoded
// index because the existing portable primitive proxy record stores
// only its first sixteen bytes. This is a semantic field, not a forged
// byte-for-byte engine CRecvProxyData ABI.
// The original Team object+0x480 contains a native pointer. Only an
// explicit host-owned token whose ARRAY REGION is entirely in the
// existing vector, 4-aligned, AFTER object reserve +0x1400, is allowed.
inline Status ApplySourceOriginalEffect(
    std::size_t table,std::size_t slot,std::uint32_t sourceVA,
    const std::array<std::uint8_t,16>& originalProxyData,
    std::uint32_t sourceDecodedElementIndex,
    std::vector<std::uint8_t>& objectAndSyntheticArena) noexcept {
    if(!IsAttestedAssignment(table,slot,sourceVA))
        return Status::UnrecognizedSource;
    if(sourceDecodedElementIndex>=kArrayElements ||
       objectAndSyntheticArena.size()<kOwnerArrayPointerOffset+4u)
        return Status::RefusedBadOwnedMemory;
    const auto ptr=ReadDword(objectAndSyntheticArena,kOwnerArrayPointerOffset);
    if(ptr<kSyntheticVirtualArenaBase+kMinSyntheticArrayOffset)
        return Status::RefusedBadOwnedMemory;
    const auto offset=static_cast<std::size_t>(
        ptr-kSyntheticVirtualArenaBase);
    if((offset&3u)!=0u || offset>objectAndSyntheticArena.size() ||
       objectAndSyntheticArena.size()-offset<kSyntheticArrayBytes)
        return Status::RefusedBadOwnedMemory;
    const auto target=offset+
        static_cast<std::size_t>(sourceDecodedElementIndex)*
        kArrayElementBytes;
    WriteDword(objectAndSyntheticArena,target,
               ReadSourceValue(originalProxyData));
    return Status::Applied;
}
inline constexpr bool CanPublishRetailProxyPointers() noexcept {return false;}
static_assert(SourceRoutesMatch());
static_assert(IsAttestedAssignment(81u,3u,kNativeTeamArrayElementVA));
static_assert(!IsAttestedAssignment(81u,4u,kNativeTeamArrayElementVA));
static_assert(kSourceDecodedDwordOffset==4u);
static_assert(kOriginalElementIndexSourceOffset==20u);
static_assert(kSyntheticArrayBytes==128u);
static_assert(!CanPublishRetailProxyPointers());
} // namespace sin_compat::original_client_custom_team_array_element_proxy
