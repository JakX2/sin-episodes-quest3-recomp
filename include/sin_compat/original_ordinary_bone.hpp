#pragma once
// Independent C++17 reconstruction of original 2006 SiN server.dll 0x220FE0C0.
// Operates on host-owned studio metadata and poses, not original x86 pointers.
#include "sin_compat/original_parent_bone_merge.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace sin_compat::jessica {
struct OriginalOrdinaryBoneInput {
    OriginalStudioBoneTable studio;
    std::vector<std::uint32_t> bone_flags;  // studio bone record +0xA0
    std::vector<OriginalBoneLocalPose> local_poses;
    TossVec3 world_origin{},world_angles{};
    std::int32_t requested_bone=-1; // -1 = every bone; other values = ancestor chain
    std::uint32_t bone_mask=0x300;
    // The native constructor skips masked-out bones and leaves their 3x4
    // output slots unchanged. Preserve previously built matrices, if any.
    std::vector<std::optional<WorldMatrix3x4>> prior_world_matrices;
};

enum class OriginalOrdinaryBoneCode:std::uint8_t {
    Built,InvalidStudio,InvalidPose,InvalidHierarchy,MissingParentMatrix,
    InvalidMatrix,HostRejected
};
struct OriginalOrdinaryBoneResult {
    OriginalOrdinaryBoneCode code=OriginalOrdinaryBoneCode::HostRejected;
    std::vector<std::optional<WorldMatrix3x4>> matrices;
    std::size_t built=0,masked_out=0;
    bool successful()const noexcept {return code==OriginalOrdinaryBoneCode::Built;}
};
// Original studiohdr: bone count +0x9c, array relative offset +0xa0,
// 216-byte records, bone flags at record +0xa0. Reuses Phase 114 parser.
std::optional<std::vector<std::uint32_t>> parse_original_studio_bone_flags(
    const std::vector<std::uint8_t>& studio_bytes) noexcept;
OriginalOrdinaryBoneResult build_original_ordinary_bones(
    const OriginalOrdinaryBoneInput& input) noexcept;

class IOriginalOrdinaryBoneSource {
public:
    virtual ~IOriginalOrdinaryBoneSource()=default;
    virtual std::optional<OriginalOrdinaryBoneInput> ordinary_inputs(
        WorldEntityId child,std::uint32_t mask)=0;
};
// Replace only Phase 113's compose_unparented_bones callback. The full
// matrices are returned to the Phase 112 cache update via Phase 113.
class OriginalOrdinaryBoneSkeletalAdapter final:public IOriginalSkeletalSetupHost {
public:
    OriginalOrdinaryBoneSkeletalAdapter(IOriginalSkeletalSetupHost& source,
                                       IOriginalOrdinaryBoneSource& input)
        :source_(source),input_(input){}
    bool lock_model_cache(WorldEntityId id) override;
    bool unlock_model_cache(WorldEntityId id) override;
    std::optional<OriginalSkeletalSetupSnapshot> snapshot(WorldEntityId id) override;
    bool refresh_world_transform(WorldEntityId id) override;
    bool increment_pose_driver_counter(WorldEntityId id) override;
    bool seed_pose_driver(WorldEntityId id,const TossVec3&,const TossVec3&,
                          std::uint32_t mask) override;
    bool evaluate_virtual_base_pose(WorldEntityId id,std::uint32_t mask) override;
    bool blend_pose_driver(WorldEntityId id,std::uint32_t mask) override;
    bool invoke_virtual_pose_hook(WorldEntityId id,std::uint32_t mask) override;
    bool finalize_pose_driver(WorldEntityId id,std::uint32_t mask) override;
    bool parent_is_animating(WorldEntityId child,WorldEntityId parent) override;
    bool prepare_parent_bone_cache(WorldEntityId parent) override;
    bool compose_parented_bones(WorldEntityId child,WorldEntityId parent,
                                std::uint32_t mask) override;
    bool compose_unparented_bones(WorldEntityId child,std::uint32_t mask) override;
    std::optional<std::vector<WorldMatrix3x4>> resulting_bone_matrices(
        WorldEntityId child) override;
private:
    IOriginalSkeletalSetupHost& source_;
    IOriginalOrdinaryBoneSource& input_;
    std::map<WorldEntityId,std::vector<WorldMatrix3x4>> composed_;
};
} // namespace sin_compat::jessica
