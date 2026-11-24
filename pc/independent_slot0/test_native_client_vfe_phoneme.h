#pragma once
#include "native_client_vfe_phoneme.h"
#include "native_client_facial_frame.h"
#include <cassert>
#include <cstring>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_client_vfe_phoneme(const ClientVfePhonemeApi& api){
    unsigned char generic[256]{},character[256]{},weak[256]{};
    auto make=[](unsigned char* b,const char* name,float weight){
        auto put=[&](unsigned o,int v){std::memcpy(b+o,&v,4);};
        std::memcpy(b,"EFV",4);
        put(0x48,256);put(0x4c,1);put(0x50,108);put(0x58,2);put(0x5c,144);
        put(0x60,1);put(0x64,152);put(0x68,156);
        put(108,52);put(116,1);put(128,24);
        put(132,0);std::memcpy(b+136,&weight,4);
        put(144,-1);put(148,0);put(152,180);put(156,-1);
        std::strcpy(reinterpret_cast<char*>(b+160),name);
        std::strcpy(reinterpret_cast<char*>(b+180),"jaw");
    };
    make(generic,"phoneme",.25f);make(character,"PHONEME",.75f);make(weak,"phoneme",.5f);
    struct Context {
        ClientPhonemeInterval interval{0,1,1,0};
        ClientEmphasisSample emphasis{.25f,.2f};
        unsigned maps{};
    } ctx;
    ClientVfePhonemeBindings b{};
    b.context=&ctx;
    b.resolve=[](void* p,std::uint32_t,ClientPhonemeSentence* out) noexcept -> int{
        auto& c=*static_cast<Context*>(p);
        *out={1,&c.interval,1,&c.emphasis,1};return 1;
    };
    b.map=[](void* p,const char* n) noexcept -> int{
        ++static_cast<Context*>(p)->maps;return std::strcmp(n,"jaw")==0?9:-1;
    };
    b.banks[0].generic={weak,sizeof(weak)};
    b.banks[1]={{generic,sizeof(generic)},{character,sizeof(character)}};
    alignas(4) unsigned char entity[client_facial_object_bytes]{};
    auto put=[&](unsigned off,auto v){std::memcpy(entity+off,&v,sizeof(v));};
    put(client_phoneme_source_offset,.25f);put(client_phoneme_source_offset+4,1u);
    put(client_phoneme_source_count_offset,std::int16_t(1));
    float weights[128]{};weights[9]=.1f;
    assert(api.apply(entity,sizeof(entity),0,.1f,false,&b,weights));
    // Emphasis .2 => weak .5 / normal .4; .1 + .5*.5 + .75*.4 = .65.
    assert(std::fabs(weights[9]-.65f)<.00001f&&ctx.maps==2);
    b.banks[1].generic={};ctx.maps=0;
    assert(api.apply(entity,sizeof(entity),0,.1f,false,&b,weights)&&ctx.maps==0);
    assert(std::fabs(weights[9]-.65f)<.00001f);
    b.banks[1].generic={generic,sizeof(generic)};
    ctx.interval.code=0;
    assert(api.apply(entity,sizeof(entity),0,.1f,false,&b,weights)&&ctx.maps==0);
    ctx.interval.code=1;
    int bad=99;std::memcpy(character+132,&bad,4);
    assert(!api.apply(entity,sizeof(entity),0,.1f,false,&b,weights));
    assert(std::fabs(weights[9]-.65f)<.00001f);
}
}
