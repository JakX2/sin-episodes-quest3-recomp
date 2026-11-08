#include "sin_compat/original_parent_bone_merge.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>
#include <utility>

namespace sin_compat::jessica {
namespace {
bool finite(float value) noexcept{return std::isfinite(value);}
bool finite(const TossVec3& v) noexcept{
    return finite(v.x)&&finite(v.y)&&finite(v.z);
}
bool finite(const WorldMatrix3x4& m) noexcept {
    for(const float value:m.row_major)if(!finite(value))return false;
    return true;
}
std::uint32_t little_u32(const std::vector<std::uint8_t>& b,std::size_t p) {
    return std::uint32_t(b[p])|(std::uint32_t(b[p+1])<<8)|
           (std::uint32_t(b[p+2])<<16)|(std::uint32_t(b[p+3])<<24);
}
std::int32_t little_i32(const std::vector<std::uint8_t>& b,std::size_t p) {
    const std::uint32_t raw=little_u32(b,p);
    std::int32_t out=0;
    static_assert(sizeof(out)==sizeof(raw),"requires 32-bit ints");
    std::memcpy(&out,&raw,4);
    return out;
}
unsigned char lower_ascii(unsigned char c) noexcept {
    if(c>='A'&&c<='Z')return static_cast<unsigned char>(c+('a'-'A'));
    return c;
}
int compare_ascii_nocase(const std::string& a,const std::string& b) noexcept {
    const std::size_t n=std::min(a.size(),b.size());
    for(std::size_t i=0;i<n;i++) {
        const auto ca=lower_ascii(static_cast<unsigned char>(a[i]));
        const auto cb=lower_ascii(static_cast<unsigned char>(b[i]));
        if(ca!=cb)return ca<cb?-1:1;
    }
    if(a.size()==b.size())return 0;
    return a.size()<b.size()?-1:1;
}
bool valid_studio(const OriginalStudioBoneTable& h,bool require_sorted) {
    const std::size_t n=h.bones.size();
    if(n>256 || (require_sorted&&h.sorted_name_indices.size()!=n))return false;
    for(std::size_t i=0;i<n;++i) {
        const auto& bone=h.bones[i];
        if(bone.name.empty()||bone.name.find('\0')!=std::string::npos||
           bone.parent_index< -1 ||
           (bone.parent_index>=0 && std::size_t(bone.parent_index)>=i))return false;
    }
    if(require_sorted) {
        std::vector<bool> seen(n,false);
        for(std::size_t j=0;j<n;++j) {
            const std::size_t idx=h.sorted_name_indices[j];
            if(idx>=n||seen[idx])return false;
            seen[idx]=true;
            if(j && compare_ascii_nocase(h.bones[h.sorted_name_indices[j-1]].name,
                                         h.bones[idx].name)>=0)return false;
        }
    }
    return true;
}
bool float_result(double x)noexcept {
    return std::isfinite(x)&&std::abs(x)<=std::numeric_limits<float>::max();
}
}
std::optional<OriginalStudioBoneTable> parse_original_studio_bones(
    const std::vector<std::uint8_t>& b) noexcept {
    try {
        constexpr std::size_t kCount=0x9C,kOffset=0xA0,kSorted=0x16C;
        constexpr std::size_t kStride=0xD8;
        if(b.size()<kSorted+4)return std::nullopt;
        const auto count=little_i32(b,kCount);
        const auto bone_start=little_i32(b,kOffset);
        const auto sorted_start=little_i32(b,kSorted);
        if(count<0 || count>256 || bone_start<0 || sorted_start<0)
            return std::nullopt;
        const auto n=static_cast<std::size_t>(count);
        const auto start=static_cast<std::size_t>(bone_start);
        const auto sorted=static_cast<std::size_t>(sorted_start);
        if(start>b.size()||n>(b.size()-start)/kStride ||
           sorted>b.size()||n>b.size()-sorted)return std::nullopt;
        OriginalStudioBoneTable out;
        out.bones.reserve(n);
        out.sorted_name_indices.assign(b.begin()+sorted,b.begin()+sorted+n);
        for(std::size_t i=0;i<n;++i) {
            const std::size_t record=start+kStride*i;
            const auto name_relative=little_i32(b,record);
            const auto parent=little_i32(b,record+4);
            if(name_relative<0)return std::nullopt;
            const std::size_t name_at=record+static_cast<std::size_t>(name_relative);
            if(name_at>=b.size())return std::nullopt;
            const auto end=std::find(b.begin()+name_at,b.end(),0);
            if(end==b.end())return std::nullopt;
            out.bones.push_back({std::string(b.begin()+name_at,end),parent});
        }
        if(!valid_studio(out,true))return std::nullopt;
        return out;
    }catch(...) {return std::nullopt;}
}
std::optional<std::uint32_t> find_original_bone_by_name(
    const OriginalStudioBoneTable& parent,const std::string& name) noexcept {
    if(!valid_studio(parent,true)||name.empty())return std::nullopt;
    // Original 0x220FFB30 binary-searches header+0x16C's byte table,
    // dereferences the 216-byte bone record and compares case-insensitively.
    std::size_t low=0,high=parent.bones.size();
    while(low<high) {
        const std::size_t mid=low+(high-low)/2;
        const std::size_t bone_idx=parent.sorted_name_indices[mid];
        const int cmp=compare_ascii_nocase(parent.bones[bone_idx].name,name);
        if(cmp<0)low=mid+1;
        else if(cmp>0)high=mid;
        else return static_cast<std::uint32_t>(bone_idx);
    }
    return std::nullopt;
}
std::optional<WorldMatrix3x4> original_quaternion_bone_matrix(
    const OriginalBoneLocalPose& pose) noexcept {
    const auto& q=pose.orientation;
    if(!finite(pose.position)||!finite(q.x)||!finite(q.y)||
       !finite(q.z)||!finite(q.w))return std::nullopt;
    const double x=q.x,y=q.y,z=q.z,w=q.w;
    const double d[12]={
        1-2*(y*y+z*z),2*(x*y-w*z),2*(x*z+w*y),pose.position.x,
        2*(x*y+w*z),1-2*(x*x+z*z),2*(y*z-w*x),pose.position.y,
        2*(x*z-w*y),2*(y*z+w*x),1-2*(x*x+y*y),pose.position.z
    };
    WorldMatrix3x4 m{};
    for(std::size_t i=0;i<12;++i) {
        if(!float_result(d[i]))return std::nullopt;
        m.row_major[i]=static_cast<float>(d[i]);
    }
    return m;
}
OriginalParentBoneMergeResult merge_original_parent_bones(
    const OriginalParentBoneMergeInput& input) noexcept {
    using Code=OriginalParentBoneMergeCode;
    try {
        const std::size_t n=input.child_studio.bones.size();
        if(!valid_studio(input.child_studio,false)||
           !valid_studio(input.parent_studio,true)||
           input.child_local_poses.size()!=n||
           !finite(input.child_world_origin)||!finite(input.child_world_angles))
            return {Code::InvalidModel,{}};
        const auto root=make_local_angle_matrix(input.child_world_angles,
                                                 input.child_world_origin);
        if(!root)return {Code::InvalidMatrix,{}};
        OriginalParentBoneMergeResult result{};
        result.matrices.reserve(n);
        for(std::size_t i=0;i<n;++i) {
            const auto& bone=input.child_studio.bones[i];
            const auto match=find_original_bone_by_name(input.parent_studio,bone.name);
            if(match) {
                const auto cached=read_original_bone_cache_matrix(
                    input.parent_bone_cache_bytes,*match);
                if(cached.code==OriginalBoneCacheReadCode::Found) {
                    if(!finite(cached.matrix))return {Code::InvalidMatrix,{}};
                    result.matrices.push_back(cached.matrix);
                    ++result.reused_parent_bones;
                    continue;
                }
                if(cached.code!=OriginalBoneCacheReadCode::NotCached)
                    return {Code::InvalidParentCache,{}};
            }
            // Original 0x2226D250 builds local quat+position, then
            // 0x22269520 combines with root modelToWorld or child parent bone.
            const auto local=original_quaternion_bone_matrix(input.child_local_poses[i]);
            if(!local)return {Code::InvalidPose,{}};
            const WorldMatrix3x4* basis=&*root;
            if(bone.parent_index>=0) {
                const auto parent=static_cast<std::size_t>(bone.parent_index);
                if(parent>=result.matrices.size())return {Code::InvalidHierarchy,{}};
                basis=&result.matrices[parent];
            }
            const auto composed=compose_world_matrices(*basis,*local);
            if(!composed)return {Code::InvalidMatrix,{}};
            result.matrices.push_back(*composed);
            ++result.locally_composed_bones;
        }
        result.code=Code::Merged;
        return result;
    }catch(...) {return {Code::HostRejected,{}};}
}
bool OriginalParentBoneMergeSkeletalAdapter::lock_model_cache(WorldEntityId id) {
    merged_.erase(id);return source_.lock_model_cache(id);
}
bool OriginalParentBoneMergeSkeletalAdapter::unlock_model_cache(WorldEntityId id) {
    return source_.unlock_model_cache(id);
}
std::optional<OriginalSkeletalSetupSnapshot>
OriginalParentBoneMergeSkeletalAdapter::snapshot(WorldEntityId id) {
    return source_.snapshot(id);
}
bool OriginalParentBoneMergeSkeletalAdapter::refresh_world_transform(WorldEntityId id) {
    return source_.refresh_world_transform(id);
}
bool OriginalParentBoneMergeSkeletalAdapter::increment_pose_driver_counter(WorldEntityId id) {
    return source_.increment_pose_driver_counter(id);
}
bool OriginalParentBoneMergeSkeletalAdapter::seed_pose_driver(
    WorldEntityId id,const TossVec3& o,const TossVec3& a,std::uint32_t mask) {
    return source_.seed_pose_driver(id,o,a,mask);
}
bool OriginalParentBoneMergeSkeletalAdapter::evaluate_virtual_base_pose(
    WorldEntityId id,std::uint32_t mask) {
    return source_.evaluate_virtual_base_pose(id,mask);
}
bool OriginalParentBoneMergeSkeletalAdapter::blend_pose_driver(WorldEntityId id,
                                                              std::uint32_t mask) {
    return source_.blend_pose_driver(id,mask);
}
bool OriginalParentBoneMergeSkeletalAdapter::invoke_virtual_pose_hook(
    WorldEntityId id,std::uint32_t mask) {
    return source_.invoke_virtual_pose_hook(id,mask);
}
bool OriginalParentBoneMergeSkeletalAdapter::finalize_pose_driver(
    WorldEntityId id,std::uint32_t mask) {
    return source_.finalize_pose_driver(id,mask);
}
bool OriginalParentBoneMergeSkeletalAdapter::parent_is_animating(
    WorldEntityId id,WorldEntityId parent) {
    return source_.parent_is_animating(id,parent);
}
bool OriginalParentBoneMergeSkeletalAdapter::prepare_parent_bone_cache(
    WorldEntityId parent) {
    return source_.prepare_parent_bone_cache(parent);
}
bool OriginalParentBoneMergeSkeletalAdapter::compose_parented_bones(
    WorldEntityId id,WorldEntityId parent,std::uint32_t mask) {
    merged_.erase(id);
    const auto args=merge_.merge_inputs(id,parent,mask);
    if(!args)return false;
    auto result=merge_original_parent_bones(*args);
    if(!result.successful())return false;
    merged_.emplace(id,std::move(result.matrices));
    return true;
}
bool OriginalParentBoneMergeSkeletalAdapter::compose_unparented_bones(
    WorldEntityId id,std::uint32_t mask) {
    merged_.erase(id);return source_.compose_unparented_bones(id,mask);
}
std::optional<std::vector<WorldMatrix3x4>>
OriginalParentBoneMergeSkeletalAdapter::resulting_bone_matrices(WorldEntityId id) {
    const auto it=merged_.find(id);
    if(it!=merged_.end())return it->second;
    return source_.resulting_bone_matrices(id);
}
} // namespace sin_compat::jessica
