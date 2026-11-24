#pragma once
#include "native_server_flex_spline.h"
#include <cassert>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_server_flex_spline(const ServerFlexSplineApi& api){
    float out=-99;
    auto approx=[](float a,float b){return std::fabs(a-b)<.00001f;};
    assert(api.sample(nullptr,0,-1,1,1,.5f,0,&out)&&out==.5f);
    assert(api.sample(nullptr,0,1,2,1,.5f,0,&out)&&out==-1); // empty neutral is NOT clamped
    assert(api.sample(nullptr,0,-1,1,1,.5f,1,&out)&&out==.5f);
    ServerFlexSample rows[]={{.2f,.2f,0},{.8f,.6f,123},{.2f,.8f,0}};
    assert(api.sample(rows,3,0,1,1,.4f,0,&out)&&approx(out,.6f)); // nonuniform outer tangents
    assert(api.sample(rows,3,0,1,1,.2f,0,&out)&&approx(out,.2f));
    assert(api.sample(rows,3,0,1,1,.6f,0,&out)&&approx(out,.8f));
    assert(api.sample(rows,3,0,1,1,1.f,0,&out)&&out==0);
    ServerFlexSample dup[]={{1,0,0},{.2f,.5f,0},{.9f,.5f,0}};
    assert(api.sample(dup,3,0,1,1,0,0,&out)&&out==0); // first closed interval is zero length
    assert(api.sample(dup,3,0,1,1,.5f,0,&out)&&approx(out,.2f)); // first matching interval
    ServerFlexSample overshoot[]={{2,.5f,0}};
    assert(api.sample(overshoot,1,0,1,1,.5f,0,&out)&&out==1);
    overshoot[0].value=-2;
    assert(api.sample(overshoot,1,0,1,1,.5f,0,&out)&&out==0);
    rows[1].time=.1f;out=77;
    assert(!api.sample(rows,3,0,1,1,.4f,0,&out)&&out==77);
    assert(!api.sample(rows,3,0,1,1,.4f,2,&out)&&out==77);
}
}
