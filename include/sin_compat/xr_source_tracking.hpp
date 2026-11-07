#pragma once

// Engine-independent tracking-space -> Source room-space conversion.
// This math supports the standard RH +Y-up/-Z-forward tracking convention
// used by OpenXR and OpenVR. Source uses +X forward, +Y left and +Z up.
// Source-unit scaling is a caller-provided verified runtime calibration.
// No OpenXR/SteamVR headers, GPU API, SourceVR vtables or game assets here.
//
// The user's separate Apache-2.0 SiN VR Windows mod archive was inspected
// as a behavior/provenance reference. This implementation is new math-only
// code; it does not copy or relicense the mod's injection/DXVK sources.
// See docs/QUEST_XR_TRACKING_COORDINATES_NOV07.md.

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>

namespace sin_compat::xr_tracking {

struct Vector3 {
    double x = 0.0, y = 0.0, z = 0.0;
};

struct SourceAngles {
    double pitch_degrees = 0.0;
    double yaw_degrees = 0.0;
    double roll_degrees = 0.0;
};

// Row-major tracking-world transform. The first 3 columns rotate a local
// vector; the fourth is translation in tracking-space metres.
// OpenXR XrPosef is position + quaternion: the runtime host must obtain
// its rotation matrix with the proper row/column convention first.
struct TrackingMatrix34 {
    std::array<std::array<double, 4>, 3> rows{};
};

struct SourceRoomPose {
    Vector3 position;
    SourceAngles angles;
};

inline constexpr double radians_to_degrees = 57.295779513082320876798154814105;

inline bool finite_positive(double scale) noexcept {
    return std::isfinite(scale) && scale > 0.0;
}

// Reject invalid, sheared, mirrored and badly non-orthogonal transforms.
// Tolerance allows normal float quaternion-to-matrix round-off. Rejecting
// malformed host poses rather than feeding them to game camera state is a
// NEW port safety choice, not a recovered 2006 game behavior.
inline bool valid_tracking_matrix(
    const TrackingMatrix34& pose, double tolerance = 2.0e-3) noexcept {
    if (!std::isfinite(tolerance) || tolerance <= 0.0 || tolerance >= 0.1)
        return false;
    for (const auto& row : pose.rows)
        for (double v : row)
            if (!std::isfinite(v)) return false;

    for (std::size_t c = 0; c < 3; ++c) {
        double norm = 0.0;
        for (std::size_t r = 0; r < 3; ++r)
            norm += pose.rows[r][c] * pose.rows[r][c];
        if (!std::isfinite(norm) || std::abs(norm - 1.0) > tolerance)
            return false;
        for (std::size_t other = c + 1; other < 3; ++other) {
            double dot = 0.0;
            for (std::size_t r = 0; r < 3; ++r)
                dot += pose.rows[r][c] * pose.rows[r][other];
            if (!std::isfinite(dot) || std::abs(dot) > tolerance)
                return false;
        }
    }
    const auto& m = pose.rows;
    const double det =
         m[0][0] * (m[1][1]*m[2][2] - m[1][2]*m[2][1])
       - m[0][1] * (m[1][0]*m[2][2] - m[1][2]*m[2][0])
       + m[0][2] * (m[1][0]*m[2][1] - m[1][1]*m[2][0]);
    return std::isfinite(det) && std::abs(det - 1.0) <= 3.0*tolerance;
}

inline std::optional<Vector3> tracking_vector_to_source(
    const Vector3& source_tracking_vector,
    double source_units_per_metre) noexcept {
    if (!finite_positive(source_units_per_metre) ||
        !std::isfinite(source_tracking_vector.x) ||
        !std::isfinite(source_tracking_vector.y) ||
        !std::isfinite(source_tracking_vector.z)) return std::nullopt;
    // Tracking +X right, +Y up, +Z backward ->
    // Source +X forward, +Y left, +Z up.
    const Vector3 out{-source_tracking_vector.z * source_units_per_metre,
                      -source_tracking_vector.x * source_units_per_metre,
                       source_tracking_vector.y * source_units_per_metre};
    if (!std::isfinite(out.x) || !std::isfinite(out.y) ||
        !std::isfinite(out.z)) return std::nullopt;
    return out;
}

inline std::optional<SourceRoomPose> tracking_pose_to_source(
    const TrackingMatrix34& tracking, double source_units_per_metre) noexcept {
    if (!finite_positive(source_units_per_metre) ||
        !valid_tracking_matrix(tracking)) return std::nullopt;

    const auto position = tracking_vector_to_source(
        Vector3{tracking.rows[0][3],
                tracking.rows[1][3],
                tracking.rows[2][3]}, source_units_per_metre);
    if (!position) return std::nullopt;

    // The local HMD forward axis is -Z in tracking space. The equivalent
    // Source-angle convention has pitch positive down and yaw about +Z.
    const double pitch_arg = tracking.rows[1][2];
    const double bounded_pitch = std::fmax(-1.0, std::fmin(1.0, pitch_arg));
    const SourceAngles angles{
        std::asin(bounded_pitch) * radians_to_degrees,
        std::atan2(tracking.rows[0][2], tracking.rows[2][2]) * radians_to_degrees,
        std::atan2(-tracking.rows[1][0], tracking.rows[1][1]) * radians_to_degrees
    };
    if (!std::isfinite(angles.pitch_degrees) ||
        !std::isfinite(angles.yaw_degrees) ||
        !std::isfinite(angles.roll_degrees)) return std::nullopt;
    return SourceRoomPose{*position, angles};
}

} // namespace sin_compat::xr_tracking
