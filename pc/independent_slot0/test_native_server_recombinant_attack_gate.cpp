#include "native_server_recombinant_attack_gate.h"
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
    alignas(4) unsigned char actor[0xc50]{},enemy[4]{},other[4]{},desc[0x48]{};
    bool live=true,special=true,cast=false,held=false;void* hit=nullptr;float z=150,origin_z=256;
    std::string calls;const ServerNpcConditionApi* ca=nullptr;ServerNpcConditionMapBindings conditions{};
    template<class T>void put(void* a,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(a)+off,&v,sizeof(v));}
    void cond(bool v){ServerNpcConditionResult r{};assert(ca->store(actor,sizeof(actor),10,NpcConditionCurrent,v,&conditions,&r)&&r.mapped);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantAttackGateApi>(GetProcAddress(server,server_recombinant_attack_gate_export));
    const auto gc=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    assert(get&&gc&&!GetProcAddress(client,server_recombinant_attack_gate_export)&&!get(2,sizeof(ServerRecombinantAttackGateApi)));
    const auto* api=get(1,sizeof(ServerRecombinantAttackGateApi));F f;f.ca=gc(1,sizeof(ServerNpcConditionApi));assert(api&&f.ca);
    f.put(f.actor,0,recombinant_minor_vtable);f.put(f.actor,0xc3c,10.f);f.put(f.actor,0x60c,10.f);f.put(f.actor,0x610,1u);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.conditions.context=&f;f.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    ServerRecombinantPolicyBindings policy{};policy.conditions=f.conditions;
    ServerRecombinantEventBindings e{};e.policy=&policy;e.context=&f;
    e.enemy=[](void* p,void*,void** out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='E';*out=f.live?f.enemy:nullptr;return 1;};
    e.special=[](void* p,void*,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='S';*out=f.special;return 1;};
    e.target_point=[](void* p,void*,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='T';v[0]=10;v[1]=20;v[2]=f.z;return 1;};
    e.center=[](void* p,void*,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='C';v[0]=10;v[1]=20;v[2]=100;return 1;};
    e.player_probe=[](void* p,void*) noexcept ->int{static_cast<F*>(p)->calls+='P';return 1;};
    ServerRecombinantAttackGateBindings b{&e,&f};
    b.origin=[](void* p,void* a,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='O';v[0]=0;v[1]=0;v[2]=a==f.actor?0:f.origin_z;return 1;};
    b.refresh_origin=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor);f.calls+='U';return 1;};
    b.enemy_cast=[](void* p,void* a,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.enemy);f.calls+='D';*out=f.cast;return 1;};
    b.hull_mins=[](void* p,unsigned token,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(token==1);f.calls+='M';v[0]=-1;v[1]=-2;v[2]=0;return 1;};
    b.hull_maxs=[](void* p,unsigned token,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(token==1);f.calls+='X';v[0]=1;v[1]=2;v[2]=4;return 1;};
    b.trace=[](void* p,void* a,const ServerRecombinantAttackTrace* t,void** out) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(a==f.actor&&t->mask==0x200400b&&!t->group&&t->start[2]==0&&t->end[2]==(f.special?f.z:110)&&t->mins[0]==-1&&t->maxs[2]==4);f.calls+='R';*out=f.hit;return 1;
    };
    b.held_object=[](void* p,void* a,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.other);f.calls+='H';*out=f.held;return 1;};
    ServerRecombinantAttackGateResult o{};
    auto spit=[&](float now,float dist){f.calls.clear();return api->spit(f.actor,sizeof(f.actor),now,dist,&b,&o);};
    auto leap=[&](float now,float dist){f.calls.clear();return api->leap(f.actor,sizeof(f.actor),now,dist,&b,&o);};
    f.live=false;assert(spit(10,100)&&!o.condition&&!o.condition_tests&&f.calls=="E");f.live=true;
    assert(spit(10,100)&&!o.condition&&o.condition_tests==1&&f.calls=="E");f.cond(true);
    assert(spit(10,-1)&&o.condition==36&&f.calls=="E");assert(spit(10,351)&&o.condition==37&&f.calls=="E");
    assert(spit(9.99f,350)&&!o.condition&&f.calls=="E");
    assert(spit(10,350)&&o.condition==21&&o.accepted&&o.height==256&&f.calls=="EOEO");
    f.origin_z=256.25f;assert(spit(10,350)&&!o.condition&&!o.accepted);
    assert(leap(9.99f,100)&&!o.condition&&!o.grounded&&f.calls.empty());
    assert(leap(10,100)&&!o.condition&&!o.grounded&&f.calls.empty());
    f.actor[0xbc]=1;assert(leap(10,-1)&&o.condition==36&&f.calls.empty());assert(leap(10,351)&&o.condition==37&&f.calls.empty());
    f.z=150.25f;assert(leap(10,350)&&!o.condition&&o.height==150.25f&&f.calls=="ET");
    f.z=150;f.hit=f.enemy;f.put(f.actor,0xb8,0x800u);
    assert(leap(10,350)&&o.condition==20&&o.accepted&&o.refreshes==2&&o.enemy_queries==4&&f.calls=="EUTDMUXEESTPRE");
    f.put(f.actor,0xb8,0u);f.hit=f.other;f.held=false;
    assert(leap(10,350)&&!o.condition&&o.held_calls==1&&f.calls=="ETDMXEESTPREH");
    f.held=true;assert(leap(10,350)&&o.condition==20&&o.held_calls==1);
    f.hit=nullptr;f.held=false;assert(leap(10,350)&&!o.condition&&!o.held_calls);
    f.cast=true;f.special=false;assert(leap(10,350)&&o.condition==20&&o.trace.end[2]==110&&f.calls=="ETDMXEESCPRE");
    FreeLibrary(server);FreeLibrary(client);return 0;
}
