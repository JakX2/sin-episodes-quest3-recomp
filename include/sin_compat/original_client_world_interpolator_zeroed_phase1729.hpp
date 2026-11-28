#pragma once
// Phase1729: source-verified zero-allocation path through original CWorld's
// 26-byte embedded constructor 0x2411AEB0 and 210-byte helper 0x2411AD50.
// This models ONLY the original 2006 PE32 CWorld startup's known zeroed
// +0x16C embedded object. It is an INERT x86-format byte snapshot.
//
// During the embedded constructor the original vtable is 0x2429806C;
// its +0x48 method is exactly 0x24119750 (single-byte RET). Thus the
// helper's original virtual calls have no side effects IN THIS PARTICULAR
// source constructor path. The six copied DWORDs come from BSS globals
// 0x24372818..20, whose live values must be supplied by a caller.
// Original .text functions, vtable addresses and globals are NEVER called
// or dereferenced by this portable model. The subsequent original CWorld
// vtable overwrite and all later methods remain separate/unimplemented.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_client_world_interpolator_zeroed_phase1729 {
inline constexpr std::size_t kOwnerBytes=1200u;
inline constexpr std::size_t kSubobjectOffset=0x16Cu;
inline constexpr std::size_t kTouchedSubobjectBytes=0x64u;
inline constexpr std::uint32_t kSourceEmbeddedCtorVA=0x2411AEB0u;
inline constexpr std::uint32_t kSourceNestedInitializerVA=0x2411AD50u;
inline constexpr std::uint32_t kSourceInitialEmbeddedVTableVA=0x2429806Cu;
inline constexpr std::uint32_t kSourceVirtual48VA=0x24119750u;
inline constexpr std::uint32_t kSourceOriginTripleVA=0x24372818u;

struct BssTripleBits final {
    std::array<std::uint32_t,3> words{};
};
enum class Status:std::uint8_t {
    InitializedInertZeroedSubobject,
    RefusedNull,
    RefusedShortAllocation,
    RefusedNotZeroed
};
struct Result final {
    Status status;
    std::uint8_t* sourceSubobjectSnapshot;
};
inline void StoreWordLE(std::uint8_t* dst,std::uint32_t word) noexcept {
    for(std::size_t i=0;i<4u;++i)
        dst[i]=static_cast<std::uint8_t>((word>>(i*8u))&0xFFu);
}
inline Result InitializeZeroedSourceEmbeddedSubobject(
    std::uint8_t* owner,std::size_t bytes,
    const BssTripleBits& sourceBssValues) noexcept {
    if(owner==nullptr) return {Status::RefusedNull,nullptr};
    if(bytes<kOwnerBytes) return {Status::RefusedShortAllocation,nullptr};
    auto* const sub=owner+kSubobjectOffset;
    // The true 2006 world constructor receives a ZEROING allocator. In
    // this specific condition, virtual+0x48 dispatch is known one-byte
    // RET, and zero float/flag tests require no unknown branch behavior.
    for(std::size_t i=0;i<kTouchedSubobjectBytes;++i)
        if(sub[i]!=0u) return {Status::RefusedNotZeroed,nullptr};
    StoreWordLE(sub,kSourceInitialEmbeddedVTableVA);
    sub[0x26u]=0xFFu;
    sub[0x27u]=0xFFu;
    // The nested helper receives argument zero and writes its zero fields
    // (including +0x04, +0x08, +0x0C, +0x10, +0x14, +0x18, +0x1C,
    // +0x2C, +0x34..+0x48). They are already zero by precondition.
    // Its conditional changes at +0x30,+0x24,+0x20,+0x28 are skipped.
    // It duplicates three 32-bit values from the original BSS vector.
    for(std::size_t repeat=0u;repeat<2u;++repeat) {
        for(std::size_t i=0u;i<3u;++i)
            StoreWordLE(sub+0x4Cu+repeat*12u+i*4u,
                        sourceBssValues.words[i]);
    }
    return {Status::InitializedInertZeroedSubobject,sub};
}
inline constexpr bool CanPublishRetailWorld() noexcept { return false; }
static_assert(kSubobjectOffset+kTouchedSubobjectBytes<=kOwnerBytes);
static_assert(kSourceInitialEmbeddedVTableVA+0x48u==0x242980B4u);
static_assert(!CanPublishRetailWorld());
} // namespace sin_compat::original_client_world_interpolator_zeroed_phase1729
