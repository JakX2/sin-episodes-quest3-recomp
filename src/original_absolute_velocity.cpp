#include "sin_compat/original_absolute_velocity.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace sin_compat::jessica {
namespace {
using Code=AbsoluteVelocityCode;
constexpr std::size_t kMaxDepth=64; // independent safety cap
bool finite(const TossVec3& x) noexcept {
    return std::isfinite(x.x)&&std::isfinite(x.y)&&std::isfinite(x.z);
}
bool finite(const AbsoluteMatrix3x4& m) noexcept {
    for(float f:m.row_major)if(!std::isfinite(f))return false;
    return true;
}
std::optional<float> checked(double d) noexcept {
    if(!std::isfinite(d)||std::abs(d)>std::numeric_limits<float>::max())
        return std::nullopt;
    return static_cast<float>(d);
}
std::optional<TossVec3> combine(const TossVec3& a,const TossVec3& b,
                                bool subtract) noexcept {
    if(!finite(a)||!finite(b))return std::nullopt;
    const double sign=subtract?-1.0:1.0;
    const auto x=checked(double(a.x)+sign*b.x);
    const auto y=checked(double(a.y)+sign*b.y);
    const auto z=checked(double(a.z)+sign*b.z);
    if(!x||!y||!z)return std::nullopt;
    return TossVec3{*x,*y,*z};
}
struct Walk {
    IOriginalAbsoluteVelocityHost& host;
    std::array<AbsoluteEntityId,kMaxDepth> ids{};
    std::size_t depth=0;
    explicit Walk(IOriginalAbsoluteVelocityHost& h):host(h){}
    Code push(AbsoluteEntityId id) noexcept {
        if(std::find(ids.begin(),ids.begin()+depth,id)!=ids.begin()+depth)
            return Code::ParentCycle;
        if(depth==kMaxDepth)return Code::DepthLimit;
        ids[depth++]=id;return Code::Refreshed;
    }
    void pop() noexcept {--depth;}
    bool contains(AbsoluteEntityId id) const noexcept {
        return std::find(ids.begin(),ids.begin()+depth,id)!=ids.begin()+depth;
    }
};
struct PopGuard {
    Walk& walk;
    explicit PopGuard(Walk& w):walk(w){}
    ~PopGuard(){walk.pop();}
};
AbsoluteVelocityResult refresh_impl(Walk&,AbsoluteEntityId);
struct ParentState {
    Code code=Code::MissingParent;
    AbsoluteVelocitySnapshot state{};
};
ParentState resolve_parent(Walk& w,AbsoluteEntityId id) {
    if(w.contains(id))return {Code::ParentCycle,{}};
    auto s=w.host.snapshot(id);
    if(!s)return {Code::MissingParent,{}};
    if(s->absolute_dirty) {
        auto r=refresh_impl(w,id);
        if(!r.successful())return {r.code,{}};
        s=w.host.snapshot(id);
        if(!s||s->absolute_dirty)return {Code::HostRejected,{}};
    }
    if(s->transform_dirty) {
        if(!w.host.refresh_world_transform(id))
            return {Code::HostRejected,{}};
        s=w.host.snapshot(id);
        if(!s||s->transform_dirty)return {Code::HostRejected,{}};
    }
    if(!finite(s->absolute)||!finite(s->entity_to_world))
        return {Code::InvalidInput,{}};
    return {Code::Refreshed,*s};
}
AbsoluteVelocityResult refresh_impl(Walk& w,AbsoluteEntityId id) {
    const Code entered=w.push(id);
    if(entered!=Code::Refreshed)return {entered,{}};
    PopGuard guard(w);
    auto s=w.host.snapshot(id);
    if(!s)return {Code::MissingEntity,{}};
    if(!s->absolute_dirty) {
        if(!finite(s->absolute))return {Code::InvalidInput,{}};
        return {Code::Unchanged,s->absolute};
    }
    if(!finite(s->local))return {Code::InvalidInput,{}};
    TossVec3 world=s->local;
    if(s->parent) {
        const auto parent=resolve_parent(w,*s->parent);
        if(parent.code!=Code::Refreshed)return {parent.code,{}};
        const auto converted=rotate_local_velocity_to_world(
            parent.state.entity_to_world,s->local);
        if(!converted)return {Code::InvalidInput,{}};
        const auto total=combine(*converted,parent.state.absolute,false);
        if(!total)return {Code::InvalidInput,{}};
        world=*total;
    }
    // Original clears dirty earlier. The portable host commits validated
    // values before clearing it, to avoid publishing an invalid cache.
    if(!w.host.store_absolute_velocity(id,world)||
       !w.host.clear_absolute_dirty(id))return {Code::HostRejected,{}};
    return {Code::Refreshed,world};
}
AbsoluteVelocityResult set_impl(Walk& w,AbsoluteEntityId id,
                                const TossVec3& desired) {
    if(!finite(desired))return {Code::InvalidInput,{}};
    const Code entered=w.push(id);
    if(entered!=Code::Refreshed)return {entered,{}};
    PopGuard guard(w);
    auto s=w.host.snapshot(id);
    if(!s)return {Code::MissingEntity,{}};
    // Original tests cached +0x250 fields before fetching the parent.
    if(s->absolute.x==desired.x&&s->absolute.y==desired.y&&
       s->absolute.z==desired.z)return {Code::Unchanged,desired};
    TossVec3 local=desired;
    if(s->parent) {
        const auto parent=resolve_parent(w,*s->parent);
        if(parent.code!=Code::Refreshed)return {parent.code,{}};
        const auto delta=combine(desired,parent.state.absolute,true);
        if(!delta)return {Code::InvalidInput,{}};
        const auto inverted=inverse_rotate_world_velocity(
            parent.state.entity_to_world,*delta);
        if(!inverted)return {Code::InvalidInput,{}};
        local=*inverted;
    }
    // Original 0x22141A00 invalidates mask=4, clears abs dirty, then
    // writes world cache and calls local-velocity setter 0x2213E1A0.
    // Host owns native invalidation and local setter side effects.
    if(!w.host.invalidate_velocity_dependents(id,4)||
       !w.host.clear_absolute_dirty(id)||
       !w.host.store_absolute_velocity(id,desired)||
       !w.host.store_local_velocity(id,local))return {Code::HostRejected,{}};
    return {Code::Set,desired};
}
}
std::optional<TossVec3> rotate_local_velocity_to_world(
    const AbsoluteMatrix3x4& mat,const TossVec3& v) noexcept {
    if(!finite(mat)||!finite(v))return std::nullopt;
    const auto& m=mat.row_major;
    const auto x=checked(double(m[0])*v.x+double(m[1])*v.y+double(m[2])*v.z);
    const auto y=checked(double(m[4])*v.x+double(m[5])*v.y+double(m[6])*v.z);
    const auto z=checked(double(m[8])*v.x+double(m[9])*v.y+double(m[10])*v.z);
    if(!x||!y||!z)return std::nullopt;
    return TossVec3{*x,*y,*z};
}
std::optional<TossVec3> inverse_rotate_world_velocity(
    const AbsoluteMatrix3x4& mat,const TossVec3& v) noexcept {
    if(!finite(mat)||!finite(v))return std::nullopt;
    const auto& m=mat.row_major;
    const auto x=checked(double(m[0])*v.x+double(m[4])*v.y+double(m[8])*v.z);
    const auto y=checked(double(m[1])*v.x+double(m[5])*v.y+double(m[9])*v.z);
    const auto z=checked(double(m[2])*v.x+double(m[6])*v.y+double(m[10])*v.z);
    if(!x||!y||!z)return std::nullopt;
    return TossVec3{*x,*y,*z};
}
AbsoluteVelocityResult set_original_absolute_velocity(
    IOriginalAbsoluteVelocityHost& h,AbsoluteEntityId id,
    const TossVec3& world) noexcept {
    try{Walk w(h);return set_impl(w,id,world);}
    catch(...){return {Code::HostRejected,{}};}
}
AbsoluteVelocityResult refresh_original_absolute_velocity(
    IOriginalAbsoluteVelocityHost& h,AbsoluteEntityId id) noexcept {
    try{Walk w(h);return refresh_impl(w,id);}
    catch(...){return {Code::HostRejected,{}};}
}
} // namespace sin_compat::jessica
