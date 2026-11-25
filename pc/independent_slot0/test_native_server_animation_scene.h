#pragma once
#include "native_server_animation_scene.h"
#include <cassert>
#include <cstring>
#include <cmath>
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
inline void check_animation_scene_frame(const ServerAnimationSceneFrameApi& api){
    struct Fixture {alignas(4) unsigned char actor[0x4a4]{},row[0x40]{},record[0x40]{},event[0x3c0]{};unsigned reads{},warps{};float fraction{},mapped=.75f;} f;
    auto put=[](void* p,unsigned off,auto v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));};
    auto get=[](void* p,unsigned off){float v=0;std::memcpy(&v,static_cast<unsigned char*>(p)+off,4);return v;};
    auto approximately_equal=[](float a,float b){return std::fabs(a-b)<.00001f;};
    put(f.actor,0x494,1u);put(f.actor,0x4a0,1);f.row[0]=1;
    put(f.record,0x10,0);put(f.record,0x20,1.f);put(f.event,0,6u);
    put(f.event,0x184,1.f);put(f.event,0x188,3.f);
    ServerAnimationSceneFrameBindings b{};b.context=&f;
    b.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(w==1);++f.reads;*out=f.row;*n=sizeof(f.row);return 1;
    };
    b.gesture_cycle=[](void* p,const void* e,float fraction,float* out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(e==f.event);++f.warps;f.fraction=fraction;*out=f.mapped;return 1;
    };
    ServerAnimationSceneFrameInput in{};in.scene_time=2;in.now=10;in.intensity=.5f;in.suppressed=true;
    unsigned out=99;
    auto run=[&](){return api.update(f.actor,sizeof(f.actor),f.record,sizeof(f.record),f.event,sizeof(f.event),&in,&b,&out);};
    assert(run()==1&&out==1&&f.warps==1&&f.fraction==.5f&&get(f.record,0x20)==1&&get(f.row,0xc)==.75f&&get(f.row,0x18)==.5f&&get(f.row,0x3c)==10);
    f.record[0x1c]=1;in.now=11;f.mapped=1.25f;
    assert(run()==1&&approximately_equal(get(f.record,0x20),.8f)&&approximately_equal(get(f.row,0x18),.448f)&&get(f.row,0xc)==1&&get(f.row,0x3c)==11);
    f.row[5]=1;in.suppressed=false;in.now=12;
    assert(run()==1&&get(f.record,0x20)==1&&get(f.row,0xc)==1.25f&&get(f.row,0x18)==.5f);
    put(f.event,0,7u);put(f.record,0x20,0.f);in.first_sequence_frame=true;in.sequence_duration=4.;in.now=13;
    assert(run()==1&&approximately_equal(get(f.record,0x20),.2f)&&get(f.row,0x18)==0&&get(f.row,0xc)==.25f&&get(f.row,0x3c)==13);
    in.first_sequence_frame=false;in.sequence_flags=1;in.now=14;
    assert(run()==1&&approximately_equal(get(f.record,0x20),.4f)&&approximately_equal(get(f.row,0x18),.176f)&&get(f.row,0xc)==.25f);
    f.row[0]=0;in.now=15;
    assert(run()==1&&approximately_equal(get(f.record,0x20),.6f)&&get(f.row,0x3c)==14);
    put(f.record,0x10,-1);const unsigned reads=f.reads;assert(run()==1&&f.reads==reads);
    put(f.record,0x10,-2);assert(run()==-1&&f.reads==reads);
    put(f.event,0,10u);assert(run()==0);
}

}
