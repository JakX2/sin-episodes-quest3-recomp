#pragma once
#include "native_vehicle_use.h"
#include <cassert>
#include <cstring>
namespace sin_port::native_weapon_tables::test {
inline void check_vehicle_use(const VehicleUseApi& api,const UseObservationApi& use,const ActionMonitorApi& actions){
    struct Fixture {
        alignas(4) unsigned char vehicle[0x900]{},actor[0x1230]{};
        HologramControl ctrl{vehicle,sizeof(vehicle)},monitor{actor+0xc40,0x5f0};
        int part=2,second=2,seat=0,last=-1;
        unsigned picks=0,players=0,applied=0,events=0,seats=0;
        bool inside=true,participant=true,fail=false;
        void put(unsigned off,std::uint32_t v){std::memcpy(actor+off,&v,4);}
        unsigned word(unsigned off){unsigned v;std::memcpy(&v,actor+off,4);return v;}
    } f;
    const char* names[]={"Door","AirConditioning","Radio","GloveBox","RightFrontWindow","RearLeftWindow","RearRightWindow","DomeLight","GPS","EmergencyButton","Wipers","GearShift","Sunroof","RearViewMirror","Trunk","Hood"};
    for(int i=0;i<16;++i)assert(!std::strcmp(api.part_name(i),names[i]));
    assert(!api.part_name(-1)&&!api.part_name(16));
    VehicleUseBindings b{};
    b.context=&f;
    b.pick_part=[](void* p,void* v,void* a,int* out) noexcept -> int {
        auto& f=*static_cast<Fixture*>(p);assert(f.ctrl.busy&&v==f.vehicle&&a==f.actor);
        *out=(f.picks++%2)?f.second:f.part;return 1;
    };
    b.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int {
        auto& f=*static_cast<Fixture*>(p);assert(h==0x1001);*out=f.inside?f.vehicle:nullptr;return 1;
    };
    b.participant=[](void* p,void*,unsigned* out) noexcept -> int {
        auto& f=*static_cast<Fixture*>(p);++f.players;*out=f.participant;return 1;
    };
    b.outside_seat=[](void* p,void*,void*,int* out) noexcept -> int {
        auto& f=*static_cast<Fixture*>(p);++f.seats;*out=f.seat;return 1;
    };
    b.apply_part=[](void* p,void*,void*,int part) noexcept -> int {
        auto& f=*static_cast<Fixture*>(p);assert(f.ctrl.busy&&!(f.word(0x93c)&0x20));++f.applied;f.last=part;return !f.fail;
    };
    unsigned allowed=99,executed=99;
    auto admit=[&](int part){f.part=f.second=part;f.picks=0;assert(api.can_use(&f.ctrl,&b,f.actor,sizeof(f.actor),&allowed)==1);return allowed;};
    f.put(0xe8,0x1001);
    assert(!admit(2)&&f.picks==0);
    f.vehicle[0x74a]=1;
    assert(!admit(-1)&&f.picks==1);
    for(int i=0;i<16;++i)assert(admit(i)==unsigned(i!=8&&i!=14&&i!=15));
    f.actor[0x9b4]=1;assert(!admit(2));f.actor[0x9b4]=0;
    f.inside=false;assert(!admit(4));f.actor[0xbc]=1;
    for(int i=0;i<16;++i)assert(admit(i)==unsigned(i==0||i==4||i==14||i==15));
    assert(f.seats==1);f.seat=-1;assert(!admit(0)&&f.seats==2);
    f.participant=false;assert(!admit(4));f.participant=true;
    f.put(0xe8,0xffffffffu);assert(admit(14));f.put(0xe8,0x1001);f.inside=true;
    auto* record=f.actor+0xf48;
    auto call=[&](int type,void* rec,int first,int second){f.part=first;f.second=second;f.picks=0;f.put(0x93c,0xffffffffu);return api.use(&f.ctrl,&b,f.actor,sizeof(f.actor),type,rec,rec?0x2e0:0,&executed);};
    assert(call(0,record,2,2)==1&&!executed&&f.picks==1&&f.word(0x93c)==0xffffffdfu);
    assert(call(3,record,2,-1)==1&&!executed&&f.picks==2);
    assert(call(3,nullptr,2,10)==1&&executed&&f.last==10&&f.applied==1);
    assert(call(3,record,2,2)==1&&executed&&f.last==2&&!std::strcmp(reinterpret_cast<char*>(record+0x2c0),"Radio"));
    // Feed the actual case-sensitive label through the recovered producer.
    f.put(0xc44,0x2001);
    UseObservationBindings u{};u.monitor.context=&f;
    u.monitor.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int {auto& f=*static_cast<Fixture*>(p);assert(h==0x2001);*out=f.actor;return 1;};
    u.monitor.clock=[](void*,float* out) noexcept -> int {*out=10;return 1;};
    u.monitor.packed_handle=[](void* p,void* v,std::uint32_t* out) noexcept -> int {auto& f=*static_cast<Fixture*>(p);assert(v==f.vehicle);*out=0x3001;return 1;};
    u.monitor.sample=[](void* p,void* v,ActionEntitySample* out) noexcept -> int {
        auto& f=*static_cast<Fixture*>(p);if(v==f.actor)std::strcpy(out->classname,"player");
        else{assert(v==f.vehicle);std::strcpy(out->classname,"prop_vehicle_sin");std::strcpy(out->name,"car");}return 1;
    };
    u.event=[](void* p,int descriptor,void* owner,void* subject,const char* part) noexcept -> int {
        auto& f=*static_cast<Fixture*>(p);assert(descriptor==1&&owner==f.actor&&subject==f.vehicle&&!std::strcmp(part,"Radio"));++f.events;return 1;
    };
    assert(use.capture(&f.monitor,&u,1,f.vehicle)==1&&f.events==1);
    ActionCriteria criteria{};
    assert(actions.use_criteria(record,0x2e0,&criteria)==1);
    assert(!std::strcmp(criteria.text,"PlayerUsing:car,PlayerUsingEnt:prop_vehicle_sin,PlayerUsingPart:Radio,PlayerUsingAction:USE_ACTION_USE"));
    assert(call(3,record,8,2)==1&&!executed&&f.picks==1&&(f.word(0x93c)&0x20));
    f.fail=true;assert(call(3,record,2,2)==-1&&f.ctrl.blocked&&!executed);
    assert(api.acknowledge(&f.ctrl)==1);
}
}
