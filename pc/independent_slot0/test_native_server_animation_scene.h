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

inline void check_gesture_tag_warp(const ServerGestureTagWarpApi& api,const ServerAnimationSceneFrameApi& frame){
    struct Fixture {
        alignas(4) unsigned char event[0x3c0]{},tags[2][0x118]{},actor[0x4a4]{},record[0x40]{},layer[0x40]{};
        unsigned reads{};
    } f;
    auto put=[](void* p,unsigned off,auto value){std::memcpy(static_cast<unsigned char*>(p)+off,&value,sizeof(value));};
    auto get=[](const void* p,unsigned off){float value=0;std::memcpy(&value,static_cast<const unsigned char*>(p)+off,4);return value;};
    auto eq=[](float a,float b){return std::fabs(a-b)<.00001f;};
    ServerAnimationSceneFrameBindings b{};b.context=&f;
    b.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.reads;
        if(token==3){*out=f.layer;*n=sizeof(f.layer);return 1;}
        if(token<1||token>2)return 0;
        *out=f.tags[token-1];*n=sizeof(f.tags[0]);return 1;
    };
    float value=99;
    auto map=[&](float x,float expected){assert(api.map(f.event,sizeof(f.event),x,&b,&value)==1&&eq(value,expected));};
    put(f.event,0,7u);map(-.25f,-.25f);assert(!f.reads);
    put(f.event,0,6u);map(1.25f,1.25f);
    put(f.event,0x2f0,2);map(.5f,.5f);assert(!f.reads);
    put(f.event,0x304,2);put(f.event,0x2e4,1u);put(f.event,0x2f8,2u);
    put(f.tags[0],0x80,.25f);put(f.tags[0],0x10c,.75f);
    put(f.tags[1],0x80,.125f);put(f.tags[1],0x10c,.5f);
    map(-.2f,0);map(0,0);assert(!f.reads);
    map(.25f,.125f);map(.75f,.5f);map(1,1);map(1.2f,1);
    // Normalized endpoints: p0.y=-.125, p3.y=1.5. At u=.5,
    // uniform spline gives 0.265625, rather than linear .3125.
    map(.5f,0.265625f);
    f.tags[0][0x84]=2;map(.5f,0.2734375f);
    f.tags[0][0x110]=2;map(.5f,.3125f);
    // Actual layer frame with no host warp, same two tag banks.
    put(f.event,0x184,1.f);put(f.event,0x188,3.f);
    put(f.actor,0x494,3u);put(f.actor,0x4a0,1);f.layer[0]=1;
    put(f.record,0x10,0);put(f.record,0x20,1.f);
    ServerAnimationSceneFrameInput in{};in.scene_time=2;in.now=10;in.intensity=1;
    unsigned accepted=0;
    assert(frame.update(f.actor,sizeof(f.actor),f.record,sizeof(f.record),f.event,sizeof(f.event),&in,&b,&accepted)==1&&accepted==1);
    assert(eq(get(f.layer,0xc),.3125f)&&get(f.layer,0x18)==1&&get(f.layer,0x3c)==10);
    put(f.tags[0],0x10c,.1f);
    assert(api.map(f.event,sizeof(f.event),.5f,&b,&value)==0);
}

inline void check_gesture_metadata(const ServerGestureMetadataApi& api,const ServerGestureTagWarpApi& warp){
    struct Fixture{alignas(4) unsigned char event[0x3c0]{},record[0x40]{},tags[2][0x230]{};unsigned reads{};} f;
    auto put=[](void* p,unsigned off,auto v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));};
    auto get=[](const void* p,unsigned off){float v=0;std::memcpy(&v,static_cast<const unsigned char*>(p)+off,4);return v;};
    put(f.event,0,6u);put(f.event,0x2e4,1u);put(f.event,0x2f8,2u);put(f.event,0x2f0,4);put(f.event,0x304,4);
    for(unsigned bank=0;bank<2;++bank){
        const char* names[]={"LoOp","loop","END","tail"};
        const float fractions[2][4]={{.25f,.375f,.75f,.875f},{.125f,.25f,.5f,.75f}};
        for(unsigned i=0;i<4;++i){
            auto* tag=f.tags[bank]+i*0x8c;
            std::strcpy(reinterpret_cast<char*>(tag),names[i]);
            put(tag,0x80,fractions[bank][i]);tag[0x84]=0x84;
        }
    }
    ServerAnimationSceneFrameBindings b{};b.context=&f;
    b.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.reads;
        if(token<1||token>2)return 0;
        *out=f.tags[token-1];*n=sizeof(f.tags[0]);return 1;
    };
    ServerGestureMetadata md{};md.type="posture";
    auto apply=[&](unsigned suppress){return api.apply(f.record,sizeof(f.record),f.event,sizeof(f.event),&md,suppress,&b);};
    auto flag=[&](unsigned bank,unsigned i){return f.tags[bank][i*0x8c+0x84];};
    assert(apply(1)&&!f.reads&&!f.record[0x1c]&&get(f.record,0x20)==1);
    md.available=1;md.type="PoStUrE";assert(apply(1)&&f.reads==2&&f.record[0x1c]==1&&get(f.record,0x20)==0);
    for(unsigned bank=0;bank<2;++bank)assert(flag(bank,0)==0x86&&flag(bank,1)==0x84&&flag(bank,2)==0x86&&flag(bank,3)==0x84);
    assert(apply(256)&&get(f.record,0x20)==1);
    md.type="gesture";md.startloop="tail";md.endloop="not_found";
    assert(apply(1)&&!f.record[0x1c]&&get(f.record,0x20)==1);
    for(unsigned bank=0;bank<2;++bank)assert(flag(bank,0)==0x86&&flag(bank,2)==0x86&&flag(bank,3)==0x86);
    // Missing second bank fails before modifying first bank.
    f.tags[0][0x84]=0x84;put(f.event,0x2f8,0u);md.startloop=nullptr;
    assert(!apply(0)&&flag(0,0)==0x84);
    put(f.event,0x2f8,2u);md.endloop=nullptr;assert(apply(0));
    float mapped=0;
    // Adjacent locked tags END and tail: linear midpoint.
    assert(warp.map(f.event,sizeof(f.event),.8125f,&b,&mapped)&&mapped==.625f);
}

inline void check_animation_priority(const ServerAnimationPriorityApi& api){
    struct Fixture {alignas(4) unsigned char actor[0x604]{},ca[0x110]{},record[0x40]{},layers[4][0x40]{},scenes[3][0x38]{};
        std::uint32_t attached[4]{1,0,2,3},channels[3]{11,22,33};unsigned reads{};} f;
    auto put=[](void* p,unsigned off,auto v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));};
    auto get=[](const void* p,unsigned off){int v=0;std::memcpy(&v,static_cast<const unsigned char*>(p)+off,4);return v;};
    ServerAnimationSceneFrameBindings b{};b.context=&f;
    b.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.reads;
        if(token>=1&&token<=3){*out=f.scenes[token-1];*n=sizeof(f.scenes[0]);return 1;}
        if(token==4){*out=f.attached;*n=sizeof(f.attached);return 1;}
        if(token==5){*out=f.channels;*n=sizeof(f.channels);return 1;}
        if(token==6){*out=f.layers;*n=sizeof(f.layers);return 1;}
        return 0;
    };
    put(f.actor,0x5ec,4u);put(f.actor,0x5f8,4);put(f.ca,0x100,5u);put(f.ca,0x10c,3);
    put(f.scenes[0],0x34,3u);put(f.scenes[1],0x34,5u);put(f.scenes[2],0x34,7u);
    int idx=99;std::uint32_t prefix=99;
    assert(api.channel(f.ca,sizeof(f.ca),22,&b,&idx)&&idx==1);
    assert(api.channel(f.ca,sizeof(f.ca),99,&b,&idx)&&idx==-1);
    assert(api.prefix(f.actor,sizeof(f.actor),1,&b,&prefix)&&prefix==0);
    assert(api.prefix(f.actor,sizeof(f.actor),2,&b,&prefix)&&prefix==3);
    assert(api.prefix(f.actor,sizeof(f.actor),3,&b,&prefix)&&prefix==8);
    assert(api.prefix(f.actor,sizeof(f.actor),99,&b,&prefix)&&prefix==15);
    put(f.actor,0x494,6u);put(f.actor,0x4a0,4);
    for(int i=0;i<3;++i){f.layers[i][0]=1;put(f.layers[i],0x30,2+i*2);put(f.layers[i],0x34,i);}
    put(f.layers[3],0x34,9);
    auto order=[&](int i){return get(f.layers[i],0x34);};
    assert(api.reorder(f.actor,sizeof(f.actor),0,5,10,&b)&&order(0)==1&&order(1)==0&&order(2)==2&&order(3)==9);
    assert(get(f.layers[0],0x30)==5&&get(f.layers[0],0x3c)==0x41200000);
    assert(api.reorder(f.actor,sizeof(f.actor),0,5,11,&b)&&get(f.layers[0],0x3c)==0x41200000);
    assert(api.reorder(f.actor,sizeof(f.actor),2,4,12,&b)&&order(1)==0&&order(2)==1&&order(0)==2);
    assert(api.reorder(f.actor,sizeof(f.actor),3,1,13,&b)&&order(3)==9);
    put(f.record,4,3u);put(f.record,0x10,1);put(f.record,0x14,1);
    const unsigned reads=f.reads;assert(api.refresh(f.actor,sizeof(f.actor),f.record,sizeof(f.record),14,&b)&&reads==f.reads);
    f.actor[0x600]=1;
    assert(api.refresh(f.actor,sizeof(f.actor),f.record,sizeof(f.record),14,&b)&&get(f.layers[1],0x30)==9&&order(1)==2&&order(2)==0&&order(0)==1);
}

}
