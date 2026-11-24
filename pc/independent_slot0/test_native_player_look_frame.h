#pragma once
#include "native_character_response.h"
#include "native_jessica_script.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct PlayerLookFrameFixture {
    alignas(4) unsigned char player[0x1230]{},npc[0x1458]{};
    HologramControl monitor{player+0xc40,0x5f0},jessica{npc,sizeof(npc)};
    const PlayerLookFrameApi* api=nullptr;
    const ActionMonitorApi* actions=nullptr;
    LookEventState event_state{};
    JessicaAttentionState attention_state{};
    CountRegionBindings regions{};
    float now=10;
    int group=2;
    const char* bone="Bip01_Spine";
    unsigned classified=0,events=0,speeches=0;
    int event_descriptor=0;
    ActionCriteria criteria{};
    char event_part[32]{};
};
inline void check_player_look_frame(const PlayerLookFrameApi& api,const ActionMonitorApi& actions,const JessicaAttentionApi& attention){
    PlayerLookFrameFixture f;f.api=&api;f.actions=&actions;
    auto* m=f.player+0xc40;
    const unsigned owner=0x1001,invalid=0xffffffffu;
    std::memcpy(m+4,&owner,4);std::memcpy(m+0x14,&invalid,4);
    f.regions.context=&f;
    f.regions.forward_xy=[](void* p,void* npc,float* xy) noexcept -> int {
        auto& f=*static_cast<PlayerLookFrameFixture*>(p);assert(npc==f.npc&&f.jessica.busy);xy[0]=1;xy[1]=0;return 1;
    };
    f.regions.hit_identity=[](void* p,void* npc,int hitbox,int* group,const char** bone) noexcept -> int {
        auto& f=*static_cast<PlayerLookFrameFixture*>(p);assert(npc==f.npc&&hitbox==7);*group=f.group;*bone=f.bone;return 1;
    };
    CountRegionQuery q{{-1,0,0},7,180};const char* label=nullptr;
    assert(api.jessica_region(&f.jessica,&f.regions,&q,&label)==1&&!std::strcmp(label,"chest"));
    q.distance=180.001f;assert(api.jessica_region(&f.jessica,&f.regions,&q,&label)==1&&!label);q.distance=180;
    f.bone="Bip01_Hand";f.group=4;assert(api.jessica_region(&f.jessica,&f.regions,&q,&label)==1&&!std::strcmp(label,"left arm"));
    f.bone="Bip01_Calf";f.group=6;assert(api.jessica_region(&f.jessica,&f.regions,&q,&label)==1&&!std::strcmp(label,"left leg"));
    f.bone="Bip01_Pelvis";q.direction[0]=1;assert(api.jessica_region(&f.jessica,&f.regions,&q,&label)==1&&!std::strcmp(label,"butt"));
    f.bone="Bip01_Spine";f.group=2;
    PlayerLookFrameBindings b{};b.context=&f;
    b.classify=[](void* p,void* target,const CountRegionQuery* q,const char** name) noexcept -> int {
        auto& f=*static_cast<PlayerLookFrameFixture*>(p);assert(target==f.npc&&f.monitor.busy&&q->hitbox==7&&q->distance==50);
        ++f.classified;return f.api->jessica_region(&f.jessica,&f.regions,q,name)==1;
    };
    b.monitor.context=&f;
    b.monitor.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int {
        auto& f=*static_cast<PlayerLookFrameFixture*>(p);assert(f.monitor.busy);
        *out=h==0x1001?f.player:h==0x1002?f.npc:nullptr;return 1;
    };
    b.monitor.clock=[](void* p,float* out) noexcept -> int {
        auto& f=*static_cast<PlayerLookFrameFixture*>(p);assert(f.monitor.busy);*out=f.now;return 1;
    };
    b.monitor.packed_handle=[](void* p,void* e,std::uint32_t* h) noexcept -> int {
        auto& f=*static_cast<PlayerLookFrameFixture*>(p);assert(e==f.npc&&f.monitor.busy);*h=0x1002;return 1;
    };
    b.monitor.sample=[](void* p,void* entity,ActionEntitySample* sample) noexcept -> int {
        auto& f=*static_cast<PlayerLookFrameFixture*>(p);assert(f.monitor.busy);
        std::strcpy(sample->classname,entity==f.player?"player":"npc_jessica");return 1;
    };
    b.event.context=&f;b.event.resolve=b.monitor.resolve;b.event.clock=b.monitor.clock;
    b.event.look_event=[](void* p,int descriptor,void* owner,void* target,float distance,const char* part) noexcept -> int {
        auto& f=*static_cast<PlayerLookFrameFixture*>(p);assert(f.monitor.busy&&owner==f.player);
        ++f.events;f.event_descriptor=descriptor;
        if(descriptor==1)assert(target==f.npc&&distance==50);
        else assert(descriptor==2&&!target&&distance==0);
        std::strcpy(f.event_part,part);return 1;
    };
    JessicaAttentionBindings ab{};ab.context=&f;
    ab.clock=[](void* p,float* out) noexcept -> int {auto& f=*static_cast<PlayerLookFrameFixture*>(p);assert(f.jessica.busy);*out=f.now;return 1;};
    ab.speak_criteria=[](void* p,void* npc,const void* record,float now,const char* concept_name) noexcept -> int {
        auto& f=*static_cast<PlayerLookFrameFixture*>(p);assert(npc==f.npc&&record==f.player+0xc58&&!std::strcmp(concept_name,"TLK_PLLOOKING"));
        ++f.speeches;return f.actions->look_criteria(record,0x2e8,now,&f.criteria);
    };
    PlayerLookObservation obs{1,f.npc,{-1,0,0},{10,20,30},{40,60,30},7};
    auto update=[&](){return api.update(f.player,sizeof(f.player),&f.monitor,&f.event_state,&b,&obs);};
    auto observe=[&](){return attention.observe(&f.jessica,&f.attention_state,&ab,m,0x5e9);};
    assert(api.update(f.player,0x1228,&f.monitor,&f.event_state,&b,&obs)==0);
    assert(update()==1&&observe()==1&&f.classified==1&&!f.events&&!f.speeches);
    f.now=11.5f;assert(update()==1&&observe()==1&&f.events==1&&f.speeches==1&&m[0x300]==1&&!std::strcmp(f.event_part,"chest"));
    assert(std::strstr(f.criteria.text,"PlayerLookingAtPartForSeconds:1.50"));
    f.now=14.5f;assert(update()==1&&observe()==1&&f.speeches==1); // strict >3 repeat
    f.now=14.75f;assert(update()==1&&observe()==1&&f.speeches==2&&std::strstr(f.criteria.text,"PlayerLookingAtPartForSeconds:4.75"));
    f.group=1;f.bone="Bip01_Head";f.now=15;assert(update()==1&&observe()==1&&f.events==1);
    f.now=16.5f;assert(update()==1&&observe()==1&&f.events==2&&!std::strcmp(f.event_part,"head"));
    obs.hit=0;f.now=17;assert(update()==1&&observe()==1&&f.events==3&&f.event_descriptor==2&&!m[0x300]&&!f.attention_state.looking);
}
}
