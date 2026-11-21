#include "sin_compat/original_grunt_waypoint_pool_phase570.hpp"
#include <cassert>
#include <vector>
using namespace sin_compat::phase570;
int main(){
    Holder h;
    std::vector<Point*> released;
    auto callback=[&released](Point* p){released.push_back(p);};
    auto empty=release_holder(h,callback);
    assert(empty.status==Status::Ok&&empty.node_count==0&&h.first==nullptr);
    Point a,b,c;
    assert(construct_and_prepend(h,&a,{1.f,2.f,3.f},1u,0u)==Status::Ok);
    assert(h.first==&a && a.forward==nullptr && (a.flags_0x20&8u));
    assert(a.identity_0x10==-1 && a.raw_0x18==-1 && a.raw_0x1C==-1);
    assert(a.cached_distance_0x14==-1.f);
    // Native node pool alloc/prepend: new prefix attaches old list as successor.
    assert(construct_and_prepend(h,&b,{4.f,5.f,6.f},3u,0u)==Status::Ok);
    assert(h.first==&b && b.forward==&a && a.backward==&b);
    assert((a.flags_0x20&8u)!=0u && (b.flags_0x20&8u)==0u);
    assert(construct_and_prepend(h,&c,{7.f,8.f,9.f},3u,0u)==Status::Ok);
    assert(h.first==&c && c.forward==&b && b.forward==&a);
    assert(valid_forward_chain(h.first));
    // Wrapper must iterate from first to last, repairing links before
    // each node reaches the original custom pool release callback.
    auto out=release_holder(h,callback);
    assert(out.status==Status::Ok && out.node_count==3);
    assert(h.first==nullptr);
    assert((released==std::vector<Point*>{&c,&b,&a}));
    // Original identity-equality branch has different splice/free behavior;
    // safe host MUST NOT claim it was faithfully reconstructed.
    Holder matched;
    Point d,e;
    d.identity_0x10=100;e.identity_0x10=100;
    matched.first=&e;
    assert(set_root_with_splice(matched,d)==Status::UnsupportedMatchedIdentity);
    assert(matched.first==&e && d.forward==nullptr);
    // Native allocator can fail; host refuses instead of reproducing unsafe
    // potential null dereferences in the original failure branch.
    assert(construct_and_prepend(matched,nullptr,{},0u,0u)==Status::UnsupportedAllocation);
    // Cyclic or malformed route data is outside original proven preconditions.
    Point cycle;cycle.forward=&cycle;
    Holder malformed{&cycle};
    assert(release_holder(malformed,callback).status==Status::UnsupportedGraph);
    assert(malformed.first==&cycle);
    return 0;
}