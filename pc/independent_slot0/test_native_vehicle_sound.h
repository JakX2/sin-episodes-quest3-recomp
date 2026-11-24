#pragma once
#include "native_vehicle_sound.h"
#include <cassert>
#include <cstring>
#include <limits>
namespace sin_port::native_weapon_tables::test {
struct VehicleSoundFixture {
    alignas(4) unsigned char vehicle[0x900]{},actor[0x1230]{},table[4*128]{};
    HologramControl control{vehicle,sizeof(vehicle)};
    const VehicleSoundApi* api=nullptr;
    VehicleSoundBindings sound{};
    float scalar=0.5f;
    int sequence_id=7;
    unsigned reads=0,emits=0,lookups=0,sets=0,resets=0,outputs=0;
    bool fail_set=false;
    char last[64]{};
    void put(unsigned offset,unsigned v){std::memcpy(vehicle+offset,&v,4);}
    void record(int i,const char* key,const char* snd){std::strcpy(reinterpret_cast<char*>(table+i*128),key);std::strcpy(reinterpret_cast<char*>(table+i*128+64),snd);}
};
inline void check_vehicle_sound(const VehicleSoundApi& api,const VehicleActionsApi& actions){
    VehicleSoundFixture f;f.api=&api;
    f.put(0x838,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.table)));f.put(0x844,4);
    f.record(0,"GEARS:Grind:0","fixture.gear");
    f.record(1,"mirror:adjust:0","fixture.mirror");
    f.record(2,"gears:grind:0","fixture.duplicate");
    f.record(3,"door:open:4","fixture.glove");
    auto& b=f.sound;b.context=&f;
    b.controller_scalar=[](void* p,void* v,float* out) noexcept -> int{
        auto& f=*static_cast<VehicleSoundFixture*>(p);assert(v==f.vehicle);++f.reads;*out=f.scalar;return 1;
    };
    b.emit=[](void* p,void* v,const char* name,unsigned a,unsigned b) noexcept -> int{
        auto& f=*static_cast<VehicleSoundFixture*>(p);assert(v==f.vehicle&&!a&&!b);++f.emits;std::strcpy(f.last,name);return 1;
    };
    b.lookup_sequence=[](void* p,void* v,const char* name,int* out) noexcept -> int{
        auto& f=*static_cast<VehicleSoundFixture*>(p);assert(v==f.vehicle&&!std::strcmp(name,"gear_shift"));++f.lookups;*out=f.sequence_id;return 1;
    };
    b.set_sequence=[](void* p,void* v,int id) noexcept -> int{
        auto& f=*static_cast<VehicleSoundFixture*>(p);assert(v==f.vehicle&&id==7);++f.sets;return !f.fail_set;
    };
    b.reset_sequence=[](void* p,void* v) noexcept -> int{
        auto& f=*static_cast<VehicleSoundFixture*>(p);assert(v==f.vehicle&&f.sets>f.resets);++f.resets;return 1;
    };
    int found=99;
    assert(api.find(f.vehicle,sizeof(f.vehicle),"gears","grind",0,&found)==1&&found==0);
    assert(api.find(f.vehicle,sizeof(f.vehicle),"door","open",4,&found)==1&&found==3);
    assert(api.find(f.vehicle,sizeof(f.vehicle),"door","close",4,&found)==1&&found==-1);
    auto play=[&](){return api.sound(f.vehicle,sizeof(f.vehicle),&b,"gears","grind",0);};
    assert(play()==1&&f.reads==1&&f.emits==1&&!std::strcmp(f.last,"fixture.gear"));
    f.scalar=0.50001f;assert(play()==1&&f.emits==1);
    f.scalar=std::numeric_limits<float>::quiet_NaN();assert(play()==1&&f.emits==2);
    f.scalar=-1;assert(play()==1&&f.emits==3);
    assert(api.sound(f.vehicle,sizeof(f.vehicle),&b,"missing","key",0)==1&&f.emits==3);
    assert(api.sequence(f.vehicle,sizeof(f.vehicle),&b,"gear_shift")==1&&f.lookups==1&&f.sets==1&&f.resets==1);
    f.sequence_id=-1;assert(api.sequence(f.vehicle,sizeof(f.vehicle),&b,"gear_shift")==1&&f.lookups==2&&f.sets==1&&f.resets==1);
    f.sequence_id=7;
    // Real native Phase756 branch -> recovered sound and sequence.
    VehicleActionBindings a{};a.context=&f;
    a.output=[](void* p,void* v,void* actor,unsigned off,int part) noexcept -> int{
        auto& f=*static_cast<VehicleSoundFixture*>(p);assert(f.control.busy&&v==f.vehicle&&actor==f.actor&&off==0x7cc&&part==11);++f.outputs;return 1;
    };
    a.resolve=[](void*,std::uint32_t,void**) noexcept -> int{return 0;};
    a.participant=[](void*,void*,unsigned*) noexcept -> int{return 0;};
    a.occupied_seat=[](void*,void*,void*,int*) noexcept -> int{return 0;};a.candidate_seat=a.occupied_seat;
    a.seat_type=[](void*,void*,int,int*) noexcept -> int{return 0;};
    a.passenger_enter=[](void*,void*,void*,int) noexcept -> int{return 0;};
    a.passenger_exit=[](void*,void*,void*,unsigned) noexcept -> int{return 0;};
    a.sound=[](void* p,void* v,const char* g,const char* act,int index) noexcept -> int{
        auto& f=*static_cast<VehicleSoundFixture*>(p);return f.api->sound(v,sizeof(f.vehicle),&f.sound,g,act,index)==1;
    };
    a.sequence=[](void* p,void* v,const char* name) noexcept -> int{
        auto& f=*static_cast<VehicleSoundFixture*>(p);return f.api->sequence(v,sizeof(f.vehicle),&f.sound,name)==1;
    };
    a.radio_next=[](void*,void*) noexcept -> int{return 0;};
    a.unimplemented=[](void*,int) noexcept -> int{return 0;};
    assert(actions.apply(&f.control,&a,f.actor,sizeof(f.actor),11)==1&&f.outputs==1&&f.emits==4&&f.sets==2&&f.resets==2);
    // Suppressed sound does not suppress the sequence request.
    f.scalar=1;
    assert(actions.apply(&f.control,&a,f.actor,sizeof(f.actor),11)==1&&f.outputs==2&&f.emits==4&&f.sets==3&&f.resets==3);
    f.fail_set=true;assert(actions.apply(&f.control,&a,f.actor,sizeof(f.actor),11)==-1&&f.control.blocked&&f.resets==3);
    assert(actions.acknowledge(&f.control)==1);
    f.scalar=0;std::memset(f.table+64,'x',64);assert(play()==-1&&f.emits==4);
    f.put(0x838,0);assert(api.find(f.vehicle,sizeof(f.vehicle),"gears","grind",0,&found)==-1);
    f.put(0x844,0);assert(api.find(f.vehicle,sizeof(f.vehicle),"gears","grind",0,&found)==1&&found==-1);
}
}
