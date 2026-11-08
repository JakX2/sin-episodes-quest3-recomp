#pragma once
// Source-observed 2006 SiN server.dll 0x2213FA60 behavioral model.
// This is not the original game code or a native SourceVR entity ABI.
#include "sin_compat/original_absolute_velocity.hpp"
#include <cstdint>
#include <optional>

namespace sin_compat::jessica {
using WorldEntityId = AbsoluteEntityId;
using WorldMatrix3x4 = AbsoluteMatrix3x4;

struct WorldTransformSnapshot {
    TossVec3 local_origin{}, local_angles{};       // original +0x360, +0x36c
    TossVec3 world_origin{}, world_angles{};       // original +0x2bc, +0x354
    WorldMatrix3x4 world_matrix{};                 // original +0x268
    std::optional<WorldEntityId> parent{};         // original EHANDLE +0xf0
    std::uint8_t parent_attachment = 0;            // original +0xec
    bool dirty = false;                            // original +0xb8 bit 0x800
};

enum class WorldRefreshCode : std::uint8_t {
    Unchanged, Refreshed, MissingEntity, MissingParent,
    ParentCycle, DepthLimit, InvalidTransform, HostRejected
};
struct WorldRefreshResult {
    WorldRefreshCode code = WorldRefreshCode::HostRejected;
    TossVec3 world_origin{}, world_angles{};
    bool successful() const noexcept {
        return code == WorldRefreshCode::Unchanged ||
               code == WorldRefreshCode::Refreshed;
    }
};

class IOriginalWorldTransformHost {
public:
    virtual ~IOriginalWorldTransformHost() = default;
    virtual std::optional<WorldTransformSnapshot> snapshot(WorldEntityId id) = 0;
    // Model of 0x2213D960's attachment-based parent matrix selection.
    // std::nullopt means the original-style fallback to parent world matrix;
    // it never implies that the parent entity itself is absent.
    virtual std::optional<WorldMatrix3x4> attachment_parent_matrix(
        WorldEntityId child, WorldEntityId parent, std::uint8_t attachment) = 0;
    // The native host owns cache writes, dirty-bit clearing, potential
    // side-effects of original 0x2221DFA0/0x22221DB0 and EHANDLE lifetime.
    virtual bool commit_world_transform(WorldEntityId id,
                                        const WorldTransformSnapshot& updated) = 0;
    virtual bool on_world_transform_rebuilt(WorldEntityId id) = 0;
};

// Original 0x2226CC20 angles+origin -> matrix; reconstructed independently.
// Angles are Source degrees {pitch, yaw, roll}. Matrix is row-major 3x4.
std::optional<WorldMatrix3x4> make_local_angle_matrix(
    const TossVec3& local_angles, const TossVec3& local_origin) noexcept;
std::optional<WorldMatrix3x4> compose_world_matrices(
    const WorldMatrix3x4& parent, const WorldMatrix3x4& local) noexcept;
std::optional<TossVec3> angles_from_world_matrix(
    const WorldMatrix3x4& world) noexcept;
WorldRefreshResult refresh_original_world_transform(
    IOriginalWorldTransformHost& host, WorldEntityId id) noexcept;
} // namespace sin_compat::jessica
