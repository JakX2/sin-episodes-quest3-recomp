#include "native_server_npc_state_reconsider.h"
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
    alignas(4) unsigned char actor[0x9c8]{},desc[0x48]{},schedule[0x30]{},enemy[4]{};
    sin_independent::EntityHandleEntry entries[2]{};
    unsigned clears=0,changes=0,lost=0,squad=0;int query=-1;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(unsigned off){T v{};std::memcpy(&v,actor+off,sizeof(v));return v;}
    void condition(int id){unsigned off=0x7cc+4*(id/32);put(actor,off,get<unsigned>(off)|(1u<<(id%32)));}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcStateApi>(GetProcAddress(s,server_npc_state_reconsider_export));
    assert(g&&!GetProcAddress(c,server_npc_state_reconsider_export)&&!g(0,sizeof(ServerNpcStateApi))&&!g(1,0));
    const auto* api=g(1,sizeof(ServerNpcStateApi));assert(api);
    F f;f.entries[1].serial=7;f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.enemy);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.put(f.actor,0x798,1u);f.put(f.actor,0x850,3);f.put(f.actor,0x858,3);
    f.put(f.actor,0x8a0,0x7001u);
    ServerNpcStateBindings b{};b.context=&f;b.entities={f.entries,2,nullptr,nullptr,nullptr};
    b.schedule.context=&f;b.schedule.conditions.context=&f;
    b.schedule.storage=[](void* p,std::uint32_t token,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(token!=1)return 0;*out=f.schedule;*n=sizeof(f.schedule);return 1;};
    b.schedule.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.clear_enemy=[](void* p,void*) noexcept -> int{auto& f=F::self(p);++f.clears;f.put(f.actor,0x8a0,0xffffffffu);return 1;};
    b.state_changed=[](void* p,void*,int old,int next) noexcept -> int{auto& f=F::self(p);assert(old!=next&&f.get<int>(0x850)==next);++f.changes;return 1;};
    b.lost_enemy=[](void* p,void*,void* e) noexcept -> int{auto& f=F::self(p);assert(e==f.enemy);++f.lost;return 1;};
    b.squad_enemy=[](void* p,std::uint32_t s,void* e) noexcept -> int{auto& f=F::self(p);assert(s==1&&e==f.enemy);++f.squad;return 1;};
    b.state_query=[](void* p,void*,int,int* out) noexcept -> int{*out=F::self(p).query;return 1;};
    ServerNpcStateResult out{};auto gate=[&](bool yes){assert(api->should_reconsider(f.actor,sizeof(f.actor),&b,&out)&&out.reconsider==yes);};
    gate(true);f.condition(0x22);gate(false);f.put(f.schedule,0x10,4u);gate(true);f.put(f.schedule,0x10,0u);
    f.put(f.actor,0x858,7);gate(false);f.put(f.actor,0x858,4);gate(false);f.put(f.actor,0x858,3);
    f.put(f.actor,0x8a0,0x8001u);gate(true);
    assert(api->choose_ideal(f.actor,sizeof(f.actor),&b,&out)&&out.ideal==2&&out.patrol_set&&(f.get<unsigned>(0x7d4)&16u));
    f.condition(0x1d);assert(api->choose_ideal(f.actor,sizeof(f.actor),&b,&out)&&out.ideal==2&&!out.patrol_set);
    f.put(f.actor,0x8a0,0x7001u);assert(api->choose_ideal(f.actor,sizeof(f.actor),&b,&out)&&out.lost_enemy_called&&f.lost==1);
    assert(api->apply(f.actor,sizeof(f.actor),1,12.f,&b,&out)&&out.changed&&out.enemy_cleared&&f.clears==1&&f.changes==1&&f.get<float>(0x854)==12.f);
    assert(api->apply(f.actor,sizeof(f.actor),1,13.f,&b,&out)&&!out.changed&&f.get<float>(0x854)==12.f);
    f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x9c4,1u);f.condition(0x19);f.query=3;
    assert(api->choose_ideal(f.actor,sizeof(f.actor),&b,&out)&&out.ideal==3&&out.squad_called&&f.squad==1);
    gate(true);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS native NPC state reconsideration and Grunt ideal-state transitions");
}
