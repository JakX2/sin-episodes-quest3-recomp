#pragma once
#include "native_server_expression.h"
#include <cassert>
#include <cstring>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_server_expression(const ServerExpressionApi& api){
    struct Fixture {unsigned char base[256]{},over[256]{};unsigned loads{},maps{},rng{};bool override_present=true;} f;
    auto make=[](unsigned char* b,const char* name,float value){
        auto put=[&](unsigned off,int x){std::memcpy(b+off,&x,4);};
        std::memcpy(b,"EFV",4);put(0x48,256);put(0x4c,1);put(0x50,108);
        put(0x60,1);put(0x64,156);put(0x68,160);
        put(108,64);put(116,2);put(128,24);put(132,0);put(144,0);
        std::memcpy(b+136,&value,4);std::memcpy(b+148,&value,4);
        put(156,192);put(160,-1);
        std::strcpy(reinterpret_cast<char*>(b+172),name);std::strcpy(reinterpret_cast<char*>(b+192),"jaw");
    };
    make(f.base,"smile",.2f);make(f.over,"SMILE",.4f);
    ServerFlexRange range{0,1};ServerExpressionBindings b{};b.context=&f;b.ranges=&range;b.count=1;b.character_stem="jessica";
    b.load=[](void* p,void*,const char* name,ServerExpressionBank* out) noexcept -> int{
        auto& x=*static_cast<Fixture*>(p);++x.loads;
        if(!std::strcmp(name,"emotion")){*out={x.base,sizeof(x.base)};return 1;}
        if(!std::strcmp(name,"jessica/emotion")&&x.override_present){*out={x.over,sizeof(x.over)};return 1;}
        return 0;
    };
    b.controller=[](void* p,void*,const char* n) noexcept -> int{++static_cast<Fixture*>(p)->maps;return !std::strcmp(n,"jaw")?0:-1;};
    b.intensity=[](void*,void*,float,float* out) noexcept -> int{*out=.5f;return 1;};
    b.random=[](void* p,int max,int* out) noexcept -> int{++static_cast<Fixture*>(p)->rng;*out=max;return 1;};
    alignas(4) unsigned char actor[server_flex_extent]{},record[0x40]{},event[0x3c0]{};
    float end=2;std::memcpy(event+0x188,&end,4);
    std::strcpy(reinterpret_cast<char*>(event+0x84),"emotion");
    std::strcpy(reinterpret_cast<char*>(event+0x104),"smile");
    actor[0x1f0]=1;
    auto value=[&](){float v=0;std::memcpy(&v,actor+0x4a8,4);return v;};
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),1,&b));
    assert(std::fabs(value()-.4f)<.00001f&&f.loads==2&&f.maps==2&&actor[0x200]&&!record[0xc]);
    f.override_present=false;record[0xc]=1;
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),1,&b));
    assert(std::fabs(value()-.6f)<.00001f&&f.maps==4);
    std::strcpy(reinterpret_cast<char*>(event+0x104),"missing");
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),1,&b)&&f.maps==4);
    end=-1;std::memcpy(event+0x188,&end,4);const unsigned loads=f.loads;
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),1,&b)&&f.loads==loads);
    // Synthetic type-1 alias: first weighted choice selects terminal setting.
    std::memset(f.base,0,sizeof(f.base));auto put=[&](unsigned off,int x){std::memcpy(f.base+off,&x,4);};
    std::memcpy(f.base,"EFV",4);put(0x48,256);put(0x4c,2);put(0x50,108);
    put(0x60,1);put(0x64,176);put(0x68,180);put(176,220);put(180,-1);
    put(108,80);put(112,1);put(116,1);put(124,-1);put(128,48);put(156,1);put(160,3);
    put(132,68);put(140,1);put(152,32);put(164,0);
    const float weight=.4f;std::memcpy(f.base+168,&weight,4);
    std::strcpy(reinterpret_cast<char*>(f.base+188),"alias");
    std::strcpy(reinterpret_cast<char*>(f.base+200),"smile");
    std::strcpy(reinterpret_cast<char*>(f.base+220),"jaw");
    end=2;std::memcpy(event+0x188,&end,4);record[0xc]=0;
    std::strcpy(reinterpret_cast<char*>(event+0x104),"alias");
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),1,&b)&&f.rng==1);
    assert(std::fabs(value()-.8f)<.00001f);
    record[0xc]=1;
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),1,&b)&&f.rng==1&&value()==1);
}
}
