#pragma once
#include "native_server_animation_scene.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
inline void check_animation_scene(const ServerAnimationSceneApi& api){
    struct Fixture {
        unsigned lookups{},missing{},removed{},blended{},type{};
        int sequence=7,layer=-1;float duration=-1,delay=-1;
    } f;
    alignas(4) unsigned char actor[4]{},row[0x40]{},event[0x3c0]{};
    auto put=[](void* p,unsigned off,auto v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));};
    auto get=[](void* p,unsigned off){int v=0;std::memcpy(&v,static_cast<unsigned char*>(p)+off,4);return v;};
    std::strcpy(reinterpret_cast<char*>(event+0x84),"test_anim");
    ServerAnimationSceneBindings b{};b.context=&f;
    b.lookup=[](void* p,void*,const char* n,int* out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(!std::strcmp(n,"test_anim"));++f.lookups;*out=f.sequence;return 1;
    };
    b.missing=[](void* p,void*,std::uint32_t ca,unsigned type,const char* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(ca==99&&!std::strcmp(n,"test_anim"));++f.missing;f.type=type;return 1;
    };
    b.remove=[](void* p,void*,int layer) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.removed;f.layer=layer;return 1;
    };
    b.blend_out=[](void* p,void*,int layer,float duration,float delay) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.blended;f.layer=layer;f.duration=duration;f.delay=delay;return 1;
    };
    unsigned out=99;
    auto begin=[&](){return api.begin(actor,row,sizeof(row),event,sizeof(event),99,&b,&out);};
    auto stop=[&](int immediate,int forced){return api.stop(actor,row,sizeof(row),event,sizeof(event),immediate,forced,&b,&out);};
    put(event,0,10u);assert(begin()==0&&out==99&&!f.lookups);
    put(event,0,6u);f.sequence=-1;put(row,8,42);put(row,0x10,123);
    assert(begin()==1&&!out&&get(row,0x18)==-1&&get(row,8)==42&&get(row,0x10)==123&&f.missing==1&&f.type==6);
    f.sequence=0;assert(begin()==1&&out==1&&get(row,0x18)==0&&get(row,8)==99&&get(row,0x10)==-2);
    assert(stop(1,1)==1&&out==1&&!f.removed&&!f.blended);
    assert(api.check(event,sizeof(event),&out)==1&&out==1);
    put(row,0x10,4);assert(stop(256,256)==1&&f.blended==1&&f.duration==.1f&&f.delay==0&&f.layer==4);
    assert(stop(0,1)==1&&f.blended==2&&f.duration==.5f);
    assert(stop(1,0)==1&&f.removed==1&&f.layer==4&&f.blended==2);
    put(event,0,7u);f.sequence=7;assert(begin()==1&&out==1&&get(row,0x18)==7&&get(row,0x10)==-2);
    put(row,0x10,2);assert(stop(0,1)==1&&f.blended==3&&f.duration==.3f&&f.layer==2);
    assert(stop(0,0)==1&&f.blended==4&&f.duration==.3f);
    put(row,0x10,-1);assert(stop(0,1)==1&&f.blended==4);
    assert(api.check(event,sizeof(event),&out)==1&&out==1);
}
}
