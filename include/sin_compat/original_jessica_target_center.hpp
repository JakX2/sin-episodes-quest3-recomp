#pragma once
// Independent C++17 reconstruction of the position returned by the original
// 2006 SiN server.dll player virtual slot +0x214 (0x2214BD40).
// Original DLL code is NOT embedded. This is NOT an engine ABI definition.
#include "sin_compat/original_jessica_toss.hpp"
#include <array>
#include <cstdint>
#include <optional>

namespace sin_compat::jessica {

// Host-provided row-major affine 3x4 entity-to-world matrix. The game must
// supply the actual matrix; do not synthesize it from guessed Source offsets.
struct TargetTransform3x4 {
    std::array<float,12> row_major{};
};

struct PlayerTargetObservation {
    TossVec3 obb_mins{};       // collision property +0x08
    TossVec3 obb_maxs{};       // collision property +0x14
    TossVec3 absolute_origin{};
    TossVec3 absolute_angles{};
    std::int32_t solid_type=0; // collision property +0x20
    std::uint32_t solid_flags=0; // collision property +0x24
    std::optional<TargetTransform3x4> entity_to_world{};
};

enum class TargetCenterCode : std::uint8_t {
    Computed,
    MissingWorldTransform,
    InvalidGeometry
};
enum class TargetCenterBranch : std::uint8_t {
    TranslateOnly,
    EntityMatrix
};
struct TargetCenterResult {
    TargetCenterCode code=TargetCenterCode::InvalidGeometry;
    TargetCenterBranch branch=TargetCenterBranch::TranslateOnly;
    TossVec3 world_center{};
    bool valid() const noexcept { return code==TargetCenterCode::Computed; }
};

// Reconstructs Source-style CBaseEntity::WorldSpaceCenter behavior from
// original instructions (0x2214B020 + 0x2214B0D0). The runtime host owns
// collision snapshots and matrix validity; no DLL vtable pointer is reused.
// Missing/NaN/out-of-range input fails closed (independent safety guard).
TargetCenterResult compute_player_world_collision_center(
    const PlayerTargetObservation& observation) noexcept;

// Combines Jessica's hand attachment position, the original player collision
// center and live sv_gravity into the geometry consumed by solve_toss().
// This is the host-neutral connection to the prior throwing implementation.
std::optional<TossGeometry> build_original_jessica_toss_geometry(
    const TossVec3& right_hand_world,
    const PlayerTargetObservation& player,
    float live_sv_gravity) noexcept;
} // namespace sin_compat::jessica
