#include "sin_compat/original_pose_bone_matrix.hpp"
#include <cmath>
#include <functional>
#include <utility>

namespace sin_compat::jessica {
namespace {
using Code=OriginalPoseBoneMatrixCode;
bool finite(const WorldMatrix3x4& m) noexcept {
    for(float v:m.row_major) if(!std::isfinite(v))return false;
    return true;
}
}
OriginalPoseBoneMatrixResult resolve_original_pose_bone_matrix(
    const OriginalPoseBoneMatrixInput& input) noexcept {
    try {
        const std::size_t n=input.parent_bones.size();
        if(n==0||n>256||input.local_poses.size()!=n)return {Code::InvalidModel,{},{},0};
        if(input.requested_bone>=n)return {Code::InvalidIndex,{},{},0};
        if(!finite(input.root_world))return {Code::InvalidMatrix,{},{},0};
        const std::size_t words=(n+31u)/32u;
        if((!input.computed_words.empty()&&input.computed_words.size()!=words)||
           (!input.previous_matrices.empty()&&input.previous_matrices.size()!=n))
            return {Code::InvalidSnapshot,{},{},0};
        OriginalPoseBoneMatrixResult result{};
        result.matrices=input.previous_matrices.empty()?
            std::vector<std::optional<WorldMatrix3x4>>(n):input.previous_matrices;
        result.computed_words=input.computed_words.empty()?
            std::vector<std::uint32_t>(words,0):input.computed_words;
        for(std::size_t i=0;i<n;++i){
            if(input.parent_bones[i]<-1 || input.parent_bones[i]>=static_cast<std::int32_t>(n))
                return {Code::InvalidHierarchy,{},{},0};
            const bool marked=(result.computed_words[i>>5]&(1u<<(i&31)))!=0;
            if(marked!=result.matrices[i].has_value())
                return {Code::InvalidSnapshot,{},{},0};
            if(result.matrices[i]&&!finite(*result.matrices[i]))
                return {Code::InvalidMatrix,{},{},0};
        }
        const std::size_t request=input.requested_bone;
        const bool was_cached=(result.computed_words[request>>5]&(1u<<(request&31)))!=0;
        std::vector<std::uint8_t> visiting(n,0);
        Code error=Code::InvalidHierarchy;
        std::function<bool(std::size_t)> build=[&](std::size_t id)->bool {
            const std::uint32_t mask=1u<<(id&31u);
            if(result.computed_words[id>>5]&mask)return true;
            if(visiting[id]){error=Code::InvalidHierarchy;return false;}
            visiting[id]=1;
            const auto local=original_quaternion_bone_matrix(input.local_poses[id]);
            if(!local){error=Code::InvalidPose;return false;}
            const std::int32_t parent=input.parent_bones[id];
            const WorldMatrix3x4* basis=&input.root_world;
            if(parent>=0){
                const auto parent_id=static_cast<std::size_t>(parent);
                if(!build(parent_id))return false;
                basis=&*result.matrices[parent_id];
            }
            const auto matrix=compose_world_matrices(*basis,*local);
            if(!matrix){error=Code::InvalidMatrix;return false;}
            result.matrices[id]=*matrix;
            result.computed_words[id>>5]|=mask;
            ++result.newly_built;
            visiting[id]=0;
            return true;
        };
        if(!build(request))return {error,{},{},0};
        result.code=was_cached?Code::AlreadyComputed:Code::Built;
        return result;
    }catch(...){return {Code::HostRejected,{},{},0};}
}
}
