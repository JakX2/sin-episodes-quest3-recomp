#pragma once
#include "native_vehicle_nested.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
namespace sin_port::native_weapon_tables::test {
struct SeatEventFixture {
    alignas(4) unsigned char vehicle[vehicle_object_bytes]{},actor[0x1230]{},doors[228]{},views[300]{};
    HologramControl v{vehicle,sizeof(vehicle)},p{actor,sizeof(actor)};
    unsigned events[32]{},n=0;
    bool player=true,predicate=true;
    void push(unsigned e){assert(v.busy&&n<32);events[n++]=e;}
    void expect(std::initializer_list<unsigned> e){assert(n==e.size()&&!std::memcmp(events,e.begin(),4*n));n=0;}
    unsigned ptr(void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p));}
    void put(void* p,unsigned off,unsigned w){std::memcpy(static_cast<unsigned char*>(p)+off,&w,4);}
    unsigned word(void* p,unsigned off){unsigned w=0;std::memcpy(&w,static_cast<unsigned char*>(p)+off,4);return w;}
};
inline void check_vehicle_seat_events(const VehicleSeatEventsApi& api,const VehicleMapApi& map,const VehicleNestedApi& nested){
    SeatEventFixture f;
    f.put(f.vehicle,0x810,f.ptr(f.doors));f.put(f.vehicle,0x81c,3);
    f.put(f.doors,0x48,0);f.put(f.doors+76,0x48,1);f.put(f.doors+152,0x48,0);
    f.doors[152+0x44]=1;
    f.put(f.vehicle,0x7fc,f.ptr(f.views));f.put(f.vehicle,0x808,3);
    f.put(f.views,0x40,0);f.put(f.views,0x44,0); // not root
    f.put(f.views+100,0x40,0xffffffffu);f.put(f.views+100,0x44,0);f.views[100+0x55]=3;
    f.put(f.views+200,0x40,0xffffffffu);f.put(f.views+200,0x44,1);f.views[200+0x55]=7;
    f.put(f.actor,0x9b0,0xffffffffu);f.put(f.actor,0x16c,0x2001);f.put(f.vehicle,0x16c,0x3001);
    f.put(f.vehicle,0x6a4,0xffffffffu);f.put(f.vehicle,0x6a8,0xffffffffu);f.vehicle[0x764]=1;
    VehicleSeatEventBindings b{};b.context=&f;b.views.context=&f;b.hint.context=&f;
    b.base_enter=[](void* p,void* v,int seat,void* actor) noexcept -> int{auto& f=*static_cast<SeatEventFixture*>(p);assert(v==f.vehicle&&actor==f.actor);f.push(1);f.put(v,0x6a4+seat*4,0x1001);return 1;};
    b.base_exit=[](void* p,void* v,int seat) noexcept -> int{auto& f=*static_cast<SeatEventFixture*>(p);assert(v==f.vehicle&&f.word(v,0x6a4+seat*4)==0x1001);f.push(2);f.put(v,0x6a4+seat*4,0xffffffffu);return 1;};
    b.participant=[](void* p,void* actor,unsigned* out) noexcept -> int{auto& f=*static_cast<SeatEventFixture*>(p);assert(actor==f.actor);f.push(3);*out=f.player;return 1;};
    b.sound=[](void* p,void*,const char* group,const char* action,int index) noexcept -> int{auto& f=*static_cast<SeatEventFixture*>(p);assert(!std::strcmp(group,"door")&&!std::strcmp(action,"open")&&index>=0&&index<3&&!f.doors[index*76+0x44]);f.push(4);return 1;};
    b.refresh_environment=[](void* p,void*) noexcept -> int{static_cast<SeatEventFixture*>(p)->push(5);return 1;};
    b.controller_predicate=[](void* p,void*,unsigned* out) noexcept -> int{auto& f=*static_cast<SeatEventFixture*>(p);f.push(6);*out=f.predicate;return 1;};
    b.controller_followup=[](void* p,void*) noexcept -> int{static_cast<SeatEventFixture*>(p)->push(7);return 1;};
    b.views.cast_player=[](void* p,void* actor,HologramControl** out) noexcept -> int{auto& f=*static_cast<SeatEventFixture*>(p);assert(actor==f.actor);f.push(8);*out=&f.p;return 1;};
    b.views.dirty_edict=[](void* p,void* actor,std::uint32_t word) noexcept -> int{auto& f=*static_cast<SeatEventFixture*>(p);assert(actor==f.actor&&word==0x2001&&f.p.busy);f.push(9);return 1;};
    b.dirty_edict=[](void* p,void* vehicle,std::uint32_t word) noexcept -> int{auto& f=*static_cast<SeatEventFixture*>(p);assert(vehicle==f.vehicle&&word==0x3001&&f.vehicle[0x748]);f.push(10);return 1;};
    b.hint.resolve=[](void* p,std::uint32_t word,void** out) noexcept -> int{auto& f=*static_cast<SeatEventFixture*>(p);assert(word==0x1001);f.push(11);*out=f.actor;return 1;};
    b.hint.participant=b.participant;
    b.hint.send=[](void* p,void* actor,unsigned reliable,const char* name,const unsigned char* bytes,unsigned count) noexcept -> int{
        auto& f=*static_cast<SeatEventFixture*>(p);assert(actor==f.actor&&reliable==1&&!std::strcmp(name,"HintText")&&bytes[0]==1);
        const char* text=reinterpret_cast<const char*>(bytes+1);assert(count==std::strlen(text)+2);
        if(!*text){assert(f.word(f.vehicle,0x6a4)==0x1001||f.word(f.vehicle,0x6a8)==0x1001);f.push(12);}
        else if(!std::strcmp(text,"#SE1_Hint_Driver")){assert(f.word(f.actor,0x9b0)==1&&f.actor[0x9b4]==3);f.push(13);}
        else{assert(!std::strcmp(text,"#SE1_Hint_Vehicle")&&f.word(f.actor,0x9b0)==2&&f.actor[0x9b4]==7);f.push(14);}
        return 1;
    };
    assert(api.entered(&f.v,&b,0,f.actor)==1&&f.doors[0x44]&&f.doors[0x45]&&f.doors[152+0x45]&&!f.doors[76+0x44]);
    f.expect({1,3,4,5,6,7,3,8,9,11,3,13});
    f.vehicle[0x748]=1;f.doors[0x44]=0;
    assert(api.exiting(&f.v,&b,0)==1&&!f.vehicle[0x748]&&f.word(f.vehicle,0x6a4)==0xffffffffu&&f.doors[0x45]);
    f.expect({11,3,4,11,3,12,10,2});
    f.predicate=false;
    assert(api.entered(&f.v,&b,1,f.actor)==1&&f.doors[76+0x44]&&f.doors[76+0x45]);f.expect({1,3,4,5,6,3,8,9,11,3,14});
    f.vehicle[0x748]=1;assert(api.exiting(&f.v,&b,1)==1&&f.vehicle[0x748]);f.expect({11,3,11,3,12,2});
    f.vehicle[0x764]=0;f.vehicle[0x1f0]=1;
    assert(api.entered(&f.v,&b,0,f.actor)==1);f.expect({1,3,5,6,3,8,9});
    assert(api.exiting(&f.v,&b,0)==1&&!f.vehicle[0x748]&&f.vehicle[0x200]);f.expect({11,3,2});
    f.player=false;f.vehicle[0x764]=1;
    assert(api.entered(&f.v,&b,1,f.actor)==1);f.expect({1,3,5,6,3,11,3});
    assert(api.exiting(&f.v,&b,1)==1);f.expect({11,3,11,3,2});
    assert(api.exiting(&f.v,&b,1)==-1&&f.v.blocked);f.expect({});
    assert(api.acknowledge(&f.v)==1&&api.entered(&f.v,&b,-1,f.actor)==0);
    // Real map input -> permitted synchronous passenger callbacks.
    struct Bridge {SeatEventFixture* f;const VehicleSeatEventBindings* b;const VehicleSeatEventsApi* api;const VehicleNestedApi* nested;bool swallow_failure=false;};
    Bridge bridge{&f,&b,&api,&nested};
    VehicleMapBindings m{};m.context=&bridge;
    m.enter=[](void* c,void* v,void* actor,int role) noexcept -> int{
        auto& x=*static_cast<Bridge*>(c);assert(v==x.f->vehicle&&x.f->v.busy!=0);
        assert(x.api->entered(&x.f->v,x.b,role,actor)==0); // standalone still rejects
        assert(x.nested->exiting(&x.f->v,x.b,role)==0); // wrong permit cannot consume
        const int result=x.nested->entered(&x.f->v,x.b,role,actor);
        assert(x.f->v.busy==1); // outer lock not released
        assert(x.nested->entered(&x.f->v,x.b,role,actor)==0); // one shot
        return x.swallow_failure||result==1;
    };
    m.occupant=[](void* c,void*,int role,void** out) noexcept -> int{
        auto& x=*static_cast<Bridge*>(c);
        *out=role>=0&&role<3&&x.f->word(x.f->vehicle,0x6a4+role*4)==0x1001?x.f->actor:nullptr;return 1;
    };
    m.exit=[](void* c,void*,void* actor,unsigned arg) noexcept -> int{
        auto& x=*static_cast<Bridge*>(c);assert(actor==x.f->actor&&arg==1);
        const int result=x.nested->exiting(&x.f->v,x.b,1);
        assert(x.f->v.busy==1&&x.nested->exiting(&x.f->v,x.b,1)==0);
        return result==1;
    };
    assert(nested.entered(&f.v,&b,1,f.actor)==0&&nested.exiting(&f.v,&b,1)==0);
    assert(map.input(&f.v,&m,"EnterVehicle",5,1,f.actor)==1&&!f.v.busy);f.expect({1,3,5,6,3,11,3});
    assert(map.input(&f.v,&m,"ExitVehicle",5,1,f.actor)==1&&!f.v.busy);f.expect({11,3,11,3,2});
    assert(map.input(&f.v,&m,"ExitVehicle",5,1,f.actor)==1);f.expect({});
    bridge.swallow_failure=true;
    assert(map.input(&f.v,&m,"EnterVehicle",5,99,f.actor)==-1&&f.v.blocked&&!f.v.busy);f.expect({});
    assert(api.acknowledge(&f.v)==1);
}
}
