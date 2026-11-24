#pragma once
#include "native_character_response.h"
#include <cassert>
#include <cstring>
#include <limits>
namespace sin_port::native_weapon_tables::test {
struct CharacterResponseFixture {
    alignas(4) unsigned char npc[0x1458]{};
    HologramControl ctrl{npc,sizeof(npc)};
    unsigned forwards=0,hits=0,filters=0;
    const char* bone="Bip01_Head";
    int group=1,hitbox=2;
    float xy[2]{1,0};
    bool bad_hit=false;
};
inline void check_character_response(const CharacterResponseApi& api){
    CharacterResponseFixture f;CountRegionBindings b{};b.context=&f;
    b.forward_xy=[](void* p,void* npc,float* out) noexcept -> int {
        auto& f=*static_cast<CharacterResponseFixture*>(p);assert(f.ctrl.busy&&npc==f.npc);++f.forwards;
        out[0]=f.xy[0];out[1]=f.xy[1];return 1;
    };
    b.hit_identity=[](void* p,void* npc,int box,int* group,const char** bone) noexcept -> int {
        auto& f=*static_cast<CharacterResponseFixture*>(p);assert(f.ctrl.busy&&npc==f.npc&&box==f.hitbox&&f.forwards==f.hits+1);++f.hits;
        *group=f.group;*bone=f.bone;return !f.bad_hit;
    };
    CountRegionQuery q{{1,0,999},2,180.01f};const char* region=nullptr;
    auto classify=[&](){return api.count_region(&f.ctrl,&b,&q,&region);};
    auto expect=[&](const char* name){assert(classify()==1&&region&&!std::strcmp(region,name));};
    assert(classify()==1&&!region&&!f.forwards&&!f.hits);
    q.distance=180;expect("head");
    f.bone="Bip01_Pelvis";f.group=7;expect("butt");
    q.direction[0]=-1;expect("crotch");q.direction[0]=0;expect("crotch");
    f.bone="prefix_Bip01_R_Hand";expect("hand");
    f.bone="Bip01_L_Calf";expect("knee");
    f.bone="Bip01_R_hand";expect("right leg"); // case-sensitive
    f.bone="bone";const char* names[]={"generic stuff","head","chest","stomach","left arm","right arm","left leg","right leg"};
    for(int i=0;i<8;++i){f.group=i;expect(names[i]);}
    f.group=2;q.direction[0]=1;expect("back");
    q.direction[0]=std::numeric_limits<float>::quiet_NaN();expect("chest");
    q.distance=std::numeric_limits<float>::quiet_NaN();expect("chest"); // unordered distance passes original
    f.group=-1;assert(classify()==1&&!region);f.group=8;assert(classify()==1&&!region);
    f.bone=nullptr;assert(classify()==-1&&f.ctrl.blocked);assert(api.acknowledge(&f.ctrl));f.bone="bone";
    f.bad_hit=true;assert(classify()==-1&&f.ctrl.blocked);assert(api.acknowledge(&f.ctrl));f.bad_hit=false;
    unsigned use=9;assert(api.uses_speech_semaphore(&f.ctrl,&use)==1&&use==1);
    f.npc[0xb6]=0x80;assert(api.uses_speech_semaphore(&f.ctrl,&use)==1&&use==1);
    f.npc[0xb6]=0x81;assert(api.uses_speech_semaphore(&f.ctrl,&use)==1&&!use);
    f.npc[0xb6]=0;
    JessicaFilterBindings j{};j.context=&f;
    j.base_filter=[](void* p,void* npc,const PlayerDamageInfo*,unsigned* result) noexcept -> int {
        auto& f=*static_cast<CharacterResponseFixture*>(p);assert(f.ctrl.busy&&npc==f.npc);++f.filters;*result=0;return 1;
    };
    PlayerDamageInfo info{};
    auto flags=[&](unsigned mask){std::memcpy(info.bytes+0x38,&mask,4);};
    unsigned allowed=7;auto filter=[&](){return api.jessica_damage_filter(&f.ctrl,&j,&info,&allowed);};
    flags(0x10000);assert(filter()==1&&allowed==1&&!f.filters);
    flags(0x30000);assert(filter()==1&&allowed==1&&!f.filters);
    flags(0x20000);assert(filter()==1&&!allowed&&f.filters==1);
    flags(0);assert(filter()==1&&!allowed&&f.filters==2);
}
}
