#include "native_server_recombinant_minor_task.h"
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
    alignas(4) unsigned char actor[0xc50]{},entity[0x380]{},desc[0x48]{};
    sin_independent::EntityHandleEntry entries[2]{};
    std::string calls;bool enemy_live=true,match=true,blocked=false,accepted=true,special=false,probe=false;
    const ServerNpcConditionApi* ca=nullptr;ServerNpcConditionMapBindings conditions{};
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    bool has(int id){ServerNpcConditionResult r{};assert(ca->test(actor,sizeof(actor),id,NpcConditionCurrent,&conditions,&r)&&r.mapped);return r.value!=0;}
    void cond(int id,bool value){ServerNpcConditionResult r{};assert(ca->store(actor,sizeof(actor),id,NpcConditionCurrent,value,&conditions,&r)&&r.mapped);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantMinorTaskApi>(GetProcAddress(server,server_recombinant_minor_task_export));
    const auto gc=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    assert(get&&gc&&!GetProcAddress(client,server_recombinant_minor_task_export)&&!get(2,sizeof(ServerRecombinantMinorTaskApi)));
    const auto* api=get(1,sizeof(ServerRecombinantMinorTaskApi));F f;f.ca=gc(1,sizeof(ServerNpcConditionApi));assert(api&&f.ca);
    f.put(f.actor,0,recombinant_minor_vtable);f.put(f.actor,0x8f8,44u);f.put(f.actor,0x908,45u);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.conditions.context=&f;f.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.entity);f.entries[1].serial=7;
    ServerRecombinantPolicyBindings policy{};policy.context=&f;policy.conditions=f.conditions;policy.entries=f.entries;policy.entry_count=2;
    policy.schedule_matches=[](void* p,void*,int id,bool ideal,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(id==85&&ideal);f.calls+='S';*out=f.match;return 1;};
    ServerRecombinantDecisionBindings decision{};decision.policy=&policy;
    ServerRecombinantTaskBindings task{};task.context=&f;task.decision=&decision;
    task.parent=[](void* p,void*,const ServerNpcScheduleTask*,bool run) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+=run?'U':'P';return 1;};
    task.enemy=[](void* p,void*,void** out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='E';*out=f.enemy_live?f.entity:nullptr;return 1;};
    task.submit_goal=[](void* p,unsigned nav,const ServerRecombinantTaskGoal* g,int mode,bool* out) noexcept ->int{
        auto& f=*static_cast<F*>(p);assert(nav==44&&mode==2&&g->kind==4&&g->activity==6&&g->position[0]==10&&g->owner==917);f.calls+='G';*out=f.accepted;return 1;
    };
    task.activity=[](void* p,void*,int id) noexcept ->int{auto& f=*static_cast<F*>(p);assert(id==922);f.calls+='A';return 1;};
    ServerRecombinantDamageBindings d{&policy,&f};ServerRecombinantMinorBindings m{&d,&f,1};
    m.refresh_velocity=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='V';f.put(a,0x250,2.f);return 1;};
    ServerRecombinantMinorTouchBindings touch{&m,&f};
    touch.set_movement=[](void* p,void*,int mode,int arg) noexcept ->int{auto& f=*static_cast<F*>(p);assert(mode==3&&!arg);f.calls+='M';return 1;};
    ServerRecombinantMinorTaskBindings b{&task,&touch,&f,917,921,922};
    b.world_center=[](void* p,void* a,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.entity);f.calls+='C';v[0]=4;v[1]=5;v[2]=6;return 1;};
    b.origin=[](void* p,void* a,float v[3]) noexcept ->int{auto& f=*static_cast<F*>(p);assert(a==f.entity||a==f.actor);f.calls+='O';v[0]=10;v[1]=20;v[2]=30;return 1;};
    b.face=[](void* p,unsigned motor,const float v[3],float delta,bool center) noexcept ->int{auto& f=*static_cast<F*>(p);assert(motor==45&&(center?(delta==0&&v[0]==4):(delta==-1&&(v[0]==10||v[0]==12))));f.calls+='F';return 1;};
    b.aim_reset=[](void* p,void*,int x,int y) noexcept ->int{auto& f=*static_cast<F*>(p);assert(!x&&!y);f.calls+='R';return 1;};
    b.leap=[](void* p,void*,float x,int y) noexcept ->int{auto& f=*static_cast<F*>(p);assert(x==5&&!y);f.calls+='L';return 1;};
    b.target_gate=[](void* p,void*,void* e,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(e==f.entity);f.calls+='B';*out=f.blocked;return 1;};
    b.goal_adjust=[](void* p,void*,void* e,ServerRecombinantTaskGoal* g) noexcept ->int{auto& f=*static_cast<F*>(p);assert(e==f.entity&&g->activity==-1&&g->distance==-1&&g->position[0]==10);g->activity=6;f.calls+='J';return 1;};
    b.set_arrival=[](void* p,unsigned nav,int id) noexcept ->int{auto& f=*static_cast<F*>(p);assert(nav==44&&id==921);f.calls+='I';return 1;};
    b.diagnostic=[](void* p,int level,const char* s) noexcept ->int{auto& f=*static_cast<F*>(p);assert(level==2&&!std::strcmp(s,"TASK_MINOR_GOTO_TARGET failed!!\n"));f.calls+='D';return 1;};
    b.special_gate=[](void* p,void*,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='Q';*out=f.special;return 1;};
    b.support_probe=[](void* p,void*,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);f.calls+='H';*out=f.probe;return 1;};
    ServerRecombinantMinorTaskResult o{};ServerNpcScheduleTask t{};
    auto call=[&](int id,bool run=false,float now=10.f){t.id=id;f.calls.clear();return api->execute(f.actor,sizeof(f.actor),&t,run,now,&b,&o);};
    for(int id=0;id<160;++id){
        assert(api->route(id,false)==(id==140?MinorTaskUnrecovered:id==13||id==67||id==131||id==141||id==142||id==143?MinorTaskNative:MinorTaskInherited));
        assert(api->route(id,true)==(id==67||id==95||id==121||id==140||id==142?MinorTaskNative:MinorTaskInherited));
    }
    assert(call(13)&&o.enemy_calls==2&&o.face_calls==1&&o.delegated&&f.calls=="EECFP");
    f.enemy_live=false;assert(call(13)&&o.enemy_calls==1&&!o.face_calls&&f.calls=="EP");f.enemy_live=true;
    assert(call(67)&&o.leap_calls==1&&!o.delegated&&f.calls=="L");
    assert(call(131)&&o.movement_calls==1&&f.calls=="MP");
    assert(!call(140)&&o.route==MinorTaskUnrecovered&&f.calls.empty());
    f.put(f.actor,0x8a8,0x8001u);assert(call(141)&&o.lifecycle.failed&&o.failure==11&&f.has(33)&&f.calls.empty());f.cond(33,false);
    f.put(f.actor,0x8a8,0x7001u);f.blocked=true;assert(call(141)&&o.lifecycle.failed&&f.calls=="B");f.cond(33,false);
    f.blocked=false;f.accepted=false;assert(call(141)&&o.lifecycle.failed&&o.diagnostic_calls==1&&f.calls=="BOJGD");f.cond(33,false);
    f.accepted=true;assert(call(141)&&o.lifecycle.completed&&o.goal_accepted&&o.target_calls==4&&f.calls=="BOJG");
    assert(call(142)&&o.activity==922&&f.calls=="A");assert(call(143)&&o.activity==921&&o.lifecycle.completed&&f.calls=="I");
    assert(call(95,true)&&o.reset_calls&&f.calls=="RU");
    assert(call(121,true)&&o.face_calls&&f.calls=="SEEOFU");
    f.match=false;assert(call(121,true)&&!o.enemy_calls&&f.calls=="SU");
    f.actor[0xbc]=1;assert(call(140,true)&&o.lifecycle.completed&&!o.face_calls&&f.calls.empty());
    f.actor[0xbc]=0;f.put(f.actor,0xb8,0x1000u);
    assert(call(140,true)&&o.velocity_refresh&&o.vector[0]==12&&o.face_calls==1&&f.calls=="VOF");
    f.put(f.actor,0x238,0xffffffffu);assert(call(142,true)&&!o.lifecycle.completed&&f.calls=="Q");
    f.put(f.actor,0x238,0x7001u);assert(call(142,true)&&o.lifecycle.completed&&o.ground_live&&f.calls=="Q");
    f.special=true;f.put(f.actor,0x238,0xffffffffu);assert(call(142,true)&&o.lifecycle.completed&&!o.target_calls&&f.calls=="Q");
    f.put(f.actor,0x9dc,10.f);assert(call(67,true,9.99f)&&!o.probe_calls&&!o.lifecycle.completed&&f.calls.empty());
    assert(call(67,true,10.f)&&o.probe_calls&&o.leap_calls&&f.calls=="HL");
    f.probe=true;assert(call(67,true,10.f)&&o.lifecycle.completed&&!o.leap_calls&&f.calls=="H");
    assert(call(2,true)&&o.route==MinorTaskInherited&&f.calls=="U");
    FreeLibrary(server);FreeLibrary(client);return 0;
}
