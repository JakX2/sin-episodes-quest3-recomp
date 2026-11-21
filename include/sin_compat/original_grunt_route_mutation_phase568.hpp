#pragma once
// Independent C++17 host reference. Original 2006 PE32 assembly is proven
// separately; these typed objects ARE NOT native Source engine ABI objects.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <unordered_set>

namespace sin_compat::phase568 {
struct Vec3 { float x=0,y=0,z=0; };
inline Vec3 plus(Vec3 a,Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
struct Waypoint {
    Vec3 position{};               // original raw +0x00 vector
    std::uint32_t raw_0x10=0;       // original special value copied to route +0x2C
    float cached_length=-1.0f;     // original float +0x14
    std::uint32_t flags=0;         // original +0x20
    std::uint32_t dimensionality=0;// original +0x24: 0=XY, nonzero=XYZ
    Waypoint* forward=nullptr;     // original +0x28
    Waypoint* backward=nullptr;    // original +0x2C
};
struct Route {
    Waypoint* first=nullptr;
    Vec3 origin{};                // original route+0x14/+0x18/+0x1C
    Vec3 endpoint{};              // original route+0x34/+0x38/+0x3C
    std::uint32_t raw_0x2c=0;
    bool endpoint_valid_0x30=false;
    std::uint32_t state_0x44=0;
    bool state_changed_0x40=false;
    float state_transition_time_0x4c=0;
};
enum class Result { Ok, NoChange, TerminalMarked, UnsupportedGraph, UnsupportedFloat };
inline bool is_original_invalid_cache(float value) {
    std::uint32_t bits=0; static_assert(sizeof(value)==sizeof(bits));
    std::memcpy(&bits,&value,sizeof(bits));return bits==0xBF800000u;
}
inline Waypoint* last_with_guard(Waypoint* first) {
    std::unordered_set<const Waypoint*> seen;
    for(Waypoint* node=first;node;node=node->forward) {
        if(!seen.insert(node).second)return nullptr;
        if(!node->forward)return node;
    }
    return nullptr;
}
inline Result set_first(Route& r,Waypoint* first,bool mark_terminal) {
    // Original 0x2209D160 itself only sets a pointer and walks to the last.
    // Cyclic-list rejection is host safety, not an original behavior.
    Waypoint* last=last_with_guard(first);
    if(first&&!last)return Result::UnsupportedGraph;
    r.first=first;
    if(!last)return Result::Ok;
    last->cached_length=-1.0f;
    if(mark_terminal) {
        r.endpoint_valid_0x30=true;
        last->flags|=8;
        r.endpoint=plus(r.origin,last->position);
    }
    return Result::Ok;
}
inline void unlink_neighbors(Waypoint& old) {
    // Original 0x220746F0 updates only neighbors; not owner.first, and
    // does not set this node's links to null.
    if(old.forward)old.forward->backward=old.backward;
    if(old.backward)old.backward->forward=old.forward;
}
inline Result advance_first(Route& r) {
    // Original code contains an unsafe null dereference when called with
    // no first node; report unsupported rather than copying that hazard.
    if(!r.first)return Result::UnsupportedGraph;
    if(r.first->flags&8)return Result::NoChange;
    Waypoint* old=r.first;
    Waypoint* next=old->forward;
    if(!next) {
        old->flags|=8;
        return Result::TerminalMarked;
    }
    if(old->flags&4)r.raw_0x2c=old->raw_0x10;
    unlink_neighbors(*old);
    r.first=next;
    Waypoint* last=last_with_guard(r.first);
    if(!last)return Result::UnsupportedGraph;
    last->cached_length=-1.0f;
    // The original releases a node to its pool allocator. This host
    // reference deliberately doesn't free externally owned test nodes.
    return Result::Ok;
}
inline void set_route_state(Route& r,std::uint32_t next_state,float engine_time) {
    // Original 0x2209D0C0 conditions the time and +0x40 flag on OLD
    // route+0x44, not the *new* state.
    if(r.state_0x44) {
        r.state_transition_time_0x4c=engine_time;
        r.state_changed_0x40=true;
    } else r.state_changed_0x40=false;
    r.state_0x44=next_state;
}
struct Scalar {Result status;float value;};
inline Scalar extension_distance(Route& r,Vec3 extra) {
    // Bounded host model for 0x2209D450. Original result is path length
    // *plus* direct distance from FIRST waypoint to external vector, using
    // FIRST waypoint's own numeric XY/XYZ dimensionality mode.
    if(!r.first)return {Result::Ok,0.f};
    if(!std::isfinite(extra.x)||!std::isfinite(extra.y)||!std::isfinite(extra.z))
        return {Result::UnsupportedFloat,0.f};
    Waypoint* last=last_with_guard(r.first);
    if(!last)return {Result::UnsupportedGraph,0.f};
    if(is_original_invalid_cache(last->cached_length)) {
        std::unordered_set<Waypoint*> seen;
        for(auto* node=last;node;node=node->backward) {
            if(!seen.insert(node).second)return {Result::UnsupportedGraph,0.f};
            if(!std::isfinite(node->position.x)||!std::isfinite(node->position.y)||
                !std::isfinite(node->position.z))return {Result::UnsupportedFloat,0.f};
            if(node->backward&&node->backward->forward!=node)
                return {Result::UnsupportedGraph,0.f};
        }
        if(!seen.count(r.first))return {Result::UnsupportedGraph,0.f};
        last->cached_length=0.f;
        for(auto* node=last;node->backward;node=node->backward) {
            const auto a=node->position,b=node->backward->position;
            const float dx=a.x-b.x,dy=a.y-b.y,dz=a.z-b.z;
            const float square=dx*dx+dy*dy+(node->dimensionality?dz*dz:0.f);
            if(!std::isfinite(square))return {Result::UnsupportedFloat,0.f};
            node->backward->cached_length=node->cached_length+std::sqrt(square);
        }
    }
    const auto a=r.first->position;
    const float dx=a.x-extra.x,dy=a.y-extra.y,dz=a.z-extra.z;
    const float square=dx*dx+dy*dy+(r.first->dimensionality?dz*dz:0.f);
    const float answer=r.first->cached_length+std::sqrt(square);
    if(!std::isfinite(answer))return {Result::UnsupportedFloat,0.f};
    return {Result::Ok,answer};
}
} // namespace sin_compat::phase568