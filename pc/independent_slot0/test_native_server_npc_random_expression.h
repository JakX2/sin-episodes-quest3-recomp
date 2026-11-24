#pragma once
#include "native_server_npc_random_expression.h"
#include <cassert>
#include <cstring>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_server_npc_random_expression(const ServerNpcRandomExpressionApi& api){
    struct Fixture {alignas(4) unsigned char bank[256]{};unsigned loads{},draws{},divisors{},maps{};bool absent{};} f;
    auto put=[](void* p,unsigned off,auto v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));};
    std::memcpy(f.bank,"EFV",4);
    put(f.bank,0x48,256);put(f.bank,0x4c,1);put(f.bank,0x50,108);
    put(f.bank,0x60,1);put(f.bank,0x64,152);put(f.bank,0x68,156);
    put(f.bank,108,52);put(f.bank,116,1);put(f.bank,128,24);
    put(f.bank,132,0);put(f.bank,136,.8f);put(f.bank,152,180);put(f.bank,156,-1);
    std::strcpy(reinterpret_cast<char*>(f.bank+160),"smile");
    std::strcpy(reinterpret_cast<char*>(f.bank+180),"jaw");
    alignas(4) unsigned char actor[server_npc_random_expression_extent]{},record[0x40]{},event[0x3c0]{};
    actor[0x1f0]=1;put(actor,0x4a8,.2f);put(actor,0x4ac,.4f);put(actor,0xf00,.4f);
    put(record,0x20,1.f);put(record,0x3c,0.f);
    put(event,0x184,0.f);put(event,0x188,30.f);
    std::strcpy(reinterpret_cast<char*>(event+0x104),"missing");
    ServerFlexRange ranges[]={{0,1},{0,1}};
    ServerNpcRandomExpressionBindings b{};b.context=&f;b.ranges=ranges;b.count=2;
    b.load=[](void* p,void*,const char* name,ServerExpressionBank* out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.loads;
        if(f.absent||std::strcmp(name,"random"))return 0;
        *out={f.bank,sizeof(f.bank)};return 1;
    };
    b.controller=[](void* p,void*,const char* name) noexcept -> int{
        ++static_cast<Fixture*>(p)->maps;return std::strcmp(name,"jaw")?-1:0;
    };
    b.random_int=[](void* p,int lo,int hi,int* out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(lo==0&&hi==0);++f.draws;*out=0;return 1;
    };
    b.random_float=[](void* p,float lo,float hi,float* out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);
        if(lo==.3f){assert(hi==.5f);*out=.4f;}else{assert(lo==2&&hi==4);++f.divisors;*out=3;}
        return 1;
    };
    b.intensity=[](void*,void*,float,float* out) noexcept -> int{*out=.5f;return 1;};
    auto get=[](void* p,unsigned off){float v=0;std::memcpy(&v,static_cast<unsigned char*>(p)+off,4);return v;};
    auto close=[](float a,float b){return std::fabs(a-b)<.00001f;};
    unsigned accepted=0;
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),1,1,false,&b,&accepted)&&accepted);
    assert(close(get(record,0x3c),13)&&close(get(actor,0xefc),.8f)&&get(actor,0xf00)==.4f);
    assert(close(get(actor,0x4a8),.3f)&&get(actor,0x4ac)==.4f&&f.loads==2&&f.draws==1&&f.divisors==1&&f.maps==1);
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),2,13,false,&b,&accepted)&&accepted);
    assert(close(get(actor,0x4a8),.38333333f)&&f.loads==2&&f.draws==1&&f.divisors==2);
    f.absent=true;
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),3,14,false,&b,&accepted)&&!accepted);
    assert(f.loads==4&&f.draws==1&&close(get(actor,0x4a8),.38333333f));
    f.absent=false;b.intensity=nullptr; // empty event ramp => 1
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),3,14,false,&b,&accepted)&&accepted);
    assert(close(get(actor,0x4a8),.52222222f)&&f.draws==2);
}
}
