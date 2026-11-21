#include "sin_compat/original_grunt_route_mutation_phase568.hpp"
#include <cassert>
#include <cmath>
using namespace sin_compat::phase568;
namespace { bool almost(float x,float y) { return std::fabs(x-y)<1.0e-4f; } }
int main(){
    Route r{};
    assert(extension_distance(r,{0,0,0}).status==Result::Ok);
    assert(extension_distance(r,{0,0,0}).value==0.0f);
    // Native state transition tests OLD flag, not the requested new value.
    set_route_state(r,7,123.f);
    assert(r.state_0x44==7 && !r.state_changed_0x40);
    set_route_state(r,0,124.f);
    assert(r.state_0x44==0 && r.state_changed_0x40);
    assert(r.state_transition_time_0x4c==124.f);
    set_route_state(r,0,125.f);
    assert(!r.state_changed_0x40 && r.state_transition_time_0x4c==124.f);
    Waypoint a; a.position={0,0,0};a.raw_0x10=0xA5;
    Waypoint b; b.position={100,0,100};b.dimensionality=0;
    a.forward=&b;b.backward=&a;
    r.origin={2,3,4};
    assert(set_first(r,&a,false)==Result::Ok);
    assert(r.first==&a && is_original_invalid_cache(b.cached_length));
    // The last node uses XY, so route length is exactly 100.
    auto scalar=extension_distance(r,{-30,0,50});
    assert(scalar.status==Result::Ok && almost(scalar.value,130));
    assert(almost(a.cached_length,100) && b.cached_length==0);
    // Extension segment uses FIRST node's mode. Switching it to 3D makes
    // extension sqrt(30^2+50^2) even though route segment remains XY.
    a.dimensionality=1;
    scalar=extension_distance(r,{-30,0,50});
    assert(scalar.status==Result::Ok && almost(scalar.value,100+std::sqrt(3400.0f)));
    // A valid terminal cache can leave first cached_length stale by design.
    a.cached_length=42.f;b.cached_length=1.f;
    scalar=extension_distance(r,{-30,0,50});
    assert(almost(scalar.value,42+std::sqrt(3400.f)));
    r.raw_0x2c=0;
    a.flags|=4;
    assert(advance_first(r)==Result::Ok);
    assert(r.first==&b && b.backward==nullptr && r.raw_0x2c==0xA5);
    assert(is_original_invalid_cache(b.cached_length));
    assert(advance_first(r)==Result::TerminalMarked);
    assert((b.flags&8)!=0);
    assert(advance_first(r)==Result::NoChange);
    // Setting first with terminal bit models native +0x30 and endpoint
    // computation from route base vector and terminal waypoint.
    b.flags=0;b.cached_length=1.f;
    assert(set_first(r,&b,true)==Result::Ok);
    assert(r.endpoint_valid_0x30 && (b.flags&8) && is_original_invalid_cache(b.cached_length));
    assert(almost(r.endpoint.x,102) && almost(r.endpoint.y,3) && almost(r.endpoint.z,104));
    Waypoint c; c.position={250,0,0};
    Waypoint d; d.position={260,0,0};
    c.forward=&d;d.backward=&c;
    unlink_neighbors(c);
    assert(d.backward==nullptr); // unlink helper does not change c's forward
    assert(c.forward==&d);
    Route empty;
    assert(advance_first(empty)==Result::UnsupportedGraph); // native precondition
    Waypoint cycle;cycle.forward=&cycle;
    assert(set_first(empty,&cycle,false)==Result::UnsupportedGraph); // host guard
    return 0;
}