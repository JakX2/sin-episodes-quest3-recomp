#include "native_server_npc_schedule_validity.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xbf4]{},desc[0x48]{},schedule[0x30]{};
    unsigned builds=0,mobility=0;int named=-1;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    unsigned get(unsigned off){unsigned v=0;std::memcpy(&v,actor+off,4);return v;}
    void condition(int id){unsigned off=0x7cc+4*(id/32);put(actor,off,get(off)|(1u<<(id%32)));}
    bool interrupt(int id){return (get(0x7ec+4*(id/32))&(1u<<(id%32)))!=0;}
    void clear(){std::memset(actor+0x7cc,0,32);}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcScheduleValidityApi>(GetProcAddress(s,server_npc_schedule_validity_export));
    assert(g&&!GetProcAddress(c,server_npc_schedule_validity_export)&&!g(0,sizeof(ServerNpcScheduleValidityApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcScheduleValidityApi));assert(api);
    F f;f.put(f.desc,4,1000000000);f.put(f.desc,8,0);f.put(f.desc,12,127);
    f.put(f.desc,0x34,1000000000);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.put(f.schedule,0,1000000001);f.put(f.schedule,8,2);
    f.put(f.schedule,0xc,(1u<<0x14)|(1u<<0x15)|(1u<<5));
    f.put(f.actor,0x798,1u);f.put(f.actor,0x79c,1000000001);f.put(f.actor,0x60c,12.f);
    ServerNpcScheduleValidityBindings b{};b.context=&f;b.schedule.context=&f;b.schedule.conditions.context=&f;
    b.schedule.storage=[](void* p,std::uint32_t tok,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(tok!=1)return 0;*out=f.schedule;*n=sizeof(f.schedule);return 1;};
    b.schedule.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.base_build=[](void* p,void*) noexcept -> int{++F::self(p).builds;return 1;};
    b.mobility=[](void* p,float factor,bool* out) noexcept -> int{assert(factor==1.f);++F::self(p).mobility;*out=true;return 1;};
    b.name_token=[](void* p,void*,int global,std::uint32_t* out) noexcept -> int{F::self(p).named=global;*out=0;return 1;};
    b.unknown_name=777;
    ServerNpcScheduleValidityResult out{};
    auto run=[&](float now=10.f){assert(api->validate(f.actor,sizeof(f.actor),now,&b,&out));};
    run();assert(out.valid&&f.builds==1&&f.mobility==1&&!f.interrupt(0x14)&&!f.interrupt(0x15)&&f.interrupt(0x46)&&f.interrupt(0x47)&&f.interrupt(0x37)&&f.interrupt(0x3f));
    run(12.f);assert(f.interrupt(0x14)&&f.interrupt(0x15));
    f.condition(0x47);f.condition(5);run();assert(out.reason==ScheduleInterrupted&&out.first_bit==5&&out.matched==2&&!out.recorded);
    b.record_interrupt=true;f.put(f.actor,0xbec,99u);run();assert(out.recorded&&f.named==1000000005&&f.get(0xbe8)==777&&f.get(0xbf0)==1&&!f.get(0xbec));
    f.clear();f.condition(0x47);b.jetpack=true;run();assert(out.valid&&out.jetpack_clear&&!f.interrupt(0x47));
    f.put(f.actor,0x79c,1000000000+0x68);f.put(f.schedule,0x14,1u<<7);
    unsigned prior=f.builds;run();assert(out.valid&&out.burning&&f.builds==prior&&!f.interrupt(0x47)&&!f.interrupt(0x46));
    b.jetpack=false;run();assert(out.reason==ScheduleInterrupted&&out.first_bit==0x47);
    f.clear();f.condition(0x22);f.condition(0x21);run();assert(out.reason==ScheduleCompleted);
    f.clear();f.condition(0x21);run();assert(out.reason==ScheduleFailed);
    f.clear();f.condition(0x3f);f.put(f.actor,0x850,4);run();assert(!out.builder_called&&out.reason==ScheduleInterrupted&&out.first_bit==0x3f);
    f.put(f.actor,0x798,0u);run();assert(out.reason==ScheduleMissing);
    f.put(f.actor,0x798,1u);f.put(f.schedule,8,0);run();assert(out.reason==ScheduleEmpty);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native schedule validity, ground and Jetpack interrupt masks");
}
