#include "native_server_npc_behavior_selection.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0xc2c]{},desc[0x48]{},rows[12]{},sched[4]{};
    std::vector<int> trace;
    bool allow=true;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    unsigned get(unsigned off){unsigned v=0;std::memcpy(&v,actor+off,4);return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcBehaviorSelectionApi>(GetProcAddress(s,server_npc_behavior_selection_export));
    assert(g&&!GetProcAddress(c,server_npc_behavior_selection_export)&&!g(0,sizeof(ServerNpcBehaviorSelectionApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcBehaviorSelectionApi));assert(api);
    F f;f.put(f.actor,0xc18,9u);f.put(f.actor,0xc1c,1u);f.put(f.actor,0xc28,3);
    for(unsigned i=0;i<3;++i)f.put(f.rows,4*i,i+2);
    ServerNpcBehaviorBindings b{};b.context=&f;
    b.storage=[](void* p,std::uint32_t token,const void** out,unsigned* n) noexcept -> int{
        auto& f=F::self(p);if(token==1){*out=f.rows;*n=sizeof(f.rows);}else if(token==10){*out=f.sched;*n=sizeof(f.sched);}else return 0;return 1;};
    b.eligible=[](void* p,std::uint32_t token,bool* out) noexcept -> int{auto& f=F::self(p);f.trace.push_back(10+token);*out=token!=2;return 1;};
    b.accept=[](void* p,void*,std::uint32_t token,bool* out) noexcept -> int{auto& f=F::self(p);f.trace.push_back(20+token);*out=f.allow&&token==4;return 1;};
    b.enter=[](void* p,std::uint32_t token) noexcept -> int{auto& f=F::self(p);assert(f.get(0xc18)==token);f.trace.push_back(30+token);return 1;};
    b.activate=[](void* p,std::uint32_t token) noexcept -> int{F::self(p).trace.push_back(40+token);return 1;};
    b.leave=[](void* p,std::uint32_t token) noexcept -> int{F::self(p).trace.push_back(50+token);return 1;};
    b.reset=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(60);return 1;};
    b.changed=[](void* p,void*,std::uint32_t old,std::uint32_t next) noexcept -> int{auto& f=F::self(p);assert(f.get(0xc18)==next);f.trace.push_back(100+old*10+next);return 1;};
    ServerNpcBehaviorResult out{};
    assert(api->select(f.actor,sizeof(f.actor),&b,&out)&&out.selected==4&&out.changed&&out.visited==3&&out.eligible==2);
    assert((f.trace==std::vector<int>{12,13,23,14,24,34,44,59,60,194}));
    f.trace.clear();assert(api->select(f.actor,sizeof(f.actor),&b,&out)&&out.selected==4&&!out.changed);
    assert((f.trace==std::vector<int>{12,13,23,14,24}));
    f.allow=false;f.trace.clear();assert(api->select(f.actor,sizeof(f.actor),&b,&out)&&!out.selected&&out.changed&&!f.get(0xc18));
    assert((f.trace==std::vector<int>{12,13,23,14,24,54,60,140}));
    ServerNpcScheduleBindings sb{};sb.context=&f;sb.storage=b.storage;
    sb.conditions.context=&f;sb.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    f.put(f.desc,4,npc_global_condition_base+100);f.put(f.desc,8,0x5e);f.put(f.desc,12,0x5f);
    f.put(f.sched,0,npc_global_condition_base+101);f.put(f.actor,0x79c,npc_global_condition_base+100);
    unsigned yes=9;
    assert(api->schedule_is(f.actor,sizeof(f.actor),0,true,&sb,&yes)&&yes);
    assert(api->schedule_is(f.actor,sizeof(f.actor),0x5e,true,&sb,&yes)&&!yes);
    f.put(f.actor,0x798,10u);
    assert(api->schedule_is(f.actor,sizeof(f.actor),0x5e,true,&sb,&yes)&&yes);
    assert(api->schedule_is(f.actor,sizeof(f.actor),0x5e,false,&sb,&yes)&&!yes);
    assert(api->schedule_is(f.actor,sizeof(f.actor),0x5f,false,&sb,&yes)&&yes);
    assert(api->schedule_is(f.actor,sizeof(f.actor),npc_global_condition_base+101,false,&sb,&yes)&&yes);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original behavior selection and schedule namespace comparison");
}
