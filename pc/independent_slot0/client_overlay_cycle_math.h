#pragma once
#include <cmath>
namespace sin_port::overlay_cycle_math {
inline bool wrap(double v,float& out) noexcept {
    if(!std::isfinite(v)||v<-2147483648.||v>=2147483648.)return false;
    v-=std::trunc(v);if(v<0.)v+=1.;
    out=static_cast<float>(v);return true;
}
inline bool linear(float t,float from,float to,float& out) noexcept {
    if(!std::isfinite(t)||!std::isfinite(from)||!std::isfinite(to))return false;
    double a=from,b=to;
    if(std::fabs(b-a)>=.5){if(a<b)a+=1.;else b+=1.;}
    return wrap(a*(1.-t)+b*t,out);
}
inline double hermite(double a,double b,double c,double t) noexcept {
    const double t2=t*t,t3=t2*t;
    return (2*t3-3*t2+1)*b+(3*t2-2*t3)*c+(t3-2*t2+t)*(b-a)+(t3-t2)*(c-b);
}
inline bool cubic(float t,float earlier,float older,float newer,bool looping,float& out) noexcept {
    if(!std::isfinite(t)||!std::isfinite(earlier)||!std::isfinite(older)||!std::isfinite(newer))return false;
    double a=earlier,b=older,c=newer;
    if(looping){
        if(std::fabs(a-b)>.5){if(a<b)a=static_cast<float>(a+1.);else b+=1.;}
        if(std::fabs(c-b)>.5){if(b<c)b+=1.;else c=static_cast<float>(c+1.);}
        return wrap(static_cast<float>(hermite(a,b,c,t)),out);
    }
    out=static_cast<float>(hermite(a,b,c,t));return std::isfinite(out);
}
}
