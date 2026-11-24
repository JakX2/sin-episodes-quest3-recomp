#pragma once
#include <cstdint>
#include <cmath>
namespace sin_port::native_weapon_tables {
inline constexpr char server_event_intensity_export[]="SiN_GetServerEventIntensityApi";
struct ServerIntensitySample {float value,time;std::uint32_t flags;};
using ServerIntensityStorage=int (__cdecl *)(void*,std::uint32_t,void**,unsigned*) noexcept;
struct ServerEventIntensityApi {
    std::uint32_t version,size;
    // 221101d0 / 22115cd0: evaluate a nonempty ramp with zero endpoints.
    int (__cdecl *ramp)(const ServerIntensitySample*,unsigned,float,float,float*) noexcept;
    // 22110f80: event-relative ramp * owning scene absolute ramp.
    int (__cdecl *event)(const void*,unsigned,float,void*,ServerIntensityStorage,float*) noexcept;
};
using GetServerEventIntensityApi=const ServerEventIntensityApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
namespace server_intensity_detail {
inline float ramp(const ServerIntensitySample* rows,unsigned count,float duration,float time) noexcept {
    if(!count)return 1;
    auto at=[&](int i)->ServerIntensitySample{
        if(i<0)return {0,0,0};
        if(i>=static_cast<int>(count))return {0,duration,0};
        return rows[i];
    };
    int i=-1;
    for(;i<static_cast<int>(count);++i)
        if(time>=at(i).time&&time<=at(i+1).time)break;
    const auto a=at(i-1),b=at(i),c=at(i+1),d=at(i+2);
    const double width=static_cast<double>(c.time)-b.time;
    float t=width>0?static_cast<float>((static_cast<double>(time)-b.time)/width):0;
    if(t>1)t=1;if(t<0)t=0;
    // 2226b630 normalizes the two outer vectors to the middle
    // segment's Euclidean length. Both callers zero the fourth x.
    auto length=[](double x,double y){return std::sqrt(x*x+y*y);};
    const float middle=static_cast<float>(length(width,static_cast<double>(c.value)-b.value));
    auto scale=[&](float x,float y,const ServerIntensitySample& p){
        const float dx=static_cast<float>(static_cast<double>(x)-p.time);
        const float dy=static_cast<float>(static_cast<double>(y)-p.value);
        const float len=static_cast<float>(length(dx,dy));
        return len>0?static_cast<float>(static_cast<double>(p.value)+static_cast<double>(dy/len)*middle):p.value;
    };
    const float av=scale(a.time,a.value,b),dv=scale(0,d.value,c);
    const double x=t,x2=x*x,x3=x2*x;
    const float y=static_cast<float>(.5*((-av+3.0*b.value-3.0*c.value+dv)*x3+
        (2.0*av-5.0*b.value+4.0*c.value-dv)*x2+(-av+c.value)*x+2.0*b.value));
    return y>1?1:y<0?0:y;
}
}
}
