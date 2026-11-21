#include "sin_compat/original_grunt_flank_path_phase566.hpp"
#include <cassert>
#include <cmath>
#include <limits>
using namespace sin_compat::phase566;
static bool near(float a,float b){return std::fabs(a-b)<0.0001f;}
int main() {
    // Native 0x223D2590 empty-route fallback zero; no flank request.
    auto empty=get_length(Route{});
    assert(empty.outcome==Outcome::Ok && empty.value==0.f);
    assert(!flank_bark_if_original_metric_eligible(empty));

    // Three waypoints, reverse links and forward links are independent.
    // XY: 140+100=240 -> original branch is STRICTLY greater, so no bark.
    Waypoint a{},b{},c{};
    a.position={0,0,0}; b.position={140,0,0}; c.position={140,100,80};
    a.forward=&b; b.backward=&a; b.forward=&c; c.backward=&b;
    c.original_distance_mode=0; b.original_distance_mode=0;
    auto exact=get_length({&a});
    assert(exact.outcome==Outcome::Ok && near(exact.value,240.f));
    assert(!flank_bark_if_original_metric_eligible(exact));
    assert(near(a.cached_cumulative,240.f));
    assert(near(b.cached_cumulative,100.f));
    assert(near(c.cached_cumulative,0.f));

    // Last node +0x14 non-sentinel means no recalc, so host returns stale
    // first node cache (original implementation also trusts cached data).
    a.cached_cumulative=301.f; c.cached_cumulative=9.f;
    auto stale=get_length({&a});
    assert(stale.outcome==Outcome::Ok && stale.value==301.f);
    assert(flank_bark_if_original_metric_eligible(stale));

    // Re-enable invalidation and use 3D only on the 2nd segment:
    // sqrt(100^2 + 80^2)+140 ~=268.06 >240 -> flank bark eligible.
    c.cached_cumulative=-1.f; c.original_distance_mode=1;
    auto xyz=get_length({&a});
    assert(xyz.outcome==Outcome::Ok && near(xyz.value,268.0624847f));
    assert(flank_bark_if_original_metric_eligible(xyz));

    // Linked chain with only one node recaches to zero.
    Waypoint solo{};
    auto one=get_length({&solo});
    assert(one.outcome==Outcome::Ok && one.value==0.f);

    // Original offers no cycle/malformed graph guard. Safe independent
    // model rejects rather than inventing finite behavior for such input.
    Waypoint bad1{},bad2{};bad1.forward=&bad2;bad2.forward=&bad1;
    assert(get_length({&bad1}).outcome==Outcome::UnsupportedGraph);
    Waypoint broken1{},broken2{};broken1.forward=&broken2;
    assert(get_length({&broken1}).outcome==Outcome::UnsupportedGraph);

    // No claim to original x87 exception/NaN results in the C++ host.
    Waypoint nanpoint{};nanpoint.position.x=std::numeric_limits<float>::quiet_NaN();
    assert(get_length({&nanpoint}).outcome==Outcome::UnsupportedFloat);
}