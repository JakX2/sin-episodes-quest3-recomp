#pragma once
#include "native_server_flex_animation.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_flex_spline_export[]="SiN_GetServerFlexSplineApi";
// Original 12-byte flex sample row: value, time, uninterpreted flags.
struct ServerFlexSample {float value,time;std::uint32_t flags;};
struct ServerFlexSplineApi {
    std::uint32_t version,size;
    // Original 2210d970 with parsed parent duration and bank supplied.
    // Returns normalized spline result, before 2210f9f0 range conversion.
    int (__cdecl *sample)(const ServerFlexSample*,unsigned,float,float,float,float,unsigned,float*) noexcept;
};
using GetServerFlexSplineApi=const ServerFlexSplineApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
namespace server_flex_spline_detail {
inline float evaluate(const ServerFlexSample* rows,unsigned count,float low,float high,float duration,float time,unsigned bank) noexcept {
    const float neutral=bank==1?.5f:low==high?0.f:static_cast<float>(-static_cast<double>(low)/(static_cast<double>(high)-low));
    if(!count)return neutral;
    auto at=[&](int i)->ServerFlexSample{
        if(i<0)return {neutral,0,0};
        if(i>=static_cast<int>(count))return {neutral,duration,0};
        return rows[i];
    };
    int i=-1;
    for(;i<static_cast<int>(count);++i)
        if(time>=at(i).time&&time<=at(i+1).time)break;
    auto a=at(i-1);const auto b=at(i),c=at(i+1);auto d=at(i+2);
    const double width=static_cast<double>(c.time)-b.time;
    float t=width>0?static_cast<float>((static_cast<double>(time)-b.time)/width):0.f;
    if(t>1)t=1;if(t<0)t=0;
    // 2226b720 reparameterizes outer tangents to the middle interval
    // width before 2226b210 Catmull-Rom, except equal-time neighbors.
    const float interval=static_cast<float>(width);
    if(interval!=0){
        if(a.time!=b.time)a.value=static_cast<float>((static_cast<double>(a.value)-b.value)*interval/(static_cast<double>(b.time)-a.time)+b.value);
        if(d.time!=c.time)d.value=static_cast<float>((static_cast<double>(d.value)-c.value)*interval/(static_cast<double>(d.time)-c.time)+c.value);
    }
    const double x=t,x2=x*x,x3=x2*x;
    const float y=static_cast<float>(.5*((-a.value+3.0*b.value-3.0*c.value+d.value)*x3+
        (2.0*a.value-5.0*b.value+4.0*c.value-d.value)*x2+(-a.value+c.value)*x+2.0*b.value));
    return y>1?1:y<0?0:y;
}
}
}
