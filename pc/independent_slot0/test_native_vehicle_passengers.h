#pragma once
#include "native_vehicle_passengers.h"
#include <cassert>
#include <cstring>
#include <limits>
namespace sin_port::native_weapon_tables::test {
struct PassengerFixture {
    alignas(4) unsigned char vehicle[0x900]{},views[500]{};
    HologramControl ctrl{vehicle,sizeof(vehicle)};
    int actors[3]{},combat[3]{},view=1;
    unsigned queries=0,scale_calls=0,participant_calls=0,view_calls=0,clocks=0,names=0,allowed=0,counts=0,maxes=0,grants=0;
    bool fail_grant=false,missing=false;
    float now=1,scale=2;
    void put(void* p,unsigned off,unsigned v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,4);}
    void real(void* p,unsigned off,float v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,4);}
    float number(unsigned off){float f=0;std::memcpy(&f,vehicle+off,4);return f;}
};
inline void check_vehicle_passengers(const VehiclePassengersApi& api){
    PassengerFixture f;VehiclePassengerBindings b{};b.context=&f;
    b.occupant=[](void* p,void* v,int seat,void** out) noexcept -> int{
        auto& f=*static_cast<PassengerFixture*>(p);assert(f.ctrl.busy&&v==f.vehicle);++f.queries;
        *out=seat>=0&&seat<3&&seat!=2&&!f.missing?&f.actors[seat]:nullptr;return 1;
    };
    b.global_damage_scale=[](void* p,float* out) noexcept -> int{
        auto& f=*static_cast<PassengerFixture*>(p);assert(f.ctrl.busy);++f.scale_calls;*out=f.scale;return 1;
    };
    b.participant=[](void* p,void* actor,unsigned* out) noexcept -> int{
        auto& f=*static_cast<PassengerFixture*>(p);assert(f.ctrl.busy);++f.participant_calls;*out=actor==&f.actors[0];return 1;
    };
    b.view_index=[](void* p,void* player,int* out) noexcept -> int{
        auto& f=*static_cast<PassengerFixture*>(p);assert(f.ctrl.busy&&(player==&f.actors[0]||player==&f.combat[0]));++f.view_calls;*out=f.view;return 1;
    };
    b.clock=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<PassengerFixture*>(p);assert(f.ctrl.busy);++f.clocks;*out=f.now;return 1;};
    b.combat_character=[](void* p,void* actor,void** out) noexcept -> int{
        auto& f=*static_cast<PassengerFixture*>(p);assert(f.ctrl.busy);*out=actor==&f.actors[0]?&f.combat[0]:&f.combat[1];return 1;
    };
    b.base_player_cast=[](void* p,void* combat,unsigned* out) noexcept -> int{
        auto& f=*static_cast<PassengerFixture*>(p);assert(f.ctrl.busy);*out=combat==&f.combat[0];return 1;
    };
    b.ammo_index=[](void* p,const char* name,int* out) noexcept -> int{
        auto& f=*static_cast<PassengerFixture*>(p);assert(f.ctrl.busy);
        const char* names[]={"MAG","AR","BUCK","ARNADE","FLAK","GRENADE"};
        const unsigned i=f.names++%6;assert(!std::strcmp(name,names[i]));*out=i==4?-1:static_cast<int>(i+1);return 1;
    };
    b.ammo_allowed=[](void* p,void*,int index,unsigned* out) noexcept -> int{
        auto& f=*static_cast<PassengerFixture*>(p);++f.allowed;*out=index!=2;return 1;
    };
    b.ammo_count=[](void* p,void*,int index,int* out) noexcept -> int{
        auto& f=*static_cast<PassengerFixture*>(p);++f.counts;*out=index==3?1:index==4?-1:0;return 1;
    };
    b.ammo_max=[](void* p,int index,int* out) noexcept -> int{
        auto& f=*static_cast<PassengerFixture*>(p);++f.maxes;*out=index==1?36:index==4?3:5;return 1;
    };
    b.give_ammo=[](void* p,void* combat,int amount,int index,unsigned argument) noexcept -> int{
        auto& f=*static_cast<PassengerFixture*>(p);assert(f.ctrl.busy&&(combat==&f.combat[0]||combat==&f.combat[1])&&!argument);
        assert((index==1&&amount==36)||(index==4&&amount==3)||(index==6&&amount==5));++f.grants;return !f.fail_grant;
    };
    assert(api.defaults(&f.ctrl)==1&&f.number(0x864)==1&&f.number(0x868)==1&&f.number(0x86c)==1&&f.vehicle[0x74c]&&f.number(0x88c)==0);
    f.put(f.vehicle,0x7fc,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.views)));f.put(f.vehicle,0x808,5);
    f.real(f.views+100,0x60,0.25f);f.real(f.vehicle,0x864,3);f.real(f.vehicle,0x868,0.5f);f.real(f.vehicle,0x86c,2);
    float out=99;
    assert(api.damage_factor(&f.ctrl,&b,2,0x43,&out)==1&&out==0&&!f.scale_calls);
    assert(api.damage_factor(&f.ctrl,&b,0,0x43,&out)==1&&out==1.5f&&f.scale_calls==1&&f.view_calls==1);
    assert(api.damage_factor(&f.ctrl,&b,1,0x43,&out)==1&&out==6.f&&f.view_calls==1);
    assert(api.damage_factor(&f.ctrl,&b,0,0,&out)==1&&out==0.5f);
    f.view=99;assert(api.damage_factor(&f.ctrl,&b,0,0,&out)==-1&&f.ctrl.blocked);
    assert(api.acknowledge(&f.ctrl)==1);f.view=0;
    assert(api.refill(&f.ctrl,&b)==1&&f.clocks==1&&!f.names);
    f.now=1.01f;
    assert(api.refill(&f.ctrl,&b)==1&&f.names==6&&f.allowed==5&&f.counts==4&&f.maxes==3&&f.grants==3&&f.number(0x88c)==f.now);
    f.view=1;f.now=2.02f;
    assert(api.refill(&f.ctrl,&b)==1&&f.names==18&&f.grants==9);
    f.view=3;f.now=3.03f;assert(api.refill(&f.ctrl,&b)==1&&f.names==30&&f.grants==15);
    f.view=4;f.now=4.04f;assert(api.refill(&f.ctrl,&b)==1&&f.names==42&&f.grants==21);
    f.view=2;f.now=5.05f;assert(api.refill(&f.ctrl,&b)==1&&f.names==48&&f.grants==24);
    f.now=std::numeric_limits<float>::quiet_NaN();assert(api.refill(&f.ctrl,&b)==1&&f.names==48);
    assert(api.enable_ammo(&f.ctrl,2,1)==1&&!f.vehicle[0x74c]);
    const unsigned clocks=f.clocks;assert(api.refill(&f.ctrl,&b)==1&&f.clocks==clocks);
    assert(api.enable_ammo(&f.ctrl,5,1)==1&&f.vehicle[0x74c]);
    f.now=7;f.fail_grant=true;assert(api.refill(&f.ctrl,&b)==-1&&f.ctrl.blocked&&f.names==49&&f.number(0x88c)==7);
    assert(api.acknowledge(&f.ctrl)==1);
}
}
