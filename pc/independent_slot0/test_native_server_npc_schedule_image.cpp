#include "native_server_npc_schedule_image.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
using namespace sin_port::native_weapon_tables;
struct F{
    unsigned char actor[0x1088]{},desc[0x48]{};
    ServerScheduleImage image{};
    const ServerNpcScheduleImageApi* api=nullptr;
    template<class T>void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto c=LoadLibraryA(argv[1]),s=LoadLibraryA(argv[2]);assert(c&&s);
    auto g=reinterpret_cast<GetServerNpcScheduleImageApi>(GetProcAddress(s,server_npc_schedule_image_export));
    auto gd=reinterpret_cast<GetServerActivityDictionaryApi>(GetProcAddress(s,server_activity_dictionary_export));
    auto gs=reinterpret_cast<GetServerNpcScheduleStateApi>(GetProcAddress(s,server_npc_schedule_state_export));
    assert(g&&gd&&gs&&!GetProcAddress(c,server_npc_schedule_image_export)&&!g(0,sizeof(ServerNpcScheduleImageApi)));
    F f;f.api=g(1,sizeof(ServerNpcScheduleImageApi));const auto* dict=gd(1,sizeof(ServerActivityDictionaryApi));const auto* state=gs(1,sizeof(ServerNpcScheduleStateApi));assert(f.api&&dict&&state);
    const int global=npc_global_condition_base;
    f.put(f.desc,4,global+100);f.put(f.desc,8,0);f.put(f.desc,12,127);f.put(f.desc,16,global+227);
    f.put(f.desc,0x34,global+20);f.put(f.desc,0x38,0);f.put(f.desc,0x3c,127);
    ServerEventDictionaryEntry names[6]{};ServerActivityDictionary activities{{names,6,0,1,920},0};
    ServerGruntRegistration ids{};ids.activities_ready=true;ids.activity_owner=&activities;ids.activity_generation=1;
    assert(dict->private_id(&activities,"ACT_COMBINE_BUGBAIT",&ids.activities[2]));
    assert(dict->private_id(&activities,"ACT_COMBINE_AR2_ALTFIRE",&ids.activities[3]));
    assert(dict->private_id(&activities,"ACT_GRUNT_SHOT_IN_HELMET",&ids.activities[5]));
    ServerScheduleImageBindings b{};b.registration=&ids;b.namespace_map.context=&f;
    b.namespace_map.descriptor=[](void* p,void*,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);*out=f.desc;*n=sizeof(f.desc);return 1;};
    b.damage_token=[](void*,const char* token,float* out) noexcept -> int{assert(!std::strcmp(token,"DMG_BURN"));*out=0;return 1;};
    ServerScheduleImageResult result{};
    assert(f.api->build(f.actor,NpcSubclassGround,1000,99,&b,&f.image,&result)&&result.rows==37&&result.tasks==220&&result.interrupts==145&&result.private_activities==3&&result.damage_tokens==1);
    assert(f.image.head==1108&&f.image.rows[0].descriptor[12]==99);
    auto find=[&](int id)->std::uint32_t{for(unsigned i=0;i<f.image.row_count;++i)if(f.image.rows[i].descriptor[0]==static_cast<unsigned>(global+100+id))return f.image.token_base+3*i;return 0;};
    auto token=find(107);assert(token);
    const void* p=nullptr;unsigned n=0;assert(f.api->storage(&f.image,token,&p,&n)&&n==52);
    auto* row=static_cast<const std::uint32_t*>(p);assert(row[2]==20&&row[3+(29+20)/32]&(1u<<((29+20)%32))&&row[3+(73+20)/32]&(1u<<((73+20)%32)));
    assert(f.api->storage(&f.image,token+2,&p,&n)&&!std::strcmp(static_cast<const char*>(p),"SCHED_GRUNT_STUNNED"));
    ServerNpcScheduleBindings sb{};sb.context=&f;sb.conditions=b.namespace_map;
    sb.storage=[](void* p,std::uint32_t token,const void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);return f.api->storage(&f.image,token,out,n);};
    sb.reset_movement=[](void*,void*,std::uint32_t) noexcept -> int{return 1;};
    ServerNpcScheduleStateResult sr{};assert(state->install(f.actor,sizeof(f.actor),token,10,&sb,&sr)&&sr.global==global+207);
    assert(state->inspect(f.actor,sizeof(f.actor),&sb,&sr)&&sr.task.id==88&&sr.task.parameter==25);
    unsigned yes=0;assert(state->interrupt(f.actor,sizeof(f.actor),73,false,&sb,&yes)&&yes);
    assert(state->advance(f.actor,sizeof(f.actor),&sb,&sr)&&state->inspect(f.actor,sizeof(f.actor),&sb,&sr)&&sr.task.id==116);
    assert(f.api->build(f.actor,NpcSubclassElite,2000,99,&b,&f.image,&result)&&result.rows==39&&result.tasks==232);
    assert(f.api->storage(&f.image,find(118)+2,&p,&n)&&!std::strcmp(static_cast<const char*>(p),"SCHED_ELITE_RANGE_ATTACK1"));
    assert(f.api->build(f.actor,NpcSubclassJetpack,3000,99,&b,&f.image,&result)&&result.rows==44&&result.tasks==258&&result.damage_tokens==2);
    assert(f.api->storage(&f.image,find(118)+2,&p,&n)&&!std::strcmp(static_cast<const char*>(p),"SCHED_JETPACK_PATROL"));
    assert(!f.api->build(f.actor,NpcSubclassJetpack,3000,3001,&b,&f.image,&result)&&f.image.head==3129);
    b.damage_token=[](void*,const char*,float* out) noexcept -> int{*out=8.f;return 1;};
    assert(!f.api->build(f.actor,NpcSubclassGround,4000,0,&b,&f.image,&result)&&f.image.token_base==3000);
    b.damage_token=nullptr;
    assert(f.api->build(f.actor,NpcSubclassJetpack,3000,99,&b,&f.image,&result)&&result.damage_tokens==2);
    for(int id: {104,105}){
        const auto token=find(id);assert(token&&f.api->storage(&f.image,token+1,&p,&n));
        const auto* tasks=static_cast<const ServerNpcScheduleTask*>(p);
        unsigned found=0;
        for(unsigned j=0;j<n/sizeof(ServerNpcScheduleTask);++j)if(tasks[j].id==144){
            std::uint32_t bits=1;std::memcpy(&bits,&tasks[j].parameter,4);assert(bits==0);++found;
        }
        assert(found==1);
    }

    auto gc=reinterpret_cast<GetServerNpcCatalogSchedulerApi>(GetProcAddress(s,server_npc_catalog_scheduler_export));
    assert(gc&&!GetProcAddress(c,server_npc_catalog_scheduler_export)&&!gc(0,sizeof(ServerNpcCatalogSchedulerApi))&&!gc(1,0));
    const auto* catalog=gc(1,sizeof(ServerNpcCatalogSchedulerApi));assert(catalog);
    ServerNpcScheduleServicesBindings runtime{};auto& scheduler=runtime.scheduler;
    scheduler.original_vtable=0x2249964c;
    auto& m=scheduler.maintenance;auto& selection=m.selection;selection.head=99;
    selection.grunt.schedule=sb;
    // Only an inherited list is external; custom tokens must use the native image.
    selection.grunt.schedule.storage=nullptr;
    selection.state.state_query=[](void*,void*,int,int* out) noexcept -> int{*out=-1;return 1;};
    scheduler.grunt.registration=&ids;
    scheduler.grunt.parent=[](void*,void* actor,const ServerNpcScheduleTask* task,bool running) noexcept -> int{
        assert(!running&&task->id==89&&task->parameter==32.f);
        int status=2;std::memcpy(static_cast<unsigned char*>(actor)+0x7a4,&status,4);return 1;};
    scheduler.jetpack.inspect_clear=[](void*,void*,float seconds) noexcept -> int{assert(seconds==5.f);return 1;};
    m.invalidated=[](void*,void*) noexcept -> int{return 1;};
    m.before_selection=m.invalidated;m.transition=m.invalidated;
    m.budget=[](void*,void*,bool* out) noexcept -> int{*out=true;return 1;};
    m.validity.base_build=m.invalidated;m.validity.mobility=[](void*,float,bool* out) noexcept -> int{*out=false;return 1;};
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);
    ServerNpcCatalogSchedulerResult run{};
    assert(catalog->run(f.actor,sizeof(f.actor),10,&f.image,&runtime,&run));
    assert(run.runtime.scheduler.jet_selector.schedule==118&&run.runtime.scheduler.jetpack.target_cleared&&run.runtime.scheduler.jetpack.lifecycle.completed);
    assert(run.runtime.scheduler.maintenance.state.schedule==find(118)&&run.native_reads&&!run.inherited_reads);
    assert(catalog->run(f.actor,sizeof(f.actor),11,&f.image,&runtime,&run)&&run.runtime.scheduler.maintenance.advanced==1&&run.runtime.scheduler.jetpack.lifecycle.completed);
    assert(run.runtime.scheduler.maintenance.state.index==1&&run.runtime.scheduler.maintenance.state.task.id==159);
    assert(catalog->run(f.actor,sizeof(f.actor),12,&f.image,&runtime,&run)&&run.runtime.scheduler.maintenance.advanced==1&&run.runtime.scheduler.jetpack.delegated&&run.runtime.scheduler.jetpack.grunt.parent_called);
    assert(run.runtime.scheduler.maintenance.state.index==2&&run.runtime.scheduler.maintenance.state.status==2&&!run.runtime.scheduler.maintenance.runs);
    // Wrong namespace and wrong class rejected before state writes.
    f.put(f.desc,0x34,global+21);assert(!catalog->run(f.actor,sizeof(f.actor),13,&f.image,&runtime,&run)&&!run.native_reads);
    f.put(f.desc,0x34,global+20);scheduler.original_vtable=0x2248df04;
    assert(!catalog->run(f.actor,sizeof(f.actor),13,&f.image,&runtime,&run));
    scheduler.original_vtable=0x2249964c;
    ++activities.names.generation;assert(!f.api->storage(&f.image,find(118),&p,&n));
    assert(!catalog->run(f.actor,sizeof(f.actor),13,&f.image,&runtime,&run));

    // Restore generation only in this synthetic fixture; build a fresh ground image.
    --activities.names.generation;
    b.damage_token=[](void*,const char*,float* out) noexcept -> int{*out=0;return 1;};
    assert(f.api->build(f.actor,NpcSubclassGround,4000,99,&b,&f.image,&result));
    auto gcombat=reinterpret_cast<GetServerNpcCombatCatalogSchedulerApi>(GetProcAddress(s,server_npc_combat_catalog_scheduler_export));
    assert(gcombat&&!GetProcAddress(c,server_npc_combat_catalog_scheduler_export)&&!gcombat(0,sizeof(ServerNpcCombatCatalogSchedulerApi)));
    const auto* combat_api=gcombat(1,sizeof(ServerNpcCombatCatalogSchedulerApi));assert(combat_api);
    scheduler.original_vtable=0x2249aaa4;
    unsigned char weapon[4]{};
    sin_independent::EntityHandleEntry slots[2]{};slots[1].serial=7;slots[1].entity=reinterpret_cast<CBaseEntity*>(weapon);
    selection.state.entities={slots,2,nullptr,[](void*,void*,unsigned* kind) noexcept -> int{*kind=WeaponActivitySmg;return 1;},nullptr};
    scheduler.grunt.special=nullptr;
    m.after_start=m.invalidated;m.after_run=m.invalidated;
    m.motion_gate=[](void*,void*,bool* out) noexcept -> int{*out=false;return 1;};
    ServerGruntCombatTaskBindings combat{};float stamp=0;combat.shared_smg_stamp=&stamp;
    combat.weapon.random=[](void*,int lo,int hi,int* out) noexcept -> int{assert(lo==4&&hi==7);*out=4;return 1;};
    combat.activity=[](void*,void*,int id,bool restart) noexcept -> int{assert(id==16&&restart);return 1;};
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);
    f.put(f.actor,0x78c,0x7001u);f.put(f.actor,0x6cc,0x7001u);
    f.actor[0x3e0]=1;f.put(f.actor,0x3ec,7);f.put(f.actor,0x884,7);
    const auto attack=find(91);assert(attack);
    assert(state->install(f.actor,sizeof(f.actor),attack,20,&sb,&sr));
    f.put(f.actor,0x79c,global+191);
    for(int i=0;i<4;++i)assert(state->advance(f.actor,sizeof(f.actor),&sb,&sr));
    assert(state->inspect(f.actor,sizeof(f.actor),&sb,&sr)&&sr.task.id==67);
    ServerNpcCombatCatalogSchedulerResult cr{};
    assert(combat_api->run(f.actor,sizeof(f.actor),20,&f.image,&runtime,&combat,&cr)&&cr.combat.calls==2&&stamp==20&&cr.catalog.runtime.scheduler.maintenance.state.index==4);
    assert(combat_api->run(f.actor,sizeof(f.actor),20.5f,&f.image,&runtime,&combat,&cr)&&cr.combat.calls==1&&stamp==20);
    assert(combat_api->run(f.actor,sizeof(f.actor),21,&f.image,&runtime,&combat,&cr)&&cr.combat.last.base.lifecycle.completed&&stamp==0);
    assert(combat_api->run(f.actor,sizeof(f.actor),22,&f.image,&runtime,&combat,&cr)&&cr.combat.calls==1&&cr.combat.last.base.task==140&&cr.combat.last.base.lifecycle.completed&&cr.catalog.runtime.scheduler.maintenance.state.index==5);

    auto gf=reinterpret_cast<GetServerNpcFullCatalogSchedulerApi>(GetProcAddress(s,server_npc_full_catalog_scheduler_export));
    assert(gf&&!GetProcAddress(c,server_npc_full_catalog_scheduler_export)&&!gf(0,sizeof(ServerNpcFullCatalogSchedulerApi)));
    const auto* full_api=gf(1,sizeof(ServerNpcFullCatalogSchedulerApi));assert(full_api);
    auto gdsp=reinterpret_cast<GetServerGruntTaskDispatchApi>(GetProcAddress(s,server_grunt_task_dispatch_export));
    auto gct=reinterpret_cast<GetServerGruntCombatTaskApi>(GetProcAddress(s,server_grunt_combat_task_export));
    auto git=reinterpret_cast<GetServerGruntInjuryTaskApi>(GetProcAddress(s,server_grunt_injury_task_export));
    auto ght=reinterpret_cast<GetServerGruntHelperTaskApi>(GetProcAddress(s,server_grunt_helper_task_export));
    assert(gdsp&&gct&&git&&ght);
    const auto* dsp=gdsp(1,sizeof(ServerGruntTaskDispatchApi));const auto* ct=gct(1,sizeof(ServerGruntCombatTaskApi));
    const auto* it=git(1,sizeof(ServerGruntInjuryTaskApi));const auto* ht=ght(1,sizeof(ServerGruntHelperTaskApi));
    unsigned coverage[2]{};
    for(int id=0;id<200;++id)for(unsigned running=0;running<2;++running){
        unsigned route=0;assert(dsp->route(id,running!=0,&route));
        const unsigned extended=ct->supports(id,running!=0)+it->supports(id,running!=0)+ht->supports(id,running!=0);
        assert(extended<=1&&(extended==0||route==GruntTaskHostedSpecial));
        if(route!=GruntTaskParent){assert(route==GruntTaskNative||extended==1);++coverage[running];}
    }
    assert(coverage[0]==26&&coverage[1]==11);
    ServerGruntHelperTaskBindings full{};full.injury.combat=combat;
    full.injury.is_player=[](void*,void*,bool* out) noexcept -> int{*out=false;return 1;};
    full.injury.damage=[](void*,void* a,void* attacker,float amount,int type,float lo,float hi) noexcept -> int{
        assert(a==attacker&&amount==35.f&&type==0&&lo==-.1f&&hi==.1f);return 1;};
    auto install_at=[&](int id,int index){
        std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);
        const auto token=find(id);assert(token&&state->install(f.actor,sizeof(f.actor),token,30,&sb,&sr));
        f.put(f.actor,0x79c,global+100+id);
        for(int i=0;i<index;++i)assert(state->advance(f.actor,sizeof(f.actor),&sb,&sr));
    };
    ServerNpcFullCatalogSchedulerResult fr{};
    install_at(107,4); // original stunned schedule, task151
    assert(full_api->run(f.actor,sizeof(f.actor),30,&f.image,&runtime,&full,&fr)&&fr.full.calls==1&&fr.full.helper_calls==1&&fr.full.last.injury.combat.base.task==151&&fr.full.last.injury.combat.base.lifecycle.completed);
    install_at(104,4); // original burning stand, task144, parsed positive zero
    f.put(f.actor,0x9c,35);f.actor[0xcb1]=1;
    assert(full_api->run(f.actor,sizeof(f.actor),31,&f.image,&runtime,&full,&fr)&&fr.full.calls==1&&fr.full.injury_calls==1&&fr.full.last.injury.damage_called&&fr.full.last.injury.combat.base.lifecycle.completed&&!f.actor[0xcb1]);
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original schedule descriptor materialization and real task cursor");
}
