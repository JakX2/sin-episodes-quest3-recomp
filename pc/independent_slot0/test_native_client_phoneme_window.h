#pragma once
#include "native_client_phoneme_window.h"
#include <cassert>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_client_phoneme_window(const ClientPhonemeWindowApi& api){
    struct Sink {unsigned n;std::uint16_t codes[8];float weights[8];};
    Sink s{};
    auto emit=[](void* p,std::uint16_t c,float w) noexcept {
        auto& x=*static_cast<Sink*>(p);assert(x.n<8);x.codes[x.n]=c;x.weights[x.n++]=w;
    };
    auto near=[](float a,float b){return std::fabs(a-b)<.00001f;};
    ClientPhonemeInterval rows[]={{0,.4f,12,0},{.4f,.8f,32,0}};
    float window=.1f;
    assert(api.sample(rows,2,.2f,&window,true,&s,emit));
    assert(near(window,.4f)&&s.n==2&&s.codes[0]==12&&s.codes[1]==32&&near(s.weights[0],.5f)&&near(s.weights[1],.5f));
    s={};window=.1f;
    assert(api.sample(rows,2,.2f,&window,false,&s,emit));
    assert(near(window,.1f)&&s.n==1&&s.codes[0]==12&&near(s.weights[0],1));
    s={};window=.1f;
    assert(api.sample(rows,2,.4f,&window,true,&s,emit));
    assert(near(window,.1f)&&s.n==1&&s.codes[0]==32&&near(s.weights[0],1));
    s={};window=.1f;
    assert(api.sample(rows,2,.8f,&window,true,&s,emit)&&!s.n);
    s={};window=.5f;
    assert(api.sample(rows,2,.2f,&window,true,&s,emit));
    assert(near(window,.5f)&&s.n==2&&near(s.weights[0],.4f)&&near(s.weights[1],.6f));
    s={};window=.1f;
    assert(api.sample(nullptr,0,0,&window,true,&s,emit)&&!s.n&&near(window,.1f));
    assert(!api.sample(nullptr,2,0,&window,true,&s,emit)&&!s.n);
    window=0;
    assert(!api.sample(rows,2,0,&window,true,&s,emit)&&!s.n);
}
}
