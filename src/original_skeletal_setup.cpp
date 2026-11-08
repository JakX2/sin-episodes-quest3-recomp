#include "sin_compat/original_skeletal_setup.hpp"
#include <cmath>
#include <limits>
#include <utility>

namespace sin_compat::jessica {
namespace {
using Code=OriginalSkeletalSetupCode;
bool finite(const TossVec3& x) noexcept {
    return std::isfinite(x.x)&&std::isfinite(x.y)&&std::isfinite(x.z);
}
bool finite(const WorldMatrix3x4& m) noexcept {
    for(float x:m.row_major)if(!std::isfinite(x))return false;
    return true;
}
OriginalSkeletalSetupResult execute(IOriginalSkeletalSetupHost& host,
                                    WorldEntityId id,std::uint32_t mask) {
    auto s=host.snapshot(id);
    if(!s)return {Code::MissingEntity,{}};
    // The original also enters its pose path without checking the studio
    // header pointer in every branch. Here we reject that invalid host
    // state rather than risking native memory corruption.
    if(!s->valid_studio)return {Code::MissingStudio,{}};
    if(s->world_transform_dirty) {
        if(!host.refresh_world_transform(id))return {Code::SetupRejected,{}};
        s=host.snapshot(id);
        if(!s || s->world_transform_dirty)return {Code::InvalidState,{}};
    }
    if(!finite(s->world_origin)||!finite(s->world_angles)||
       !std::isfinite(s->vertical_offset))return {Code::InvalidState,{}};
    const double z=double(s->world_origin.z)+double(s->vertical_offset);
    if(!std::isfinite(z)||std::abs(z)>std::numeric_limits<float>::max())
        return {Code::InvalidState,{}};
    TossVec3 posed_origin=s->world_origin;
    posed_origin.z=static_cast<float>(z);
    if(s->has_pose_driver) {
        // x86 +0x3DC counter increments before 0x22106840 and the first
        // virtual +0x2AC. Native host remains responsible for exact pose
        // registers/overlay arrays and curtime.
        if(!host.increment_pose_driver_counter(id))return {Code::SetupRejected,{}};
        const auto updated=host.snapshot(id);
        if(!updated)return {Code::MissingEntity,{}};
        if(updated->world_transform_dirty &&
           !host.refresh_world_transform(id))return {Code::SetupRejected,{}};
        if(!host.seed_pose_driver(id,posed_origin,s->world_angles,mask))
            return {Code::SetupRejected,{}};
    }
    if(!host.evaluate_virtual_base_pose(id,mask))return {Code::SetupRejected,{}};
    if(s->has_pose_driver) {
        if(!host.blend_pose_driver(id,mask) ||
           !host.invoke_virtual_pose_hook(id,mask) ||
           !host.finalize_pose_driver(id,mask))
            return {Code::SetupRejected,{}};
    }
    // Bone merge can be skipped by invalid EHANDLE, nonanimating parent,
    // missing parent cache or absence of a parent altogether.
    bool use_parent_cache=false;
    WorldEntityId parent=0;
    const auto parent_state=host.snapshot(id);
    if(!parent_state)return {Code::MissingEntity,{}};
    if(parent_state->parent &&
       host.parent_is_animating(id,*parent_state->parent)) {
        parent=*parent_state->parent;
        use_parent_cache=host.prepare_parent_bone_cache(parent);
    }
    if(use_parent_cache) {
        const auto final=host.snapshot(id);
        if(!final)return {Code::MissingEntity,{}};
        if(final->world_transform_dirty && !host.refresh_world_transform(id))
            return {Code::SetupRejected,{}};
        if(!host.compose_parented_bones(id,parent,mask))
            return {Code::SetupRejected,{}};
    }else if(!host.compose_unparented_bones(id,mask)){
        return {Code::SetupRejected,{}};
    }
    auto matrices=host.resulting_bone_matrices(id);
    if(!matrices)return {Code::MissingMatrices,{}};
    for(const auto& m:*matrices)if(!finite(m))return {Code::InvalidState,{}};
    return {use_parent_cache?Code::ParentMerged:Code::Unparented,
            std::move(*matrices)};
}
}
OriginalSkeletalSetupResult run_original_skeletal_setup(
    IOriginalSkeletalSetupHost& host,WorldEntityId id,std::uint32_t mask) noexcept {
    OriginalSkeletalSetupResult result{};
    bool locked=false;
    try {
        if(!host.lock_model_cache(id))return {Code::ModelCacheRejected,{}};
        locked=true;
        result=execute(host,id,mask);
    }catch(...) {result={Code::HostRejected,{}};}
    if(locked) {
        // Always release the native MDL-cache lock, including failures.
        try {if(!host.unlock_model_cache(id))return {Code::HostRejected,{}};}
        catch(...) {return {Code::HostRejected,{}};}
    }
    return result;
}
} // namespace sin_compat::jessica
