#include "sin_compat/original_jessica_target_center.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace sin_compat::jessica;

bool close(float a,float b) { return std::fabs(a-b)<0.0001f; }
int main() {
    PlayerTargetObservation p;
    p.obb_mins={-16,-16,0};p.obb_maxs={16,16,72};
    p.absolute_origin={100,200,300};
    auto v=compute_player_world_collision_center(p);
    assert(v.valid() && v.branch==TargetCenterBranch::TranslateOnly);
    assert(close(v.world_center.x,100) && close(v.world_center.y,200) &&
           close(v.world_center.z,336));

    // Both uneven collision bounds and a translated origin matter.
    p.obb_mins={-20,-6,-12};p.obb_maxs={40,14,64};
    v=compute_player_world_collision_center(p);
    assert(v.valid()&&close(v.world_center.x,110)&&
           close(v.world_center.y,204)&&close(v.world_center.z,326));

    // The original vtable's +0x214 target is collision center, never a
    // guessed eye point; the toss geometry uses that same world target.
    const auto geo=build_original_jessica_toss_geometry({30,40,50},p,600);
    assert(geo&&close(geo->source.x,30)&&close(geo->target.x,110)&&
           close(geo->target.z,326)&&geo->world_gravity==600.0f);
    // Connect recovered player target position to the already-reconstructed
    // ballistic solver, then independently verify its projectile endpoint.
    const auto toss=solve_toss(*geo);
    assert(toss.valid());
    const double t=toss.flight_seconds;
    assert(std::fabs(geo->source.x+t*toss.velocity.x-geo->target.x)<0.02);
    assert(std::fabs(geo->source.y+t*toss.velocity.y-geo->target.y)<0.02);
    assert(std::fabs(geo->source.z+t*toss.velocity.z-0.5*geo->world_gravity*t*t-
                     geo->target.z)<0.02);


    // Entity with OBB and nonzero angles takes the world-matrix route.
    p.obb_mins={0,0,0};p.obb_maxs={20,0,10};
    p.absolute_angles={0,90,0};p.solid_type=3;p.solid_flags=0;
    p.entity_to_world=TargetTransform3x4{{
        0,-1,0,100,
        1,0,0,200,
        0,0,1,300
    }};
    v=compute_player_world_collision_center(p);
    assert(v.valid()&&v.branch==TargetCenterBranch::EntityMatrix);
    assert(close(v.world_center.x,100)&&close(v.world_center.y,210)&&
           close(v.world_center.z,305));

    // Original collision flag 0x40 disables rotation in this conversion.
    p.solid_flags=0x40;
    v=compute_player_world_collision_center(p);
    assert(v.valid()&&v.branch==TargetCenterBranch::TranslateOnly);
    assert(close(v.world_center.x,110)&&close(v.world_center.y,200)&&
           close(v.world_center.z,305));

    // Type 0 and type 2 also use translation even with angled entities.
    p.solid_flags=0;
    for (int type:{0,2}) {
        p.solid_type=type;
        v=compute_player_world_collision_center(p);
        assert(v.valid()&&v.branch==TargetCenterBranch::TranslateOnly);
    }
    // Rotation-free collision transforms also use translation.
    p.solid_type=3;p.absolute_angles={0,0,0};
    v=compute_player_world_collision_center(p);
    assert(v.valid()&&v.branch==TargetCenterBranch::TranslateOnly);

    // For rotated OBBs, the real engine must provide a valid matrix.
    p.absolute_angles={0,90,0};p.entity_to_world.reset();
    v=compute_player_world_collision_center(p);
    assert(v.code==TargetCenterCode::MissingWorldTransform);
    assert(!build_original_jessica_toss_geometry({30,40,50},p,600));
    p.entity_to_world=TargetTransform3x4{{0,-1,0,100,1,0,0,200,0,0,1,300}};
    auto inf=std::numeric_limits<float>::infinity();
    p.entity_to_world->row_major[11]=inf;
    assert(compute_player_world_collision_center(p).code==TargetCenterCode::InvalidGeometry);
    p.entity_to_world->row_major[11]=300;
    p.obb_maxs.x=std::numeric_limits<float>::quiet_NaN();
    assert(compute_player_world_collision_center(p).code==TargetCenterCode::InvalidGeometry);
    p.obb_maxs.x=20;
    assert(!build_original_jessica_toss_geometry({30,40,50},p,0.0f));
    assert(!build_original_jessica_toss_geometry({30,inf,50},p,600.0f));
    // Finite collision bounds can still yield an out-of-range world center.
    p.absolute_angles={0,0,0};p.absolute_origin={inf,0,0};
    assert(compute_player_world_collision_center(p).code==TargetCenterCode::InvalidGeometry);
    std::cout<<"Original Jessica world-collision-center reconstruction PASS\n";
}
