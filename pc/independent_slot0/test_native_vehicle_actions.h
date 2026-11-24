#pragma once
#include "native_vehicle_actions.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
namespace sin_port::native_weapon_tables::test {
struct VehicleActionsFixture {
    alignas(4) unsigned char vehicle[0x900]{},actor[0x1230]{},doors[5*76]{},windows[5*76]{};
    HologramControl ctrl{vehicle,sizeof(vehicle)};
    const VehicleActionsApi* api=nullptr;
    VehicleActionBindings b{};
    unsigned log[32]{},count=0;
    int occupied=-1,candidate=1,type=0,radio=7,last_output=-1,last_index=-1;
    bool in_vehicle=true,radio_live=true,player=true,fail_sound=false;
    const char* group=nullptr;const char* action=nullptr;const char* sequence=nullptr;
    void push(unsigned n){assert(ctrl.busy&&count<32);log[count++]=n;}
    void expect(std::initializer_list<unsigned> e){assert(e.size()==count&&!std::memcmp(e.begin(),log,count*4));count=0;}
    void put(void* o,unsigned off,unsigned w){std::memcpy(static_cast<unsigned char*>(o)+off,&w,4);}
    unsigned ptr(void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p));}
};
inline void check_vehicle_actions(const VehicleActionsApi& api,const VehicleUseApi& use){
    VehicleActionsFixture f;f.api=&api;auto& b=f.b;b.context=&f;
    f.put(f.vehicle,0x810,f.ptr(f.doors));f.put(f.vehicle,0x81c,5);
    f.put(f.vehicle,0x824,f.ptr(f.windows));f.put(f.vehicle,0x830,5);
    f.put(f.actor,0xe8,0x1001);f.put(f.vehicle,0x768,0x2001);f.vehicle[0x74a]=1;
    b.output=[](void* p,void* v,void* a,unsigned off,int part) noexcept -> int {
        auto& f=*static_cast<VehicleActionsFixture*>(p);assert(v==f.vehicle&&a==f.actor&&off==0x7cc);
        f.push(1);f.last_output=part;return 1;
    };
    b.resolve=[](void* p,std::uint32_t handle,void** out) noexcept -> int {
        auto& f=*static_cast<VehicleActionsFixture*>(p);f.push(2);
        if(handle==0x1001)*out=f.in_vehicle?f.vehicle:nullptr;
        else{assert(handle==0x2001);*out=f.radio_live?&f.radio:nullptr;}return 1;
    };
    b.participant=[](void* p,void*,unsigned* out) noexcept -> int{auto& f=*static_cast<VehicleActionsFixture*>(p);f.push(3);*out=f.player;return 1;};
    b.occupied_seat=[](void* p,void*,void*,int* out) noexcept -> int{auto& f=*static_cast<VehicleActionsFixture*>(p);f.push(4);*out=f.occupied;return 1;};
    b.candidate_seat=[](void* p,void*,void*,int* out) noexcept -> int{auto& f=*static_cast<VehicleActionsFixture*>(p);f.push(5);*out=f.candidate;return 1;};
    b.seat_type=[](void* p,void*,int seat,int* out) noexcept -> int{auto& f=*static_cast<VehicleActionsFixture*>(p);assert(seat==f.candidate);f.push(6);*out=f.type;return 1;};
    b.passenger_enter=[](void* p,void*,void*,int requested) noexcept -> int{auto& f=*static_cast<VehicleActionsFixture*>(p);assert(requested==-1);f.push(7);return 1;};
    b.passenger_exit=[](void* p,void*,void*,unsigned argument) noexcept -> int{auto& f=*static_cast<VehicleActionsFixture*>(p);assert(!argument);f.push(8);return 1;};
    b.sound=[](void* p,void*,const char* group,const char* action,int index) noexcept -> int{
        auto& f=*static_cast<VehicleActionsFixture*>(p);f.push(9);f.group=group;f.action=action;f.last_index=index;return !f.fail_sound;
    };
    b.sequence=[](void* p,void*,const char* name) noexcept -> int{auto& f=*static_cast<VehicleActionsFixture*>(p);f.push(10);f.sequence=name;return 1;};
    b.radio_next=[](void* p,void* radio) noexcept -> int{auto& f=*static_cast<VehicleActionsFixture*>(p);assert(radio==&f.radio);f.push(11);return 1;};
    b.unimplemented=[](void* p,int part) noexcept -> int{auto& f=*static_cast<VehicleActionsFixture*>(p);assert(part==f.last_output&&(part==1||part==7||part==9));f.push(12);return 1;};
    auto run=[&](int part){assert(api.apply(&f.ctrl,&b,f.actor,sizeof(f.actor),part)==1&&f.last_output==part);};
    auto sound=[&](const char* g,const char* a,int i){assert(!std::strcmp(f.group,g)&&!std::strcmp(f.action,a)&&f.last_index==i);};
    for(int part:{1,7,9}){run(part);f.expect({1,12});}
    run(-1);f.expect({1});run(16);f.expect({1});
    run(0);f.expect({1,3,4,5,6,7});
    f.vehicle[0x751]=1;run(0);f.expect({1,3,4,5,6});
    f.type=1;run(0);f.expect({1,3,4,5,6,7});
    f.occupied=0;run(0);f.expect({1,3,4,8});
    f.occupied=-1;f.candidate=-1;run(0);f.expect({1,3,4,5});
    f.candidate=1;f.in_vehicle=false;run(4);f.expect({1,2,3,4,5,6,7});
    f.in_vehicle=true;
    for(const int part:{3,14,15,4,5,6,12}){
        const bool w=part==4||part==5||part==6||part==12;
        const int index=part==3?4:part==14?2:part==15?3:part==4?1:part==5?3:part==6?2:4;
        auto* flag=(w?f.windows:f.doors)+index*76+0x44;
        run(part);if(part==4)f.expect({1,2,9});else f.expect({1,9});
        sound(w?"window":"door","open",index);assert(*flag==1);
        run(part);if(part==4)f.expect({1,2,9});else f.expect({1,9});
        sound(w?"window":"door","close",index);assert(!*flag);
    }
    f.put(f.vehicle,0x81c,2);run(3);f.expect({1});
    run(2);f.expect({1,2,2,11});f.radio_live=false;run(2);f.expect({1,2});
    run(8);f.expect({1,9});sound("monitor","open",0);assert(f.vehicle[0x758]);
    run(8);f.expect({1,9});sound("monitor","close",0);assert(!f.vehicle[0x758]);
    run(10);f.expect({1});assert(f.vehicle[0x74e]);run(10);f.expect({1});assert(!f.vehicle[0x74e]);
    run(11);f.expect({1,9,10});sound("gears","grind",0);assert(!std::strcmp(f.sequence,"gear_shift"));
    run(13);f.expect({1,9,10});sound("mirror","adjust",0);assert(!std::strcmp(f.sequence,"rear_view_mirror"));
    // Compose Phase755's admitted use with Phase756's action body.
    VehicleUseBindings u{};u.context=&f;
    u.pick_part=[](void*,void*,void*,int* out) noexcept -> int{*out=10;return 1;};
    u.resolve=b.resolve;u.participant=b.participant;
    u.outside_seat=[](void*,void*,void*,int* out) noexcept -> int{*out=0;return 1;};
    u.apply_part=[](void* p,void* vehicle,void* actor,int part) noexcept -> int{
        auto& f=*static_cast<VehicleActionsFixture*>(p);
        return f.api->apply_leaf(vehicle,sizeof(f.vehicle),&f.b,actor,sizeof(f.actor),part)==1;
    };
    unsigned executed=0;alignas(4) unsigned char record[0x2e0]{};
    assert(use.use(&f.ctrl,&u,f.actor,sizeof(f.actor),3,record,sizeof(record),&executed)==1&&executed&&f.vehicle[0x74e]);
    f.expect({2,3,3,1});assert(!std::strcmp(reinterpret_cast<char*>(record+0x2c0),"Wipers"));
    f.fail_sound=true;assert(api.apply(&f.ctrl,&b,f.actor,sizeof(f.actor),8)==-1&&f.ctrl.blocked&&!f.vehicle[0x758]);f.expect({1,9});
    assert(api.acknowledge(&f.ctrl)==1);
}
}
