#pragma once
// Phase1711: three source-SHA authenticated, self-contained 2006 PE32
// receive callbacks, reconstructed only against bounded HOST OWNED memory.
//
// 0x240C61E0 (18 bytes): movecollide -> native owner +0x148.
// SHA256 f496a564e61e8a5b3fac73fab8e2f9a0c45afd6fdfa36c38b0eed0d857208c00
// 0x240C7330 (28 bytes): movetype -> +0x144 and resets +0x148.
// SHA256 4929e26544167a91cc8298181e40544cb9831cdcb426ebd73b0c1a450b0d730a
// 0x240D2590 (27 bytes): m_iDefaultFOV -> conditional reset of +0x2C
// and update of +0x38 only when the received value differs.
// SHA256 a127871117094cba6b47fa51952629dfcd2bb7a447c82e3957719a20c98531dc
//
// These original Win32 addresses are labels, never callable function
// pointers. No original networking packet or client class is exposed.
// RecvProp +44 is ZERO for movetype/movecollide: the ORIGINAL callbacks
// use their own hardcoded entity offsets, NOT that metadata member base.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "sin_compat/original_client_native32_recvtable_manifest.hpp"
#include "sin_compat/original_client_native_member_offsets.hpp"
#include "sin_compat/original_client_source_proxy_catalog.hpp"

namespace sin_compat::original_client_custom_movement_fov_proxies {
namespace schema=sin_compat::original_client_native32_recvtable_manifest;
namespace members=sin_compat::original_client_native_member_offsets;
namespace source=sin_compat::original_client_source_proxy_catalog;

inline constexpr std::uint32_t kNativeMoveCollideVA=0x240C61E0u;
inline constexpr std::uint32_t kNativeMoveTypeVA=0x240C7330u;
inline constexpr std::uint32_t kNativeDefaultFovVA=0x240D2590u;
inline constexpr std::size_t kNativeMoveCollideBytes=18u;
inline constexpr std::size_t kNativeMoveTypeBytes=28u;
inline constexpr std::size_t kNativeDefaultFovBytes=27u;
inline constexpr std::size_t kOriginalDecodedProxyValueOffset=4u;
inline constexpr std::size_t kSourceCustomAssignments=3u;
inline constexpr std::size_t kMoveTypeObjectField=0x144u;
inline constexpr std::size_t kMoveCollideObjectField=0x148u;
inline constexpr std::size_t kDefaultFovObjectField=0x38u;
inline constexpr std::size_t kDefaultFovResetField=0x2Cu;

enum class Effect : std::uint8_t { Unsupported, MoveType, MoveCollide, DefaultFov };
constexpr Effect SourceEffect(std::uint32_t va) noexcept {
    return va==kNativeMoveTypeVA?Effect::MoveType:
           va==kNativeMoveCollideVA?Effect::MoveCollide:
           va==kNativeDefaultFovVA?Effect::DefaultFov:
           Effect::Unsupported;
}
constexpr bool IsOriginalMovementOrFovProxy(std::uint32_t va) noexcept {
    return SourceEffect(va)!=Effect::Unsupported;
}
// The exact three ORIGINAL source table/property names and callback
// identities, with no attempt to infer other source functions.
constexpr bool IsAttestedAssignment(std::size_t table,std::size_t slot,
                                     std::uint32_t va) noexcept {
    return (table==120u && slot==17u && va==kNativeMoveTypeVA) ||
           (table==120u && slot==18u && va==kNativeMoveCollideVA) ||
           (table==202u && slot==4u && va==kNativeDefaultFovVA);
}
constexpr bool SourceRoutesMatch() noexcept {
    return source::SourceVAForSlot(120u,17u)==kNativeMoveTypeVA &&
           source::SourceVAForSlot(120u,18u)==kNativeMoveCollideVA &&
           source::SourceVAForSlot(202u,4u)==kNativeDefaultFovVA &&
           schema::kTypes[schema::kTypePrefixes[120u]+17u]==0u &&
           schema::kTypes[schema::kTypePrefixes[120u]+18u]==0u &&
           schema::kTypes[schema::kTypePrefixes[202u]+4u]==0u &&
           members::kSourceNativeMemberOffsets[
               schema::kTypePrefixes[120u]+17u]==0u &&
           members::kSourceNativeMemberOffsets[
               schema::kTypePrefixes[120u]+18u]==0u &&
           members::kSourceNativeMemberOffsets[
               schema::kTypePrefixes[202u]+4u]==0x38u;
}

enum class Status : std::uint8_t {
    Applied, UnrecognizedSource, RefusedBadOwnedMemory
};

inline std::uint32_t ReadLittleEndianDword(
    const std::uint8_t* src) noexcept {
    return std::uint32_t(src[0]) |
           (std::uint32_t(src[1])<<8u) |
           (std::uint32_t(src[2])<<16u) |
           (std::uint32_t(src[3])<<24u);
}
inline std::uint32_t ReadOwnedDword(const std::vector<std::uint8_t>& mem,
                                    std::size_t offset) noexcept {
    return ReadLittleEndianDword(mem.data()+offset);
}
inline void WriteOwnedDword(std::vector<std::uint8_t>& mem,
                            std::size_t offset,std::uint32_t value) noexcept {
    for(std::size_t i=0u;i<4u;++i)
        mem[offset+i]=static_cast<std::uint8_t>(value>>(i*8u));
}

// Source x86 CRecvProxyData+4 (decoded native DWORD), and a bounded host
// object. All writes happen AFTER complete bounds validation. This is
// independent reference behavior only, never actual engine callbacks.
inline Status ApplySourceOriginalEffect(
    std::size_t table,std::size_t slot,std::uint32_t sourceVA,
    const std::array<std::uint8_t,16>& originalProxyData,
    std::vector<std::uint8_t>& objectBytes) noexcept {
    if(!IsAttestedAssignment(table,slot,sourceVA))
        return Status::UnrecognizedSource;
    const auto effect=SourceEffect(sourceVA);
    const auto needed=effect==Effect::DefaultFov?
        kDefaultFovObjectField+4u:kMoveCollideObjectField+4u;
    if(objectBytes.size()<needed)
        return Status::RefusedBadOwnedMemory;
    const auto raw=ReadLittleEndianDword(
        originalProxyData.data()+kOriginalDecodedProxyValueOffset);
    switch(effect) {
        case Effect::MoveCollide:
            WriteOwnedDword(objectBytes,kMoveCollideObjectField,raw);
            break;
        case Effect::MoveType:
            WriteOwnedDword(objectBytes,kMoveTypeObjectField,raw);
            WriteOwnedDword(objectBytes,kMoveCollideObjectField,0u);
            break;
        case Effect::DefaultFov:
            if(ReadOwnedDword(objectBytes,kDefaultFovObjectField)!=raw) {
                WriteOwnedDword(objectBytes,kDefaultFovResetField,0u);
                WriteOwnedDword(objectBytes,kDefaultFovObjectField,raw);
            }
            break;
        case Effect::Unsupported:
            return Status::UnrecognizedSource;
    }
    return Status::Applied;
}
inline constexpr bool CanPublishRetailProxyPointers() noexcept {return false;}
static_assert(SourceRoutesMatch());
static_assert(IsAttestedAssignment(120u,17u,kNativeMoveTypeVA));
static_assert(IsAttestedAssignment(120u,18u,kNativeMoveCollideVA));
static_assert(IsAttestedAssignment(202u,4u,kNativeDefaultFovVA));
static_assert(!IsAttestedAssignment(120u,17u,kNativeMoveCollideVA));
static_assert(!CanPublishRetailProxyPointers());
} // namespace sin_compat::original_client_custom_movement_fov_proxies
