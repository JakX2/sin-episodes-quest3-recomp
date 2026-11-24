#pragma once
#include <cstdint>
#include <cmath>
namespace sin_port::native_weapon_tables {
inline constexpr char client_phoneme_emphasis_export[]="SiN_GetClientPhonemeEmphasisApi";
struct ClientEmphasisSample {float time,value;};
struct ClientPhonemeBankWeights {float weak,normal,strong;};
// Resolved contribution rows; the original bank lookup and local-to-global
// remap are outside this interface. Order within and across banks is retained.
struct ClientPhonemeContribution {unsigned global;float weight;};
struct ClientPhonemeBank {const ClientPhonemeContribution* rows;unsigned count;};
struct ClientPhonemeEmphasisApi {
    std::uint32_t version,size;
    int (__cdecl *curve)(const ClientEmphasisSample*,unsigned,float,float,float*) noexcept;
    int (__cdecl *blend)(float,bool,bool,ClientPhonemeBankWeights*) noexcept;
    int (__cdecl *accumulate)(float,const ClientPhonemeBankWeights*,const ClientPhonemeBank*,float*) noexcept;
};
using GetClientPhonemeEmphasisApi=const ClientPhonemeEmphasisApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline float client_emphasis_curve(const ClientEmphasisSample* rows,unsigned count,float time,float duration) noexcept {
    if(!count)return .5f;
    auto at=[&](int i)->ClientEmphasisSample {
        if(i<0)return {0,.5f};
        if(i>=static_cast<int>(count))return {duration,.5f};
        return rows[i];
    };
    int i=-1;
    for(;i<static_cast<int>(count);++i)
        if(time>=at(i).time&&time<at(i+1).time)break;
    const auto a=at(i-1),b=at(i),c=at(i+1),d=at(i+2);
    double dt=static_cast<double>(c.time)-b.time;
    if(dt>1)dt=1;
    if(dt<.01f)dt=.01f;
    float t=static_cast<float>((static_cast<double>(time)-b.time)/dt);
    if(t>1)t=1;
    if(t<0)t=0;
    const double x=t,x2=x*x,x3=x2*x;
    // Original 2416acd0 accumulates cubic, quadratic and linear vector
    // terms, storing intermediates to float; this is the scalar equivalent.
    const float y=static_cast<float>(.5*((-a.value+3.0*b.value-3.0*c.value+d.value)*x3+
        (2.0*a.value-5.0*b.value+4.0*c.value-d.value)*x2+
        (-a.value+c.value)*x+2.0*b.value));
    return y>1?1:y<0?0:y;
}
inline ClientPhonemeBankWeights client_emphasis_blend(float e,bool weak,bool strong) noexcept {
    ClientPhonemeBankWeights out{};
    if(e>.6f){
        if(strong){const double k=(1.0-e)*2.500000238418579;out.normal=static_cast<float>(1.2f*k);out.strong=static_cast<float>(1-k);}
        else out.normal=1.2f;
    }else if(e<.4f){
        if(weak){const double k=(static_cast<double>(.4f)-e)*2.5;out.normal=static_cast<float>(.8f*(1-k));out.weak=static_cast<float>(k);}
        else out.normal=.8f;
    }else out.normal=2*e;
    return out;
}
}
