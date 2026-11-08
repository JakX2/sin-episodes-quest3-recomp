#include "sin_compat/original_world_transform.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace sin_compat::jessica {
namespace {
constexpr double kPi=3.14159265358979323846264338327950288;
constexpr std::size_t kMaxDepth=64; // added safety guard, not original code
using Code=WorldRefreshCode;
bool finite(const TossVec3& v) noexcept {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
}
bool finite(const WorldMatrix3x4& m) noexcept {
    for(float f:m.row_major) if(!std::isfinite(f)) return false;
    return true;
}
std::optional<float> checked(double value) noexcept {
    if(!std::isfinite(value) ||
       std::abs(value)>std::numeric_limits<float>::max()) return std::nullopt;
    return static_cast<float>(value);
}
bool eqzero(const TossVec3& v) noexcept {
    return v.x==0.0f && v.y==0.0f && v.z==0.0f;
}
struct Walk {
    IOriginalWorldTransformHost& host;
    std::array<WorldEntityId,kMaxDepth> active{};
    std::size_t depth=0;
    explicit Walk(IOriginalWorldTransformHost& h):host(h){}
    Code enter(WorldEntityId id) noexcept {
        if(std::find(active.begin(),active.begin()+depth,id)!=active.begin()+depth)
            return Code::ParentCycle;
        if(depth==kMaxDepth) return Code::DepthLimit;
        active[depth++]=id;return Code::Refreshed;
    }
    void leave() noexcept {--depth;}
};
struct Guard {Walk& w; ~Guard(){w.leave();}};
WorldRefreshResult refresh_impl(Walk& walk,WorldEntityId id) {
    const auto entered=walk.enter(id);
    if(entered!=Code::Refreshed) return {entered,{},{}};
    Guard guard{walk};
    const auto initial=walk.host.snapshot(id);
    if(!initial) return {Code::MissingEntity,{},{}};
    if(!initial->dirty) {
        if(!finite(initial->world_origin)||!finite(initial->world_angles)||
           !finite(initial->world_matrix)) return {Code::InvalidTransform,{},{}};
        return {Code::Unchanged,initial->world_origin,initial->world_angles};
    }
    if(!finite(initial->local_origin)||!finite(initial->local_angles))
        return {Code::InvalidTransform,{},{}};
    const auto local=make_local_angle_matrix(initial->local_angles,
                                             initial->local_origin);
    if(!local) return {Code::InvalidTransform,{},{}};
    WorldTransformSnapshot updated=*initial;
    updated.world_matrix=*local;
    updated.world_origin=initial->local_origin;
    updated.world_angles=initial->local_angles;
    // The 2006 EHANDLE generation check treats a stale/deleted parent
    // as absent; in that case retain the direct local->world fallback.
    if(initial->parent && walk.host.snapshot(*initial->parent)) {
        auto parent_result=refresh_impl(walk,*initial->parent);
        if(!parent_result.successful()) {
            return {parent_result.code==Code::MissingEntity?
                    Code::MissingParent:parent_result.code,{},{}};
        }
        const auto parent=walk.host.snapshot(*initial->parent);
        if(!parent || parent->dirty) return {Code::HostRejected,{},{}};
        if(!finite(parent->world_matrix)||!finite(parent->world_angles))
            return {Code::InvalidTransform,{},{}};
        auto selected=parent->world_matrix;
        if(initial->parent_attachment!=0) {
            const auto attached=walk.host.attachment_parent_matrix(
                id,*initial->parent,initial->parent_attachment);
            if(attached) selected=*attached;
        }
        const auto world=compose_world_matrices(selected,*local);
        if(!world) return {Code::InvalidTransform,{},{}};
        updated.world_matrix=*world;
        updated.world_origin={world->row_major[3],world->row_major[7],
                              world->row_major[11]};
        // 0x2213FB90: zero local angles + no attachment shortcuts to
        // parent angles rather than calling matrix-to-angle conversion.
        if(initial->parent_attachment==0 && eqzero(initial->local_angles))
            updated.world_angles=parent->world_angles;
        else {
            const auto angles=angles_from_world_matrix(*world);
            if(!angles)return {Code::InvalidTransform,{},{}};
            updated.world_angles=*angles;
        }
    }
    updated.dirty=false;
    if(!walk.host.commit_world_transform(id,updated) ||
       !walk.host.on_world_transform_rebuilt(id))
        return {Code::HostRejected,{},{}};
    return {Code::Refreshed,updated.world_origin,updated.world_angles};
}
} // namespace
std::optional<WorldMatrix3x4> make_local_angle_matrix(
    const TossVec3& angles,const TossVec3& origin) noexcept {
    if(!finite(angles)||!finite(origin))return std::nullopt;
    const auto pitch=double(angles.x)*kPi/180.0;
    const auto yaw=double(angles.y)*kPi/180.0;
    const auto roll=double(angles.z)*kPi/180.0;
    const double sp=std::sin(pitch),cp=std::cos(pitch);
    const double sy=std::sin(yaw),cy=std::cos(yaw);
    const double sr=std::sin(roll),cr=std::cos(roll);
    const double vals[12]={
        cp*cy, sr*sp*cy-cr*sy, cr*sp*cy+sr*sy, origin.x,
        cp*sy, sr*sp*sy+cr*cy, cr*sp*sy-sr*cy, origin.y,
        -sp,   sr*cp,          cr*cp,          origin.z};
    WorldMatrix3x4 out;
    for(std::size_t i=0;i<12;++i) {
        const auto f=checked(vals[i]);if(!f)return std::nullopt;
        out.row_major[i]=*f;
    }
    return out;
}
std::optional<WorldMatrix3x4> compose_world_matrices(
    const WorldMatrix3x4& parent,const WorldMatrix3x4& local) noexcept {
    if(!finite(parent)||!finite(local))return std::nullopt;
    WorldMatrix3x4 out;
    const auto& p=parent.row_major;
    const auto& l=local.row_major;
    for(std::size_t row=0;row<3;++row) {
        for(std::size_t col=0;col<4;++col) {
            double sum=col==3?double(p[row*4+3]):0.0;
            for(std::size_t inner=0;inner<3;++inner)
                sum+=double(p[row*4+inner])*l[inner*4+col];
            const auto f=checked(sum);if(!f)return std::nullopt;
            out.row_major[row*4+col]=*f;
        }
    }
    return out;
}
std::optional<TossVec3> angles_from_world_matrix(
    const WorldMatrix3x4& world) noexcept {
    if(!finite(world))return std::nullopt;
    const auto& m=world.row_major;
    const double xy=std::hypot(double(m[0]),double(m[4]));
    const double pitch=std::atan2(-double(m[8]),xy);
    const double yaw=xy>0.001?std::atan2(double(m[4]),double(m[0])):
                           std::atan2(-double(m[1]),double(m[5]));
    const double roll=xy>0.001?std::atan2(double(m[9]),double(m[10])):0.0;
    const auto p=checked(pitch*180.0/kPi);
    const auto y=checked(yaw*180.0/kPi);
    const auto r=checked(roll*180.0/kPi);
    if(!p||!y||!r)return std::nullopt;
    return TossVec3{*p,*y,*r};
}
WorldRefreshResult refresh_original_world_transform(
    IOriginalWorldTransformHost& host,WorldEntityId id) noexcept {
    try {Walk walk(host);return refresh_impl(walk,id);}
    catch(...){return {Code::HostRejected,{},{}};}
}
} // namespace sin_compat::jessica
