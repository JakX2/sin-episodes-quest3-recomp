#pragma once
#include "native_client_facial_frame.h"
#include <cassert>
#include <cstring>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_client_facial_frame(const ClientFacialFrameApi& api){
    alignas(4) unsigned char entity[client_facial_object_bytes]{};
    auto put=[&](unsigned off,auto value){std::memcpy(entity+off,&value,sizeof(value));};
    auto word=[&](unsigned off){std::uint32_t x=0;std::memcpy(&x,entity+off,4);return x;};
    auto fl=[&](unsigned off){float x=0;std::memcpy(&x,entity+off,4);return x;};
    put(0x860,.25f);put(0x864,1.5f);put(0x868,.75f);put(0xac0,1u);put(0xacc,-1);
    unsigned lookups=0;
    ClientFacialFrameBindings b{};b.context=&lookups;
    b.lookup=[](void* p,const char* name,int* out) noexcept -> int{
        assert(!std::strcmp(name,"blink"));++*static_cast<unsigned*>(p);*out=9;return 1;};
    ClientFacialController rows[]={{3,-1,1},{5,0,2},{7,0,1}};
    float weights[128];for(auto& w:weights)w=99;
    assert(api.project(entity,sizeof(entity),nullptr,0,10,.5f,&b,weights)==1);
    for(float w:weights)assert(w==0);
    assert(!lookups&&word(0xacc)==0xffffffffu&&!word(0xac8));
    assert(api.project(entity,sizeof(entity),rows,3,10,.5f,&b,weights)==1);
    assert(lookups==1&&word(0xacc)==9&&word(0xac8)==1&&fl(0xac4)==10.5f);
    assert(weights[3]==-.5f&&weights[5]==3.f&&weights[7]==.75f&&weights[8]==0&&std::fabs(weights[9])<.0001f);
    assert(api.project(entity,sizeof(entity),rows,3,10.125f,.5f,&b,weights)==1&&lookups==1&&std::fabs(weights[9]-.7627718f)<.0001f);
    assert(api.project(entity,sizeof(entity),rows,3,10.25f,.5f,&b,weights)==1&&std::fabs(weights[9]-.3182072f)<.0001f);
    assert(api.project(entity,sizeof(entity),rows,3,10.5f,.5f,&b,weights)==1&&weights[9]==0);
    put(0xac0,0u);
    assert(api.project(entity,sizeof(entity),rows,3,11,.5f,&b,weights)==1&&word(0xac8)==0&&fl(0xac4)==11.5f);
    rows[2].global=128;weights[0]=7;
    assert(!api.project(entity,sizeof(entity),rows,3,12,.5f,&b,weights)&&weights[0]==7&&fl(0xac4)==11.5f);
    rows[2].global=3;
    assert(api.project(entity,sizeof(entity),rows,3,12,.5f,&b,weights)==1&&weights[3]==.75f);
    assert(!api.project(entity,sizeof(entity)-1,rows,3,12,.5f,&b,weights));
    float previous[]={1.f,0.f},current[]={0.f,1.f};
    assert(api.smooth(previous,current,2,0)==1&&previous[0]==1&&previous[1]==0);
    assert(api.smooth(previous,current,2,.033f)==1&&std::fabs(previous[0]-.8f)<.0001f&&std::fabs(previous[1]-.2f)<.0001f);
    assert(api.smooth(previous,current,2,.033f)==1&&std::fabs(previous[0]-.64f)<.0001f&&std::fabs(previous[1]-.36f)<.0001f);
    assert(!api.smooth(previous,current,129,.033f));
}
}
