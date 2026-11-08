#include "sin_compat/original_ordinary_bone.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

namespace sin_compat::jessica {
namespace {
bool finite(const TossVec3& v) noexcept {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}
bool finite(const WorldMatrix3x4& m) noexcept {
    for(float f:m.row_major)if(!std::isfinite(f))return false;
    return true;
}
std::uint32_t u32(const std::vector<std::uint8_t>& b,std::size_t i) noexcept {
    return std::uint32_t(b[i])|(std::uint32_t(b[i+1])<<8)|
           (std::uint32_t(b[i+2])<<16)|(std::uint32_t(b[i+3])<<24);
}
std::int32_t i32(const std::vector<std::uint8_t>& b,std::size_t i) noexcept {
    std::uint32_t raw=u32(b,i);std::int32_t out;
    std::memcpy(&out,&raw,4);return out;
}
}
std::optional<std::vector<std::uint32_t>> parse_original_studio_bone_flags(
    const std::vector<std::uint8_t>& b) noexcept {
    try {
        if(b.size()<0xa4)return std::nullopt;
        const auto count=i32(b,0x9c), offset=i32(b,0xa0);
        if(count<0||count>256||offset<0)return std::nullopt;
        constexpr std::size_t stride=0xd8,flag=0xa0;
        const std::size_t base=static_cast<std::size_t>(offset);
        const std::size_t n=static_cast<std::size_t>(count);
        if(base>b.size()||n>(b.size()-base)/stride)return std::nullopt;
        std::vector<std::uint32_t> flags;flags.reserve(n);
        for(std::size_t i=0;i<n;++i)flags.push_back(u32(b,base+i*stride+flag));
        return flags;
    }catch(...){return std::nullopt;}
}
OriginalOrdinaryBoneResult build_original_ordinary_bones(
    const OriginalOrdinaryBoneInput& input) noexcept {
    using Code=OriginalOrdinaryBoneCode;
    try {
        const auto n=input.studio.bones.size();
        if(n>256||input.bone_flags.size()!=n||input.local_poses.size()!=n||
           (!input.prior_world_matrices.empty()&&input.prior_world_matrices.size()!=n))
            return {Code::InvalidStudio,{}};
        if(!finite(input.world_origin)||!finite(input.world_angles))
            return {Code::InvalidPose,{}};
        for(std::size_t i=0;i<n;++i){
            const auto parent=input.studio.bones[i].parent_index;
            if(parent< -1||parent>=static_cast<std::int32_t>(i)||
               input.studio.bones[i].name.empty())
                return {Code::InvalidHierarchy,{}};
        }
        std::vector<std::optional<WorldMatrix3x4>> out=
            input.prior_world_matrices.empty()
            ?std::vector<std::optional<WorldMatrix3x4>>(n)
            :input.prior_world_matrices;
        for(const auto& m:out)if(m&&!finite(*m))return {Code::InvalidMatrix,{}};
        const auto root=make_local_angle_matrix(input.world_angles,input.world_origin);
        if(!root)return {Code::InvalidMatrix,{}};
        // A single requested bone first walks its parent chain from leaf
        // to root. The native implementation pushes the indices on the
        // stack and processes them in reverse order.
        std::vector<std::size_t> order;
        if(input.requested_bone==-1) {
            for(std::size_t i=0;i<n;++i)order.push_back(i);
        }else if(n) {
            const auto request=input.requested_bone< -1||
                input.requested_bone>=static_cast<std::int32_t>(n)
                ?std::size_t(0):static_cast<std::size_t>(input.requested_bone);
            std::size_t index=request;
            for(std::size_t depth=0;depth<n;++depth) {
                order.push_back(index);
                const auto parent=input.studio.bones[index].parent_index;
                if(parent<0)break;
                index=static_cast<std::size_t>(parent);
            }
            if(order.size()>n)return {Code::InvalidHierarchy,{}};
            std::reverse(order.begin(),order.end());
        }
        OriginalOrdinaryBoneResult result{};result.matrices=std::move(out);
        for(const auto index:order) {
            if((input.bone_flags[index]&input.bone_mask)==0){
                ++result.masked_out;
                continue;
            }
            const auto local=original_quaternion_bone_matrix(input.local_poses[index]);
            if(!local)return {Code::InvalidPose,{}};
            const auto parent=input.studio.bones[index].parent_index;
            const WorldMatrix3x4* basis=&*root;
            if(parent>=0) {
                const auto parent_idx=static_cast<std::size_t>(parent);
                if(!result.matrices[parent_idx])return {Code::MissingParentMatrix,{}};
                basis=&*result.matrices[parent_idx];
            }
            const auto composed=compose_world_matrices(*basis,*local);
            if(!composed)return {Code::InvalidMatrix,{}};
            result.matrices[index]=*composed;
            ++result.built;
        }
        result.code=Code::Built;
        return result;
    }catch(...){return {Code::HostRejected,{}};}
}
bool OriginalOrdinaryBoneSkeletalAdapter::lock_model_cache(WorldEntityId id){
    composed_.erase(id);return source_.lock_model_cache(id);
}
bool OriginalOrdinaryBoneSkeletalAdapter::unlock_model_cache(WorldEntityId id){
    return source_.unlock_model_cache(id);
}
std::optional<OriginalSkeletalSetupSnapshot>
OriginalOrdinaryBoneSkeletalAdapter::snapshot(WorldEntityId id){return source_.snapshot(id);}
bool OriginalOrdinaryBoneSkeletalAdapter::refresh_world_transform(WorldEntityId id){
    return source_.refresh_world_transform(id);
}
bool OriginalOrdinaryBoneSkeletalAdapter::increment_pose_driver_counter(WorldEntityId id){
    return source_.increment_pose_driver_counter(id);
}
bool OriginalOrdinaryBoneSkeletalAdapter::seed_pose_driver(
    WorldEntityId id,const TossVec3& o,const TossVec3& a,std::uint32_t mask){
    return source_.seed_pose_driver(id,o,a,mask);
}
bool OriginalOrdinaryBoneSkeletalAdapter::evaluate_virtual_base_pose(
    WorldEntityId id,std::uint32_t mask){return source_.evaluate_virtual_base_pose(id,mask);}
bool OriginalOrdinaryBoneSkeletalAdapter::blend_pose_driver(
    WorldEntityId id,std::uint32_t mask){return source_.blend_pose_driver(id,mask);}
bool OriginalOrdinaryBoneSkeletalAdapter::invoke_virtual_pose_hook(
    WorldEntityId id,std::uint32_t mask){return source_.invoke_virtual_pose_hook(id,mask);}
bool OriginalOrdinaryBoneSkeletalAdapter::finalize_pose_driver(
    WorldEntityId id,std::uint32_t mask){return source_.finalize_pose_driver(id,mask);}
bool OriginalOrdinaryBoneSkeletalAdapter::parent_is_animating(
    WorldEntityId child,WorldEntityId parent){return source_.parent_is_animating(child,parent);}
bool OriginalOrdinaryBoneSkeletalAdapter::prepare_parent_bone_cache(
    WorldEntityId parent){return source_.prepare_parent_bone_cache(parent);}
bool OriginalOrdinaryBoneSkeletalAdapter::compose_parented_bones(
    WorldEntityId child,WorldEntityId parent,std::uint32_t mask){
    composed_.erase(child);return source_.compose_parented_bones(child,parent,mask);
}
bool OriginalOrdinaryBoneSkeletalAdapter::compose_unparented_bones(
    WorldEntityId child,std::uint32_t mask){
    composed_.erase(child);
    const auto args=input_.ordinary_inputs(child,mask);
    if(!args)return false;
    auto result=build_original_ordinary_bones(*args);
    if(!result.successful())return false;
    std::vector<WorldMatrix3x4> complete;complete.reserve(result.matrices.size());
    for(const auto& m:result.matrices) {
        if(!m)return false; // don't invent values for masked-out matrix slots
        complete.push_back(*m);
    }
    composed_.emplace(child,std::move(complete));return true;
}
std::optional<std::vector<WorldMatrix3x4>>
OriginalOrdinaryBoneSkeletalAdapter::resulting_bone_matrices(WorldEntityId child){
    const auto it=composed_.find(child);
    if(it!=composed_.end())return it->second;
    return source_.resulting_bone_matrices(child);
}
} // namespace sin_compat::jessica
