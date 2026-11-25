#include "native_server_npc_schedule_state.h"
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
struct F {
    alignas(4) unsigned char actor[0xbf4]{},desc[0x48]{},schedule[0x30]{},tasks[16]{};
    unsigned resets=0;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    unsigned get(unsigned off){unsigned v=0;std::memcpy(&v,actor+off,4);return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcScheduleStateApi>(GetProcAddress(s,server_npc_schedule_state_export));
    assert(g&&!GetProcAddress(c,server_npc_schedule_state_export)&&!g(0,sizeof(ServerNpcScheduleStateApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcScheduleStateApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base+10);f.put(f.desc,0x38,1);f.put(f.desc,0x3c,40);
    f.put(f.schedule,0,npc_global_condition_base+7);f.put(f.schedule,4,2u);f.put(f.schedule,8,2);
    f.put(f.schedule,0xc,1u<<11);
    f.put(f.tasks,0,40);f.put(f.tasks,4,.5f);f.put(f.tasks,8,41);f.put(f.tasks,12,2.f);
    ServerNpcScheduleBindings b{};b.context=&f;b.conditions.context=&f;
    b.conditions.descriptor=[](void* p,void* a,const void** out,unsigned* n) noexcept -> int {auto& f=F::self(p);assert(a==f.actor);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.storage=[](void* p,std::uint32_t tok,const void** out,unsigned* n) noexcept -> int {
        auto& f=F::self(p);if(tok==1){*out=f.schedule;*n=sizeof(f.schedule);}
        else if(tok==2){*out=f.tasks;*n=sizeof(f.tasks);}else return 0;return 1;};
    b.reset_movement=[](void* p,void* a,std::uint32_t tok) noexcept -> int {
        auto& f=F::self(p);assert(a==f.actor&&tok==0x1234&&f.get(0x798)==1&&f.get(0x7cc)==(1u<<10));++f.resets;return 1;};
    f.put(f.actor,0x8f8,0x1234u);f.put(f.actor,0x7cc,0xffffffffu);f.put(f.actor,0x80c,0x12345678u);f.actor[0x989]=0xff;
    f.put(f.actor,0x79c,99u);f.put(f.actor,0x7bc,42u);
    ServerNpcScheduleStateResult out{};assert(api->install(f.actor,sizeof(f.actor),1,12.f,&b,&out)&&out.preserved_condition&&out.count==2&&f.resets==1);
    assert(f.get(0x79c)==99&&f.get(0x7bc)==0&&f.get(0x7cc)==(1u<<10)&&f.get(0x80c)==0x12345678u&&f.actor[0x989]==0xdf);
    for(unsigned i=0;i<8;++i)assert(f.get(0x82c+4*i)==0xffffffffu);
    assert(api->inspect(f.actor,sizeof(f.actor),&b,&out)&&out.has_task&&out.task.id==40&&out.task.parameter==.5f);
    unsigned yes=9;
    assert(api->interrupt(f.actor,sizeof(f.actor),2,false,&b,&yes)&&yes);
    assert(api->interrupt(f.actor,sizeof(f.actor),2,true,&b,&yes)&&!yes);
    f.put(f.actor,0x7cc,(1u<<10)|(1u<<11));
    assert(api->interrupt(f.actor,sizeof(f.actor),2,true,&b,&yes)&&yes);
    f.put(f.actor,0x7a4,3);f.put(f.actor,0x7b4,9);
    assert(api->advance(f.actor,sizeof(f.actor),&b,&out)&&out.index==1&&!out.complete&&f.get(0x7a4)==0&&f.get(0x7b4)==0);
    assert(api->inspect(f.actor,sizeof(f.actor),&b,&out)&&out.task.id==41);
    f.put(f.actor,0xbec,77u);f.put(f.actor,0xbf0,88u);
    assert(api->advance(f.actor,sizeof(f.actor),&b,&out)&&out.complete&&out.index==2&&!f.get(0xbec)&&!f.get(0xbf0));
    assert(f.get(0x7d0)&(1u<<11)); // local34 -> global bit43
    assert(!api->advance(f.actor,sizeof(f.actor),&b,&out));
    assert(api->clear(f.actor,sizeof(f.actor))&&!f.get(0x798)&&!f.get(0x79c)&&f.get(0x7d0)==(1u<<11));
    assert(api->inspect(f.actor,sizeof(f.actor),&b,&out)&&!out.schedule&&!out.has_task);
    assert(api->interrupt(f.actor,sizeof(f.actor),2,false,&b,&yes)&&!yes);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original native schedule state, task cursor and declared interrupts");
}
