#include "sin_compat/original_jessica_toss.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace sin_compat::jessica;
bool near(double a,double b,double tol=0.002) {return std::abs(a-b)<tol;}
void check(const TossGeometry& g) {
    const TossResult v=solve_toss(g);assert(v.valid());
    const double t=v.flight_seconds;
    assert(near(g.source.x+v.velocity.x*t,g.target.x));
    assert(near(g.source.y+v.velocity.y*t,g.target.y));
    assert(near(g.source.z+v.velocity.z*t-0.5*g.world_gravity*t*t,g.target.z));
    const double up_t=v.velocity.z/g.world_gravity;
    assert(near(v.velocity.z*up_t-0.5*g.world_gravity*up_t*up_t,v.apex_above_source));
}
int main() {
    TossGeometry g{{0,0,0},{100,0,0},600};
    check(g);auto v=solve_toss(g);
    assert(near(v.velocity.x,223.606798));
    assert(near(v.velocity.z,134.164079));
    assert(near(v.flight_seconds,0.4472136));
    assert(near(v.apex_above_source,15));
    g.target={0,100,30};check(g);auto uphill=solve_toss(g);
    assert(near(uphill.apex_above_source,45));
    g.target={0,100,-30};check(g);auto downhill=solve_toss(g);
    assert(near(downhill.apex_above_source,15));
    assert(near(uphill.velocity.y,downhill.velocity.y));
    assert(uphill.velocity.z>downhill.velocity.z);
    g.source={100,200,300};g.target={100,200,315};check(g);
    assert(near(solve_toss(g).velocity.x,0));
    assert(near(solve_toss(g).velocity.y,0));
    g.target=g.source;
    assert(solve_toss(g,0).code==TossCode::Degenerate);
    g.world_gravity=0;assert(solve_toss(g).code==TossCode::InvalidInput);
    g.world_gravity=-1;assert(solve_toss(g).code==TossCode::InvalidInput);
    g.world_gravity=600;
    g.target.x=std::numeric_limits<float>::quiet_NaN();
    assert(solve_toss(g).code==TossCode::InvalidInput);
    g.target={100,200,300};
    assert(solve_toss(g,-1).code==TossCode::InvalidInput);
    std::cout<<"Original Jessica ballistic toss reference tests PASS\n";
}
