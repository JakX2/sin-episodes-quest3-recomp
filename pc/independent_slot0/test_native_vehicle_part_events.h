#pragma once
#include "native_vehicle_part_events.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
#include <limits>
namespace sin_port::native_weapon_tables::test {
struct PartEventsFixture {
    alignas(4) unsigned char vehicle[vehicle_object_bytes]{},doors[152]{},windows[152]{};
    HologramControl c{vehicle,sizeof(vehicle)};
    float pose[4]{},frame=.125f;
    unsigned events[64]{},n=0,reads=0,frames=0;
    bool fail_output=false;
    void push(unsigned e){assert(c.busy&&n<64);events[n++]=e;}
    void expect(std::initializer_list<unsigned> e){assert(n==e.size()&&!std::memcmp(events,e.begin(),4*n));n=0;}
    void put(void* p,unsigned off,unsigned word){std::memcpy(static_cast<unsigned char*>(p)+off,&word,4);}
    unsigned ptr(void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p));}
};
inline void check_vehicle_part_events(const VehiclePartEventsApi& api){
    PartEventsFixture f;
    f.put(f.vehicle,0x810,f.ptr(f.doors));f.put(f.vehicle,0x81c,2);
    f.put(f.vehicle,0x824,f.ptr(f.windows));f.put(f.vehicle,0x830,2);
    f.put(f.doors,0x40,0);f.put(f.doors+76,0x40,1);
    f.put(f.windows,0x40,2);f.put(f.windows+76,0x40,3);
    f.doors[0x44]=1;f.doors[0x45]=1;
    f.windows[0x44]=1;f.windows[76+0x44]=0;
    f.pose[3]=1;
    VehiclePartEventBindings b{};b.context=&f;
    b.pose=[](void* p,void* v,int index,float* out) noexcept -> int{
        auto& f=*static_cast<PartEventsFixture*>(p);assert(f.c.busy&&v==f.vehicle&&index>=0&&index<4);++f.reads;*out=f.pose[index];return 1;
    };
    b.frame_time=[](void* p,float* out) noexcept -> int{auto& f=*static_cast<PartEventsFixture*>(p);assert(f.c.busy);++f.frames;*out=f.frame;return 1;};
    b.set_pose=[](void* p,void*,int index,float v) noexcept -> int{
        auto& f=*static_cast<PartEventsFixture*>(p);assert(index>=0&&index<4);f.push(100+index);f.pose[index]=v;return 1;
    };
    b.output=[](void* p,void* v,unsigned offset,int index) noexcept -> int{
        auto& f=*static_cast<PartEventsFixture*>(p);assert(v==f.vehicle&&index>=0&&index<2);
        if(offset==0x784){assert(f.pose[index]<1);f.push(200+index);}
        else if(offset==0x76c){assert(f.pose[index]>0);f.push(210+index);}
        else if(offset==0x7b4){assert(f.pose[index+2]<1);f.push(220+index);}
        else{assert(offset==0x79c&&f.pose[index+2]>0);f.push(230+index);}
        return !f.fail_output;
    };
    b.sound=[](void* p,void*,const char* group,const char* action,int index) noexcept -> int{
        auto& f=*static_cast<PartEventsFixture*>(p);assert(!std::strcmp(group,"door")&&!std::strcmp(action,"close")&&index==0&&!f.doors[0x44]&&!f.doors[0x45]&&f.pose[0]<1);f.push(300);return 1;
    };
    auto tick=[&](){return api.tick(&f.c,&b);};
    assert(tick()==1&&f.pose[0]==.5f&&f.pose[2]==.5f&&f.pose[3]==.5f&&f.doors[0x45]&&f.reads==13&&f.frames==9);f.expect({100,102,103});
    assert(tick()==1&&f.pose[0]==1&&f.pose[2]==1&&f.pose[3]==0&&!f.doors[0x44]&&!f.doors[0x45]);
    f.expect({200,300,100,220,102,231,103});
    assert(tick()==1&&f.pose[0]==.5f);f.expect({100});
    assert(tick()==1&&!f.pose[0]);f.expect({210,100});
    assert(tick()==1);f.expect({});
    // Overshoot clamps and emits once, including normal doors.
    f.doors[76+0x44]=1;f.frame=1;
    assert(tick()==1&&f.pose[1]==1);f.expect({201,101});
    assert(tick()==1);f.expect({});
    f.doors[76+0x44]=0;f.windows[0x44]=0;
    assert(tick()==1&&!f.pose[1]&&!f.pose[2]);f.expect({211,101,230,102});
    // Unordered initial pose is skipped by the original endpoint gate.
    f.pose[0]=std::numeric_limits<float>::quiet_NaN();f.doors[0x44]=1;
    assert(tick()==1);f.expect({});
    f.pose[0]=0;f.fail_output=true;
    assert(tick()==-1&&f.c.blocked&&f.pose[0]==0&&f.doors[0x44]);f.expect({200});
    assert(api.acknowledge(&f.c)==1);f.fail_output=false;
    f.put(f.vehicle,0x810,0);assert(tick()==-1&&f.c.blocked);f.expect({});
}
}
