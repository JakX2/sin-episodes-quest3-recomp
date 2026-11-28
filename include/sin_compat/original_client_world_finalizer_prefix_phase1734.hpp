#pragma once
// Phase1734: specific zero-allocated original 2006 PE32 CWorld finalizer
// prefix, 357 of 414 source instruction bytes (0x240C9F40..0x240CA0A5).
// PRECONDITIONS: SHA-pinned CWorld Phase1732 inert owner, two typed vector
// records from Phase1731, correct synthetic owner token, and no x87-dirty
// setter branch for the supplied original BSS Vec3 inputs. This is NOT a
// real runtime CWorld and CANNOT invoke 2006 game/engine vtables.
//
// Source trace: two 210B interpolator resets use ESI (owner pointer);
// +E8/+F0 virtuals use 101B source Vec3 setters (no-change branches);
// 0x24094240 is 11B MOV owner+0x2E0,0; 0x2411B060 and 0x2411B110
// take original zero-equality early returns; original 0x240C90B0
// returns without engine callbacks because owner+0x60==0 and the
// handle WORDs +0xB8/+0x136 are FFFF; 0x241858A0 gets false/zero
// and clears a flag at +0x143. The first unported engine-global
// DEREFERENCE is at 0x240CA0A5, of the original 0x242FA458 state.
#include <array>
#include <cstddef>
#include <cstdint>
#include "sin_compat/original_client_world_pre_finalizer_phase1732.hpp"
#include "sin_compat/original_client_world_vec3_setters_phase1733.hpp"

namespace sin_compat::original_client_world_finalizer_prefix_phase1734 {
namespace pre=sin_compat::original_client_world_pre_finalizer_phase1732;
namespace v=sin_compat::original_client_world_vec3_setters_phase1733;
namespace vec=sin_compat::original_client_world_two_vector_entries_phase1731;
inline constexpr std::size_t kOwnerBytes=1200u;
inline constexpr std::uint32_t kFinalizerVA=0x240C9F40u;
inline constexpr std::uint32_t kBlockedEngineReadVA=0x240CA0A5u;
inline constexpr std::uint32_t kBlockedEngineGlobalVA=0x242FA458u;
inline constexpr std::size_t kNativeFinalizerBytes=414u;
inline constexpr std::size_t kAuthenticatedPrefixBytes=357u;
inline constexpr std::uint32_t kSourceInterpolatedSubobjectVtable=0x24288E3Cu;
inline constexpr std::size_t kInterpolatedSubobjectOffset=0x16Cu;

enum class Status:std::uint8_t {
    ProducedInertPrefixBeforeEngineGlobalRead,
    RefusedNull,
    RefusedShort,
    RefusedInvalidOwnerToken,
    RefusedOriginalVectorMetadataMismatch,
    RefusedUnprovenPriorSourceState,
    RefusedRequiresOriginalDirtyPropagation,
    RefusedAlreadyApplied
};
struct Result final {
    Status status;
    std::uint32_t nextBlockedInstructionVA;
    std::uint32_t nextBlockedEngineGlobalVA;
};
inline bool ReadAllZero32(const std::uint8_t* owner,std::size_t offset,
                          std::size_t count) noexcept {
    for(std::size_t i=0u;i<count;++i)
        if(v::Load32LE(owner+offset+4u*i)!=0u)return false;
    return true;
}
inline Result ExtendZeroedCWorldFinalizerPrefix(
    std::uint8_t* owner,std::size_t bytes,
    const vec::HostOwnedSidecar& sourceVectorSidecar,
    std::uint32_t syntheticOwnerToken,
    const v::Vec3Bits& firstGlobalVec,
    const v::Vec3Bits& secondGlobalVec) noexcept {
    const auto refuse=[](Status status) noexcept {
        return Result{status,0u,0u};
    };
    if(owner==nullptr)return refuse(Status::RefusedNull);
    if(bytes<kOwnerBytes)return refuse(Status::RefusedShort);
    if(!vec::SafeSyntheticToken(syntheticOwnerToken))
        return refuse(Status::RefusedInvalidOwnerToken);
    if(sourceVectorSidecar.state!=vec::HostOwnedSidecar::State::Initialized ||
       sourceVectorSidecar.words[0][0]!=2u ||
       sourceVectorSidecar.words[0][1]!=syntheticOwnerToken+0x220u ||
       sourceVectorSidecar.words[0][2]!=syntheticOwnerToken+0x22Cu ||
       sourceVectorSidecar.words[1][0]!=2u ||
       sourceVectorSidecar.words[1][1]!=syntheticOwnerToken+0x25Cu ||
       sourceVectorSidecar.words[1][2]!=syntheticOwnerToken+0x268u ||
       v::Load32LE(owner+0x14u)!=3u ||
       v::Load32LE(owner+0x1Cu)!=2u)
        return refuse(Status::RefusedOriginalVectorMetadataMismatch);
    if(owner[0x119u]!=0u)
        return refuse(Status::RefusedAlreadyApplied);
    const auto* sub=owner+kInterpolatedSubobjectOffset;
    if(v::Load32LE(sub)!=kSourceInterpolatedSubobjectVtable ||
       v::Load32LE(sub+4u)!=0u ||
       v::Load32LE(sub+0x20u)!=0u ||
       v::Load32LE(sub+0x30u)!=0u ||
       sub[0x24u]!=0u || sub[0x25u]!=0u ||
       v::Load32LE(owner+0x45Cu)!=0xFFFFFFFFu ||
       v::Load32LE(owner+0x478u)!=0u ||
       v::Load32LE(owner+0x50u)!=0u ||
       v::Load32LE(owner+0x60u)!=0u ||
       v::Load32LE(owner+0x2E0u)!=0u ||
       v::Load32LE(owner+0xB4u)!=0xFFFFFFFFu)
        return refuse(Status::RefusedUnprovenPriorSourceState);
    // Both virtual Vec3 setter calls must follow ORIGINAL x87 no-write
    // branches. Otherwise original 0x24096290 has to invalidate entity
    // and parent/child state: do not fake it or partially mutate owner.
    if(!v::X87OrderedVec3Equal(v::ReadExisting(owner,v::Channel::First),
                                firstGlobalVec) ||
       !v::X87OrderedVec3Equal(v::ReadExisting(owner,v::Channel::Second),
                                secondGlobalVec))
        return refuse(Status::RefusedRequiresOriginalDirtyPropagation);
    // Source first two calls to 0x2411AD50: owner pointer token at +4,
    // cleared interpolation fields, duplicate original BSS first Vec3.
    auto* const interpolator=owner+kInterpolatedSubobjectOffset;
    v::Store32LE(interpolator+4u,syntheticOwnerToken);
    constexpr std::array<std::size_t,13> interpZeroes{{
        0x08u,0x0Cu,0x10u,0x14u,0x18u,0x1Cu,0x2Cu,
        0x34u,0x38u,0x3Cu,0x40u,0x44u,0x48u
    }};
    for(const auto offset:interpZeroes)v::Store32LE(interpolator+offset,0u);
    for(std::size_t repeat=0;repeat<2u;++repeat)
        for(std::size_t i=0;i<3u;++i)
            v::Store32LE(interpolator+0x4Cu+12u*repeat+4u*i,
                         firstGlobalVec.bits[i]);
    // Deterministic writes around both nested resets.
    owner[0x119u]=1u;
    v::Store32LE(owner+0xB4u,0xFFFFFFFFu);
    for(const auto off:std::array<std::size_t,3>{{0x134u,0x136u,0xB8u}}) {
        owner[off]=0xFFu;owner[off+1u]=0xFFu;
    }
    v::Store32LE(owner+0x13Cu,0x0000FFFFu);
    v::Store32LE(owner+0x478u,0xFFFFFFFFu);
    v::Store32LE(owner+0x50u,0xFFFFFFFFu);
    // Original virtual setters +0xE8/+0xF0: ordered-equal, no source
    // write and no dirty propagation. The preflight above proves this.
    const auto a=v::ApplyInertSourceVec3Setter(owner,bytes,v::Channel::First,
                                               firstGlobalVec);
    const auto b=v::ApplyInertSourceVec3Setter(owner,bytes,v::Channel::Second,
                                               secondGlobalVec);
    // Both are guaranteed original no-write cases by the preflight.
    (void)a;(void)b;
    constexpr std::array<std::size_t,29> zeroDwords{{
        0x60u,0x1F0u,0x1F4u,0x1F8u,0x1FCu,0x200u,0x204u,
        0xECu,0xF0u,0xF4u,0x2E0u,0xE0u,0xE4u,0xE8u,
        0x124u,0x128u,0x12Cu,0x8Cu,0x64u,0x6Cu,
        0x144u,0x148u,0x78u,0x140u,0x7Cu,0x80u,
        0x54u,0x1ECu,0x130u
    }};
    for(const auto offset:zeroDwords)
        v::Store32LE(owner+offset,0u);
    // Native first virtual method through owner+4 returns zero because
    // owner+0x60 has just been cleared. Its +0xB8/+0x136 handle paths
    // both skip because they are FFFF. The two interpolator setters
    // +0x20/+0x24 are no-ops on verified zero source fields.
    // Source conditional source check sets owner+0x5C to FFFFFFFF.
    v::Store32LE(owner+0x5Cu,0xFFFFFFFFu);
    // Native 0x241858A0 receives zero argument: AND byte+0x143,0xFE.
    owner[0x143u]=static_cast<std::uint8_t>(owner[0x143u]&0xFEu);
    v::Store32LE(owner+0x1D8u,0xFFFFFFFFu);
    return {Status::ProducedInertPrefixBeforeEngineGlobalRead,
            kBlockedEngineReadVA,kBlockedEngineGlobalVA};
}
inline constexpr bool CanPublishRetailWorld() noexcept{return false;}
static_assert(kFinalizerVA+kAuthenticatedPrefixBytes==kBlockedEngineReadVA);
static_assert(kAuthenticatedPrefixBytes<kNativeFinalizerBytes);
static_assert(kInterpolatedSubobjectOffset+0x64u<=kOwnerBytes);
static_assert(!CanPublishRetailWorld());
} // namespace sin_compat::original_client_world_finalizer_prefix_phase1734
