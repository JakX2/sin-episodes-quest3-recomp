#pragma once
// Phase1733: source-authenticated 101-byte CWorld virtual setters,
// vtable byte +0xE8 and +0xF0 in the original 2006 PE32 client.dll.
// This models ONLY the x87 equality decision, source field copy, and
// an optional HOST-RECORDED dirty-state intent. The original source
// calls 0x24096290 before copying a changed Vec3; that function reaches
// original engine vtables, handles, and entity state. It is NOT executed.
// Original x86 vtables, source VAs, and owner tokens are never host pointers.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_client_world_vec3_setters_phase1733 {
inline constexpr std::size_t kOwnerBytes = 1200u;
inline constexpr std::uint32_t kWorldPrimaryVtableVA = 0x242895ECu;
inline constexpr std::uint32_t kWorldFirstSetterVA = 0x240C51B0u;
inline constexpr std::uint32_t kWorldSecondSetterVA = 0x240C5280u;
inline constexpr std::uint32_t kSourceDirtyPropagatorVA = 0x24096290u;
inline constexpr std::size_t kOriginalSetterBytes = 101u;
enum class Channel:std::uint8_t { First, Second };
struct Vec3Bits final {
    std::array<std::uint32_t,3> bits{};
};
constexpr std::size_t OffsetFor(Channel channel) noexcept {
    return channel == Channel::First ? 0x220u : 0x25Cu;
}
constexpr std::uint8_t OriginalDirtyFlag(Channel channel) noexcept {
    return channel == Channel::First ? 1u : 2u;
}
// x87 FUCOMPP with the original TEST AH,0x44 / JP(JNP) sequence:
// signed zeros compare equal; infinities compare by sign; unordered NaNs
// force the changed branch even if both payload DWORDs are identical.
// FP status/exceptions are NOT simulated. This pure comparison avoids
// NaN canonicalization and host floating-point exception side effects.
constexpr bool X87OrderedEqualBits(std::uint32_t a,std::uint32_t b) noexcept {
    constexpr std::uint32_t e=0x7F800000u, m=0x007FFFFFu;
    if(((a&e)==e && (a&m)!=0u) || ((b&e)==e && (b&m)!=0u))
        return false;
    if(a==b) return true;
    return (a&0x7FFFFFFFu)==0u && (b&0x7FFFFFFFu)==0u;
}
constexpr bool X87OrderedVec3Equal(const Vec3Bits& a,const Vec3Bits& b) noexcept {
    for(std::size_t i=0u;i<3u;++i)
        if(!X87OrderedEqualBits(a.bits[i],b.bits[i])) return false;
    return true;
}
inline std::uint32_t Load32LE(const std::uint8_t* src) noexcept {
    return std::uint32_t(src[0]) | (std::uint32_t(src[1]) << 8u) |
           (std::uint32_t(src[2]) << 16u) | (std::uint32_t(src[3]) << 24u);
}
inline void Store32LE(std::uint8_t* dst,std::uint32_t x) noexcept {
    for(std::size_t i=0u;i<4u;++i)
        dst[i]=static_cast<std::uint8_t>((x>>(8u*i))&0xFFu);
}
inline Vec3Bits ReadExisting(const std::uint8_t* owner,Channel channel) noexcept {
    const auto offset=OffsetFor(channel);
    Vec3Bits result{};
    for(std::size_t i=0u;i<3u;++i)
        result.bits[i]=Load32LE(owner+offset+4u*i);
    return result;
}
// This hook is an injected TEST-WITNESS, not an engine callback. True means
// the consumer recorded a *request* to propagate native invalidation.
// It does not mean the real 2006 propagator has been executed.
using HostDirtyIntentRecorder=bool(*)(std::uint8_t sourceFlag,
                                      void* user) noexcept;
enum class Status:std::uint8_t {
    OriginalOrderedEqualNoWrite,
    ChangedInertSnapshotAndRecordedDirtyIntent,
    RefusedNull,
    RefusedShort,
    RefusedDirtyPropagationNotPorted,
    RefusedDirtyIntent
};
struct Result final {
    Status status;
    std::uint8_t sourceDirtyFlag;
};
inline Result ApplyInertSourceVec3Setter(
    std::uint8_t* owner,std::size_t bytes,Channel channel,
    const Vec3Bits& newValue,HostDirtyIntentRecorder recorder=nullptr,
    void* user=nullptr) noexcept {
    if(owner==nullptr) return {Status::RefusedNull,0u};
    if(bytes<kOwnerBytes) return {Status::RefusedShort,0u};
    const auto old=ReadExisting(owner,channel);
    if(X87OrderedVec3Equal(old,newValue))
        return {Status::OriginalOrderedEqualNoWrite,0u};
    const auto flag=OriginalDirtyFlag(channel);
    if(recorder==nullptr)
        return {Status::RefusedDirtyPropagationNotPorted,flag};
    if(!recorder(flag,user)) return {Status::RefusedDirtyIntent,flag};
    const auto off=OffsetFor(channel);
    for(std::size_t i=0u;i<3u;++i)
        Store32LE(owner+off+4u*i,newValue.bits[i]);
    return {Status::ChangedInertSnapshotAndRecordedDirtyIntent,flag};
}
inline constexpr bool CanPublishRetailWorld() noexcept { return false; }
static_assert(OffsetFor(Channel::First)+12u<=kOwnerBytes);
static_assert(OffsetFor(Channel::Second)+12u<=kOwnerBytes);
static_assert(X87OrderedEqualBits(0u,0x80000000u));
static_assert(!X87OrderedEqualBits(0x7FC00001u,0x7FC00001u));
static_assert(X87OrderedEqualBits(0x7F800000u,0x7F800000u));
static_assert(!CanPublishRetailWorld());
} // namespace sin_compat::original_client_world_vec3_setters_phase1733
