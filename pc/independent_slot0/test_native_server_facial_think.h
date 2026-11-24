#pragma once
#include "native_server_facial_think.h"
#include "native_server_composed_facial_think.h"
#include "native_server_flex_spline.h"
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

inline void check_active_server_facial_records(const ServerFacialThinkApi& api,const ServerComposedFacialThinkApi* composed=nullptr){
    struct Fixture {
        alignas(4) unsigned char rows[0x80]{},scene[0x21c]{},events[2][0x3c0]{},track[0x50]{},vfe[256]{};
        std::uint32_t tracks[1]{6};
        char name[4]="jaw";
        ServerFlexSample samples[1]{{.8f,.5f,0}};
        ServerIntensitySample ramp[1]{{.5f,.5f,0}};
        unsigned loads{},expr_maps{},flex_maps{},intensity_calls{};
    } f;
    auto put=[](void* p,unsigned off,auto v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));};
    auto close=[](float a,float b){return std::fabs(a-b)<.00001f;};
    alignas(4) unsigned char actor[server_flex_scene_extent]{};
    actor[0x1f0]=1;actor[0x600]=1;
    put(actor,0x4a8,.2f);put(actor,0x5b8,1u);put(actor,0x5c4,2);
    put(f.scene,0x78,1.5f);
    put(f.rows,0,3u);put(f.rows,4,2u);put(f.rows,0x40,4u);put(f.rows,0x44,2u);
    put(f.rows,0x60,1.f);
    put(f.events[0],0,2u);put(f.events[0],0x184,1.f);put(f.events[0],0x188,2.f);
    std::strcpy(reinterpret_cast<char*>(f.events[0]+0x84),"emotion");
    std::strcpy(reinterpret_cast<char*>(f.events[0]+0x104),"smile");
    put(f.events[1],0,10u);put(f.events[1],0x184,1.f);put(f.events[1],0x188,2.f);
    put(f.events[1],0x30c,5u);put(f.events[1],0x318,1);
    for(auto& e:f.events){put(e,0x19c,9u);put(e,0x1a8,1);}
    put(f.track,4,7u);f.track[8]=1;put(f.track,0x10,1.f);
    put(f.track,0x14,8u);put(f.track,0x20,1);
    std::memcpy(f.vfe,"EFV",4);
    put(f.vfe,0x48,256);put(f.vfe,0x4c,1);put(f.vfe,0x50,108);
    put(f.vfe,0x60,1);put(f.vfe,0x64,156);put(f.vfe,0x68,160);
    put(f.vfe,108,64);put(f.vfe,116,2);put(f.vfe,128,24);
    put(f.vfe,132,0);put(f.vfe,136,.2f);put(f.vfe,144,0);put(f.vfe,148,.2f);
    put(f.vfe,156,192);put(f.vfe,160,-1);
    std::strcpy(reinterpret_cast<char*>(f.vfe+172),"smile");
    std::strcpy(reinterpret_cast<char*>(f.vfe+192),"jaw");
    ServerFlexRange range{0,1};
    ServerFacialThinkBindings b{};b.context=&f;b.ranges=&range;b.count=1;b.base_expression=b.base_flex=1;
    b.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);
        switch(w){
        case 1:*out=f.rows;*n=sizeof(f.rows);break;
        case 2:*out=f.scene;*n=sizeof(f.scene);break;
        case 3:case 4:*out=f.events[w-3];*n=sizeof(f.events[0]);break;
        case 5:*out=f.tracks;*n=sizeof(f.tracks);break;
        case 6:*out=f.track;*n=sizeof(f.track);break;
        case 7:*out=f.name;*n=sizeof(f.name);break;
        case 8:*out=f.samples;*n=sizeof(f.samples);break;
        case 9:*out=f.ramp;*n=sizeof(f.ramp);break;
        default:return 0;
        }
        return 1;
    };
    ServerExpressionBindings expression{};expression.context=&f;expression.ranges=&range;expression.count=1;
    expression.load=[](void* p,void*,const char* name,ServerExpressionBank* out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.loads;
        if(std::strcmp(name,"emotion"))return 0;*out={f.vfe,sizeof(f.vfe)};return 1;
    };
    expression.controller=[](void* p,void*,const char* name) noexcept -> int{
        ++static_cast<Fixture*>(p)->expr_maps;return std::strcmp(name,"jaw")?-1:0;
    };
    expression.intensity=[](void* p,void*,float time,float* out) noexcept -> int{
        ++static_cast<Fixture*>(p)->intensity_calls;assert(time==1.5f);*out=.5f;return 1;
    };
    ServerFlexAnimationBindings flex{};flex.context=&f;flex.ranges=&range;flex.count=1;flex.storage=b.storage;
    flex.controller=[](void* p,const char* name) noexcept -> int{
        ++static_cast<Fixture*>(p)->flex_maps;return std::strcmp(name,"jaw")?-1:0;
    };
    flex.intensity=expression.intensity; // no sample callback: native raw spline.
    b.expression=&expression;b.flex=&flex;
    ServerComposedFacialThinkBindings cb{};cb.base=b;cb.native_intensity=true;
    if(composed){expression.intensity=nullptr;flex.intensity=nullptr;}
    auto run=[&](float now){return composed?composed->run(actor,sizeof(actor),now,&cb):api.run(actor,sizeof(actor),now,&b);};
    auto value=[&](){float v=0;std::memcpy(&v,actor+0x4a8,4);return v;};
    // Decay .2 -> .19, expression adds .2, spline blends half way to .8.
    assert(run(10));
    assert(close(value(),.595f)&&f.rows[0xc]&&f.rows[0x4c]&&f.events[1][0x320]);
    assert(f.loads==1&&f.expr_maps==2&&f.flex_maps==1&&f.intensity_calls==(composed?0u:2u)&&actor[0x200]&&!actor[0x600]);
    assert(run(11));
    assert(close(value(),.782625f)&&f.loads==2&&f.expr_maps==4&&f.flex_maps==1&&f.intensity_calls==(composed?0u:4u));
    // Swap vector ordering: flex first, then expression, so result differs.
    unsigned char tmp[0x40];std::memcpy(tmp,f.rows,0x40);std::memcpy(f.rows,f.rows+0x40,0x40);
    std::memcpy(f.rows+0x40,tmp,0x40);put(actor,0x4a8,.2f);
    assert(run(12));
    assert(close(value(),.695f)&&f.flex_maps==1&&f.intensity_calls==(composed?0u:6u));
}

inline void check_composed_npc_random_record(const ServerComposedFacialThinkApi& api){
    alignas(4) unsigned char actor[server_npc_random_expression_extent]{},row[0x40]{},scene[0x220]{},event[0x3c0]{};
    auto put=[](void* p,unsigned off,auto v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));};
    put(actor,0x5b8,1u);put(actor,0x5c4,1);put(actor,0x4a8,.4f);put(actor,0xefc,.8f);actor[0x1f0]=1;
    put(row,0,2u);put(row,4,3u);put(row,0x38,6u);put(row,0x3c,100.f);put(row,0x20,1.f);
    put(event,0,16u);put(event,0x184,0.f);put(event,0x188,5.f);
    put(scene,0x78,2.f);
    struct Fixture {void* row;void* event;void* scene;unsigned other{},random{};} f{row,event,scene};
    ServerFlexRange range{0,1};
    ServerNpcRandomExpressionBindings random{};random.context=&f;random.ranges=&range;random.count=1;
    random.load=[](void*,void*,const char*,ServerExpressionBank*) noexcept -> int {return 0;};
    random.controller=[](void*,void*,const char*) noexcept -> int {return 0;};
    random.random_int=[](void*,int,int,int*) noexcept -> int {return 0;};
    random.random_float=[](void* p,float lo,float hi,float* out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(lo==2&&hi==4);++f.random;*out=2;return 1;
    };
    ServerComposedFacialThinkBindings b{};b.native_intensity=true;b.random_expression=&random;
    b.base.context=&f;b.base.ranges=&range;b.base.count=1;
    b.base.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);
        if(w==1){*out=f.row;*n=0x40;}else if(w==2){*out=f.event;*n=0x3c0;}
        else if(w==3){*out=f.scene;*n=0x220;}else return 0;return 1;
    };
    b.base.other=[](void* p,void*,void*,void*,void*,unsigned* out) noexcept -> int{
        ++static_cast<Fixture*>(p)->other;*out=0;return 1;
    };
    assert(api.run(actor,sizeof(actor),10,&b)&&row[0xc]&&f.random==1&&!f.other);
    float value=0;std::memcpy(&value,actor+0x4a8,4);
    assert(std::fabs(value-.59f)<.00001f);
    put(row,0x38,7u);row[0xc]=0;
    assert(api.run(actor,sizeof(actor),11,&b)&&!row[0xc]&&f.other==1&&f.random==1);
}
}
