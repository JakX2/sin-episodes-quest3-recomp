#pragma once
#include "native_client_phoneme_source.h"
#include "native_client_facial_frame.h"
#include <cassert>
#include <cstring>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_client_phoneme_source(const ClientPhonemeSourceApi& api){
    struct Context {
        unsigned resolves{},lookups{};
        ClientPhonemeInterval interval{0,1,12,0};
        ClientPhonemeContribution normal{5,.5f};
        bool invalid{};
    } ctx;
    ClientPhonemeSourceBindings b{};
    b.context=&ctx;
    b.resolve=[](void* p,std::uint32_t h,ClientPhonemeSentence* out) noexcept -> int{
        auto& x=*static_cast<Context*>(p);++x.resolves;
        if(h==0)return 0;
        *out={1,&x.interval,1,nullptr,0};return 1;
    };
    b.lookup=[](void* p,std::uint16_t c,ClientPhonemeResolvedBanks* out) noexcept -> int{
        auto& x=*static_cast<Context*>(p);++x.lookups;assert(c==12);
        if(x.invalid)return -1;
        out->banks[1]={&x.normal,1};return 1;
    };
    alignas(4) unsigned char entity[client_facial_object_bytes]{};
    auto put=[&](unsigned off,auto value){std::memcpy(entity+off,&value,sizeof(value));};
    auto source=[&](unsigned i,float elapsed,std::uint32_t handle){
        put(client_phoneme_source_offset+i*8,elapsed);put(client_phoneme_source_offset+i*8+4,handle);
    };
    put(client_phoneme_source_count_offset,std::int16_t(4));
    source(0,.4f,1);source(1,3.f,2);source(2,.4f,0);source(3,.4f,3);
    float weights[128]{};weights[5]=.25f;weights[9]=.75f;
    assert(api.apply(entity,sizeof(entity),.2f,.1f,false,&b,weights));
    assert(ctx.resolves==4&&ctx.lookups==2&&std::fabs(weights[5]-1.25f)<.00001f&&weights[9]==.75f);
    ctx.invalid=true;
    assert(!api.apply(entity,sizeof(entity),.2f,.1f,false,&b,weights)&&weights[5]==1.25f);
    ctx.invalid=false;ctx.lookups=0;
    source(0,.1f,1);source(3,.1f,3); // adjusted time -.1; window ends at start
    assert(api.apply(entity,sizeof(entity),.2f,.05f,false,&b,weights)&&!ctx.lookups&&weights[5]==1.25f);
    put(client_phoneme_source_count_offset,std::int16_t(5));
    assert(!api.apply(entity,sizeof(entity),0,.1f,false,&b,weights));
    put(client_phoneme_source_count_offset,std::int16_t(-1));
    assert(api.apply(entity,sizeof(entity),0,.1f,false,&b,weights)&&weights[5]==1.25f);
}
}
