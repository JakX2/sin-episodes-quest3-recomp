#pragma once
// Independently authored value-owned model of the 2006 SiN server.dll
// recursive bone world-matrix evaluator 0x22100F30.
#include "sin_compat/original_parent_bone_merge.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace sin_compat::jessica {
struct OriginalPoseBoneMatrixInput {
    std::vector<std::int32_t> parent_bones;
    std::vector<OriginalBoneLocalPose> local_poses;
    WorldMatrix3x4 root_world{};
    std::uint32_t requested_bone=0;
    // Owner-supplied bitset and previously computed world matrices.
    // Both arrays are omitted for a fresh calculation.
    std::vector<std::uint32_t> computed_words;
    std::vector<std::optional<WorldMatrix3x4>> previous_matrices;
};
enum class OriginalPoseBoneMatrixCode : std::uint8_t {
    Built, AlreadyComputed, InvalidModel, InvalidIndex, InvalidHierarchy,
    InvalidPose, InvalidMatrix, InvalidSnapshot, HostRejected
};
struct OriginalPoseBoneMatrixResult {
    OriginalPoseBoneMatrixCode code=OriginalPoseBoneMatrixCode::HostRejected;
    std::vector<std::optional<WorldMatrix3x4>> matrices;
    std::vector<std::uint32_t> computed_words;
    std::size_t newly_built=0;
    bool successful() const noexcept {
        return code==OriginalPoseBoneMatrixCode::Built ||
               code==OriginalPoseBoneMatrixCode::AlreadyComputed;
    }
};
// Original: bitset [index >> 5] and (1u << (index & 31)), local
// QuaternionMatrix, read parent at studio bone record +4 (stride 216),
// recurse until parent == -1, then ConcatenateTransforms and mark computed.
// Host must provide already sampled local bone poses and root matrix.
OriginalPoseBoneMatrixResult resolve_original_pose_bone_matrix(
    const OriginalPoseBoneMatrixInput& input) noexcept;
}
