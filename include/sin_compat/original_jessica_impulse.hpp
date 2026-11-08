#pragma once
// Independent behavioral reconstruction of CBaseEntity velocity impulse path
// observed at original SiN Episodes 2006 server.dll VA 0x2214BA20.
// NOT an ABI mapping to a real Source/SourceVR/Android entity.
#include "sin_compat/original_jessica_toss.hpp"
#include <cstdint>
#include <optional>

namespace sin_compat::jessica {
using ImpulseEntity = std::uint64_t;

enum class ImpulseCode : std::uint8_t {
    IgnoredZero, AppliedPhysics, AppliedNonPhysics, InvalidImpulse, HostRejected
};

class IOriginalImpulseHost {
public:
    virtual ~IOriginalImpulseHost() = default;
    // Original +0x204 is compared with numeric motion type 6.
    virtual std::optional<std::int32_t> movement_type(ImpulseEntity) = 0;
    // For type 6 the entity's +0x218 physics interface receives a linear
    // velocity vector and a null angular velocity via vtable byte +0xB0.
    virtual bool add_physics_velocity(ImpulseEntity, const TossVec3& linear) = 0;

    // Non-physics path checks entity flags +0xB8, bit 12; when set,
    // calls 0x2213D750 before reading its absolute velocity fields.
    virtual std::optional<bool> absolute_velocity_dirty(ImpulseEntity) = 0;
    virtual bool refresh_absolute_velocity(ImpulseEntity) = 0;
    // +0x250, +0x254, +0x258: existing absolute velocity components.
    virtual std::optional<TossVec3> read_absolute_velocity(ImpulseEntity) = 0;
    // Original 0x22141A00 may perform extra entity/engine side effects.
    virtual bool set_absolute_velocity(ImpulseEntity, const TossVec3&) = 0;
};

// Original impulse argument is an ADDITION, not replacement. This portable
// contract guards NaN/Infinity, overflow, missing entities and failed host
// callbacks; those safety decisions were NOT recovered from original x87.
// In the original, an all-zero sentinel is compared before either path.
ImpulseCode apply_original_velocity_impulse(
    IOriginalImpulseHost& host, ImpulseEntity entity,
    const TossVec3& impulse) noexcept;
} // namespace sin_compat::jessica
