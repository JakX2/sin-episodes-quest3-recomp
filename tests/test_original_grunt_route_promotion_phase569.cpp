#include "sin_compat/original_grunt_route_promotion_phase569.hpp"
#include <cassert>
using namespace sin_compat::phase569;

int main(){
    Navigator n;
    assert(promote_candidate(n,1.f)==TransferResult::NoCandidate);
    assert(n.active.first==nullptr);
    // No active route means original refresh virtual cannot be consulted.
    int called=0;
    auto callback=[&called](Navigator& ctx){++called;ctx.candidate.first=nullptr;return true;};
    assert(promote_candidate(n,2.f,true,callback)==TransferResult::NoCandidate);
    assert(called==0);
    Waypoint live;live.dimensionality=2;
    n.active.first=&live;
    assert(promote_candidate(n,3.f,false,callback)==TransferResult::NoCandidate);
    assert(called==0 && n.candidate_stamp==-1.f);
    assert(promote_candidate(n,4.f,true,callback)==TransferResult::NoCandidate);
    assert(called==1 && n.candidate_stamp==4.f);
    // The original 0x22075480 uses type IDs 1 or 3 to skip debug gate.
    live.dimensionality=3;
    assert(promote_candidate(n,5.f,false,callback)==TransferResult::NoCandidate);
    assert(called==2 && n.candidate_stamp==5.f);
    Waypoint a; a.position={4,5,6};
    Waypoint b; b.position={9,11,15};
    a.forward=&b;b.backward=&a;
    n.candidate.first=&a;
    n.active.origin={100,200,300};
    n.active.state_0x44=7u;
    n.extra_0x30=0xABCDu;
    n.extra_0x34=0x13u;
    const auto old_clears=n.holder_clear_count;
    assert(promote_candidate(n,123.f)==TransferResult::Installed);
    assert(n.active.first==&a && n.candidate.first==nullptr);
    assert(n.holder_clear_count==old_clears+1); // existing active list cleared
    assert((b.flags&8u)!=0);
    assert(sin_compat::phase568::is_original_invalid_cache(b.cached_length));
    assert(n.active.endpoint_valid_0x30);
    assert(n.active.endpoint.x==109.f && n.active.endpoint.y==211.f && n.active.endpoint.z==315.f);
    assert(n.active.state_0x44==4u);
    // set 7->0 sets stamp and byte; set 0->4 clears byte (leaves stamp).
    assert(!n.active.state_changed_0x40 && n.active.state_transition_time_0x4c==123.f);
    assert(n.active_raw_0x08==0xABCDu);
    assert(n.active_raw_0x24==0x13u && n.active_raw_0x28==0xFFFFFFFFu);
    assert(n.active.raw_0x2c==0xffffffffu);
    assert(n.candidate_stamp==5.f); // direct path does not retimestamp
    // 0x2209D000 (no origin addition) differs from 0x2209D040.
    Route distinct;
    distinct.origin={100,100,100};
    distinct.first=&a;
    a.forward=&b;b.backward=&a;b.flags=0;
    mark_terminal_from_last(distinct);
    assert(distinct.endpoint.x==9.f && distinct.endpoint.y==11.f && distinct.endpoint.z==15.f);
    endpoint_as_origin_plus(distinct,{1,2,3});
    assert(distinct.endpoint.x==101.f && distinct.endpoint.y==102.f && distinct.endpoint.z==103.f);
    // Original +0x10 handle resolver: low 12 bits and serial generation.
    int obj=5;
    std::vector<EntitySlot> slots(3);
    slots[2]={&obj,6u};
    assert(resolve_nav_handle((6u<<12)|2u,slots)==&obj);
    assert(resolve_nav_handle((5u<<12)|2u,slots)==nullptr);
    assert(resolve_nav_handle(0xffffffffu,slots)==nullptr);
    assert(resolve_nav_handle((6u<<12)|7u,slots)==nullptr); // host bounds
    // Original transfer has no candidate-validity guard; safe model does.
    Navigator cyclic;
    Waypoint cycle;cycle.forward=&cycle;
    cyclic.candidate.first=&cycle;
    assert(promote_candidate(cyclic,0.f)==TransferResult::UnsupportedGraph);
    return 0;
}