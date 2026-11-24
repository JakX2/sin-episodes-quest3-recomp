#pragma once
#include "native_count_scheduler.h"
#include "native_action_monitor.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
namespace sin_port::native_weapon_tables::test {
struct CountScheduleFixture {
    alignas(4) unsigned char npc[0x103c]{},obs[0x5ec]{};
    HologramControl control{npc,sizeof(npc)};
    CountSchedulerState state{};
    std::uint32_t entries[4]{};
    unsigned events[64]{},n=0,external=0x12345678,other=0x87654321;
    bool external_ok=true,replace_entry=false,gather_fail=false;
    int schedule=100000,exp=1;
    float now=5;
    const char* last=nullptr;
    const ActionMonitorApi* actions=nullptr;
    std::uint32_t ptr(const void* p){return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));}
    std::uint32_t embedded(){return ptr(npc+0x1000);}
    void put(unsigned off,std::uint32_t v){std::memcpy(npc+off,&v,4);}
    std::uint32_t word(unsigned off){std::uint32_t v=0;std::memcpy(&v,npc+off,4);return v;}
    void push(unsigned v){assert(control.busy&&n<64);events[n++]=v;}
    void expect(std::initializer_list<unsigned> a){assert(n==a.size()&&!std::memcmp(events,a.begin(),4*n));n=0;}
};
inline void check_count_scheduler(const CountSchedulerApi& api,const CountInterrogationApi& count,const ActionMonitorApi& actions){
    CountScheduleFixture f;f.actions=&actions;f.put(0xc1c,f.ptr(f.entries));
    CountSchedulerBindings b{};b.context=&f;
    b.append=[](void* p,void* npc,std::uint32_t behavior) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(npc==f.npc&&behavior==f.embedded());
        f.push(1);f.entries[0]=behavior;f.put(0xc28,1);return 1;
    };
    b.register_base=[](void* p,void* npc) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(npc==f.npc&&f.word(0x1004)==f.ptr(f.npc)&&f.word(0x100c)==f.ptr(f.npc+0xc14));
        f.push(2);f.entries[1]=f.external;f.entries[2]=f.other;f.put(0xc28,3);return 1;
    };
    b.eligible=[](void* p,std::uint32_t behavior,unsigned* out) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(behavior==f.external||behavior==f.other);
        f.push(3);*out=behavior==f.external&&f.external_ok;
        if(*out&&f.replace_entry)f.entries[1]=f.other;
        return 1;
    };
    b.begin=[](void* p,std::uint32_t behavior) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(f.state.busy&&!f.state.gathering&&f.word(0xc18)==behavior);f.push(4);return 1;
    };
    b.gather=[](void* p,std::uint32_t behavior) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(f.state.busy&&f.state.gathering&&f.word(0xc18)==behavior);f.push(5);return !f.gather_fail;
    };
    b.end=[](void* p,std::uint32_t behavior) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(f.state.busy&&!f.state.gathering&&f.word(0xc18)!=behavior);f.push(6);return 1;
    };
    b.reset=[](void* p,void* npc) noexcept -> int{auto& f=*static_cast<CountScheduleFixture*>(p);assert(npc==f.npc);f.push(7);return 1;};
    b.changed=[](void* p,void* npc,std::uint32_t prior,std::uint32_t selected) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(npc==f.npc&&prior!=selected&&selected==f.word(0xc18));f.push(8);return 1;
    };
    b.schedule=[](void* p,std::uint32_t behavior,int* out) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(f.npc[0xc30]==1&&behavior==f.word(0xc18));f.push(9);*out=f.schedule;return 1;
    };
    b.is_local=[](void* p,std::uint32_t behavior,int schedule,unsigned* out) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(behavior!=f.embedded()&&schedule==f.schedule);f.push(10);*out=0;return 1;
    };
    b.translate=[](void* p,std::uint32_t behavior,int schedule,int* out) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(behavior==f.embedded()&&schedule==f.schedule);f.push(11);*out=500;return 1;
    };
    b.base_schedule=[](void* p,void* npc,int* out) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(npc==f.npc&&!f.word(0xc18)&&f.npc[0xc30]);f.push(12);*out=99;return 1;
    };
    b.observe_other=[](void* p,std::uint32_t behavior,const void* obs,unsigned bytes) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(behavior!=f.embedded()&&obs==f.obs&&bytes==sizeof(f.obs));f.push(13);return 1;
    };
    assert(count.initialize(&f.control)==1&&api.register_interrogation(&f.control,&b)==1);f.expect({1,2});
    int result=0;auto select=[&](){return api.select(&f.control,&f.state,&b,&result);};
    assert(select()==1&&f.word(0xc18)==f.external&&result==100000);f.expect({3,4,5,8,9,10});
    assert(select()==1&&f.word(0xc18)==f.external);f.expect({3,9,10});
    assert(api.observe(&f.control,&b,nullptr,f.obs,sizeof(f.obs))==1);f.expect({13});
    assert(count.input(&f.control,1)==1&&select()==1&&f.word(0xc18)==f.embedded()&&result==500);f.expect({4,5,6,7,8,9,11});
    for(int n:{99999,110000}){f.schedule=n;assert(select()==1&&result==n);f.expect({9});}
    f.schedule=109999;assert(select()==1&&result==500);f.expect({9,11});
    CountBindings cb{};cb.context=&f;
    cb.resolve=[](void* p,std::uint32_t h,void** out) noexcept -> int{auto& f=*static_cast<CountScheduleFixture*>(p);assert(h==0x1001);*out=f.npc;return 1;};
    cb.clock=[](void* p,float* out) noexcept -> int{*out=static_cast<CountScheduleFixture*>(p)->now;return 1;};
    cb.expeditor=[](void* p,void* npc,void** out) noexcept -> int{auto& f=*static_cast<CountScheduleFixture*>(p);assert(npc==f.npc);*out=&f.exp;return 1;};
    cb.speaking=[](void*,void*,unsigned* out) noexcept -> int{*out=0;return 1;};
    cb.criteria=[](void* p,const void* record,float now,CountCriteria* out) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);ActionCriteria c{};assert(record==f.obs+0x18);
        if(f.actions->look_criteria(record,0x2e8,now,&c)!=1)return 0;
        std::memcpy(out->bytes,c.text,sizeof(c.text));return 1;
    };
    cb.speak=[](void* p,void*,const char* name,const CountCriteria* criteria,unsigned* played) noexcept -> int{
        auto& f=*static_cast<CountScheduleFixture*>(p);assert(std::strstr(reinterpret_cast<const char*>(criteria->bytes),"PlayerLookingAtPart:head"));
        f.last=name;*played=0;f.push(14);return 1;
    };
    const unsigned target=0x1001;std::memcpy(f.obs+0x14,&target,4);
    std::strcpy(reinterpret_cast<char*>(f.obs+0x18+0x260),"npc_count");
    std::strcpy(reinterpret_cast<char*>(f.obs+0x2d8),"head");
    assert(api.observe(&f.control,&b,&cb,f.obs,sizeof(f.obs))==1&&!std::strcmp(f.last,"TLK_INTERROGATE_COCKY")&&f.word(0x1014)==1);f.expect({14});
    assert(count.input(&f.control,0)==1);
    // Stop changes eligibility, not currently installed active pointer.
    assert(api.observe(&f.control,&b,&cb,f.obs,sizeof(f.obs))==1);f.expect({});
    f.external_ok=false;assert(select()==1&&!f.word(0xc18)&&result==99);f.expect({3,3,6,7,8,12});
    assert(api.observe(&f.control,&b,&cb,f.obs,sizeof(f.obs))==1);f.expect({});
    f.external_ok=true;f.replace_entry=true;
    assert(select()==1&&f.word(0xc18)==f.other);f.expect({3,4,5,8,9,10});
    assert(count.input(&f.control,1)==1);f.gather_fail=true;
    assert(select()==-1&&f.control.blocked&&!f.state.busy&&!f.state.gathering);f.expect({4,5});
    assert(count.acknowledge(&f.control)==1);f.gather_fail=false;
    f.put(0xc1c,0);assert(select()==-1&&f.control.blocked);f.expect({});
}
}
