#pragma once
#include "native_use_observation.h"
#include "native_jessica_script.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
struct UseObservationFixture {
    alignas(4) unsigned char player[0x1230]{},npc[0x1458]{};
    HologramControl monitor{player+0xc40,0x5f0},jessica{npc,sizeof(npc)};
    LookEventState look_state{};
    JessicaAttentionState attention_state{};
    JessicaAttentionBindings speech{};
    const JessicaAttentionApi* attention=nullptr;
    const ActionMonitorApi* actions=nullptr;
    ActionCriteria criteria{};
    int subject=1,clients=1;
    float now=10;
    unsigned resolves=0,samples=0,events=0,lookups=0,observations=0,speeches=0;
    bool player_present=true,short_player=false,fail_event=false;
    unsigned char* m(){return player+0xc40;}
    unsigned word(unsigned off){unsigned w=0;std::memcpy(&w,m()+off,4);return w;}
    void put(unsigned off,unsigned w){std::memcpy(m()+off,&w,4);}
};
inline void check_use_observation(const UseObservationApi& api,const ActionMonitorApi& actions,const LookEventApi& look,const JessicaAttentionApi& attention){
    UseObservationFixture f;f.attention=&attention;f.actions=&actions;f.put(4,0x1001);f.put(0x14,0xffffffffu);
    std::strcpy(reinterpret_cast<char*>(f.m()+0x5c8),"radio");
    UseObservationBindings b{};auto& m=b.monitor;m.context=&f;
    m.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{
        auto& f=*static_cast<UseObservationFixture*>(p);assert(f.monitor.busy);++f.resolves;
        *out=h==0x1001?static_cast<void*>(f.player):nullptr;return 1;
    };
    m.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<UseObservationFixture*>(p);assert(f.monitor.busy);*out=f.now;return 1;};
    m.packed_handle=[](void* p,void* obj,std::uint32_t* out) noexcept -> int{
        auto& f=*static_cast<UseObservationFixture*>(p);assert(f.monitor.busy&&obj==&f.subject);*out=0x1002;return 1;
    };
    m.sample=[](void* p,void* obj,ActionEntitySample* out) noexcept -> int{
        auto& f=*static_cast<UseObservationFixture*>(p);assert(f.monitor.busy);++f.samples;
        if(obj==f.player)std::strcpy(out->classname,"player");
        else{assert(obj==&f.subject);std::strcpy(out->classname,"prop_vehicle_sin");std::strcpy(out->name,"car");}
        return 1;
    };
    b.event=[](void* p,int descriptor,void* owner,void* subject,const char* part) noexcept -> int{
        auto& f=*static_cast<UseObservationFixture*>(p);assert(f.monitor.busy&&descriptor==1&&owner==f.player&&subject==&f.subject&&!std::strcmp(part,"radio"));
        ++f.events;return !f.fail_event;
    };
    LookEventBindings l{};l.context=&f;l.clock=m.clock;l.resolve=m.resolve;
    l.look_event=[](void*,int,void*,void*,float,const char*) noexcept -> int{return 0;}; // no look record
    f.speech.context=&f;
    f.speech.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<UseObservationFixture*>(p);assert(f.jessica.busy);*out=f.now;return 1;};
    f.speech.speak_criteria=[](void* p,void* npc,const void* record,float,const char* concept_name) noexcept -> int{
        auto& f=*static_cast<UseObservationFixture*>(p);assert(f.jessica.busy&&npc==f.npc&&record==f.m()+0x308&&!std::strcmp(concept_name,"TLK_PLUSING"));
        ++f.speeches;return f.actions->use_criteria(record,0x2e0,&f.criteria);
    };
    NpcObservationBindings n{};n.context=&f;
    n.max_clients=[](void* p,int* out) noexcept -> int{*out=static_cast<UseObservationFixture*>(p)->clients;return 1;};
    n.single_player=[](void* p,void** out,unsigned* bytes) noexcept -> int{
        auto& f=*static_cast<UseObservationFixture*>(p);++f.lookups;*out=f.player_present?f.player:nullptr;*bytes=f.short_player?0x1228:sizeof(f.player);return 1;
    };
    n.observe=[](void* p,void* npc,const void* monitor,unsigned bytes) noexcept -> int{
        auto& f=*static_cast<UseObservationFixture*>(p);assert(npc==f.npc&&monitor==f.m()&&bytes==0x5e9);
        ++f.observations;return f.attention->observe(&f.jessica,&f.attention_state,&f.speech,monitor,bytes)==1;
    };
    auto tick=[&](){return api.npc_tick(f.npc,&n);};
    auto eligible=[&](){return look.tick(&f.monitor,&f.look_state,&l);};
    assert(api.capture(&f.monitor,&b,0,&f.subject)==1&&f.events==0&&f.resolves==1&&f.samples==2&&f.word(0x304)==0x1002);
    assert(api.capture(&f.monitor,&b,1,&f.subject)==1&&f.events==1&&f.resolves==3&&f.samples==4&&f.word(0x5c0)==1&&f.word(0x5c4)==0x41200000);
    assert(!std::strcmp(reinterpret_cast<char*>(f.m()+0x5c8),"radio"));
    assert(tick()==1&&!f.observations); // producer doesn't set enable
    f.now=10.25f;assert(eligible()==1&&f.m()[0x5e8]);
    f.clients=2;assert(tick()==1&&f.lookups==1&&!f.observations);
    f.clients=0;assert(tick()==1&&f.lookups==1);
    f.clients=1;f.player_present=false;assert(tick()==1&&f.lookups==2&&!f.observations);
    f.player_present=true;f.short_player=true;assert(tick()==-1&&f.lookups==3);
    f.short_player=false;assert(tick()==1&&f.observations==1&&f.speeches==1);
    assert(!std::strcmp(f.criteria.text,"PlayerUsing:car,PlayerUsingEnt:prop_vehicle_sin,PlayerUsingPart:radio,PlayerUsingAction:USE_ACTION_USE"));
    f.now=10.99f;assert(eligible()==1&&tick()==1&&f.observations==2&&f.speeches==1);
    f.now=11;assert(eligible()==1&&!f.m()[0x5e8]&&tick()==1&&f.observations==2);
    // NPC gate skips both-disabled monitor; it does not force a reset
    // of Jessica's shared timer. Direct receiver tests remain separate.
    assert(f.attention_state.using_part==0.25f);
    assert(api.capture(&f.monitor,&b,0,nullptr)==1&&f.word(0x304)==0xffffffffu&&f.word(0x5c0)==0&&f.events==1);
    assert(!f.m()[0x468]&&!f.m()[0x568]&&!std::strcmp(reinterpret_cast<char*>(f.m()+0x5c8),"radio"));
    f.fail_event=true;assert(api.capture(&f.monitor,&b,1,&f.subject)==-1&&f.monitor.blocked&&f.events==2);
    assert(api.acknowledge(&f.monitor)==1);
}
}
