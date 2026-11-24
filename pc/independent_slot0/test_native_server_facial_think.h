#pragma once
#include "native_server_facial_think.h"
#include <cassert>
#include <cstring>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_server_facial_think(const ServerFacialThinkApi& api){
    struct Fixture {
        alignas(4) unsigned char rows[0xc0]{},scene[0x21c]{},events[3][0x3c0]{};
        unsigned called{};bool mutate{};
    } f;
    auto put=[](void* p,unsigned off,auto v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));};
    alignas(4) unsigned char actor[server_flex_scene_extent]{};
    put(actor,0x5b8,1u);put(actor,0x5c4,3);put(actor,0x4a8,.8f);
    actor[0x1f0]=1;actor[0x600]=1;
    put(f.scene,0x78,1.25f);
    for(unsigned i=0;i<3;++i){put(f.rows,i*0x40,i+3);put(f.rows,i*0x40+4,2u);}
    put(f.events[0],0,5u);put(f.events[1],0,2u);put(f.events[2],0,10u);
    // Base expression/flex end sentinel: successfully handled no-op.
    put(f.events[1],0x188,-1.f);put(f.events[2],0x188,-1.f);
    ServerFlexRange range{0,1};
    ServerExpressionBindings expression{};expression.ranges=&range;expression.count=1;
    expression.load=[](void*,void*,const char*,ServerExpressionBank*) noexcept -> int{return 0;};
    expression.controller=[](void*,void*,const char*) noexcept -> int{return -1;};
    expression.intensity=[](void*,void*,float,float* out) noexcept -> int{*out=1;return 1;};
    ServerFlexAnimationBindings flex{};flex.ranges=&range;flex.count=1;
    flex.storage=[](void*,std::uint32_t,void**,unsigned*) noexcept -> int{return 0;};
    flex.controller=[](void*,const char*) noexcept -> int{return -1;};
    flex.intensity=expression.intensity;
    ServerFacialThinkBindings b{};b.context=&f;b.ranges=&range;b.count=1;b.expression=&expression;b.flex=&flex;
    b.base_expression=b.base_flex=b.base_speech=1;
    b.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);
        if(w==1){*out=f.rows;*n=sizeof(f.rows);}else if(w==2){*out=f.scene;*n=sizeof(f.scene);}
        else if(w>=3&&w<=5){*out=f.events[w-3];*n=sizeof(f.events[0]);}else return 0;
        return 1;
    };
    b.other=[](void* p,void* actor,void*,void*,void*,unsigned* ok) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.called;*ok=0;
        if(f.mutate){const int n=0;std::memcpy(static_cast<unsigned char*>(actor)+0x5c4,&n,4);}
        return 1;
    };
    auto value=[&](){float v=0;std::memcpy(&v,actor+0x4a8,4);return v;};
    assert(api.run(actor,sizeof(actor),10,&b));
    assert(std::fabs(value()-.76f)<.00001f&&actor[0x200]&&!actor[0x600]&&!f.called);
    assert(f.rows[0xc]&&f.rows[0x4c]&&f.rows[0x8c]);
    // Derived expression override declines processing; previously active bit remains.
    b.base_expression=0;actor[0x600]=1;
    assert(api.run(actor,sizeof(actor),11,&b)&&f.called==1&&f.rows[0x4c]&&!actor[0x600]);
    f.rows[0x4c]=0;assert(api.run(actor,sizeof(actor),12,&b)&&!f.rows[0x4c]&&f.called==2);
    f.mutate=true;actor[0x600]=1;
    assert(!api.run(actor,sizeof(actor),13,&b)&&actor[0x600]);
    assert(api.run(actor,sizeof(actor),14,&b)&&!actor[0x600]);
}
}
