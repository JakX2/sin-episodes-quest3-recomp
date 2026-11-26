#include "native_server_npc_schedule_image.h"
#include "server_fixed_difficulty_service.h"
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
    unsigned char motor[0x18]{},nav[0x50]{},path[0x48]{};
    unsigned clear_paths=0,stop_paths=0,move_updates=0;
    unsigned char think_component[9]{};unsigned think_component_tasks=0;
    unsigned char sound_record[0x2c]{},sound_cache[0x2c]{};
    bool sound_goal_accept=false;
    bool observation_pvs=false;
    bool align_on_step=false;unsigned begins=0,steps=0;int last_pending=-1;
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

    auto gshared=reinterpret_cast<GetServerNpcSharedCatalogSchedulerApi>(GetProcAddress(s,server_npc_shared_catalog_scheduler_export));
    assert(gshared&&!GetProcAddress(c,server_npc_shared_catalog_scheduler_export)&&!gshared(0,sizeof(ServerNpcSharedCatalogSchedulerApi)));
    const auto* shared_api=gshared(1,sizeof(ServerNpcSharedCatalogSchedulerApi));assert(shared_api);
    ServerNpcSharedTaskBindings shared{};
    scheduler.grunt.parent=nullptr;
    ServerNpcSharedCatalogSchedulerResult sh{};
    install_at(82,2); // original victory dance: TASK_WAIT 1.5
    assert(shared_api->run(f.actor,sizeof(f.actor),40,&f.image,&runtime,&full,&shared,&sh)&&sh.shared.calls==2&&sh.shared.native==2&&!sh.shared.last.lifecycle.completed);
    float deadline=0;std::memcpy(&deadline,f.actor+0x9dc,4);assert(deadline==41.5f);
    assert(shared_api->run(f.actor,sizeof(f.actor),41,&f.image,&runtime,&full,&shared,&sh)&&sh.shared.calls==1&&!sh.shared.last.lifecycle.completed);
    assert(shared_api->run(f.actor,sizeof(f.actor),41.5f,&f.image,&runtime,&full,&shared,&sh)&&sh.shared.calls==1&&sh.shared.last.lifecycle.completed);
    install_at(84,4); // original hide-and-reload: TASK_REMEMBER INCOVER
    assert(shared_api->run(f.actor,sizeof(f.actor),42,&f.image,&runtime,&full,&shared,&sh)&&sh.shared.calls==1&&sh.shared.last.task==119&&sh.shared.last.lifecycle.completed);
    unsigned flags=0;std::memcpy(&flags,f.actor+0x988,4);assert(flags&2u);
    install_at(93,7); // original cover schedule: TASK_SET_SCHEDULE wait-in-cover
    assert(shared_api->run(f.actor,sizeof(f.actor),43,&f.image,&runtime,&full,&shared,&sh)&&sh.shared.calls==1&&sh.shared.last.schedule_set&&sh.shared.last.lifecycle.completed);
    assert(sh.shared.last.schedule.schedule==find(90)&&sh.shared.last.resolution.requested==90);
    scheduler.grunt.context=&f;
    scheduler.grunt.finish_hook=[](void* p,void* a) noexcept -> int{auto& f=F::self(p);assert(a==f.actor);f.actor[0xcb1]=1;return 1;};
    install_at(85,0);
    assert(shared_api->run(f.actor,sizeof(f.actor),44,&f.image,&runtime,&full,&shared,&sh)&&!sh.shared.calls&&f.actor[0xcb1]);

    auto ga=reinterpret_cast<GetServerNpcAttackCatalogSchedulerApi>(GetProcAddress(s,server_npc_attack_catalog_scheduler_export));
    assert(ga&&!GetProcAddress(c,server_npc_attack_catalog_scheduler_export)&&!ga(0,sizeof(ServerNpcAttackCatalogSchedulerApi)));
    const auto* attack_api=ga(1,sizeof(ServerNpcAttackCatalogSchedulerApi));assert(attack_api);
    ServerNpcSharedAttackBindings ab{};ab.shared=shared;ab.shared.pending=[](void*,void*,int id) noexcept -> int{assert(id==17||id==66);return 1;};
    ab.shared.turn=[](void*,void*) noexcept -> int{return 1;};
    ab.aim_vector=[](void*,void*,float* v) noexcept -> int{v[0]=v[1]=v[2]=0;return 1;};
    scheduler.grunt.lifecycle.random_int=[](void*,unsigned lo,unsigned hi,unsigned* out) noexcept -> int{assert(lo==2&&hi==3);*out=2;return 1;};
    ServerNpcAttackCatalogSchedulerResult ar{};
    install_at(97,1); // actual TASK_RANGE_ATTACK2
    assert(attack_api->run(f.actor,sizeof(f.actor),50,&f.image,&runtime,&full,&ab,&ar)&&ar.attack.calls==2&&ar.attack.last.task==68&&!ar.attack.last.lifecycle.completed);
    f.actor[0x3e0]=1;f.put(f.actor,0x3ec,7);f.put(f.actor,0x884,7);
    assert(attack_api->run(f.actor,sizeof(f.actor),51,&f.image,&runtime,&full,&ab,&ar)&&ar.attack.calls==1&&ar.attack.last.lifecycle.completed);
    install_at(84,6); // actual TASK_RELOAD
    f.put(f.actor,0x9b0,std::uint16_t(2));f.put(f.actor,0x9b2,std::uint16_t(3));
    f.actor[0x3e0]=1;f.put(f.actor,0x3ec,7);f.put(f.actor,0x884,7);
    assert(attack_api->run(f.actor,sizeof(f.actor),52,&f.image,&runtime,&full,&ab,&ar)&&ar.attack.calls==2&&ar.attack.last.task==71&&ar.attack.last.lifecycle.completed&&ar.attack.last.timer_reset);
    auto gface=reinterpret_cast<GetServerNpcFacingCatalogSchedulerApi>(GetProcAddress(s,server_npc_facing_catalog_scheduler_export));
    assert(gface&&!GetProcAddress(c,server_npc_facing_catalog_scheduler_export)&&!gface(0,sizeof(ServerNpcFacingCatalogSchedulerApi))&&!gface(1,0));
    const auto* face_api=gface(1,sizeof(ServerNpcFacingCatalogSchedulerApi));assert(face_api);
    ServerNpcSharedFacingBindings fb{};fb.context=&f;
    fb.motor_storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{if(token!=88)return 0;auto& f=F::self(p);*out=f.motor;*n=sizeof(f.motor);return 1;};
    fb.aim_vector=[](void*,void*,float* v) noexcept -> int{v[0]=1;v[1]=2;v[2]=3;return 1;};
    fb.aligned=[](void*,void*,const float*,bool* out) noexcept -> int{*out=true;return 1;};
    fb.desired_yaw=[](void*,void*,bool flag,double* out) noexcept -> int{assert(flag);*out=90;return 1;};
    fb.begin_turn=[](void* p,void*) noexcept -> int{++F::self(p).begins;return 1;};
    fb.step_turn=[](void* p,void* motor,int mode) noexcept -> int{auto& f=F::self(p);assert(motor==f.motor&&mode==-1);++f.steps;if(f.align_on_step)f.put(f.actor,0x370,90.f);return 1;};
    ServerNpcFacingCatalogSchedulerResult facing_run{};
    install_at(83,2); // SCHED_GRUNT_COMBAT_FACE: TASK_FACE_ENEMY
    f.put(f.actor,0x908,88u);f.put(f.motor,0x14,0.f);
    assert(face_api->run(f.actor,sizeof(f.actor),52,&f.image,&runtime,&full,&ab,&fb,&facing_run)&&facing_run.facing.calls==2&&facing_run.facing.native==2&&!facing_run.facing.last.completed&&f.begins==1&&f.steps==1);
    f.align_on_step=true;
    assert(face_api->run(f.actor,sizeof(f.actor),53,&f.image,&runtime,&full,&ab,&fb,&facing_run)&&facing_run.facing.calls==1&&facing_run.facing.last.completed&&f.steps==2);
    assert(face_api->run(f.actor,sizeof(f.actor),54,&f.image,&runtime,&full,&ab,&fb,&facing_run)&&facing_run.facing.calls==0&&facing_run.attack.shared.shared.last.task==2&&facing_run.attack.shared.shared.last.timer_written==0);
    auto ganim=reinterpret_cast<GetServerNpcAnimationCatalogSchedulerApi>(GetProcAddress(s,server_npc_animation_catalog_scheduler_export));
    assert(ganim&&!GetProcAddress(c,server_npc_animation_catalog_scheduler_export)&&!ganim(0,sizeof(ServerNpcAnimationCatalogSchedulerApi))&&!ganim(1,0));
    const auto* anim_api=ganim(1,sizeof(ServerNpcAnimationCatalogSchedulerApi));assert(anim_api);
    ServerNpcSharedTargetAnimationBindings tb{};tb.context=&f;tb.motor_storage=fb.motor_storage;tb.step_turn=fb.step_turn;
    tb.current_target=[](void* p,void*,void** out) noexcept -> int{*out=p;return 1;};
    tb.absolute_position=[](void* p,void* target,float* v) noexcept -> int{assert(p==target);v[0]=0;v[1]=2;v[2]=0;return 1;};
    auto old_pending=ab.shared.pending;ab.shared.context=&f;
    ab.shared.pending=[](void* p,void*,int id) noexcept -> int{F::self(p).last_pending=id;return 1;};
    ServerNpcAnimationCatalogSchedulerResult nar{};
    install_at(86,2); // SCHED_GRUNT_SIGNAL_SUPPRESS: animation task95, activity54
    f.put(f.actor,0x908,88u);f.put(f.motor,0x14,0.f);
    assert(anim_api->run(f.actor,sizeof(f.actor),55,&f.image,&runtime,&full,&ab,&fb,&tb,&nar)&&nar.facing.attack.shared.shared.calls==1&&f.last_pending==54&&nar.target_animation.calls==1&&nar.target_animation.last.yaw==90&&!nar.target_animation.last.animation_finished);
    f.actor[0x3e0]=1;
    assert(anim_api->run(f.actor,sizeof(f.actor),56,&f.image,&runtime,&full,&ab,&fb,&tb,&nar)&&nar.target_animation.calls==1&&nar.target_animation.last.shared.lifecycle.completed);
    assert(anim_api->run(f.actor,sizeof(f.actor),57,&f.image,&runtime,&full,&ab,&fb,&tb,&nar)&&!nar.target_animation.calls&&nar.facing.attack.shared.shared.last.task==2);
    install_at(106,2); // SCHED_GRUNT_ALTFIRE: parsed private activity
    f.put(f.actor,0x908,88u);f.actor[0x3e0]=1;
    assert(anim_api->run(f.actor,sizeof(f.actor),58,&f.image,&runtime,&full,&ab,&fb,&tb,&nar)&&f.last_pending==ids.activities[3]&&nar.target_animation.calls==1&&nar.target_animation.last.shared.lifecycle.completed);
    auto gmove=reinterpret_cast<GetServerNpcMovementCatalogSchedulerApi>(GetProcAddress(s,server_npc_movement_catalog_scheduler_export));
    assert(gmove&&!GetProcAddress(c,server_npc_movement_catalog_scheduler_export)&&!gmove(0,sizeof(ServerNpcMovementCatalogSchedulerApi))&&!gmove(1,0));
    const auto* move_api=gmove(1,sizeof(ServerNpcMovementCatalogSchedulerApi));assert(move_api);
    ServerNpcSharedMovementBindings mb{};mb.context=&f;
    mb.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(token==100){*out=f.nav;*n=sizeof(f.nav);return 1;}if(token==101){*out=f.path;*n=sizeof(f.path);return 1;}return 0;};
    mb.clear_path=[](void* p,void* nav) noexcept -> int{auto& f=F::self(p);assert(nav==f.nav);++f.clear_paths;f.put(f.path,0,0u);f.put(f.path,0x44,0u);return 1;};
    mb.stop_path=[](void* p,void* nav,bool finished) noexcept -> int{auto& f=F::self(p);assert(nav==f.nav&&finished);++f.stop_paths;return 1;};
    mb.pose_lookup=[](void*,void*,const char* name,int* out) noexcept -> int{assert(!std::strcmp(name,"move_yaw"));*out=-1;return 1;};
    mb.movement_update=[](void* p,void*) noexcept -> int{++F::self(p).move_updates;return 1;};
    auto bind_nav=[&](){f.put(f.actor,0x8f8,100u);f.put(f.nav,0x24,101u);f.put(f.nav,0xc,0);f.put(f.path,0,0u);f.put(f.path,0x44,0u);};
    ServerNpcMovementCatalogSchedulerResult mvr{};
    install_at(83,0);bind_nav(); // SCHED_GRUNT_COMBAT_FACE starts TASK_STOP_MOVING
    assert(move_api->run(f.actor,sizeof(f.actor),59,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&mvr)&&mvr.movement.calls==1&&mvr.movement.last.shared.lifecycle.completed&&f.clear_paths==1&&f.last_pending==1);
    assert(move_api->run(f.actor,sizeof(f.actor),60,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&mvr)&&!mvr.movement.calls&&mvr.animation.facing.attack.shared.shared.last.task==86);
    install_at(84,3);bind_nav(); // SCHED_GRUNT_HIDE_AND_RELOAD: TASK_WAIT_FOR_MOVEMENT
    f.put(f.path,0,1u);f.put(f.path,0x44,2u);
    assert(move_api->run(f.actor,sizeof(f.actor),61,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&mvr)&&mvr.movement.calls==2&&f.move_updates==2&&!mvr.movement.last.shared.lifecycle.completed);
    f.put(f.path,0x44,0u);
    assert(move_api->run(f.actor,sizeof(f.actor),62,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&mvr)&&mvr.movement.calls==1&&f.stop_paths==1&&mvr.movement.last.shared.lifecycle.completed);
    assert(move_api->run(f.actor,sizeof(f.actor),63,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&mvr)&&!mvr.movement.calls&&mvr.animation.facing.attack.shared.shared.last.task==119&&mvr.animation.facing.attack.shared.shared.last.lifecycle.completed);
    // Original movement-configuration task89, no animation callback.
    install_at(87,1);bind_nav();float stored_distance=0;
    assert(move_api->run(f.actor,sizeof(f.actor),64,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&mvr)&&mvr.movement.calls==1&&mvr.movement.last.shared.task==89&&mvr.movement.last.shared.lifecycle.completed);
    std::memcpy(&stored_distance,f.path+4,4);assert(stored_distance==48.f);
    auto gcue=reinterpret_cast<GetServerNpcCueCatalogSchedulerApi>(GetProcAddress(s,server_npc_cue_catalog_scheduler_export));
    assert(gcue&&!GetProcAddress(c,server_npc_cue_catalog_scheduler_export)&&!gcue(0,sizeof(ServerNpcCueCatalogSchedulerApi))&&!gcue(1,0));
    const auto* cue_api=gcue(1,sizeof(ServerNpcCueCatalogSchedulerApi));assert(cue_api);
    ServerNpcSharedCueBindings cb{};cb.context=&f; // class is rebound by scheduler
    cb.random=[](void*,int lo,int hi,int* out) noexcept -> int{assert(lo==0&&hi==5);*out=0;return 1;};
    cb.target=[](void* p,void*,void** out) noexcept -> int{*out=F::self(p).motor;return 1;};
    cb.as_npc=[](void*,void* e,void** out) noexcept -> int{*out=e;return 1;};
    cb.center=[](void*,void*,float* out) noexcept -> int{out[0]=out[1]=out[2]=0;return 1;};
    cb.target_gate=[](void*,void*,void*,bool* out) noexcept -> int{*out=true;return 1;};
    cb.visible=[](void*,void*,void*,unsigned mask,bool* out) noexcept -> int{assert(mask==0x4081);*out=true;return 1;};
    cb.path_distance=[](void*,std::uint32_t token,float* out) noexcept -> int{assert(token==100);*out=241;return 1;};
    cb.cue=[](void* p,void*,const char* name,int priority,int type) noexcept -> int{assert(priority==0&&type==2);auto& f=F::self(p);if(!std::strcmp(name,"GRUNT_ASSAULT"))f.last_pending=1000;else{assert(!std::strcmp(name,"GRUNT_FLANK"));f.last_pending=1001;}return 1;};
    ServerNpcCueCatalogSchedulerResult crun{};
    install_at(87,4);bind_nav(); // SCHED_GRUNT_ASSAULT original mode0
    assert(cue_api->run(f.actor,sizeof(f.actor),65,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&crun)&&crun.cue.calls==1&&crun.cue.native==1&&crun.cue.last.shared.lifecycle.completed&&crun.cue.last.emitted&&f.last_pending==1000);
    install_at(88,3);bind_nav(); // SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE mode1
    assert(cue_api->run(f.actor,sizeof(f.actor),66,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&crun)&&crun.cue.calls==1&&crun.cue.last.mode==1&&crun.cue.last.emitted&&f.last_pending==1001);
    auto gobservation=reinterpret_cast<GetServerNpcObservationCatalogSchedulerApi>(GetProcAddress(s,server_npc_observation_catalog_scheduler_export));
    assert(gobservation&&!GetProcAddress(c,server_npc_observation_catalog_scheduler_export)&&!gobservation(0,sizeof(ServerNpcObservationCatalogSchedulerApi))&&!gobservation(1,0));
    const auto* observation_api=gobservation(1,sizeof(ServerNpcObservationCatalogSchedulerApi));assert(observation_api);
    ServerNpcSharedObservationBindings ob{};ob.context=&f; // actual class rebound by scheduler
    ob.pvs_query=[](void* p,std::uint32_t token,bool* out) noexcept -> int{assert(token==99);*out=F::self(p).observation_pvs;return 1;};
    ServerNpcObservationCatalogSchedulerResult observed{};
    install_at(81,3); // final task of original SCHED_GRUNT_COMBAT_FAIL
    f.put(f.actor,0x16c,99u);f.observation_pvs=false;
    assert(observation_api->run(f.actor,sizeof(f.actor),67,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&observed)&&observed.cue.movement.animation.facing.attack.shared.shared.calls==1&&observed.cue.movement.animation.facing.attack.shared.shared.last.task==6&&observed.observation.calls==1&&!observed.observation.last.shared.lifecycle.completed);
    f.observation_pvs=true;
    assert(observation_api->run(f.actor,sizeof(f.actor),68,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&observed)&&observed.observation.calls==1&&observed.observation.last.pvs&&observed.observation.last.shared.lifecycle.completed);
    install_at(81,3);f.put(f.actor,0xb4,0x400u);
    assert(observation_api->run(f.actor,sizeof(f.actor),69,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&observed)&&observed.observation.calls==1&&observed.observation.last.flag_gate&&!observed.observation.last.pvs_called&&observed.observation.last.shared.lifecycle.completed);
    auto gcover=reinterpret_cast<GetServerNpcSoundCoverCatalogSchedulerApi>(GetProcAddress(s,server_npc_sound_cover_catalog_scheduler_export));
    assert(gcover&&!GetProcAddress(c,server_npc_sound_cover_catalog_scheduler_export)&&!gcover(0,sizeof(ServerNpcSoundCoverCatalogSchedulerApi))&&!gcover(1,0));
    const auto* cover_api=gcover(1,sizeof(ServerNpcSoundCoverCatalogSchedulerApi));assert(cover_api);
    ServerNpcSharedSoundCoverBindings scb{};scb.context=&f;
    scb.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{if(token!=222)return 0;auto& f=F::self(p);*out=f.sound_cache;*n=sizeof(f.sound_cache);return 1;};
    scb.best_sound=[](void* p,void*,unsigned mask,const void** out,unsigned* n) noexcept -> int{assert(mask==0xfff1f);auto& f=F::self(p);*out=f.sound_record;*n=sizeof(f.sound_record);return 1;};
    scb.cover_position=[](void*,void*,const void*,float* pos,bool* out) noexcept -> int{pos[0]=10;pos[1]=20;pos[2]=30;*out=true;return 1;};
    scb.prepare_navigation=[](void*,std::uint32_t nav) noexcept -> int{assert(nav==100);return 1;};
    scb.goal_context=[](void*,std::uint32_t* out) noexcept -> int{*out=333;return 1;};
    scb.submit_goal=[](void* p,std::uint32_t nav,const ServerNpcSoundCoverGoal* goal,bool* accepted) noexcept -> int{auto& f=F::self(p);assert(nav==100&&goal->type==4&&goal->position[2]==30&&goal->range==100.f&&goal->activity==10&&goal->context_token==333);*accepted=f.sound_goal_accept;return 1;};
    ServerNpcSoundCoverCatalogSchedulerResult cover_run{};
    const bool previous_inner_budget=m.check_inner_budget;m.check_inner_budget=true;
    install_at(94,4);bind_nav(); // actual TASK_FIND_COVER_FROM_BEST_SOUND
    f.put(f.actor,0x89c,222u);f.put(f.sound_record,4,200);f.put(f.sound_record,0xc,2u);
    assert(cover_api->run(f.actor,sizeof(f.actor),69.1f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&cover_run)&&cover_run.observation.cue.movement.animation.facing.attack.shared.shared.calls==1&&cover_run.sound_cover.calls==1&&cover_run.sound_cover.last.advanced&&cover_run.sound_cover.last.cache_copied);
    assert(cover_api->run(f.actor,sizeof(f.actor),69.2f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&cover_run)&&cover_run.sound_cover.calls==1&&cover_run.sound_cover.last.submitted&&!cover_run.sound_cover.last.accepted);
    f.sound_goal_accept=true;
    assert(cover_api->run(f.actor,sizeof(f.actor),69.3f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&cover_run)&&cover_run.sound_cover.calls==1&&cover_run.sound_cover.last.accepted&&cover_run.sound_cover.last.deadline_written);
    float sound_deadline=0;std::memcpy(&sound_deadline,f.actor+0x8f0,4);assert(sound_deadline==69.3f);
    assert(cover_run.observation.cue.movement.animation.facing.attack.shared.full.combat.catalog.runtime.scheduler.maintenance.state.index==4);
    m.check_inner_budget=previous_inner_budget;
    auto gtactical=reinterpret_cast<GetServerNpcTacticalCoverCatalogSchedulerApi>(GetProcAddress(s,server_npc_tactical_cover_catalog_scheduler_export));
    assert(gtactical&&!GetProcAddress(c,server_npc_tactical_cover_catalog_scheduler_export)&&!gtactical(0,sizeof(ServerNpcTacticalCoverCatalogSchedulerApi))&&!gtactical(1,0));
    const auto* tactical_api=gtactical(1,sizeof(ServerNpcTacticalCoverCatalogSchedulerApi));assert(tactical_api);
    ServerNpcSharedTacticalCoverBindings tcb{};tcb.context=&f;
    tcb.default_range=[](void*,void*,float* out) noexcept -> int{*out=768;return 1;};
    tcb.absolute_origin=[](void* p,void* e,float* out) noexcept -> int{assert(e==F::self(p).actor);out[0]=1;out[1]=2;out[2]=3;return 1;};
    tcb.viewpoint=[](void*,void*,float* out) noexcept -> int{out[0]=4;out[1]=5;out[2]=6;return 1;};
    tcb.search=[](void*,std::uint32_t token,const float* origin,const float* eye,float minimum,float maximum,float* pos,bool* found) noexcept -> int{assert(token==444&&origin[0]==1&&eye[2]==6&&minimum==384&&maximum==768);pos[0]=10;pos[1]=20;pos[2]=30;*found=true;return 1;};
    tcb.goal_context=scb.goal_context;
    tcb.submit_goal=[](void*,std::uint32_t nav,const ServerNpcSoundCoverGoal* goal,bool* accepted) noexcept -> int{assert(nav==100&&goal->type==7&&goal->position[2]==30&&goal->range==-1&&goal->activity==10);*accepted=true;return 1;};
    ServerNpcTacticalCoverCatalogSchedulerResult tactical_run;
    tactical_run.sound_cover={};tactical_run.tactical={};
    install_at(96,2);bind_nav(); // SCHED_GRUNT_GRENADE_COVER1 Task103(384)
    f.put(f.actor,0x9d8,444u);
    assert(tactical_api->run(f.actor,sizeof(f.actor),69.4f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&tactical_run)&&tactical_run.tactical.calls==1&&tactical_run.tactical.last.shared.lifecycle.completed&&tactical_run.tactical.last.minimum==384&&tactical_run.tactical.last.maximum==768&&tactical_run.tactical.last.accepted);
    assert(tactical_run.sound_cover.observation.cue.movement.animation.facing.attack.shared.full.combat.catalog.runtime.scheduler.maintenance.state.index==2);
    // Full catalog route: valid schedule leaves gathering untouched; invalidated
    // schedule reaches native +420 and propagates a component service failure.
    auto gg=reinterpret_cast<GetServerNpcNativeGatherCatalogSchedulerApi>(GetProcAddress(s,server_npc_native_gather_catalog_scheduler_export));
    assert(gg&&!GetProcAddress(c,server_npc_native_gather_catalog_scheduler_export)&&!gg(0,sizeof(ServerNpcNativeGatherCatalogSchedulerApi))&&!gg(1,0));
    const auto* gather_api=gg(1,sizeof(ServerNpcNativeGatherCatalogSchedulerApi));assert(gather_api);
    ServerNpcGruntGatherNativeBindings gb{};
    ServerNpcNativeGatherCatalogSchedulerResult gather_run;
    auto old_before=m.before_selection;m.before_selection=nullptr;
    install_at(96,2);bind_nav();f.put(f.actor,0x9d8,444u);
    assert(gather_api->run(f.actor,sizeof(f.actor),69.41f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&gb,&gather_run)&&!gather_run.gather_calls&&gather_run.scheduler.tactical.last.accepted);
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);f.put(f.actor,0xc18,711u);
    unsigned component_calls=0;gb.gather.context=&component_calls;
    gb.gather.component_update=[](void* p,std::uint32_t token) noexcept -> int{assert(token==711);++*static_cast<unsigned*>(p);return 0;};
    assert(!gather_api->run(f.actor,sizeof(f.actor),69.42f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&gb,&gather_run)&&gather_run.gather_calls==1&&component_calls==1&&!gather_run.gather.gather.completed);
    // Complete native gathering, then deliberately fail ideal-state query.
    // This verifies the gather occurs before state reconsideration, without
    // requiring unrelated engine selection services in this fixture.
    gb.gather.max_clients=2;gb.gather.component_update=[](void* p,std::uint32_t token) noexcept -> int{assert(token==711);++*static_cast<unsigned*>(p);return 1;};
    auto old_query=selection.state.state_query;
    selection.state.state_query=[](void*,void* actor,int current,int*) noexcept -> int{assert(current==1);static_cast<unsigned char*>(actor)[0x100]=1;return 0;};
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);f.put(f.actor,0xc18,711u);
    f.put(f.actor,0xcb4,0xffffffffu);
    assert(!gather_api->run(f.actor,sizeof(f.actor),69.43f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&gb,&gather_run)&&gather_run.gather_calls==1&&component_calls==2&&gather_run.gather.gather.completed&&gather_run.gather.gather.component_called&&f.actor[0x100]==1);
    selection.state.state_query=old_query;
    // Outer +41c: gather once on valid schedule, before preschedule/task dispatch.
    auto gthink=reinterpret_cast<GetServerNpcThinkCatalogApi>(GetProcAddress(s,server_npc_think_catalog_export));
    assert(gthink&&!GetProcAddress(c,server_npc_think_catalog_export)&&!gthink(0,sizeof(ServerNpcThinkCatalogApi))&&!gthink(1,0));
    const auto* think_api=gthink(1,sizeof(ServerNpcThinkCatalogApi));assert(think_api);
    // A live +c18 component receives task dispatch before inherited native work.
    ab.shared.component_storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        auto& f=F::self(p);assert(token==711);*out=f.think_component;*n=sizeof(f.think_component);return 1;
    };
    ab.shared.component_task=[](void* p,std::uint32_t token,const ServerNpcScheduleTask* task,bool running) noexcept -> int{
        auto& f=F::self(p);assert(token==711&&task->id==103&&!running&&f.think_component[8]==1);
        ++f.think_component_tasks;f.think_component[8]=0;return 1;
    };
    ServerNpcThinkCatalogBindings think{};think.gather=gb;think.context=&f;
    think.preschedule=[](void* p,void* a,std::uint32_t cls,float) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&cls==0x2249aaa4&&f.actor[0x84d]==1&&!f.actor[0x84e]);f.actor[0x101]=1;return 1;};
    auto old_submit=tcb.submit_goal;tcb.submit_goal=[](void* p,std::uint32_t nav,const ServerNpcSoundCoverGoal*,bool* accepted) noexcept -> int{
        auto& f=F::self(p);assert(nav==100&&f.actor[0x101]==1);*accepted=true;
        // Simulate transient conditions raised during execution, then native cleanup.
        f.put(f.actor,0x7d0,7u<<4);f.put(f.actor,0x7d4,1u<<18);return 1;
    };tcb.context=&f;
    ServerNpcThinkCatalogResult think_run;
    install_at(96,2);bind_nav();f.put(f.actor,0x9d8,444u);f.put(f.actor,0xc18,711u);f.actor[0x84d]=f.actor[0x84e]=1;
    component_calls=0;
    const int think_ok=think_api->run(f.actor,sizeof(f.actor),69.44f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&think,&think_run);
    std::fprintf(stderr,"outer think diagnostic: status=%d outer=%u gather=%u pre=%u maintenance=%u completed=%u inner_gather=%u tactical=%u components=%u cleared=%u\n",think_ok,think_run.outer_called,think_run.outer_gather.gather.completed,think_run.preschedule_called,think_run.maintenance_called,think_run.completed,think_run.scheduler.gather_calls,think_run.scheduler.scheduler.tactical.last.accepted,component_calls,think_run.conditions_cleared);
    assert(think_ok);
    assert(think_run.outer_called);
    assert(think_run.outer_gather.gather.completed);
    assert(think_run.preschedule_called);
    assert(think_run.maintenance_called);
    assert(think_run.completed);
    assert(!think_run.scheduler.gather_calls);
    assert(think_run.scheduler.scheduler.tactical.last.accepted);
    assert(component_calls==1);
    assert(think_run.conditions_cleared==4);
    assert(f.think_component_tasks==1&&think_run.scheduler.scheduler.tactical.last.shared.component_called&&!think_run.scheduler.scheduler.tactical.last.shared.component_handled);
    tcb.submit_goal=old_submit;
    // Invalidated schedule can invoke +420 again because +84e remains false.
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);f.put(f.actor,0xc18,711u);component_calls=0;
    think.gather.gather.component_update=[](void* p,std::uint32_t token) noexcept -> int{assert(token==711);return ++*static_cast<unsigned*>(p)==1;};
    assert(!think_api->run(f.actor,sizeof(f.actor),69.45f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&think,&think_run)&&think_run.outer_gather.gather.completed&&think_run.preschedule_called&&think_run.maintenance_called&&think_run.scheduler.gather_calls==1&&component_calls==2&&!think_run.completed);
    // Same outer route, now remove the complete hosted +424 method.
    auto gnpt=reinterpret_cast<GetServerNpcNativePrescheduleThinkApi>(GetProcAddress(s,server_npc_native_preschedule_think_export));
    assert(gnpt&&!GetProcAddress(c,server_npc_native_preschedule_think_export)&&!gnpt(0,sizeof(ServerNpcNativePrescheduleThinkApi))&&!gnpt(1,0));
    const auto* npt=gnpt(1,sizeof(ServerNpcNativePrescheduleThinkApi));assert(npt);
    ServerNpcNativePrescheduleThinkBindings nt{};nt.think=think;nt.think.preschedule=nullptr;nt.think.gather=gb;
    nt.preschedule.context=&f;
    nt.preschedule.component_tick=[](void* p,std::uint32_t token) noexcept -> int{assert(token==711);F::self(p).actor[0x101]=1;return 1;};
    nt.preschedule.speech_queue_tick=[](void* p,void* a,float) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&f.actor[0x101]);f.actor[0x102]=1;return 1;};
    ServerNpcNativePrescheduleThinkResult ntr;
    install_at(96,2);bind_nav();f.put(f.actor,0x9d8,444u);f.put(f.actor,0xc18,711u);component_calls=0;
    assert(npt->run(f.actor,sizeof(f.actor),69.46f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&nt,&ntr)&&ntr.think.completed&&ntr.preschedule.completed&&ntr.preschedule.component_called&&ntr.preschedule.speech_called&&f.actor[0x102]&&component_calls==1&&ntr.think.scheduler.scheduler.tactical.last.accepted&&!ntr.elite_called&&!ntr.jet_called);
    assert(f.think_component_tasks==2);
    // Full outer AI cycle: native pending speech, no broad speech/preschedule callback.
    auto gst=reinterpret_cast<GetServerNpcNativeSpeechThinkApi>(GetProcAddress(s,server_npc_native_speech_think_export));
    assert(gst&&!GetProcAddress(c,server_npc_native_speech_think_export)&&!gst(0,sizeof(ServerNpcNativeSpeechThinkApi))&&!gst(1,0));
    const auto* stapi=gst(1,sizeof(ServerNpcNativeSpeechThinkApi));assert(stapi);
    ServerNpcNativeSpeechThinkBindings stbind{};stbind.preschedule=nt;stbind.preschedule.preschedule.speech_queue_tick=nullptr;
    stbind.speech.context=&f;stbind.speech.volume=1;stbind.speech.level=75;
    stbind.speech.deliver=[](void* p,std::uint32_t token,int index,float volume,int level,int zero,std::uint32_t extra) noexcept -> int{
        auto& f=F::self(p);assert(f.actor[0x101]&&token==37&&index==17&&volume==1&&level==75&&!zero&&extra==9);f.actor[0x102]=1;return 1;
    };
    stbind.speech.random_float=[](void* p,float lo,float hi,float* out) noexcept -> int{
        assert(F::self(p).actor[0x102]&&lo==1.5f&&hi==2.f);*out=1.5f;return 1;
    };
    auto cover_submit=tcb.submit_goal;
    tcb.submit_goal=[](void* p,std::uint32_t nav,const ServerNpcSoundCoverGoal*,bool* accepted) noexcept -> int{
        auto& f=F::self(p);int queued=0;std::memcpy(&queued,f.actor+0xccc,4);assert(nav==100&&queued==-1&&f.actor[0x102]);*accepted=true;return 1;
    };
    ServerNpcNativeSpeechThinkResult strun;
    install_at(96,2);bind_nav();f.put(f.actor,0x9d8,444u);f.put(f.actor,0xc18,711u);component_calls=0;
    f.put(f.actor,0x16c,37u);f.put(f.actor,0xcc8,9u);f.put(f.actor,0xccc,17);f.put(f.actor,0xcd0,69.47f);f.put(f.actor,0xcd4,2);
    assert(stapi->run(f.actor,sizeof(f.actor),69.47f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&stbind,&strun));
    assert(strun.preschedule.think.completed&&strun.preschedule.preschedule.completed&&strun.speech.delivered&&strun.speech.owner_written&&strun.speech.cleared&&strun.preschedule.think.scheduler.scheduler.tactical.last.accepted&&component_calls==1&&f.think_component_tasks==3);
    // Unified immediate and pending dialogue bindings across both +420 call sites.
    auto gdt=reinterpret_cast<GetServerNpcNativeDialogueThinkApi>(GetProcAddress(s,server_npc_native_dialogue_think_export));
    assert(gdt&&!GetProcAddress(c,server_npc_native_dialogue_think_export)&&!gdt(0,sizeof(ServerNpcNativeDialogueThinkApi))&&!gdt(1,0));
    const auto* dt=gdt(1,sizeof(ServerNpcNativeDialogueThinkApi));assert(dt);
    ServerNpcNativeDialogueThinkBindings dbind{};dbind.preschedule=stbind.preschedule;dbind.dialogue.sentence.speech=stbind.speech;
    ServerNpcNativeDialogueThinkResult dtr;
    install_at(96,2);bind_nav();f.put(f.actor,0x9d8,444u);f.put(f.actor,0xc18,711u);component_calls=0;
    f.put(f.actor,0x16c,37u);f.put(f.actor,0xcc8,9u);f.put(f.actor,0xccc,17);f.put(f.actor,0xcd0,69.48f);f.put(f.actor,0xcd4,2);
    assert(dt->run(f.actor,sizeof(f.actor),69.48f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&dbind,&dtr));
    assert(dtr.cycle.preschedule.think.completed&&dtr.cycle.speech.delivered&&dtr.cycle.speech.cleared&&component_calls==1&&f.think_component_tasks==4&&!dtr.outer_dialogue.idle_calls&&!dtr.maintenance_dialogue.idle_calls);
    // Invalidated schedule: both gathers must use the new route, preserving the component failure.
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);f.put(f.actor,0xc18,711u);f.put(f.actor,0xccc,-1);component_calls=0;
    dbind.preschedule.think.gather.gather.component_update=[](void* p,std::uint32_t token) noexcept -> int{assert(token==711);return ++*static_cast<unsigned*>(p)==1;};
    assert(!dt->run(f.actor,sizeof(f.actor),69.49f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&dbind,&dtr)&&dtr.cycle.preschedule.think.outer_gather.gather.completed&&dtr.cycle.speech.empty&&dtr.cycle.preschedule.think.maintenance_called&&dtr.cycle.preschedule.think.scheduler.gather_calls==1&&component_calls==2);
    auto gnst=reinterpret_cast<GetServerNpcNativeSquadThinkApi>(GetProcAddress(s,server_npc_native_squad_think_export));
    assert(gnst&&!GetProcAddress(c,server_npc_native_squad_think_export)&&!gnst(0,sizeof(ServerNpcNativeSquadThinkApi))&&!gnst(1,0));
    const auto* nst=gnst(1,sizeof(ServerNpcNativeSquadThinkApi));assert(nst);
    ServerNpcNativeSquadThinkBindings nbind{};nbind.preschedule=stbind.preschedule;nbind.perception.dialogue.sentence.speech=stbind.speech;
    ServerNpcNativeSquadThinkResult nsrun;
    install_at(96,2);bind_nav();f.put(f.actor,0x9d8,444u);f.put(f.actor,0xc18,711u);component_calls=0;
    f.put(f.actor,0x16c,37u);f.put(f.actor,0xcc8,9u);f.put(f.actor,0xccc,17);f.put(f.actor,0xcd0,69.49f);f.put(f.actor,0xcd4,2);
    assert(nst->run(f.actor,sizeof(f.actor),69.49f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&nbind,&nsrun)&&nsrun.cycle.preschedule.think.completed&&nsrun.cycle.speech.delivered&&nsrun.cycle.speech.cleared&&!nsrun.selections&&!nsrun.outer.release_calls&&component_calls==1&&f.think_component_tasks==5);
    auto gsrthink=reinterpret_cast<GetServerNpcNativeSquadReportThinkApi>(GetProcAddress(s,server_npc_native_squad_report_think_export));
    assert(gsrthink&&!GetProcAddress(c,server_npc_native_squad_report_think_export)&&!gsrthink(0,sizeof(ServerNpcNativeSquadReportThinkApi))&&!gsrthink(1,0));
    const auto* srthink=gsrthink(1,sizeof(ServerNpcNativeSquadReportThinkApi));assert(srthink);
    ServerNpcNativeSquadReportThinkBindings srbind{};srbind.preschedule=nbind.preschedule;srbind.reports.perception=nbind.perception;
    ServerNpcNativeSquadReportThinkResult srrun;
    install_at(96,2);bind_nav();f.put(f.actor,0x9d8,444u);f.put(f.actor,0xc18,711u);component_calls=0;
    f.put(f.actor,0x16c,37u);f.put(f.actor,0xcc8,9u);f.put(f.actor,0xccc,17);f.put(f.actor,0xcd0,69.491f);f.put(f.actor,0xcd4,2);
    assert(srthink->run(f.actor,sizeof(f.actor),69.491f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&srbind,&srrun)&&srrun.squad.cycle.preschedule.think.completed&&srrun.squad.cycle.speech.delivered&&!srrun.outer_reports.forwarded&&!srrun.squad.selections&&component_calls==1&&f.think_component_tasks==6);
    auto gsst=reinterpret_cast<GetServerNpcNativeSquadStateThinkApi>(GetProcAddress(s,server_npc_native_squad_state_think_export));
    assert(gsst&&!GetProcAddress(c,server_npc_native_squad_state_think_export)&&!gsst(0,sizeof(ServerNpcNativeSquadStateThinkApi))&&!gsst(1,0));
    const auto* sst=gsst(1,sizeof(ServerNpcNativeSquadStateThinkApi));assert(sst);
    ServerNpcNativeSquadStateThinkBindings ssbind{};ssbind.think=srbind;
    ServerNpcNativeSquadStateThinkResult ssrun;
    install_at(96,2);bind_nav();f.put(f.actor,0x9d8,444u);f.put(f.actor,0xc18,711u);component_calls=0;
    f.put(f.actor,0x16c,37u);f.put(f.actor,0xcc8,9u);f.put(f.actor,0xccc,17);f.put(f.actor,0xcd0,69.492f);f.put(f.actor,0xcd4,2);
    assert(sst->run(f.actor,sizeof(f.actor),69.492f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&ssbind,&ssrun)&&ssrun.think.squad.cycle.preschedule.think.completed&&ssrun.think.squad.cycle.speech.delivered&&!ssrun.choose_calls&&!ssrun.apply_calls&&f.think_component_tasks==7);
    // Force invalidation with component-owned gather, real ideal-state broadcast,
    // and a deliberately failed state query after the native broadcast.
    unsigned char squad_data[0x78]{},member[0xdd8]{};
    sin_independent::EntityHandleEntry reg[3]{};reg[1]=slots[1];reg[2].serial=8;reg[2].entity=reinterpret_cast<CBaseEntity*>(member);
    const auto old_entities=selection.state.entities;selection.state.entities.entries=reg;selection.state.entities.count=3;
    f.put(squad_data,8,0x8002u);f.put(squad_data,0x48,1);
    f.put(member,0x8a0,0xffffffffu);f.put(member,0x9cc,-1);
    struct StateFixture{F* f;unsigned char* squad;unsigned char* member;void* target;ServerNpcConditionMapBindings conditions;};
    StateFixture sf{&f,squad_data,member,weapon,sb.conditions};
    ssbind.think.reports.perception.slots.storage.context=&sf;
    ssbind.think.reports.perception.slots.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{auto& q=*static_cast<StateFixture*>(p);if(token!=91)return 0;*out=q.squad;*n=0x78;return 1;};
    ssbind.state.squad.context=&sf;
    ssbind.state.squad.member=[](void* p,void* a,std::uint32_t* vt,unsigned* n,ServerNpcEnemyStateBindings* e,ServerNpcEnemyMemoryQueryBindings*) noexcept -> int{
        auto& q=*static_cast<StateFixture*>(p);assert(a==q.member);*vt=0x2249aaa4;*n=0xdd8;e->context=p;e->conditions=q.conditions;
        e->entity_handle=[](void* p,void* target,std::uint32_t* out) noexcept -> int{assert(target==static_cast<StateFixture*>(p)->target);*out=0x7001u;return 1;};return 1;
    };
    auto state_query=selection.state.state_query;
    selection.state.state_query=[](void*,void*,int current,int*) noexcept -> int{assert(current==1);return 0;};
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);f.put(f.actor,0xc18,711u);f.put(f.actor,0xccc,-1);f.put(f.actor,0x9c4,91u);f.put(f.actor,0x8a0,0x7001u);f.put(f.actor,0x9cc,-1);
    f.put(f.actor,0x7d0,1u<<13); // condition 0x19 maps to bit 45 (global+20).
    component_calls=0;
    assert(!sst->run(f.actor,sizeof(f.actor),69.493f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&ssbind,&ssrun)&&ssrun.choose_calls==1&&ssrun.choose.broadcast_calls==1&&ssrun.choose.broadcast.assigned==1&&!ssrun.apply_calls&&component_calls==2);
    unsigned assigned=0;std::memcpy(&assigned,member+0x8a0,4);assert(assigned==0x7001u);
    selection.state.state_query=state_query;selection.state.entities=old_entities;
    auto gtaskthink=reinterpret_cast<GetServerNpcNativeSquadTaskThinkApi>(GetProcAddress(s,server_npc_native_squad_task_think_export));
    assert(gtaskthink&&!GetProcAddress(c,server_npc_native_squad_task_think_export)&&!gtaskthink(0,sizeof(ServerNpcNativeSquadTaskThinkApi))&&!gtaskthink(1,0));
    const auto* taskthink=gtaskthink(1,sizeof(ServerNpcNativeSquadTaskThinkApi));assert(taskthink);
    ServerNpcNativeSquadTaskThinkBindings taskbind{};taskbind.think=ssbind;
    taskbind.speech_tasks.context=&f;taskbind.speech_tasks.flank_metric=cb.path_distance;
    auto saved_component_task=ab.shared.component_task;
    ab.shared.component_task=[](void* p,std::uint32_t token,const ServerNpcScheduleTask* task,bool running) noexcept -> int{
        auto& f=F::self(p);assert(token==711&&task->id==84&&!running&&f.think_component[8]);++f.think_component_tasks;f.think_component[8]=0;return 1;
    };
    install_at(88,3);bind_nav();f.put(f.actor,0xc18,711u);f.put(f.actor,0xccc,-1);component_calls=0;
    auto no_legacy_cue=cb;no_legacy_cue.cue=nullptr;
    // MSVC x86 C1054: static zero-initialization avoids recursive aggregate initialization.
    static ServerNpcNativeSquadTaskThinkResult taskrun;
    assert(taskthink->run(f.actor,sizeof(f.actor),69.494f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&taskbind,&taskrun)&&taskrun.think.think.squad.cycle.preschedule.think.completed&&taskrun.speech_tasks.calls==1&&taskrun.speech_tasks.native==1&&taskrun.speech_tasks.last.mode==1&&taskrun.speech_tasks.last.event.metric==241&&!taskrun.speech_tasks.last.event.sentence.mode_allowed&&taskrun.speech_tasks.last.shared.lifecycle.completed&&component_calls==1&&f.think_component_tasks==8);
    // New outer-think route: retained descriptor performs the same native speech task.
    const auto grt=reinterpret_cast<GetServerNpcNativeRequestThinkApi>(GetProcAddress(s,server_npc_native_request_think_export));
    assert(grt&&!GetProcAddress(c,server_npc_native_request_think_export)&&!grt(0,sizeof(ServerNpcNativeRequestThinkApi))&&!grt(1,0));
    const auto* request_think=grt(1,sizeof(ServerNpcNativeRequestThinkApi));assert(request_think);
    ServerNpcGruntAssaultScheduleBindings req_schedule{};req_schedule.admission.actor_token=0x10000u;
    ServerNpcGruntAssaultActivationBindings req_activation{};req_activation.schedule=&req_schedule;
    ServerNpcGruntAssaultArbitrationBindings req_arbitration{};req_arbitration.activation=&req_activation;
    req_arbitration.shared_select=[](void*,void*,int* id) noexcept -> int{*id=26;return 1;};
    ServerNpcGruntScheduleAcquireBindings req_acquire{};req_acquire.arbitration=&req_arbitration;
    ServerNpcGruntScheduleRequestBindings req_binding{};req_binding.acquisition=&req_acquire;
    ServerNpcGruntSelectorTailBindings req_tail{};req_tail.request=&req_binding;
    ServerNpcGruntCombatSelectorBindings req_combat{};req_combat.tail=&req_tail;
    ServerNpcGruntSpecialCombatBindings req_selector{};req_selector.combat=&req_combat;
    ServerNpcGruntScheduleSceneServices req_scene{};
    ServerNpcGruntScheduleLineServices req_line{};req_line.scene=&req_scene;
    ServerNpcNativeRequestThinkBindings req_think{};req_think.think=taskbind;req_think.selector=&req_selector;req_think.line=&req_line;
    static ServerNpcNativeRequestThinkResult req_run;
    const auto cone_think_getter= reinterpret_cast<GetServerNpcNativeConeRequestThinkApi>(GetProcAddress(s,server_npc_native_cone_request_think_export));
    assert(cone_think_getter&&!GetProcAddress(c,server_npc_native_cone_request_think_export)&&!cone_think_getter(0,sizeof(ServerNpcNativeConeRequestThinkApi))&&!cone_think_getter(1,0));
    const auto* cone_think=cone_think_getter(1,sizeof(ServerNpcNativeConeRequestThinkApi));assert(cone_think);
    ServerNpcGruntScheduleConeServices req_cone{};req_cone.line=&req_line;
    ServerNpcNativeConeRequestThinkBindings cone_bind{};cone_bind.think=taskbind;cone_bind.selector=&req_selector;cone_bind.cone=&req_cone;
    install_at(88,3);bind_nav();f.put(f.actor,0xc18,711u);f.put(f.actor,0xccc,-1);component_calls=0;
    assert(request_think->run(f.actor,sizeof(f.actor),69.495f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&req_think,&req_run));
    assert(!req_run.requests.requests&&req_run.think.think.think.squad.cycle.preschedule.think.completed&&req_run.think.speech_tasks.calls==1&&component_calls==1);
    // Inactive state7 avoids unrelated perception; shared selection26 is remapped to the
    // real catalog descriptor94, whose initial task88 stores fail schedule95.
    auto old_trans=selection.translate;auto old_fail=selection.fail_select;auto old_special=selection.special_select;
    selection.translate=nullptr;selection.fail_select=nullptr;selection.special_select=nullptr;
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,7);f.put(f.actor,0x858,7);f.put(f.actor,0xccc,-1);
    assert(cone_think->run(f.actor,sizeof(f.actor),69.496f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&cone_bind,&req_run));
    assert(req_run.requests.requests==1&&!req_run.requests.failure_requests&&req_run.requests.request.requested==26&&req_run.requests.request.acquisition.translated==94&&req_run.requests.request.acquisition.schedule_token==find(94));
    assert(req_run.requests.scheduler.maintenance.state.schedule==find(94)&&req_run.requests.scheduler.maintenance.started==1&&req_run.think.think.think.squad.cycle.preschedule.think.completed);
    int fail_id=0;std::memcpy(&fail_id,f.actor+0x7bc,4);assert(fail_id==95);
    // The original toss-grenade-cover schedule ends in TASK_SET_SCHEDULE(25).
    // It must use the native policy, even with the legacy resolution callback disabled.
    ServerNpcGruntSchedulePolicyServices req_policy{};req_scene.policy=&req_policy;
    unsigned task_rolls=0;req_policy.context=&task_rolls;
    req_policy.random_int=[](void* p,int lo,int hi,int* v) noexcept -> int{
        assert(lo==0&&hi==1);++*static_cast<unsigned*>(p);*v=0;return 1;
    };
    install_at(97,2);f.put(f.actor,0x850,7);f.put(f.actor,0x858,7);f.put(f.actor,0xccc,-1);
    assert(cone_think->run(f.actor,sizeof(f.actor),69.4965f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&cone_bind,&req_run));
    const auto& switch_stats=req_run.think.think.think.squad.cycle.preschedule.think.scheduler.scheduler.sound_cover.observation.cue.movement.animation.facing.attack.shared.shared;
    assert(!req_run.requests.requests&&task_rolls==1&&switch_stats.last.task==87&&switch_stats.last.schedule_set&&switch_stats.last.resolution.requested==25&&switch_stats.last.resolution.translated==93);
    assert(req_run.requests.scheduler.maintenance.state.schedule==find(93)&&req_run.requests.scheduler.maintenance.state.index==0);
    int owner_ideal=0;std::memcpy(&owner_ideal,f.actor+0x79c,4);assert(owner_ideal==global+125);
    // Missing native service fails closed, without silently delegating to legacy translate.
    req_policy.random_int=nullptr;install_at(97,2);f.put(f.actor,0x850,7);f.put(f.actor,0x858,7);f.put(f.actor,0xccc,-1);
    assert(!cone_think->run(f.actor,sizeof(f.actor),69.4966f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&cone_bind,&req_run));
    assert(req_run.requests.scheduler.maintenance.state.schedule==find(97)&&task_rolls==1);
    selection.translate=old_trans;selection.fail_select=old_fail;selection.special_select=old_special;
    req_acquire.special_translate=[](void*,void*,int id,float,int* out) noexcept -> int{*out=id;return 1;};
    unsigned char before[sizeof(f.actor)];std::memcpy(before,f.actor,sizeof(before));
    assert(!request_think->run(f.actor,sizeof(f.actor),69.497f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&req_think,&req_run)&&!std::memcmp(before,f.actor,sizeof(before)));
    ab.shared.component_task=saved_component_task;
    tcb.submit_goal=cover_submit;
    ab.shared.component_storage=nullptr;ab.shared.component_task=nullptr;
    m.before_selection=old_before;
    ab.shared.pending=old_pending;ab.shared.context=nullptr;
    install_at(96,4); // SCHED_GRUNT_GRENADE_COVER1: original timestamp task53
    assert(shared_api->run(f.actor,sizeof(f.actor),69.5f,&f.image,&runtime,&full,&shared,&sh)&&sh.shared.calls==1&&sh.shared.last.task==53&&sh.shared.last.lifecycle.completed);
    float cover_stamp=0;std::memcpy(&cover_stamp,f.actor+0x8f0,4);assert(cover_stamp==69.5f);
    // Original range-attack schedule: task138 is the fourth entry (0.3 seconds).
    using DifficultyApi=sin_port::fixed_service::Api;
    using GetDifficultyApi=const DifficultyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
    auto gdiff=reinterpret_cast<GetDifficultyApi>(GetProcAddress(s,sin_port::fixed_service::export_name));
    assert(gdiff);const auto* diff=gdiff(1,sizeof(DifficultyApi));assert(diff&&diff->begin_campaign(1));
    shared.random=[](void*,int lo,int hi,int* out) noexcept -> int{assert(lo==1&&hi>=lo);*out=hi;return 1;};
    float easy_deadline=0;
    install_at(91,3);
    assert(shared_api->run(f.actor,sizeof(f.actor),70,&f.image,&runtime,&full,&shared,&sh)&&sh.shared.calls==2&&sh.shared.native==2&&sh.shared.last.task==138&&!sh.shared.last.lifecycle.completed);
    std::memcpy(&easy_deadline,f.actor+0x9dc,4);assert(easy_deadline>70);
    assert(shared_api->run(f.actor,sizeof(f.actor),easy_deadline-.01f,&f.image,&runtime,&full,&shared,&sh)&&sh.shared.calls==1&&!sh.shared.last.lifecycle.completed);
    assert(shared_api->run(f.actor,sizeof(f.actor),easy_deadline,&f.image,&runtime,&full,&shared,&sh)&&sh.shared.calls==1&&sh.shared.last.lifecycle.completed);
    assert(diff->set_skill(3));
    install_at(91,3);
    assert(shared_api->run(f.actor,sizeof(f.actor),70,&f.image,&runtime,&full,&shared,&sh)&&sh.shared.calls==2&&sh.shared.native==2&&!sh.shared.last.lifecycle.completed);
    float hard_deadline=0;std::memcpy(&hard_deadline,f.actor+0x9dc,4);assert(hard_deadline>70&&hard_deadline<easy_deadline);
    assert(shared_api->run(f.actor,sizeof(f.actor),hard_deadline,&f.image,&runtime,&full,&shared,&sh)&&sh.shared.last.lifecycle.completed);
    assert(diff->end_campaign());shared.random=nullptr;
    // Original Elite schedule: ready -> reconstructed base start -> inherited run.
    assert(f.api->build(f.actor,NpcSubclassElite,5000,99,&b,&f.image,&result));
    scheduler.original_vtable=0x2248df04;unsigned char chain[0x598]{};
    slots[1].entity=reinterpret_cast<CBaseEntity*>(chain);
    selection.state.entities.classify=[](void*,void*,unsigned* kind) noexcept -> int{*kind=WeaponActivityChaingun;return 1;};
    scheduler.elite.reload.original.weapon.extent=[](void*,void*,unsigned* n) noexcept -> int{*n=0x598;return 1;};
    full.injury.combat.weapon.random=[](void*,int lo,int hi,int* out) noexcept -> int{assert(lo==1&&hi==3);*out=2;return 1;};
    install_at(118,1); // Elite schedule's inherited face task; no double component pass.
    f.put(f.actor,0x908,88u);f.put(f.motor,0x14,0.f);
    assert(face_api->run(f.actor,sizeof(f.actor),55,&f.image,&runtime,&full,&ab,&fb,&facing_run)&&facing_run.facing.calls==2&&facing_run.facing.last.completed);
    install_at(118,3);f.put(chain,0x594,2);
    f.put(f.actor,0xfdc,0x7001u);f.put(f.actor,0x78c,0x7001u);f.put(f.actor,0x6cc,0x7001u);
    f.actor[0x3e0]=1;f.put(f.actor,0x3ec,7);f.put(f.actor,0x884,7);
    assert(attack_api->run(f.actor,sizeof(f.actor),53,&f.image,&runtime,&full,&ab,&ar)&&ar.attack.calls==1&&ar.attack.last.task==67&&ar.attack.last.lifecycle.completed&&ar.shared.full.combat.combat.calls==2&&f.actor[0xfd9]);
    // Elite +424 prefix uses the existing native spin update before base speech.
    install_at(118,1);f.put(f.actor,0xc18,711u);f.put(f.actor,0xfdc,0x7001u);
    f.put(chain,0x594,1);f.put(chain,0x590,79.f);component_calls=0;
    nt.preschedule.speech_queue_tick=[](void*,void*,float) noexcept -> int{return 0;}; // stop after native prefix, no unrelated task services
    assert(!npt->run(f.actor,sizeof(f.actor),80,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&nt,&ntr)&&ntr.elite_called&&ntr.elite.spin_changed&&ntr.preschedule.component_called&&ntr.preschedule.speech_called&&!ntr.think.maintenance_called);
    int spin=0;std::memcpy(&spin,chain+0x594,4);assert(spin==2);
    // Elite now uses the same original request/acquire cycle, not the older
    // class translation adapter. Start on the real catalog's first stop task.
    req_acquire.special_translate=nullptr;
    req_arbitration.shared_select=[](void*,void*,int* id) noexcept -> int{*id=25;return 1;};
    req_arbitration.shared_translate=nullptr;
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,7);f.put(f.actor,0x858,7);f.put(f.actor,0xccc,-1);
    f.put(f.actor,0xfdc,0x7001u);f.put(chain,0x594,1);f.put(chain,0x590,79.f);bind_nav();
    ab.shared.context=&f;ab.shared.pending=[](void* p,void*,int id) noexcept -> int{F::self(p).last_pending=id;return 1;};
    assert(request_think->run(f.actor,sizeof(f.actor),80,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&req_think,&req_run));
    const auto& elite_cycle=req_run.think.think.think.squad.cycle.preschedule;
    assert(elite_cycle.elite_called&&elite_cycle.elite.spin_changed&&elite_cycle.think.completed);
    assert(req_run.requests.requests==1&&req_run.requests.request.requested==25&&req_run.requests.request.acquisition.translated==118&&req_run.requests.scheduler.maintenance.state.schedule==find(118));
    assert(req_run.requests.scheduler.subclass==NpcSubclassElite&&req_run.requests.scheduler.maintenance.started==1);
    ab.shared.context=nullptr;ab.shared.pending=old_pending;

    // Jetpack prefix invokes attachment +8 only after true +10.
    assert(f.api->build(f.actor,NpcSubclassJetpack,6000,99,&b,&f.image,&result));
    scheduler.original_vtable=0x2249964c;std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0xc18,711u);f.put(f.actor,0x218,321u);
    bool ready=true;nt.jet_context=&ready;
    nt.jet_ready=[](void* p,std::uint32_t token,bool* out) noexcept -> int{assert(token==321);*out=*static_cast<bool*>(p);return 1;};
    nt.jet_tick=[](void*,std::uint32_t token) noexcept -> int{assert(token==321);return 1;};
    assert(!npt->run(f.actor,sizeof(f.actor),81,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&nt,&ntr)&&ntr.jet_queried&&ntr.jet_called&&ntr.preschedule.component_called&&ntr.preschedule.speech_called&&!ntr.think.maintenance_called);
    ready=false;
    assert(!npt->run(f.actor,sizeof(f.actor),82,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&cb,&ob,&scb,&tcb,&nt,&ntr)&&ntr.jet_queried&&!ntr.jet_called&&ntr.preschedule.speech_called);
    // Full Jet request-think: original attachment tick -> shared request -> Jet
    // remap41->122 -> real schedule initial TASK_SET_FAIL_SCHEDULE(119).
    const auto gjrt=reinterpret_cast<GetServerNpcNativeJetRequestThinkApi>(GetProcAddress(s,server_npc_native_jet_request_think_export));
    assert(gjrt&&!GetProcAddress(c,server_npc_native_jet_request_think_export)&&!gjrt(0,sizeof(ServerNpcNativeJetRequestThinkApi))&&!gjrt(1,0));
    const auto* jet_think=gjrt(1,sizeof(ServerNpcNativeJetRequestThinkApi));assert(jet_think);
    const float invalid_jet[3]{};
    ServerNpcGruntJetSelectorBindings jet_request{};jet_request.combat=&req_combat;jet_request.invalid_point=invalid_jet;
    ServerNpcNativeJetRequestThinkBindings jet_bind{};jet_bind.think=taskbind;jet_bind.selector=&jet_request;jet_bind.line=&req_line;
    auto& jp=jet_bind.think.think.think.preschedule;
    jp.jet_context=&ready;jp.jet_ready=nt.jet_ready;jp.jet_tick=nt.jet_tick;
    static ServerNpcNativeJetRequestThinkResult jet_run;
    const auto gjc=reinterpret_cast<GetServerNpcNativeJetConeRequestThinkApi>(GetProcAddress(s,server_npc_native_jet_cone_request_think_export));
    assert(gjc&&!GetProcAddress(c,server_npc_native_jet_cone_request_think_export)&&!gjc(0,sizeof(ServerNpcNativeJetConeRequestThinkApi))&&!gjc(1,0));
    const auto* jet_cone_think=gjc(1,sizeof(ServerNpcNativeJetConeRequestThinkApi));assert(jet_cone_think);
    ServerNpcNativeJetConeRequestThinkBindings jet_cone_bind{};jet_cone_bind.think=jet_bind.think;jet_cone_bind.selector=&jet_request;jet_cone_bind.cone=&req_cone;
    req_arbitration.shared_select=[](void*,void*,int* id) noexcept -> int{*id=41;return 1;};
    selection.translate=nullptr;selection.fail_select=nullptr;selection.special_select=nullptr;
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,7);f.put(f.actor,0x858,7);f.put(f.actor,0xccc,-1);f.put(f.actor,0x218,321u);ready=true;
    assert(jet_cone_think->run(f.actor,sizeof(f.actor),83,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&jet_cone_bind,&jet_run));
    const auto& jet_cycle=jet_run.think.think.think.squad.cycle.preschedule;
    assert(jet_cycle.jet_queried&&jet_cycle.jet_called&&jet_cycle.think.completed);
    assert(jet_run.requests.base.requests==1&&jet_run.requests.base.request.requested==41&&jet_run.requests.base.request.acquisition.translated==122&&jet_run.requests.base.scheduler.maintenance.state.schedule==find(122));
    int jet_fail=0;std::memcpy(&jet_fail,f.actor+0x7bc,4);assert(jet_fail==119);
    // Retained original chase schedule, Jet custom task160 writes movement mode.
    install_at(119,1);f.put(f.actor,0x850,7);f.put(f.actor,0x858,7);f.put(f.actor,0xccc,-1);f.put(f.actor,0x218,321u);ready=false;
    assert(jet_think->run(f.actor,sizeof(f.actor),84,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&jet_bind,&jet_run));
    assert(!jet_run.requests.base.requests&&jet_cycle.jet_queried&&!jet_cycle.jet_called&&jet_cycle.think.completed&&jet_run.requests.base.scheduler.jetpack.lifecycle.completed);
    int mode=0;std::memcpy(&mode,f.actor+0xfec,4);assert(mode==2);
    std::memcpy(before,f.actor,sizeof(before));
    assert(!jet_think->run(f.actor,0x1084,85,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&jet_bind,&jet_run)&&!std::memcmp(before,f.actor,sizeof(before)));
    assert(!request_think->run(f.actor,sizeof(f.actor),85,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&req_think,&req_run)&&!std::memcmp(before,f.actor,sizeof(before)));
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original schedule descriptor materialization and real task cursor");
}
