#include "native_server_npc_special_schedule.h"
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
struct F{
    alignas(4) unsigned char actor[0x974]{},desc[0x48]{},script[4]{};
    sin_independent::EntityHandleEntry slots[2]{};
    bool death=false;std::vector<int> trace;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcSpecialScheduleApi>(GetProcAddress(s,server_npc_special_schedule_export));
    assert(g&&!GetProcAddress(c,server_npc_special_schedule_export)&&!g(0,sizeof(ServerNpcSpecialScheduleApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcSpecialScheduleApi));assert(api);
    F f;f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.slots[1].serial=7;f.slots[1].entity=reinterpret_cast<CBaseEntity*>(f.script);
    ServerNpcSpecialScheduleBindings b{};b.context=&f;b.state.entities={f.slots,2,nullptr,nullptr,nullptr};
    b.state.schedule.conditions.context=&f;b.state.schedule.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.clear_parent=[](void* p,void*) noexcept -> int{auto& f=F::self(p);assert(f.get<float>(0x2a4)==1.f);f.trace.push_back(1);return 1;};
    b.reset_script=[](void* p,void*) noexcept -> int{F::self(p).trace.push_back(2);return 1;};
    b.death_query=[](void* p,void*,bool* out) noexcept -> int{auto& f=F::self(p);f.trace.push_back(3);*out=f.death;return 1;};
    b.death_prepare=[](void* p,void*,int x,int y) noexcept -> int{assert(x==0&&y==1);F::self(p).trace.push_back(4);return 1;};
    ServerNpcSpecialScheduleResult out{};
    auto select=[&](int id){f.trace.clear();assert(api->select(f.actor,sizeof(f.actor),&b,&out)&&out.schedule==id);};
    f.put(f.actor,0x850,4);f.put(f.actor,0x970,0x7001u);select(0x36);assert(out.controller_live&&f.trace.empty());
    f.put(f.actor,0x970,0x8001u);select(1);assert(out.script_reset&&f.trace==std::vector<int>{2});
    f.put(f.actor,0x850,7);f.put(f.actor,0x8c,2);select(0);assert(out.death_query&&!out.death_prepared&&f.trace==std::vector<int>{3});
    f.death=true;select(0x34);assert(out.death_prepared&&(f.trace==std::vector<int>{3,4}));
    f.death=false;f.put(f.actor,0x8c,1);select(0x33);
    f.put(f.actor,0x7d0,1u<<26);select(0x4c);assert(out.override_condition&&out.parent_cleared&&f.trace==std::vector<int>{1});
    f.put(f.actor,0x850,4);select(0x4c);assert(!out.script_reset);
    f.put(f.actor,0x850,1);assert(!api->select(f.actor,sizeof(f.actor),&b,&out));
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original special state schedule routes");
}
