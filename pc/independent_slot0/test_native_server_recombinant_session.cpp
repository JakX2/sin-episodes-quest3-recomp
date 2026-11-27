#include "native_server_recombinant_session.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
using namespace sin_port::native_weapon_tables;
struct F{
    alignas(4) unsigned char actor[0xc70]{},desc[0x48]{};
    unsigned inherited_desc[13]{},resets=0,parents=0,goals=0,activities=0;
    ServerNpcScheduleTask inherited_task{116,0};
    ServerRecombinantSession session{};
    const ServerNpcConditionApi* ca=nullptr;const ServerNpcTaskLifecycleApi* life=nullptr;const ServerNpcScheduleStateApi* state=nullptr;
    ServerNpcTaskLifecycleBindings lifecycle{};
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T>T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    void cond(int id,bool v){ServerNpcConditionResult r{};assert(ca->store(actor,sizeof(actor),id,NpcConditionCurrent,v,&lifecycle.conditions,&r)&&r.mapped);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetServerRecombinantSessionApi>(GetProcAddress(server,server_recombinant_session_export));
    const auto gc=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(server,server_npc_condition_export));
    const auto gl=reinterpret_cast<GetServerNpcTaskLifecycleApi>(GetProcAddress(server,server_npc_task_lifecycle_export));
    const auto gs=reinterpret_cast<GetServerNpcScheduleStateApi>(GetProcAddress(server,server_npc_schedule_state_export));
    const auto gd=reinterpret_cast<GetServerActivityDictionaryApi>(GetProcAddress(server,server_activity_dictionary_export));
    const auto gi=reinterpret_cast<GetServerRecombinantScheduleApi>(GetProcAddress(server,server_recombinant_schedule_export));
    assert(get&&gc&&gl&&gs&&gd&&gi&&!GetProcAddress(client,server_recombinant_session_export)&&!get(2,sizeof(ServerRecombinantSessionApi)));
    const auto* api=get(1,sizeof(ServerRecombinantSessionApi));F f;f.ca=gc(1,sizeof(ServerNpcConditionApi));f.life=gl(1,sizeof(ServerNpcTaskLifecycleApi));f.state=gs(1,sizeof(ServerNpcScheduleStateApi));
    const auto* da=gd(1,sizeof(ServerActivityDictionaryApi));const auto* image_api=gi(1,sizeof(ServerRecombinantScheduleApi));assert(api&&f.ca&&f.life&&f.state&&da&&image_api);
    f.put(f.desc,4,npc_global_condition_base+100);f.put(f.desc,8,0);f.put(f.desc,12,127);f.put(f.desc,16,npc_global_condition_base+227);
    f.put(f.desc,0x34,npc_global_condition_base);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    f.lifecycle.conditions.context=&f;f.lifecycle.conditions.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    ServerEventDictionaryEntry names[16]{};ServerActivityDictionary dictionary{{names,16,0,1,920},0};
    const char* private_names[]{"ACT_FRENZY","ACT_BIG_SLASH","ACT_LEAP_ATTACK","ACT_FLY_ATTACK","ACT_FLY_LAND","ACT_SIDESTEP","ACT_FLY_ATTACK_POUNCE_OFF","ACT_FLY_AWAY","ACT_FLY_LAND_AWAY"};
    int ids[9]{};for(unsigned i=0;i<9;++i)assert(da->private_id(&dictionary,private_names[i],&ids[i]));
    ServerRecombinantPolicyBindings p{};p.context=&f;p.conditions=f.lifecycle.conditions;p.activities=&dictionary;
    p.parent_schedule=[](void*,void*,int id,int* out) noexcept ->int{*out=id;return 1;};
    ServerRecombinantDecisionBindings decision{};decision.policy=&p;decision.context=&f;
    ServerRecombinantTaskBindings t{};t.decision=&decision;t.context=&f;t.lifecycle=f.lifecycle;t.goal_owner=916;
    t.parent=[](void* p,void* a,const ServerNpcScheduleTask*,bool) noexcept ->int{
        auto& f=*static_cast<F*>(p);++f.parents;ServerNpcTaskLifecycleResult o{};return f.life->complete(a,sizeof(f.actor),10.f,false,&f.lifecycle,&o);
    };
    t.submit_goal=[](void* p,unsigned nav,const ServerRecombinantTaskGoal* g,int mode,bool* out) noexcept ->int{auto& f=*static_cast<F*>(p);assert(nav==44&&g->kind==4&&g->activity==10&&!mode);++f.goals;*out=true;return 1;};
    t.enemy=[](void*,void*,void** out) noexcept ->int{*out=nullptr;return 1;};
    t.goal_enemy=[](void*,unsigned nav,void* e) noexcept ->int{return nav==44&&!e;};
    t.activity=[](void* p,void*,int id) noexcept ->int{auto& f=*static_cast<F*>(p);assert(id==932);++f.activities;return 1;};
    ServerRecombinantDamageBindings d{&p,&f};ServerRecombinantMinorBindings minor{&d,&f,1};
    ServerRecombinantMinorTouchBindings touch{&minor,&f};ServerRecombinantMinorTaskBindings mt{&t,&touch,&f,917,931,932};
    mt.special_gate=[](void*,void*,bool* yes) noexcept ->int{*yes=false;return 1;};
    touch.set_movement=[](void*,void*,int type,int mode) noexcept ->int{return type==3&&!mode;};
    ServerRecombinantLaunchBindings launch{&mt,&f};ServerRecombinantDispatchBindings dispatch{&t,&mt,&launch};
    ServerRecombinantScheduleImage im{};ServerRecombinantScheduleBindings ib{p.conditions,&dictionary};ServerRecombinantScheduleResult ir{};
    ServerRecombinantSessionBindings b{};b.image=&im;b.dispatch=&dispatch;b.inherited.conditions=p.conditions;b.inherited.context=&f;b.context=&f;
    b.inherited.storage=[](void* p,unsigned token,const void** out,unsigned* n) noexcept ->int{auto& f=*static_cast<F*>(p);if(token==800){*out=f.inherited_desc;*n=52;return 1;}if(token==801){*out=&f.inherited_task;*n=8;return 1;}return 0;};
    b.inherited.reset_movement=[](void* p,void*,unsigned) noexcept ->int{++static_cast<F*>(p)->resets;return 1;};
    b.inherited_lookup=[](void*,void*,int id,unsigned* token) noexcept ->int{*token=id==3?800:0;return 1;};
    f.inherited_desc[0]=npc_global_condition_base+103;f.inherited_desc[1]=801;f.inherited_desc[2]=1;
    f.put(f.actor,0,recombinant_major_vtable);f.put(f.actor,0x8f8,44u);
    assert(image_api->build(f.actor,sizeof(f.actor),1000,800,&ib,&im,&ir)&&api->bind(f.actor,sizeof(f.actor),&b,&f.session));
    ServerRecombinantSessionResult o{};ServerNpcScheduleStateResult state{};
    f.cond(67,true);assert(api->select(&f.session,&o)&&o.requested==86);
    assert(api->resolve(&f.session,o.requested,&o)&&o.owned&&o.translated==86);
    assert(f.state->install(f.actor,sizeof(f.actor),o.token,10.f,&f.session.schedule,&state)&&state.count==8);
    // Owner explicitly maintains task status and cursor. No fake native
    // scheduler: each actual descriptor is passed to the native dispatch.
    for(unsigned i=0;i<8;++i){
        f.put(f.actor,0x7a4,1);
        assert(api->current_task(&f.session,false,10.f,&o)&&o.owned&&o.state.index==int(i));
        assert(o.task.route==(i==1||i==3?RecombinantDispatchMajor:RecombinantDispatchInherited));
        assert(f.get<int>(f.actor,0x7a4)==4&&f.state->advance(f.actor,sizeof(f.actor),&f.session.schedule,&state));
    }
    assert(state.complete&&f.goals==1&&f.parents==7&&!api->current_task(&f.session,false,10,&o));
    assert(api->resolve(&f.session,3,&o)&&!o.owned&&o.token==800);
    assert(f.state->install(f.actor,sizeof(f.actor),o.token,10.f,&f.session.schedule,&state));
    assert(api->current_task(&f.session,false,10.f,&o)&&!o.owned&&o.task.route==RecombinantDispatchInherited);
    assert(f.state->clear(f.actor,sizeof(f.actor)));
    f.put(f.actor,0,recombinant_minor_vtable);
    assert(!api->select(&f.session,&o));assert(image_api->build(f.actor,sizeof(f.actor),1000,800,&ib,&im,&ir)&&api->bind(f.actor,sizeof(f.actor),&b,&f.session));
    f.cond(67,true);assert(api->select(&f.session,&o)&&o.requested==81);
    assert(api->resolve(&f.session,o.requested,&o)&&o.owned&&o.translated==81);
    assert(f.state->install(f.actor,sizeof(f.actor),o.token,10.f,&f.session.schedule,&state)&&state.count==5);
    f.put(f.actor,0x7a4,1);assert(api->current_task(&f.session,false,10.f,&o)&&o.task.route==RecombinantDispatchMinor&&f.activities==1&&f.get<int>(f.actor,0x7a4)==1);
    f.put(f.actor,0x238,0xffffffffu);
    assert(api->current_task(&f.session,true,10.1f,&o)&&!o.task.task_complete);
    f.actor[0xbc]=1;assert(api->current_task(&f.session,true,10.2f,&o)&&o.task.task_complete);
    assert(f.state->advance(f.actor,sizeof(f.actor),&f.session.schedule,&state)&&state.index==1);
    for(unsigned i=1;i<5;++i){
        f.put(f.actor,0x7a4,1);
        assert(api->current_task(&f.session,false,10.f,&o)&&o.state.index==int(i)&&f.get<int>(f.actor,0x7a4)==4);
        assert(f.state->advance(f.actor,sizeof(f.actor),&f.session.schedule,&state));
    }
    assert(state.complete&&api->preschedule(&f.session,10.f,&o)&&!o.delegated);
    assert(api->fail_select(&f.session,72,1,11,&o)==0); // grounded trace owner missing
    f.actor[0xbc]=0;assert(api->fail_select(&f.session,72,1,11,&o)&&o.requested==86);
    auto copied=f.session;assert(!api->select(&copied,&o));
    ++dictionary.names.generation;assert(!api->resolve(&f.session,86,&o));
    FreeLibrary(server);FreeLibrary(client);return 0;
}
