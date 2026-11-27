#include "native_server_recombinant_launch.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstring>
#include <string>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xc70]{},ground[0x220]{},desc[0x48]{};
    sin_independent::EntityHandleEntry entries[2]{};
    std::string calls;float fraction=1;unsigned flags=4,draws=0;bool zero=false;
    const ServerNpcConditionApi* ca=nullptr;ServerNpcConditionMapBindings conditions{};
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    void cond(int id,bool value){ServerNpcConditionResult r{};assert(ca->store(actor,sizeof(actor),id,NpcConditionCurrent,value,&conditions,&r)&&r.mapped);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantLaunchApi>(GetProcAddress(server,server_recombinant_launch_export));
    const auto gc=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    assert(get&&gc&&!GetProcAddress(client,server_recombinant_launch_export)&&!get(2,sizeof(ServerRecombinantLaunchApi)));
    const auto* api=get(1,sizeof(ServerRecombinantLaunchApi));F f;f.ca=gc(1,sizeof(ServerNpcConditionApi));assert(api&&f.ca);
    f.put(f.actor,0,recombinant_minor_vtable);f.put(f.actor,0x238,0xffffffffu);f.put(f.actor,0x214,91u);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.conditions.context=&f;f.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.ground);f.entries[1].serial=7;
    ServerRecombinantPolicyBindings policy{};policy.context=&f;policy.conditions=f.conditions;policy.entries=f.entries;policy.entry_count=2;
    policy.random=[](void* p,int lo,int hi,int* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(lo==-1&&hi==1);f.calls+='R';++f.draws;*out=f.zero||f.draws<=2?0:f.draws%2?1:0;return 1;};
    ServerRecombinantDecisionBindings decision{};decision.policy=&policy;
    ServerRecombinantTaskBindings task{};task.context=&f;task.decision=&decision;
    ServerRecombinantDamageBindings d{&policy,&f};
    d.extent=[](void* p,void* a,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*n=a==f.ground?sizeof(f.ground):0;return *n!=0;};
    ServerRecombinantMinorBindings m{&d,&f,1};
    m.object_flags=[](void* p,unsigned token,unsigned* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(token==92);f.calls+='O';*out=f.flags;return 1;};
    m.set_velocity=[](void* p,void*,const float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(v[0]==200&&v[1]==0&&v[2]==200);f.calls+='V';return 1;};
    ServerRecombinantMinorTouchBindings touch{&m,&f};ServerRecombinantMinorTaskBindings mt{&task,&touch,&f};
    ServerRecombinantLaunchBindings b{&mt,&f};
    b.refresh_origin=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='U';f.put(a,0x2c4,30.f);return 1;};
    b.trace=[](void* p,void* a,const ServerRecombinantTraceRequest* t,float* out) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(a==f.actor&&t->mask==0x202400b&&t->collision_token==91&&t->start[2]==30&&t->end[2]==(t->upward?31:18));f.calls+=t->upward?'T':'B';*out=f.fraction;return 1;
    };
    b.ground_object_effect=[](void* p,void* a,void* g,int mode,float value) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&g==f.ground&&mode==1&&value==.5f);f.calls+='E';return 1;};
    b.clear_ground=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='C';f.put(a,0x238,0xffffffffu);return 1;};
    b.normalize=[](void* p,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(v[0]==1&&v[1]==0&&v[2]==0);f.calls+='N';return 1;};
    b.relocate=[](void* p,void* a,const float v[3],int mode) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&v[0]==4&&v[1]==5&&v[2]==7&&!mode);f.calls+='M';return 1;};
    b.parent_failure=[](void* p,void* a,int schedule,int id,int reason,int* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&id>=0&&reason==11);if(f.get<unsigned>(a,0)==recombinant_major_vtable)assert(f.get<unsigned>(a,0xc68)==0xffffffffu);f.calls+='P';*out=schedule+100;return 1;};
    ServerRecombinantLaunchResult o{};
    assert(api->start140(f.actor,sizeof(f.actor),10,&b,&o)&&!o.ground_live&&o.lifecycle.completed&&f.calls.empty());
    f.put(f.actor,0x238,0x7001u);f.put(f.ground,0x218,92u);f.put(f.actor,0xb8,0x800u);f.put(f.actor,0x360,4.f);f.put(f.actor,0x364,5.f);f.put(f.actor,0x368,6.f);
    assert(api->start140(f.actor,sizeof(f.actor),10,&b,&o)&&o.object_effect&&o.random_calls==4&&o.clearance&&o.relocated&&o.velocity_set&&!o.lifecycle.completed&&o.origin_refreshes==2&&f.calls=="OERRRRNCUUTMV");
    f.calls.clear();f.draws=2;f.flags=0;f.fraction=.5f;f.put(f.actor,0x238,0x7001u);f.put(f.actor,0xb8,0u);
    assert(api->start140(f.actor,sizeof(f.actor),10,&b,&o)&&!o.object_effect&&!o.clearance&&!o.relocated&&o.velocity_set&&f.calls=="ORRNCTV");
    f.calls.clear();f.zero=true;f.draws=0;f.put(f.actor,0x238,0x7001u);
    assert(!api->start140(f.actor,sizeof(f.actor),10,&b,&o)&&o.random_calls==128&&!o.normalized&&!o.ground_cleared);
    f.calls.clear();f.cond(10,true);
    assert(api->fail_select(f.actor,sizeof(f.actor),85,0,11,&b,&o)&&o.selected==41&&o.condition_tests==1&&!o.trace_calls&&f.calls.empty());
    assert(api->fail_select(f.actor,sizeof(f.actor),23,100,11,&b,&o)&&o.selected==41);
    f.cond(10,false);assert(api->fail_select(f.actor,sizeof(f.actor),23,100,11,&b,&o)&&o.selected==86&&!o.trace_calls);
    f.actor[0xbc]=1;f.fraction=1;f.calls.clear();
    assert(api->fail_select(f.actor,sizeof(f.actor),72,1,11,&b,&o)&&o.selected==86&&!o.clearance&&f.calls=="B");
    f.fraction=.5f;f.calls.clear();assert(api->fail_select(f.actor,sizeof(f.actor),73,1,11,&b,&o)&&o.selected==173&&o.delegated&&o.clearance&&f.calls=="BP");
    f.calls.clear();assert(api->fail_select(f.actor,sizeof(f.actor),23,99,11,&b,&o)&&o.selected==123&&!o.condition_tests&&!o.trace_calls&&f.calls=="P");
    f.put(f.actor,0,recombinant_major_vtable);f.put(f.actor,0xc68,0x7001u);f.calls.clear();
    assert(api->fail_select(f.actor,sizeof(f.actor),85,0,11,&b,&o)&&o.selected==185&&o.handle_cleared&&o.delegated&&f.calls=="P");
    assert(!api->start140(f.actor,sizeof(f.actor),10,&b,&o)&&!o.completed);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
