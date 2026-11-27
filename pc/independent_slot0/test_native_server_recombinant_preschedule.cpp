#include "native_server_recombinant_preschedule.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xc68]{},object[4]{},enemy[4]{},desc[0x48]{};
    sin_independent::EntityHandleEntry entries[2]{};
    int schedule=0;bool live_enemy=true;
    unsigned candidate=0x7001;
    float position[3]{};
    std::string calls;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    const ServerNpcConditionApi* ca=nullptr;ServerNpcConditionMapBindings conditions{};
    bool has(int id){ServerNpcConditionResult r{};assert(ca->test(actor,sizeof(actor),id,NpcConditionCurrent,&conditions,&r)&&r.mapped);return r.value!=0;}
    void cond(int id,bool value){ServerNpcConditionResult r{};assert(ca->store(actor,sizeof(actor),id,NpcConditionCurrent,value,&conditions,&r)&&r.mapped);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantPrescheduleApi>(GetProcAddress(server,server_recombinant_preschedule_export));
    const auto gc=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    assert(get&&gc&&!GetProcAddress(client,server_recombinant_preschedule_export)&&!get(2,sizeof(ServerRecombinantPrescheduleApi)));
    const auto* api=get(1,sizeof(ServerRecombinantPrescheduleApi));F f;f.ca=gc(1,sizeof(ServerNpcConditionApi));assert(api&&f.ca);
    f.put(f.actor,0,recombinant_major_vtable);f.put(f.actor,0xc44,0xffffffffu);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.conditions.context=&f;f.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.object);f.entries[1].serial=7;
    ServerRecombinantPolicyBindings policy{};policy.context=&f;policy.conditions=f.conditions;policy.entries=f.entries;policy.entry_count=2;
    policy.schedule_matches=[](void* p,void* a,int id,bool ideal,bool* out) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(a==f.actor&&((id==80&&!ideal)||(id==86&&ideal)));f.calls+=id==80?'S':'T';*out=f.schedule==id;return 1;
    };
    ServerRecombinantPrescheduleBindings b{&policy,&f};
    b.refresh_velocity=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor);f.calls+='V';f.put(a,0x258,0.f);return 1;};
    b.enemy=[](void* p,void* a,void** out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor);f.calls+='E';*out=f.live_enemy?f.enemy:nullptr;return 1;};
    b.world_center=[](void* p,void* a,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.object);f.calls+='C';std::memcpy(v,f.position,12);return 1;};
    b.find_object=[](void* p,void* a,int distance,unsigned* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&distance==300);f.calls+='F';*out=f.candidate;return 1;};
    b.debug_box=[](void* p,const float v[3],const float mins[3],const float maxs[3],int r,int g,int bl,int alpha,float duration) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(v[0]==f.position[0]&&mins[0]==-32&&maxs[2]==32&&r==255&&g==255&&bl==255&&alpha==1&&duration==.1f);f.calls+='D';return 1;
    };
    ServerRecombinantPrescheduleResult o{};
    f.put(f.actor,0xc3c,12.f);f.put(f.actor,0xc40,10.f);f.actor[0xc14]=1;
    f.put(f.actor,0x258,1.f);
    assert(api->run_major(f.actor,sizeof(f.actor),9.f,&b,&o)&&!o.expiry.cleared&&!o.landing_latched&&f.calls=="ST");
    f.calls.clear();f.put(f.actor,0xb8,0x1000u);f.schedule=80;
    assert(api->run_major(f.actor,sizeof(f.actor),10.f,&b,&o)&&o.expiry.cleared&&o.velocity_refreshed&&o.landing_latched&&f.actor[0xc15]&&o.search_live&&o.final_handle==0x7001u&&f.has(67)&&f.calls=="VSF");
    f.calls.clear();assert(api->run_major(f.actor,sizeof(f.actor),11.f,&b,&o)&&o.goal_live&&!o.search_called&&!o.velocity_refreshed&&f.calls=="S");
    f.put(f.actor,0xc44,0x8001u);f.candidate=0x8001u;f.calls.clear();
    assert(api->run_major(f.actor,sizeof(f.actor),12.f,&b,&o)&&o.search_called&&!o.search_live&&!f.has(67)&&o.final_handle==0x8001u&&f.calls=="SF");
    f.schedule=86;f.put(f.actor,0xc44,0x7001u);f.position[0]=-100;f.position[1]=100;f.position[2]=56;f.cond(68,true);f.calls.clear();
    assert(api->run_major(f.actor,sizeof(f.actor),12.f,&b,&o)&&o.displacement==256.&&!o.goal_cleared&&f.has(67)&&!f.has(68)&&f.calls=="STEC");
    f.position[2]=56.25f;f.calls.clear();
    assert(api->run_major(f.actor,sizeof(f.actor),12.f,&b,&o)&&o.displacement==256.25&&o.goal_cleared&&!f.has(67)&&f.has(68)&&o.final_handle==0xffffffffu&&f.calls=="STEC");
    f.put(f.actor,0xc44,0x7001u);f.live_enemy=false;f.cond(67,true);f.cond(68,false);f.calls.clear();
    assert(api->monitor_major(f.actor,sizeof(f.actor),&b,&o)&&o.goal_cleared&&!o.enemy_live&&f.has(67)&&f.has(68)&&o.condition_writes==1&&f.calls=="E");
    f.put(f.actor,0xc44,0x8001u);f.calls.clear();
    assert(api->monitor_major(f.actor,sizeof(f.actor),&b,&o)&&!o.goal_live&&!o.goal_cleared&&o.final_handle==0x8001u&&f.calls=="E");
    f.put(f.actor,0xc44,0x7001u);f.live_enemy=true;f.position[2]=55;f.calls.clear();b.debug=true;
    assert(api->monitor_major(f.actor,sizeof(f.actor),&b,&o)&&o.debug_called&&f.calls=="ECCD");
    f.position[2]=57;f.calls.clear();
    assert(!api->monitor_major(f.actor,sizeof(f.actor),&b,&o)&&o.debug_invalid_goal&&o.goal_cleared&&!o.completed&&f.calls=="EC");
    f.actor[0xc14]=1;f.actor[0xc15]=0;f.put(f.actor,0xb8,0u);f.put(f.actor,0x258,-1.f);f.schedule=0;f.calls.clear();
    assert(api->run_major(f.actor,sizeof(f.actor),13.f,&b,&o)&&o.landing_latched&&!o.velocity_refreshed&&f.calls=="ST");
    f.put(f.actor,0,recombinant_minor_vtable);assert(!api->run_major(f.actor,sizeof(f.actor),13.f,&b,&o)&&!o.completed);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
