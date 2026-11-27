#include "native_server_recombinant_minor_touch.h"
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
    alignas(4) unsigned char actor[0xc50]{},other[0x220]{},desc[0x48]{};
    std::string calls;bool same=false;unsigned flags=4;
    const ServerNpcConditionApi* ca=nullptr;ServerNpcConditionMapBindings conditions{};
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    bool has(int id){ServerNpcConditionResult r{};assert(ca->test(actor,sizeof(actor),id,NpcConditionCurrent,&conditions,&r)&&r.mapped);return r.value!=0;}
    void cond(int id,bool value){ServerNpcConditionResult r{};assert(ca->store(actor,sizeof(actor),id,NpcConditionCurrent,value,&conditions,&r)&&r.mapped);}
    void reset(unsigned marker,bool grounded){put(actor,0x7c,marker);actor[0xbc]=grounded;actor[0xc41]=1;calls.clear();}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantMinorTouchApi>(GetProcAddress(server,server_recombinant_minor_touch_export));
    const auto gc=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    assert(get&&gc&&!GetProcAddress(client,server_recombinant_minor_touch_export)&&!get(2,sizeof(ServerRecombinantMinorTouchApi)));
    const auto* api=get(1,sizeof(ServerRecombinantMinorTouchApi));F f;f.ca=gc(1,sizeof(ServerNpcConditionApi));assert(api&&f.ca);
    f.put(f.actor,0,recombinant_minor_vtable);f.put(f.actor,0xc38,10.f);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.conditions.context=&f;f.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    ServerRecombinantPolicyBindings policy{};policy.context=&f;policy.conditions=f.conditions;
    policy.schedule_matches=[](void* p,void*,int id,bool ideal,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(id==82&&!ideal);f.calls+='S';*out=f.same;return 1;};
    ServerRecombinantDamageBindings d{&policy,&f};
    ServerRecombinantMinorBindings m{&d,&f,1};
    m.set_velocity=[](void* p,void* a,const float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&v[0]==0&&v[1]==0&&(v[2]==0||v[2]==-.01f));f.calls+=v[2]==0?'V':'D';return 1;};
    m.schedule_touch_think=[](void* p,void* a,float now,int context) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&now==10.f&&!context);f.calls+='T';return 1;};
    m.object_flags=[](void* p,unsigned token,unsigned* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(token==91&&!f.actor[0xc41]);f.calls+='O';*out=f.flags;return 1;};
    ServerRecombinantMinorTouchBindings b{&m,&f};
    b.entity_handle=[](void* p,void* a,unsigned* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.other);f.calls+='H';*out=0x7001u;return 1;};
    b.release_player_object=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.other);f.calls+='R';return 1;};
    b.set_movement=[](void* p,void* a,int type,int mode) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&!type&&!mode&&f.has(68));f.calls+='M';return 1;};
    b.install_think=[](void* p,void* a,unsigned marker,int x,int y) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.actor&&!f.get<unsigned>(a,0x7c)&&marker==recombinant_default_think_marker&&!x&&!y);f.calls+='I';return 1;};
    ServerRecombinantMinorTouchResult o{};
    auto call=[&](float now){return api->touch(f.actor,sizeof(f.actor),f.other,sizeof(f.other),now,&b,&o);};
    f.put(f.other,0x218,91u);f.put(f.other,0x94,1u);f.put(f.other,0x9c,10u);
    f.reset(recombinant_minor_contact_marker,false);
    assert(call(10)&&o.object_released&&o.target_written&&o.target_handle==0x7001u&&f.get<unsigned>(f.actor,0xc30)==0x7001u&&f.has(68)&&o.velocity_set&&o.movement_set&&o.think_scheduled==1&&o.touch_cleared&&o.final_marker==0&&f.calls=="ORHSVMTI");
    f.same=true;f.flags=0;f.reset(recombinant_minor_contact_marker,false);
    assert(call(10)&&o.target_written&&!o.object_released&&!o.velocity_set&&!f.has(68)&&!o.think_scheduled&&f.calls=="OHSI");
    f.put(f.other,0x94,0u);f.put(f.other,0x218,0u);
    f.reset(recombinant_minor_contact_marker,true);
    assert(call(10)&&o.grounded&&o.touch_cleared&&!o.target_written&&!o.think_scheduled&&f.calls=="I");
    f.reset(recombinant_minor_contact_marker,false);f.put(f.other,0x11c,1u);
    assert(call(9.99f)&&o.cooldown_blocked&&!o.touch_cleared&&o.final_marker==recombinant_minor_contact_marker&&f.calls.empty());
    f.put(f.other,0x120,0x20u);f.reset(recombinant_minor_contact_marker,false);
    assert(call(10)&&o.solid_blocked&&!o.detached&&!o.touch_cleared&&f.calls.empty());
    f.put(f.other,0x120,0u);f.reset(recombinant_minor_contact_marker,false);
    assert(call(10)&&o.detached&&o.detach.think_scheduled&&o.touch_cleared&&o.final_marker==0&&f.has(67)&&f.calls=="DTI");
    f.reset(recombinant_minor_detached_contact_marker,true);
    assert(call(10)&&o.grounded&&o.touch_cleared&&o.think_scheduled==1&&o.final_marker==0&&f.calls=="IT");
    f.reset(recombinant_minor_detached_contact_marker,false);
    assert(call(9.99f)&&o.cooldown_blocked&&!o.touch_cleared&&!o.detached&&f.calls.empty());
    f.put(f.other,0x11c,0u);f.reset(recombinant_minor_detached_contact_marker,false);
    assert(call(10)&&o.solid_blocked&&!o.detached&&f.calls.empty());
    f.put(f.other,0x11c,1u);f.reset(recombinant_minor_detached_contact_marker,false);
    assert(call(10)&&o.detached&&!o.touch_cleared&&o.final_marker==recombinant_minor_detached_contact_marker&&f.calls=="DT");
    f.reset(0,false);assert(!call(10)&&!o.completed&&f.actor[0xc41]==1);
    f.reset(recombinant_minor_contact_marker,false);
    assert(!api->touch(f.actor,sizeof(f.actor),f.other,0x94,10,&b,&o)&&!o.completed&&f.actor[0xc41]==1);
    FreeLibrary(server);FreeLibrary(client);return 0;
}
