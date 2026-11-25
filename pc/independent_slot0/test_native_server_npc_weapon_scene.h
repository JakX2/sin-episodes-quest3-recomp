#pragma once
#include "native_server_npc_weapon_scene.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
inline void check_npc_weapon_scene(const ServerNpcWeaponSceneApi& api){
    struct Fixture {
        alignas(4) unsigned char actor[0x790]{},event[0x3c0]{},record[0x40]{},layers[0x80]{};
        std::uint32_t active=0,inventory[48]{},selected=0;
        unsigned active_calls=0,slot_calls=0,gestures=0,selects=0,durations=0,reads=0;
        int activity=0,layer=1,sequence=0;
    } f;
    auto put=[](void* p,unsigned off,auto value){std::memcpy(static_cast<unsigned char*>(p)+off,&value,sizeof(value));};
    auto get=[](void* p,unsigned off){int v=0;std::memcpy(&v,static_cast<unsigned char*>(p)+off,4);return v;};
    auto name=[&](const char* s){std::strcpy(reinterpret_cast<char*>(f.event+0x84),s);};
    put(f.event,0,16u);put(f.event,0x188,3.f);
    put(f.actor,0x494,1u);put(f.actor,0x4a0,2);
    f.layers[0x40]=1;put(f.layers,0x48,7);put(f.layers,0x4c,.25f);
    ServerNpcWeaponSceneBindings b{};b.context=&f;
    b.active=[](void* p,void*,std::uint32_t* out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.active_calls;*out=f.active;return 1;
    };
    b.weapon_at=[](void* p,void*,unsigned slot,std::uint32_t* out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(slot<48);++f.slot_calls;*out=f.inventory[slot];return 1;
    };
    b.gesture=[](void* p,void*,int activity,int autokill,int* out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(autokill==1);++f.gestures;f.activity=activity;*out=f.layer;return 1;
    };
    b.select=[](void* p,void*,std::uint32_t weapon) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.selects;f.selected=weapon;return 1;
    };
    b.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);assert(token==1);++f.reads;*out=f.layers;*n=sizeof(f.layers);return 1;
    };
    b.duration=[](void* p,void*,int sequence,double* out) noexcept -> int{
        auto& f=*static_cast<Fixture*>(p);++f.durations;f.sequence=sequence;*out=sequence==-1?.10000000149011612:2.;return 1;
    };
    auto begin=[&](){return api.begin(f.actor,sizeof(f.actor),f.record,sizeof(f.record),f.event,sizeof(f.event),&b);};
    unsigned out=99;
    auto check=[&](float time,unsigned expected){assert(api.check(f.actor,sizeof(f.actor),f.record,sizeof(f.record),f.event,sizeof(f.event),time,&b,&out)==1&&out==expected);};
    name("AI_AIM");assert(begin()==0&&!f.active_calls&&!f.gestures);
    name("ai_holster");assert(begin()==1&&get(f.record,0x38)==2&&get(f.record,0x10)==-1&&f.active_calls==1&&!f.gestures);
    check(2.9f,1);assert(!f.reads&&!f.durations);
    f.active=22;assert(begin()==1&&get(f.record,0x10)==1&&f.activity==0x45&&f.gestures==1&&!f.selects);
    check(1.5f,1);check(1.51f,0);assert(f.sequence==7&&f.reads==2&&f.durations==2);
    check(3.f,1);assert(f.durations==2);
    f.layers[0x40]=0;check(2.8f,1);check(2.95f,0);assert(f.sequence==-1);
    f.layers[0x40]=1;
    name("AI_UNHOLSTER");assert(begin()==1&&get(f.record,0x38)==3&&get(f.record,0x10)==-1&&f.slot_calls==48&&!f.selects);
    f.inventory[3]=43;f.inventory[4]=44;
    assert(begin()==1&&get(f.record,0x10)==1&&f.activity==0x44&&f.slot_calls==53&&f.selected==43&&f.selects==1);
    // Selection still occurs when the gesture service returns -1.
    f.layer=-1;assert(begin()==1&&get(f.record,0x10)==-1&&f.selects==2);
    unsigned char before[sizeof(f.record)];std::memcpy(before,f.record,sizeof(before));
    assert(api.noop(f.record,sizeof(f.record),f.event,sizeof(f.event),&out)==1&&out==1&&!std::memcmp(before,f.record,sizeof(before)));
    put(f.record,0x38,6u);assert(api.noop(f.record,sizeof(f.record),f.event,sizeof(f.event),&out)==0);
}
}
