#include "native_server_recombinant_decision.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <initializer_list>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0xc50]{},goal[0x380]{},desc[0x48]{},sched[0x34]{};
    sin_independent::EntityHandleEntry entries[2]{};
    float draws[2]{};
    unsigned draws_used=0,parents=0,gathers=0,interrupts=0,invalidated=0,flinches=0;
    int parent=41,ideal=0,sequence=2;
    bool model=true,small=true;
    const ServerNpcConditionApi* ca=nullptr;ServerNpcConditionMapBindings conditions{};
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    void cond(int id,bool value,unsigned bank=NpcConditionCurrent){ServerNpcConditionResult r{};assert(ca->store(actor,sizeof(actor),id,bank,value,&conditions,&r)&&r.mapped);}
    bool has(int id,unsigned bank=NpcConditionCurrent){ServerNpcConditionResult r{};assert(ca->test(actor,sizeof(actor),id,bank,&conditions,&r)&&r.mapped);return r.value!=0;}
    void clear(){std::memset(actor+0x7cc,0,64);draws_used=0;}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantDecisionApi>(GetProcAddress(server,server_recombinant_decision_export));
    const auto gc=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    assert(get&&gc&&!GetProcAddress(client,server_recombinant_decision_export)&&!get(2,sizeof(ServerRecombinantDecisionApi)));
    const auto* api=get(1,sizeof(ServerRecombinantDecisionApi));F f;f.ca=gc(1,sizeof(ServerNpcConditionApi));assert(api&&f.ca);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.conditions.context=&f;f.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.goal);f.entries[1].serial=7;
    ServerRecombinantPolicyBindings policy{};policy.context=&f;policy.conditions=f.conditions;policy.entries=f.entries;policy.entry_count=2;
    policy.schedule_matches=[](void* p,void*,int id,bool ideal,bool* out) noexcept ->int{assert(ideal);*out=static_cast<F*>(p)->ideal==id;return 1;};
    ServerNpcScheduleBindings schedule{};schedule.conditions=f.conditions;schedule.context=&f;
    schedule.storage=[](void* p,unsigned token,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);if(token!=44)return 0;*out=f.sched;*n=sizeof(f.sched);return 1;};
    ServerRecombinantDecisionBindings b{&policy,&schedule,&f,100.f};
    b.extent=[](void* p,void* a,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*n=a==f.goal?sizeof(f.goal):0;return *n!=0;};
    b.parent_select=[](void* p,void*,int* out) noexcept ->int{auto& f=*static_cast<F*>(p);++f.parents;*out=f.parent;return 1;};
    b.parent_gather=[](void* p,void*) noexcept ->int{auto& f=*static_cast<F*>(p);assert(!f.has(66));++f.gathers;f.cond(66,true);return 1;};
    b.parent_interrupts=[](void* p,void*) noexcept ->int{auto& f=*static_cast<F*>(p);++f.interrupts;f.cond(16,true,NpcConditionInterrupt);f.cond(17,true,NpcConditionInterrupt);return 1;};
    b.parent_invalidated=[](void* p,void*) noexcept ->int{++static_cast<F*>(p)->invalidated;return 1;};
    b.flinch_activity=[](void*,void*,int big,int gesture,int* out) noexcept ->int{assert(!gesture);*out=big?101:102;return 1;};
    b.model_available=[](void* p,void*,bool* out) noexcept ->int{*out=static_cast<F*>(p)->model;return 1;};
    b.sequence_for_activity=[](void* p,void*,int a,int* out) noexcept ->int{assert(a==101);*out=static_cast<F*>(p)->sequence;return 1;};
    b.small_flinch=[](void* p,void*,int a,bool* out) noexcept ->int{assert(a==102);*out=static_cast<F*>(p)->small;return 1;};
    b.play_flinch=[](void* p,void*) noexcept ->int{++static_cast<F*>(p)->flinches;return 1;};
    b.random_float=[](void* p,float lo,float hi,float* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(lo==0&&hi==1&&f.draws_used<2);*out=f.draws[f.draws_used++];return 1;};
    ServerRecombinantDecisionResult o{};
    f.put(f.actor,0,recombinant_major_vtable);f.put(f.actor,0xc44,0x7001u);
    f.cond(69,true);f.cond(67,true);f.cond(22,true);
    assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==83&&o.condition_tests==1);
    f.cond(69,false);assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==86&&o.condition_tests==2);
    f.cond(67,false);f.put(f.actor,0xc28,static_cast<unsigned char>(1));
    assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==86&&o.latch_cleared&&o.goal_live&&!f.actor[0xc28]&&f.get<unsigned>(f.actor,0xc44)==0x7001u);
    f.put(f.actor,0xc44,0x8001u);assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==39&&!o.goal_live);
    f.clear();f.cond(17,true);f.cond(16,true);
    assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==22&&o.activity==101&&o.sequence==2&&!o.flinch_calls);
    f.model=false;assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==41&&o.delegated&&!o.sequence_calls&&!o.flinch_calls);
    f.model=true;f.sequence=-1;assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==41&&o.sequence_calls==1&&!o.flinch_calls);
    f.cond(17,false);assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==21&&o.activity==102&&o.flinch_calls==1);
    f.put(f.actor,0x988,0x40u);assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==41&&!o.activity_calls);
    f.clear();f.put(f.actor,0x988,0u);f.put(f.actor,0x850,3);f.put(f.actor,0x8a0,0x7001u);f.cond(25,true);
    f.put(f.goal,0x360,99.f);assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==41&&o.enemy_live&&o.distance==99.f);
    f.put(f.goal,0x360,100.f);assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==81&&!o.delegated&&o.distance==100.f);
    f.put(f.actor,0x8a0,0x8001u);assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==41&&!o.enemy_live);
    f.clear();f.cond(66,true);f.put(f.actor,0xc44,0x7001u);
    assert(api->gather_major(f.actor,sizeof(f.actor),&b,&o)&&o.goal_live&&f.has(66)&&f.has(67)&&!f.has(20)&&f.gathers==1);
    f.put(f.actor,0xc44,0x8001u);assert(api->gather_major(f.actor,sizeof(f.actor),&b,&o)&&!o.goal_live&&!f.has(67)&&f.has(20)&&f.gathers==2);
    f.clear();f.ideal=39;
    assert(api->interrupts(f.actor,sizeof(f.actor),&b,&o)&&f.has(67,NpcConditionInterrupt)&&!f.has(16,NpcConditionInterrupt)&&!f.has(17,NpcConditionInterrupt)&&o.schedule_tests==2);
    f.clear();f.ideal=86;assert(api->interrupts(f.actor,sizeof(f.actor),&b,&o)&&!f.has(67,NpcConditionInterrupt)&&f.has(16,NpcConditionInterrupt)&&f.has(17,NpcConditionInterrupt));
    f.put(f.actor,0xbf,static_cast<unsigned char>(8));assert(api->interrupts(f.actor,sizeof(f.actor),&b,&o)&&!f.has(16,NpcConditionInterrupt)&&!f.has(17,NpcConditionInterrupt));
    f.put(f.actor,0xc20,11.f);assert(api->invalidated_major(f.actor,sizeof(f.actor),10.f,&b,&o)&&!o.deadline_changed&&o.deadline==11.f);
    f.put(f.actor,0xc20,11.25f);assert(api->invalidated_major(f.actor,sizeof(f.actor),10.f,&b,&o)&&o.deadline_changed&&o.deadline==10.5f&&f.invalidated==2);
    f.clear();f.cond(17,true);f.cond(16,true);f.put(f.actor,0x988,0x140u);f.put(f.actor,0x9e0,10.f);
    assert(api->flinch_maintenance_major(f.actor,sizeof(f.actor),10.f,&b,&o)&&o.flinch_calls==1&&!o.latch_cleared&&!f.has(17)&&f.has(16));
    assert(api->flinch_maintenance_major(f.actor,sizeof(f.actor),10.25f,&b,&o)&&o.flinch_calls==1&&o.latch_cleared&&!f.has(16)&&f.get<unsigned>(f.actor,0x988)==0x100u);
    f.cond(17,true);f.put(f.actor,0x798,44u);f.put(f.sched,0xc,1u<<17);
    assert(api->flinch_maintenance_major(f.actor,sizeof(f.actor),10.5f,&b,&o)&&!o.flinch_calls&&f.has(17));
    f.put(f.sched,0xc,0u);assert(api->flinch_maintenance_major(f.actor,sizeof(f.actor),10.5f,&b,&o)&&o.flinch_calls==1&&f.has(17));
    f.clear();f.put(f.actor,0,recombinant_minor_vtable);f.cond(67,true);f.cond(68,true);f.cond(66,true);
    assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==81&&!f.has(67)&&f.has(68)&&f.has(66)&&!o.random_calls);
    assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==82&&!f.has(68)&&f.has(66)&&o.condition_tests==2);
    assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==82&&f.has(66)&&o.condition_tests==3);
    f.clear();f.parent=9;assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==9&&!o.random_calls);
    f.parent=41;f.draws[0]=.59f;assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==91&&o.random_calls==1);
    f.draws_used=0;f.draws[0]=.6f;f.draws[1]=.49f;f.put(f.actor,0xc44,.5f);
    assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==41&&o.random_calls==2);
    f.draws_used=0;f.draws[1]=.5f;f.parent=42;assert(api->select(f.actor,sizeof(f.actor),&b,&o)&&o.selected==42&&o.random_calls==2);
    f.ideal=86;assert(api->interrupts(f.actor,sizeof(f.actor),&b,&o)&&!f.has(66,NpcConditionInterrupt));
    f.ideal=0;assert(api->interrupts(f.actor,sizeof(f.actor),&b,&o)&&f.has(66,NpcConditionInterrupt));
    assert(!api->gather_major(f.actor,sizeof(f.actor),&b,&o)&&!o.completed);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
