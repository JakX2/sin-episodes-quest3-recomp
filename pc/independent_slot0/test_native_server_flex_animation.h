#pragma once
#include "native_server_flex_animation.h"
#include <cassert>
#include <cstring>
#include <cmath>
namespace sin_port::native_weapon_tables::test {
inline void check_server_flex_animation(const ServerFlexAnimationApi& api){
    struct Fixture {alignas(4) unsigned char track[0x50]{};std::uint32_t words[1]{1};char name[8]="smile";unsigned binds{},samples{};} f;
    alignas(4) unsigned char actor[server_flex_scene_extent]{},record[0x40]{},event[0x3c0]{};
    auto put=[](void* p,unsigned o,auto v){std::memcpy(static_cast<unsigned char*>(p)+o,&v,sizeof(v));};
    auto get=[](void* p,unsigned o){float v=0;std::memcpy(&v,static_cast<unsigned char*>(p)+o,4);return v;};
    auto close=[](float a,float b){return std::fabs(a-b)<.00001f;};
    ServerFlexRange ranges[]={{0,1},{0,1}};
    ServerFlexAnimationBindings b{};b.context=&f;b.ranges=ranges;b.count=2;
    b.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);
        if(w==1){*out=f.track;*n=sizeof(f.track);}else if(w==2){*out=f.words;*n=sizeof(f.words);}
        else if(w==3){*out=f.name;*n=sizeof(f.name);}else return 0;return 1;
    };
    b.controller=[](void* p,const char* name) noexcept -> int{
        ++static_cast<Fixture*>(p)->binds;
        if(!std::strcmp(name,"right_smile"))return 0;
        if(!std::strcmp(name,"left_smile"))return 1;
        return -1;
    };
    b.sample=[](void* p,void*,unsigned bank,float t,float* out) noexcept -> int{
        ++static_cast<Fixture*>(p)->samples;assert(t==.5f);*out=bank?.75f:.8f;return 1;
    };
    b.intensity=[](void*,void*,float,float* out) noexcept -> int {*out=.5f;return 1;};
    put(event,0x184,1.f);put(event,0x188,2.f);put(event,0x30c,2u);put(event,0x318,1);
    put(f.track,4,3u);f.track[8]=f.track[9]=1;put(f.track,0x10,1.f);
    actor[0x1f0]=1;float fade=0;
    assert(api.fade(actor,sizeof(actor),record,sizeof(record),false,10,true,&fade)&&fade==1&&get(actor,0x604)==10);
    put(actor,0x28,9.9f);put(actor,0x2c,10.1f);
    assert(api.fade(actor,sizeof(actor),record,sizeof(record),true,10.1f,false,&fade)&&close(fade,.8f));
    assert(api.fade(actor,sizeof(actor),record,sizeof(record),true,11,false,&fade)&&close(fade,.9f));
    put(record,0x20,1.f);
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),1.5f,11,false,&b));
    assert(f.binds==2&&f.samples==2&&event[0x320]&&record[0xc]&&actor[0x200]);
    assert(close(get(actor,0x4a8),.2f)&&close(get(actor,0x4ac),.4f));
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),1.5f,11,false,&b));
    assert(f.binds==2&&close(get(actor,0x4a8),.3f)&&close(get(actor,0x4ac),.6f));
    assert(api.decay(actor,sizeof(actor),ranges,2,nullptr));
    assert(close(get(actor,0x4a8),.285f)&&close(get(actor,0x4ac),.57f));
    put(event,0x188,-1.f);f.samples=0;record[0xc]=0;
    assert(api.apply(actor,sizeof(actor),record,sizeof(record),event,sizeof(event),1.5f,11,false,&b)&&!f.samples&&!record[0xc]);
}
}
