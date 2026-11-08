#pragma once
// Independently authored C++17 behavior model; not Ritual/Valve source or ABI.
// Original owner-provided 2006 SiN server.dll: 0x22141A00 and 0x2213D750.
#include "sin_compat/original_jessica_toss.hpp"
#include <array>
#include <cstdint>
#include <optional>
namespace sin_compat::jessica {
using AbsoluteEntityId=std::uint64_t;
struct AbsoluteMatrix3x4 {
    // Row-major Source transform: translation at indices 3,7,11.
    std::array<float,12> row_major{};
};
struct AbsoluteVelocitySnapshot {
    TossVec3 absolute{}; // +0x250 cached world velocity
    TossVec3 local{}; // +0x2C8 local velocity
    std::optional<AbsoluteEntityId> parent{}; // +0xF0 entity handle
    bool absolute_dirty=false; // +0xB8 bit 12
    bool transform_dirty=false; // +0xB8 bit 11
    AbsoluteMatrix3x4 entity_to_world{}; // +0x268
};
class IOriginalAbsoluteVelocityHost {
public:
    virtual ~IOriginalAbsoluteVelocityHost()=default;
    virtual std::optional<AbsoluteVelocitySnapshot> snapshot(AbsoluteEntityId)=0;
    // Original invalidation entry 0x2214B620 with mask 4.
    virtual bool invalidate_velocity_dependents(AbsoluteEntityId,std::uint32_t)=0;
    virtual bool clear_absolute_dirty(AbsoluteEntityId)=0;
    virtual bool store_absolute_velocity(AbsoluteEntityId,const TossVec3&)=0;
    // Original 0x2213E1A0 can invoke virtual +0x27C on change.
    virtual bool store_local_velocity(AbsoluteEntityId,const TossVec3&)=0;
    virtual bool refresh_world_transform(AbsoluteEntityId)=0;
};
enum class AbsoluteVelocityCode : std::uint8_t {
    Unchanged,Set,Refreshed,InvalidInput,MissingEntity,
    MissingParent,ParentCycle,DepthLimit,HostRejected
};
struct AbsoluteVelocityResult {
    AbsoluteVelocityCode code=AbsoluteVelocityCode::HostRejected;
    TossVec3 value{};
    bool successful() const noexcept {
        return code==AbsoluteVelocityCode::Unchanged ||
               code==AbsoluteVelocityCode::Set ||
               code==AbsoluteVelocityCode::Refreshed;
    }
};
// Host-neutral behavior. Finite guards and cycle/depth limits are added safety
// policy, not claimed as checks present in the original 2006 x87.
AbsoluteVelocityResult set_original_absolute_velocity(
    IOriginalAbsoluteVelocityHost&,AbsoluteEntityId,const TossVec3&) noexcept;
AbsoluteVelocityResult refresh_original_absolute_velocity(
    IOriginalAbsoluteVelocityHost&,AbsoluteEntityId) noexcept;
// Original helper 0x22268C80 uses rotation; 0x22268E90 uses transpose.
// Both ignore translation. They are not generic matrix inversion routines.
std::optional<TossVec3> rotate_local_velocity_to_world(
    const AbsoluteMatrix3x4&,const TossVec3&) noexcept;
std::optional<TossVec3> inverse_rotate_world_velocity(
    const AbsoluteMatrix3x4&,const TossVec3&) noexcept;
} // namespace sin_compat::jessica
