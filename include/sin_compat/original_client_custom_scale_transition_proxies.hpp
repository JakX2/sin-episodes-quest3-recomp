#pragma once
// Phase1712: source-reconstructed original 2006 PE32 m_flScale
// receive callbacks for DT_Plasma and DT_FireSmoke. Only original
// x86 function effects on BOUNDED host-owned object bytes are modeled.
//
// Original 0x240EAA20, 56 executable bytes, SHA256:
// 5d57913b243190d93e89a433b6cd6168eb08047f1e4db82d171cbc9db251a530
// Original 0x240DA3E0, 70 executable bytes, SHA256:
// 99b2ce915c93019fa10726580b4b971ab35cbdc28403bc171b8309ec4be053b6
//
// Original FUCOMPP/FNSTSW/TEST AH,0x44/JNP means the branch is taken
// for ORDERED EQUALITY, including +0/-0. Never replace this with a
// bit-for-bit non-equality test. All IEEE754 non-NaN binary32 equality
// is representable by integer encodings plus the signed-zero exception.
// NaN payload/quieting under old x87 FLD/FSTP remains UNVERIFIED, so
// NaN inputs/stored comparison values are deliberately refused.
// No old PE32 address is called. VClient011.slot5 stays guarded.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "sin_compat/original_client_native32_recvtable_manifest.hpp"
#include "sin_compat/original_client_native_member_offsets.hpp"
#include "sin_compat/original_client_source_proxy_catalog.hpp"

namespace sin_compat::original_client_custom_scale_transition_proxies {
namespace schema=sin_compat::original_client_native32_recvtable_manifest;
namespace members=sin_compat::original_client_native_member_offsets;
namespace source=sin_compat::original_client_source_proxy_catalog;

inline constexpr std::uint32_t kNativePlasmaScaleVA=0x240EAA20u;
inline constexpr std::uint32_t kNativeFireSmokeScaleVA=0x240DA3E0u;
inline constexpr std::size_t kNativePlasmaMethodBytes=56u;
inline constexpr std::size_t kNativeFireSmokeMethodBytes=70u;
inline constexpr std::size_t kSourceCustomAssignments=2u;
inline constexpr std::size_t kSourceDecodedValueOffset=4u;
inline constexpr std::size_t kSourceScaleCurrent=0x484u;
inline constexpr std::size_t kSourceFireSmokePrevious=0x494u;
inline constexpr std::size_t kSourceFireSmokeSaved=0x498u;
inline constexpr std::size_t kSourceScaleTarget=0x49Cu;
inline constexpr std::size_t kSourcePlasmaSavedTarget=0x4A0u;
inline constexpr std::size_t kSourcePlasmaTransition=0x4A4u;

constexpr bool IsSourceScaleProxy(std::uint32_t va) noexcept {
    return va==kNativePlasmaScaleVA || va==kNativeFireSmokeScaleVA;
}
constexpr bool IsAttestedAssignment(std::size_t table,std::size_t slot,
                                     std::uint32_t nativeVA) noexcept {
    return (table==94u && slot==2u && nativeVA==kNativePlasmaScaleVA) ||
           (table==111u && slot==2u && nativeVA==kNativeFireSmokeScaleVA);
}
constexpr bool SourceRoutesMatch() noexcept {
    return source::SourceVAForSlot(94u,2u)==kNativePlasmaScaleVA &&
           source::SourceVAForSlot(111u,2u)==kNativeFireSmokeScaleVA &&
           schema::kTypes[schema::kTypePrefixes[94u]+2u]==1u &&
           schema::kTypes[schema::kTypePrefixes[111u]+2u]==1u &&
           members::kSourceNativeMemberOffsets[
               schema::kTypePrefixes[94u]+2u]==0x484u &&
           members::kSourceNativeMemberOffsets[
               schema::kTypePrefixes[111u]+2u]==0x484u;
}

enum class Status : std::uint8_t {
    Applied, UnrecognizedSource, RefusedBadOwnedMemory,
    RefusedUnverifiedNaNBehavior
};
constexpr bool IsBinary32NaN(std::uint32_t bits) noexcept {
    return (bits&0x7F800000u)==0x7F800000u &&
           (bits&0x007FFFFFu)!=0u;
}
constexpr bool OriginalOrderedFloatEqual(std::uint32_t a,
                                         std::uint32_t b) noexcept {
    // Calling code refuses NaN values first. IEEE754 binary32 has one
    // unique encoding per nonzero value, but +0 and -0 compare equal.
    return a==b || ((a&0x7FFFFFFFu)==0u && (b&0x7FFFFFFFu)==0u);
}
inline std::uint32_t ReadDword(const std::uint8_t* src) noexcept {
    return std::uint32_t(src[0]) |
           (std::uint32_t(src[1])<<8u) |
           (std::uint32_t(src[2])<<16u) |
           (std::uint32_t(src[3])<<24u);
}
inline std::uint32_t ReadOwnedDword(const std::vector<std::uint8_t>& bytes,
                                    std::size_t at) noexcept {
    return ReadDword(bytes.data()+at);
}
inline void WriteOwnedDword(std::vector<std::uint8_t>& bytes,
                            std::size_t at,std::uint32_t value) noexcept {
    for(std::size_t k=0u;k<4u;++k)
        bytes[at+k]=static_cast<std::uint8_t>(value>>(k*8u));
}

// Source x87 routines do not call original engine virtual methods,
// but do update multiple fields conditionally. Enforce all required
// endpoint bounds BEFORE even checking values. The unsupported NaN
// case is fail-closed, with no modification to the host-owned object.
inline Status ApplySourceOriginalEffect(
    std::size_t table,std::size_t slot,std::uint32_t sourceVA,
    const std::array<std::uint8_t,16>& originalProxyData,
    std::vector<std::uint8_t>& objectBytes) noexcept {
    if(!IsAttestedAssignment(table,slot,sourceVA))
        return Status::UnrecognizedSource;
    const auto needed=sourceVA==kNativePlasmaScaleVA?
        kSourcePlasmaTransition+4u:kSourceScaleTarget+4u;
    if(objectBytes.size()<needed)
        return Status::RefusedBadOwnedMemory;
    const auto newBits=ReadDword(
        originalProxyData.data()+kSourceDecodedValueOffset);
    const auto currentBits=ReadOwnedDword(objectBytes,kSourceScaleCurrent);
    if(IsBinary32NaN(newBits) || IsBinary32NaN(currentBits))
        return Status::RefusedUnverifiedNaNBehavior;

    if(sourceVA==kNativePlasmaScaleVA) {
        // Original FUCOMPP: JNP to return on ordered equality.
        // On change, the old target +0x49C becomes saved +0x4A0,
        // new scale is written to both +0x4A4 and +0x484.
        if(!OriginalOrderedFloatEqual(newBits,currentBits)) {
            const auto oldTarget=ReadOwnedDword(objectBytes,kSourceScaleTarget);
            WriteOwnedDword(objectBytes,kSourcePlasmaTransition,newBits);
            WriteOwnedDword(objectBytes,kSourceScaleCurrent,newBits);
            WriteOwnedDword(objectBytes,kSourcePlasmaSavedTarget,oldTarget);
        }
        return Status::Applied;
    }

    // FireSmoke: on change, an additional ordered comparison against
    // +0x49C decides whether to transfer +0x494 into +0x498 and update
    // +0x49C to the new value. The original final FSTP ALWAYS writes
    // new bits to +0x484, including on ordered +0/-0 equality.
    if(!OriginalOrderedFloatEqual(newBits,currentBits)) {
        const auto targetBits=ReadOwnedDword(objectBytes,kSourceScaleTarget);
        if(IsBinary32NaN(targetBits))
            return Status::RefusedUnverifiedNaNBehavior;
        if(!OriginalOrderedFloatEqual(newBits,targetBits)) {
            const auto previous=ReadOwnedDword(
                objectBytes,kSourceFireSmokePrevious);
            WriteOwnedDword(objectBytes,kSourceScaleTarget,newBits);
            WriteOwnedDword(objectBytes,kSourceFireSmokeSaved,previous);
        }
    }
    WriteOwnedDword(objectBytes,kSourceScaleCurrent,newBits);
    return Status::Applied;
}
inline constexpr bool CanPublishRetailProxyPointers() noexcept {return false;}
static_assert(SourceRoutesMatch());
static_assert(IsAttestedAssignment(94u,2u,kNativePlasmaScaleVA));
static_assert(IsAttestedAssignment(111u,2u,kNativeFireSmokeScaleVA));
static_assert(!IsAttestedAssignment(94u,3u,kNativePlasmaScaleVA));
static_assert(OriginalOrderedFloatEqual(0x00000000u,0x80000000u));
static_assert(!OriginalOrderedFloatEqual(0x3f800000u,0x40000000u));
static_assert(IsBinary32NaN(0x7FC00001u));
static_assert(IsBinary32NaN(0x7FA00001u));
static_assert(!IsBinary32NaN(0x7F800000u));
static_assert(!IsBinary32NaN(0xFF800000u));
static_assert(!CanPublishRetailProxyPointers());
} // namespace sin_compat::original_client_custom_scale_transition_proxies
