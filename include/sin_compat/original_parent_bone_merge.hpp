#pragma once
// Independently authored C++17 reconstruction of original SiN 2006
// server.dll 0x220ED290 and studio bone name resolver 0x220FFB30.
// No 2006 engine or CStudioHdr C++ pointer/ABI access is performed here.
#include "sin_compat/original_skeletal_setup.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace sin_compat::jessica {
struct OriginalStudioBoneDescriptor {
    std::string name;
    std::int32_t parent_index=-1;
};
struct OriginalStudioBoneTable {
    std::vector<OriginalStudioBoneDescriptor> bones;
    // Signedness of original byte index is irrelevant for <= 256 bones.
    // Original studiohdr +0x16C stores a byte offset to sorted indices.
    std::vector<std::uint8_t> sorted_name_indices;
};
struct OriginalBoneQuaternion {float x=0,y=0,z=0,w=1;};
struct OriginalBoneLocalPose {TossVec3 position{};OriginalBoneQuaternion orientation{};};

// Extract just the verified bone-name/parent metadata from a *copied*
// studiohdr byte span, never from a live PE32 object or untrusted pointer.
// Original: count +0x9C, bone records at +0xA0 (stride 216),
// bone name relative offset +0, parent index +4, sorted byte indices +0x16C.
std::optional<OriginalStudioBoneTable> parse_original_studio_bones(
    const std::vector<std::uint8_t>& studio_bytes) noexcept;
std::optional<std::uint32_t> find_original_bone_by_name(
    const OriginalStudioBoneTable& parent,const std::string& child_name) noexcept;
std::optional<WorldMatrix3x4> original_quaternion_bone_matrix(
    const OriginalBoneLocalPose& pose) noexcept;

struct OriginalParentBoneMergeInput {
    OriginalStudioBoneTable child_studio, parent_studio;
    std::vector<OriginalBoneLocalPose> child_local_poses;
    // Same format verified in Phase 111: negative bone remap -> not cached.
    std::vector<std::uint8_t> parent_bone_cache_bytes;
    TossVec3 child_world_origin{},child_world_angles{};
};
enum class OriginalParentBoneMergeCode:std::uint8_t {
    Merged,InvalidModel,InvalidPose,InvalidParentCache,
    InvalidHierarchy,InvalidMatrix,HostRejected
};
struct OriginalParentBoneMergeResult {
    OriginalParentBoneMergeCode code=OriginalParentBoneMergeCode::HostRejected;
    std::vector<WorldMatrix3x4> matrices;
    std::size_t reused_parent_bones=0,locally_composed_bones=0;
    bool successful()const noexcept {
        return code==OriginalParentBoneMergeCode::Merged;
    }
};
OriginalParentBoneMergeResult merge_original_parent_bones(
    const OriginalParentBoneMergeInput& input) noexcept;

// Phase 113's one stage (compose_parented_bones) can now be resolved
// by portable matrices while remaining setup callbacks remain native-host
// controlled. The host must provide an already prepared parent cache.
class IOriginalParentBoneMergeSource {
public:
    virtual ~IOriginalParentBoneMergeSource()=default;
    virtual std::optional<OriginalParentBoneMergeInput> merge_inputs(
        WorldEntityId child,WorldEntityId parent,std::uint32_t mask)=0;
};
class OriginalParentBoneMergeSkeletalAdapter final:public IOriginalSkeletalSetupHost {
public:
    OriginalParentBoneMergeSkeletalAdapter(IOriginalSkeletalSetupHost& source,
        IOriginalParentBoneMergeSource& merge):source_(source),merge_(merge){}
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
    IOriginalParentBoneMergeSource& merge_;
    std::map<WorldEntityId,std::vector<WorldMatrix3x4>> merged_;
};
} // namespace sin_compat::jessica
