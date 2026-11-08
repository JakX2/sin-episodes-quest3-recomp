#include "sin_compat/original_jessica_impulse.hpp"
#include <cmath>
#include <limits>

namespace sin_compat::jessica {
namespace {
bool finite(const TossVec3& v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
bool zero(const TossVec3& v) noexcept {
    return v.x == 0.0f && v.y == 0.0f && v.z == 0.0f;
}
bool add(float a, float b, float& out) noexcept {
    const double sum=static_cast<double>(a)+static_cast<double>(b);
    if (!std::isfinite(sum) ||
        sum > std::numeric_limits<float>::max() ||
        sum < -std::numeric_limits<float>::max()) return false;
    out=static_cast<float>(sum);
    return std::isfinite(out);
}
} // namespace

ImpulseCode apply_original_velocity_impulse(IOriginalImpulseHost& host,
                                            ImpulseEntity entity,
                                            const TossVec3& impulse) noexcept {
    if (!finite(impulse)) return ImpulseCode::InvalidImpulse;
    // The x86 implementation compares the input to a shared sentinel
    // vector and exits before touching either the physics or NPC path.
    if (zero(impulse)) return ImpulseCode::IgnoredZero;
    try {
        const auto type=host.movement_type(entity);
        if (!type) return ImpulseCode::HostRejected;
        if (*type == 6) {
            return host.add_physics_velocity(entity,impulse)
                ? ImpulseCode::AppliedPhysics : ImpulseCode::HostRejected;
        }
        const auto dirty=host.absolute_velocity_dirty(entity);
        if (!dirty) return ImpulseCode::HostRejected;
        if (*dirty && !host.refresh_absolute_velocity(entity))
            return ImpulseCode::HostRejected;
        const auto current=host.read_absolute_velocity(entity);
        if (!current || !finite(*current)) return ImpulseCode::HostRejected;
        TossVec3 result;
        if (!add(current->x,impulse.x,result.x) ||
            !add(current->y,impulse.y,result.y) ||
            !add(current->z,impulse.z,result.z))
            return ImpulseCode::InvalidImpulse;
        return host.set_absolute_velocity(entity,result)
            ? ImpulseCode::AppliedNonPhysics : ImpulseCode::HostRejected;
    } catch (...) {
        return ImpulseCode::HostRejected;
    }
}
} // namespace sin_compat::jessica
