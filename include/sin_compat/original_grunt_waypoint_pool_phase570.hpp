#pragma once
// Original-free C++17 host model for verified 2006 waypoint-pool routines.
// It does NOT implement the game's custom 0x2226EF40 allocator or native ABI.
#include <cstdint>
#include <functional>
#include <unordered_set>
#include <vector>

namespace sin_compat::phase570 {
struct Vec3 {float x=0.f,y=0.f,z=0.f;};
struct Point {
    Vec3 position{};
    std::uint32_t raw_0x0C=0;
    std::int32_t identity_0x10=-1;
    float cached_distance_0x14=-1.f;
    std::int32_t raw_0x18=-1,raw_0x1C=-1;
    std::uint32_t flags_0x20=0;
    std::uint32_t kind_0x24=0;
    Point* forward=nullptr;
    Point* backward=nullptr;
};
struct Holder { Point* first=nullptr; };
enum class Status { Ok, UnsupportedGraph, UnsupportedMatchedIdentity, UnsupportedAllocation };
struct Release { Status status; std::size_t node_count; };
inline bool valid_forward_chain(Point* first){
    std::unordered_set<Point*> seen;
    for(Point* p=first;p;p=p->forward){
        if(!seen.insert(p).second)return false;
        if(p->forward&&p->forward->backward!=p)return false;
    }
    return true;
}
inline Release release_forward_chain(Point* first,
                                    const std::function<void(Point*)>& pool_release){
    // Original 0x220B2C80 visits forward links +0x28 and delegates node
    // release to native pool 0x2226EF40. Host validates BEFORE mutating.
    if(!valid_forward_chain(first))return {Status::UnsupportedGraph,0};
    std::size_t n=0;
    for(Point* p=first;p;){
        Point* next=p->forward;
        if(next)next->backward=p->backward;
        if(p->backward)p->backward->forward=next;
        if(pool_release)pool_release(p);
        ++n;
        p=next;
    }
    return {Status::Ok,n};
}
inline Release release_holder(Holder& h,
                             const std::function<void(Point*)>& pool_release={}) {
    // 0x220B2CD0 wrapper invokes 0x220B2C80, then first=null.
    Release out=release_forward_chain(h.first,pool_release);
    if(out.status==Status::Ok)h.first=nullptr;
    return out;
}
inline Point* guarded_tail(Point* first){
    std::unordered_set<Point*> visited;
    for(Point* n=first;n;n=n->forward){
        if(!visited.insert(n).second)return nullptr;
        if(!n->forward)return n;
    }
    return nullptr;
}
inline Status splice_new_prefix(Point& prefix,Point* old_chain){
    // High-confidence *identity-mismatch* path of native 0x220B2B10.
    // The matching +0x10 identity branch performs additional detach,
    // native pool release and link substitutions, not yet modeled here.
    Point* prefix_tail=guarded_tail(&prefix);
    if(!prefix_tail)return Status::UnsupportedGraph;
    if(old_chain&&!valid_forward_chain(old_chain))return Status::UnsupportedGraph;
    if(old_chain&&prefix_tail->identity_0x10!=-1&&
       prefix_tail->identity_0x10==old_chain->identity_0x10)
        return Status::UnsupportedMatchedIdentity;
    // Native clears terminal bit 8 on prefix tail before splicing.
    prefix_tail->flags_0x20 &= ~8u;
    if(old_chain){
        if(old_chain->backward)old_chain->backward->forward=nullptr;
        old_chain->backward=prefix_tail;
    }
    prefix_tail->forward=old_chain;
    Point* last=guarded_tail(&prefix);
    if(!last)return Status::UnsupportedGraph;
    last->flags_0x20|=8u;
    return Status::Ok;
}
inline Status set_root_with_splice(Holder& h,Point& prefix){
    Status s=splice_new_prefix(prefix,h.first);
    if(s==Status::Ok)h.first=&prefix;
    return s;
}
inline Status construct_and_prepend(Holder& h,Point* freshly_allocated,
                                    Vec3 xyz,std::uint32_t kind,std::uint32_t flags){
    // Native 0x220B2BF0 obtains a 0x30-byte node from its game pool, then
    // creates zero/sentinel fields and calls 0x220B2B10(prefix,old-first).
    // An external host owns the supplied Point; null is a safe failure.
    if(!freshly_allocated)return Status::UnsupportedAllocation;
    Point& p=*freshly_allocated;
    p={};p.position=xyz;p.identity_0x10=-1;
    p.raw_0x18=-1;p.raw_0x1C=-1;p.raw_0x0C=0;
    p.cached_distance_0x14=-1.f;p.kind_0x24=kind;p.flags_0x20=flags;
    return set_root_with_splice(h,p);
}
} // namespace sin_compat::phase570