#include "native_server_npc_schedule_image.h"
#include "native_server_weapon_script_view.h"
#include "native_server_npc_reload_services.h"
#include "native_server_npc_active_weapon_step.h"
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
#include <cmath>
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

    auto greload_image=reinterpret_cast<GetServerNpcReloadScheduleImageApi>(GetProcAddress(s,server_npc_reload_schedule_image_export));
    assert(greload_image&&!GetProcAddress(c,server_npc_reload_schedule_image_export)&&!greload_image(0,sizeof(ServerNpcScheduleImageApi))&&!greload_image(1,0));
    const auto* reload_image=greload_image(1,sizeof(ServerNpcScheduleImageApi));assert(reload_image);
    {
        ServerScheduleImage ext{};ServerScheduleImageResult er{};
        for(unsigned family:{NpcSubclassGround,NpcSubclassElite,NpcSubclassJetpack}){
            assert(reload_image->build(f.actor,family,7000,99,&b,&ext,&er));
            assert(er.rows==(family==NpcSubclassGround?39u:family==NpcSubclassElite?41u:46u));
            assert(er.tasks==(family==NpcSubclassGround?230u:family==NpcSubclassElite?242u:268u));
            assert(ext.rows[0].descriptor[0]==static_cast<unsigned>(global+148)&&ext.rows[1].descriptor[0]==static_cast<unsigned>(global+149));
            assert(ext.rows[0].descriptor[12]==99&&ext.rows[1].descriptor[12]==7000&&ext.rows[2].descriptor[12]==7003);
            assert(reload_image->storage(&ext,7002,&p,&n)&&!std::strcmp(static_cast<const char*>(p),"SCHED_HIDE_AND_RELOAD"));
            assert(reload_image->storage(&ext,7005,&p,&n)&&!std::strcmp(static_cast<const char*>(p),"SCHED_RELOAD"));
            assert(reload_image->storage(&ext,7004,&p,&n)&&n==2*sizeof(ServerNpcScheduleTask));
            const auto* tasks=static_cast<const ServerNpcScheduleTask*>(p);
            assert(tasks[0].id==116&&tasks[1].id==71&&tasks[0].parameter==0&&tasks[1].parameter==0);
            assert(ext.rows[0].descriptor[3+(48+20)/32]==(1u<<((48+20)%32))&&ext.rows[1].descriptor[3+(48+20)/32]==(1u<<((48+20)%32)));
        }
        assert(!reload_image->build(f.actor,NpcSubclassJetpack,7000,7001,&b,&ext,&er)&&ext.row_count==46&&ext.task_count==268);
    }
    const auto get_scripts_image=reinterpret_cast<GetServerNpcScriptedScheduleImageApi>(GetProcAddress(s,server_npc_scripted_schedule_image_export));
    assert(get_scripts_image&&!GetProcAddress(c,server_npc_scripted_schedule_image_export)&&!get_scripts_image(0,sizeof(ServerNpcScheduleImageApi))&&!get_scripts_image(1,0));
    const auto* scripts_image=get_scripts_image(1,sizeof(ServerNpcScheduleImageApi));assert(scripts_image);
    {
        ServerScheduleImage ext{};ServerScheduleImageResult er{};
        const int expected_ids[]{53,55,56,57,58,59};
        const unsigned counts[]{2,10,10,10,5,6};
        const int task_ids[6][10]{{116,115},{89,107,9,121,111,112,110,106,108,109},{89,107,10,121,111,112,110,106,108,109},{89,107,11,121,111,112,110,106,108,109},{116,110,106,108,109},{116,112,110,106,108,109}};
        for(unsigned family:{NpcSubclassGround,NpcSubclassElite,NpcSubclassJetpack}){
            assert(scripts_image->build(f.actor,family,8000,99,&b,&ext,&er)&&er.rows==6&&er.tasks==43&&er.interrupts==12&&!er.private_activities&&ext.head==8015&&ext.family==family);
            for(unsigned i=0;i<6;++i){
                const unsigned token=8000+3*i;
                assert(scripts_image->storage(&ext,token,&p,&n)&&n==52);
                const auto* desc=static_cast<const unsigned*>(p);
                assert(desc[0]==static_cast<unsigned>(global+100+expected_ids[i])&&desc[2]==counts[i]&&desc[12]==(i?token-3:99));
                assert(desc[4]==((1u<<4)|(1u<<5)));
                assert(scripts_image->storage(&ext,token+1,&p,&n)&&n==counts[i]*sizeof(ServerNpcScheduleTask));
                const auto* tasks=static_cast<const ServerNpcScheduleTask*>(p);
                for(unsigned j=0;j<counts[i];++j)assert(tasks[j].id==task_ids[i][j]&&tasks[j].parameter==(tasks[j].id==89?2.f:0.f));
            }
        }
        assert(!scripts_image->build(f.actor,NpcSubclassGround,8000,8001,&b,&ext,&er)&&ext.head==8015);
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
    {
        const auto chain_bind=reinterpret_cast<BindServerNpcScriptedScheduleChain>(GetProcAddress(s,server_npc_scripted_schedule_chain_export));
        assert(chain_bind&&!GetProcAddress(c,server_npc_scripted_schedule_chain_export));
        ServerScheduleImage scripts{},derived{};ServerScheduleImageResult er{};
        assert(scripts_image->build(f.actor,NpcSubclassJetpack,8000,99,&b,&scripts,&er));
        assert(f.api->build(f.actor,NpcSubclassJetpack,9000,scripts.head,&b,&derived,&er));
        ServerNpcScriptedScheduleChain chain{};
        assert(chain_bind(f.actor,&scripts,&derived,&runtime,&chain)&&chain.bound.scheduler.maintenance.selection.head==8015&&selection.head==99&&!selection.grunt.schedule.storage);
        auto& chain_schedule=chain.bound.scheduler.maintenance.selection.grunt.schedule;
        // Descriptor, task array and name are separate tokens (base + 3*i + kind).
        assert(chain_schedule.storage(chain_schedule.context,8012,&p,&n)&&n==52);
        assert(chain_schedule.storage(chain_schedule.context,8013,&p,&n)&&n==5*sizeof(ServerNpcScheduleTask));
        assert(static_cast<const ServerNpcScheduleTask*>(p)[4].id==109);
        assert(chain_schedule.storage(chain_schedule.context,8014,&p,&n)&&!std::strcmp(static_cast<const char*>(p),"SCHED_SCRIPTED_WAIT"));
        assert(chain_schedule.storage(chain_schedule.context,8015,&p,&n)&&n==52);
        assert(chain_schedule.storage(chain_schedule.context,8016,&p,&n)&&n==6*sizeof(ServerNpcScheduleTask));
        assert(static_cast<const ServerNpcScheduleTask*>(p)[1].id==112);
        assert(chain_schedule.storage(chain_schedule.context,8017,&p,&n)&&!std::strcmp(static_cast<const char*>(p),"SCHED_SCRIPTED_FACE"));
        assert(chain_schedule.storage(chain_schedule.context,derived.head+2,&p,&n)&&!std::strcmp(static_cast<const char*>(p),derived.rows[derived.row_count-1].source->name));
        assert(!chain_schedule.storage(chain_schedule.context,8018,&p,&n));
        assert(state->install(f.actor,sizeof(f.actor),8012,9.f,&chain_schedule,&sr)&&sr.global==global+158);
        assert(state->advance(f.actor,sizeof(f.actor),&chain_schedule,&sr)&&state->inspect(f.actor,sizeof(f.actor),&chain_schedule,&sr)&&sr.task.id==110);
        scripts.tasks[0].id=0;
        assert(!chain_bind(f.actor,&scripts,&derived,&runtime,&chain)&&chain.bound.scheduler.maintenance.selection.head==8015);
        scripts.tasks[0].id=116;
        assert(chain_bind(f.actor,&scripts,&derived,&runtime,&chain));
        assert(!chain_bind(f.actor,&scripts,&derived,&chain.bound,&chain));
    }
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
    const auto combat_think_getter=reinterpret_cast<GetServerNpcNativeCombatLineThinkApi>(GetProcAddress(s,server_npc_native_combat_line_think_export));
    assert(combat_think_getter&&!GetProcAddress(c,server_npc_native_combat_line_think_export)&&!combat_think_getter(0,sizeof(ServerNpcNativeCombatLineThinkApi))&&!combat_think_getter(1,0));
    const auto* combat_think=combat_think_getter(1,sizeof(ServerNpcNativeCombatLineThinkApi));assert(combat_think);
    ServerNpcGruntCombatLineBindings think_line{};think_line.admission=&req_schedule.admission;think_line.line=&req_line;think_line.cone=&req_cone;
    ServerNpcNativeCombatLineThinkBindings combat_bind{};combat_bind.think=taskbind;combat_bind.selector=&req_selector;combat_bind.cone=&req_cone;combat_bind.combat_line=&think_line;
    ServerNpcGruntNativeLineAttackResult outer_line{},inner_line{};
    const auto weapon_think_getter=reinterpret_cast<GetServerNpcNativeWeaponPolicyThinkApi>(GetProcAddress(s,server_npc_native_weapon_policy_think_export));
    assert(weapon_think_getter&&!GetProcAddress(c,server_npc_native_weapon_policy_think_export)&&!weapon_think_getter(0,sizeof(ServerNpcNativeWeaponPolicyThinkApi))&&!weapon_think_getter(1,0));
    const auto* weapon_think=weapon_think_getter(1,sizeof(ServerNpcNativeWeaponPolicyThinkApi));assert(weapon_think);
    ServerWeaponScriptView weapon_scripts{};ServerNpcWeaponAmmoBindings weapon_ammo{};weapon_ammo.scripts=&weapon_scripts;
    ServerNpcWeaponMeleeBindings weapon_melee{};ServerNpcWeaponPolicyBindings weapon_policy{&weapon_ammo,&weapon_melee};
    ServerNpcWeaponPolicyResult outer_weapon{},inner_weapon{};
    install_at(88,3);bind_nav();f.put(f.actor,0xc18,711u);f.put(f.actor,0xccc,-1);component_calls=0;
    assert(request_think->run(f.actor,sizeof(f.actor),69.495f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&req_think,&req_run));
    assert(!req_run.requests.requests&&req_run.think.think.think.squad.cycle.preschedule.think.completed&&req_run.think.speech_tasks.calls==1&&component_calls==1);
    // Inactive state7 avoids unrelated perception; shared selection26 is remapped to the
    // real catalog descriptor94, whose initial task88 stores fail schedule95.
    auto old_trans=selection.translate;auto old_fail=selection.fail_select;auto old_special=selection.special_select;
    selection.translate=nullptr;selection.fail_select=nullptr;selection.special_select=nullptr;
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,7);f.put(f.actor,0x858,7);f.put(f.actor,0xccc,-1);
    const auto script_think=reinterpret_cast<RunServerNpcWeaponPolicyThinkWithScripts>(GetProcAddress(s,server_npc_weapon_policy_think_scripts_export));
    assert(script_think&&!GetProcAddress(c,server_npc_weapon_policy_think_scripts_export));
    namespace nm=sin_port::native_map;
    nm::ScriptedSequenceControlBindings script_control{};
    nm::ScriptedSequenceAnimationBindings script_animation{};script_animation.control=&script_control;
    nm::ScriptedSequencePossessionBindings script_possession{};
    nm::ScriptedSequenceFinishBindings script_finish{&script_animation,&script_possession};
    nm::ScriptedNpcTaskBindings script_run{&script_finish};
    nm::ScriptedNpcStartTaskBindings script_start{&script_run};
    ServerNpcScriptedSchedulerStats script_stats{};
    ServerNpcScriptedThinkServices script_services{&script_run,&script_start,nullptr,nullptr,&script_stats};
    auto invalid_script=script_services;invalid_script.start=nullptr;
    unsigned char before_script[sizeof(f.actor)];std::memcpy(before_script,f.actor,sizeof(f.actor));
    assert(!script_think(f.actor,sizeof(f.actor),69.496f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&combat_bind,&req_run,&outer_line,&inner_line,&weapon_policy,&outer_weapon,&inner_weapon,&invalid_script)&&!std::memcmp(before_script,f.actor,sizeof(f.actor)));
    assert(script_think(f.actor,sizeof(f.actor),69.496f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&combat_bind,&req_run,&outer_line,&inner_line,&weapon_policy,&outer_weapon,&inner_weapon,&script_services)&&!outer_line.attack.line_calls&&!inner_line.attack.line_calls&&!outer_weapon.primary.completed&&!inner_weapon.primary.completed);
    assert(!script_stats.starts&&!script_stats.runs);
    assert(req_run.requests.requests==1&&!req_run.requests.failure_requests&&req_run.requests.request.requested==26&&req_run.requests.request.acquisition.translated==94&&req_run.requests.request.acquisition.schedule_token==find(94));
    assert(req_run.requests.scheduler.maintenance.state.schedule==find(94)&&req_run.requests.scheduler.maintenance.started==1&&req_run.think.think.think.squad.cycle.preschedule.think.completed);
    int fail_id=0;std::memcpy(&fail_id,f.actor+0x7bc,4);assert(fail_id==95);

    // Active, equipped NPC with no enemy: native ammo must run at both +420
    // call sites. Deliberately stop after the second gather at ideal-state query.
    {
        struct EquippedThinkFixture {F* f;unsigned char weapon[0x520]{},enemy[0x900]{},memory[0x40]{},node[12]{},record[0x38]{},nav[0x50]{},path[0x6c]{},senses[0x60]{},empty_vectors[3][0x10]{},relationship[16]{};unsigned queries=0,traces=0,shots=0,sounds=0,flashes=0,prepares=0;void* scripted_entity=nullptr;void* scripted_other=nullptr;} equipped{&f};
        sin_independent::EntityHandleEntry equipped_entries[5]{};
        equipped_entries[0].serial=3;equipped_entries[0].entity=reinterpret_cast<CBaseEntity*>(f.actor);
        equipped_entries[1].serial=7;equipped_entries[1].entity=reinterpret_cast<CBaseEntity*>(equipped.weapon);
        const auto previous_entities=selection.state.entities;
        const auto previous_admission=req_schedule.admission;
        const auto previous_query=selection.state.state_query;
        const auto previous_context=selection.state.context;
        selection.state.entities.entries=equipped_entries;selection.state.entities.count=2;
        selection.state.context=&equipped;
        selection.state.state_query=[](void* p,void*,int current,int*) noexcept -> int{
            assert(current==1);++static_cast<EquippedThinkFixture*>(p)->queries;return 0;
        };
        auto extent=[](void* p,void* e,unsigned* n) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);
            if(e==q.f->actor)*n=sizeof(q.f->actor);else if(e==q.weapon)*n=sizeof(q.weapon);else if(e==q.enemy)*n=sizeof(q.enemy);else if(e&&(e==q.scripted_entity||e==q.scripted_other))*n=sin_port::native_map::scripted_sequence_extent;else return 0;return 1;
        };
        req_schedule.admission.context=&equipped;req_schedule.admission.extent=extent;
        auto active=combat_bind;
        auto& gather=active.think.think.think.preschedule.think.gather.gather;
        gather.base.gate.context=&f;gather.base.gate.storage.context=&f;
        gather.base.gate.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
            if(token!=88)return 0;auto& q=F::self(p);*out=q.motor;*n=sizeof(q.motor);return 1;
        };
        gather.base.gate.pvs_index=[](void*,unsigned,bool* yes) noexcept -> int{*yes=false;return 1;};
        gather.base.post.aux.visibility.context=&equipped;gather.base.post.aux.visibility.extent=extent;
        gather.base.post.aux.weapon_predicate=nullptr;
        unsigned char script[0x168]{};f.put(script,0x160,8);f.put(script,0x164,1);
        ServerWeaponScriptView scripts{};scripts.fallback=script;scripts.fallback_bytes=sizeof(script);
        ServerNpcWeaponAmmoBindings ammo{};ammo.scripts=&scripts;ammo.context=&equipped;
        // Original Grunt +fc returns this; no hosted owner cast needed.
        const ServerNpcWeaponPolicyBindings policy{&ammo,&weapon_melee};
        std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);
        f.put(f.actor,0,0x2249aaa4u);f.put(f.actor,0x908,88u);f.put(f.motor,0x14,0.f);f.put(f.actor,0xccc,-1);
        f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0x8a8,0xffffffffu);
        f.put(f.actor,0x78c,0x7001u);f.put(f.actor,0x650,3);
        f.put(equipped.weapon,0,0x2247875cu);f.put(equipped.weapon,0x494,0x3000u);
        f.put(equipped.weapon,0x4e4,1);f.put(equipped.weapon,0x4e8,2);
        f.put(equipped.weapon,0x51c,static_cast<std::uint16_t>(0xffff));
        assert(!weapon_think->run(f.actor,sizeof(f.actor),69.4961f,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&active,&req_run,&outer_line,&inner_line,&policy,&outer_weapon,&inner_weapon));
        const auto& cycle=req_run.think.think.think.squad.cycle.preschedule.think;
        assert(cycle.outer_gather.gather.completed&&cycle.outer_gather.gather.base.post.ammo.secondary_empty);
        assert(cycle.maintenance_called&&cycle.scheduler.gather_calls==1&&cycle.scheduler.gather.gather.completed&&equipped.queries==1);
        assert(outer_weapon.primary.reserve==3&&inner_weapon.primary.reserve==3&&outer_weapon.primary.available&&inner_weapon.primary.available);
        assert(outer_weapon.secondary.completed&&inner_weapon.secondary.completed&&!outer_weapon.secondary.available&&!inner_weapon.secondary.available);
        // Repeat the same full exported route with a visible living enemy.
        // A pre-existing enemy-memory record avoids requiring a hosted allocator.
        equipped_entries[2].serial=8;equipped_entries[2].entity=reinterpret_cast<CBaseEntity*>(equipped.enemy);
        selection.state.entities.count=3;
        auto storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);
            switch(token){
            case 601:*out=q.memory;*n=sizeof(q.memory);return 1;
            case 602:*out=q.node;*n=sizeof(q.node);return 1;
            case 603:*out=q.record;*n=sizeof(q.record);return 1;
            case 604:*out=q.nav;*n=sizeof(q.nav);return 1;
            case 605:*out=q.path;*n=sizeof(q.path);return 1;
            case 606:*out=q.senses;*n=sizeof(q.senses);return 1;
            case 607:case 608:case 609:*out=q.empty_vectors[token-607];*n=0x10;return 1;
            case 610:*out=q.relationship;*n=sizeof(q.relationship);return 1;
            default:return 0;
            }
        };
        auto key=[](void* p,void* e,std::uint32_t* out) noexcept -> int{
            if(e!=static_cast<EquippedThinkFixture*>(p)->enemy)return 0;*out=1000;return 1;
        };
        auto center=[](void* p,void* e,float* v) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);
            if(e!=q.enemy&&e!=q.f->actor)return 0;
            v[0]=e==q.enemy?100.f:0.f;v[1]=v[2]=0;return 1;
        };
        auto& post=gather.base.post;
        auto& vis=post.aux.visibility;
        vis.memory.context=&equipped;vis.memory.storage.context=&equipped;vis.memory.storage.storage=storage;vis.memory.entity_key=key;
        vis.trace=[](void* p,void* a,void* e,unsigned mask,bool* yes,void** blocker) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);assert(a==q.f->actor&&e==q.enemy&&mask==0x4081u);*yes=true;*blocker=nullptr;return 1;
        };
        vis.in_view=[](void*,void*,void*,bool* yes) noexcept -> int{*yes=true;return 1;};
        post.combat.range.visibility=vis;
        post.combat.range.context=&equipped;post.combat.range.center=center;post.combat.range.origin=center;
        post.combat.range.velocity=[](void*,void*,float* v) noexcept -> int{v[0]=v[1]=v[2]=0;return 1;};
        post.combat.range.enemy_facing_actor=[](void*,void*,void*,bool* yes) noexcept -> int{*yes=true;return 1;};
        auto& mutation=post.combat.range.report.mutation;
        mutation.context=&equipped;mutation.storage=vis.memory.storage;mutation.entity_key=key;
        post.aux.goals.memory=vis.memory;float refresh_deadline=1000.f;post.aux.goals.global_deadline=&refresh_deadline;
        auto& attack_binding=post.combat.attack;
        attack_binding.context=&equipped;attack_binding.origin=center;
        attack_binding.forward=[](void*,void*,float* v) noexcept -> int{v[0]=1;v[1]=v[2]=0;return 1;};
        attack_binding.aim_point=[](void*,void*,const float*,float* v) noexcept -> int{v[0]=100;v[1]=v[2]=0;return 1;};
        attack_binding.line=nullptr;attack_binding.weapon_caps=nullptr;attack_binding.weapon_condition=nullptr;
        auto engaged_line=req_line;engaged_line.context=&equipped;
        engaged_line.shoot_position=[](void*,void*,float* v) noexcept -> int{v[0]=v[1]=v[2]=0;return 1;};
        engaged_line.line_trace=[](void* p,void* a,const float*,const float* end,unsigned mask,int group,float* fraction,void** hit) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);assert(a==q.f->actor&&end[0]==100&&mask==0x46004003u&&group==6);++q.traces;*fraction=1;*hit=nullptr;return 1;
        };
        auto engaged_cone=req_cone;engaged_cone.line=&engaged_line;
        auto engaged_combat=think_line;engaged_combat.line=&engaged_line;engaged_combat.cone=&engaged_cone;
        active.cone=&engaged_cone;active.combat_line=&engaged_combat;
        auto prepare_engaged=[&](int clip,int reserve){
            std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,1);f.put(f.actor,0x858,1);
            f.put(f.actor,0,0x2249aaa4u);f.put(f.actor,0x908,88u);f.put(f.actor,0xccc,-1);f.put(f.actor,0x8a0,0x8002u);f.put(f.actor,0x8a8,0xffffffffu);
            f.put(f.actor,0x78c,0x7001u);f.put(f.actor,0x650,reserve);f.put(f.actor,0x988,0x20000u);
            f.put(f.actor,0x984,601u);f.put(f.actor,0x8f8,604u);f.put(f.actor,0xa20,300.f);
            f.put(equipped.weapon,0x4ec,clip);f.put(equipped.weapon,0x4f0,1);
            f.put(equipped.weapon,0x4f8,50.f);f.put(equipped.weapon,0x500,300.f);
            f.put(equipped.memory,0,0x22068d70u);f.put(equipped.memory,4,602u);f.put(equipped.memory,8,1u);
            f.put(equipped.memory,0x18,0x22068b40u);equipped.memory[0x10]=0;
            equipped.node[0]=equipped.node[1]=255;f.put(equipped.node,4,1000u);f.put(equipped.node,8,603u);
            f.put(equipped.record,0,0x8002u);f.put(equipped.record,4,100.f);f.put(equipped.record,0x1c,69.f);
            f.put(equipped.nav,0x24,605u);equipped.nav[0x3a]=1;
            equipped.queries=equipped.traces=0;
        };
        auto run_engaged=[&](float now){
            return weapon_think->run(f.actor,sizeof(f.actor),now,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&active,&req_run,&outer_line,&inner_line,&policy,&outer_weapon,&inner_weapon);
        };
        for(int mode=0;mode<3;++mode){
            prepare_engaged(mode==0?2:0,mode==1?3:0);
            const int expected=mode==2?4:20;
            assert(!run_engaged(69.4962f));
            assert(cycle.outer_gather.gather.completed&&cycle.scheduler.gather.gather.completed&&cycle.maintenance_called&&cycle.scheduler.gather_calls==1&&equipped.queries==1);
            assert(cycle.outer_gather.gather.base.post.combat.attack_called&&cycle.scheduler.gather.gather.base.post.combat.attack_called);
            assert(outer_line.attack.conditions[0]==expected&&inner_line.attack.conditions[0]==expected&&outer_line.attack.line_clear&&inner_line.attack.line_clear&&equipped.traces==2);
            assert(outer_weapon.primary.available==(mode!=2)&&inner_weapon.primary.available==(mode!=2));
            assert(cycle.outer_gather.gather.base.post.combat.range.report.mutation.updated&&cycle.scheduler.gather.gather.base.post.combat.range.report.mutation.updated);
            assert(outer_weapon.secondary.clip_positive&&inner_weapon.secondary.clip_positive);
        }
        // Keep a valid original SCHED_GRUNT_RANGE_ATTACK1 descriptor and
        // execute TASK_RANGE_ATTACK1 (67) across complete outer-think ticks.
        // RTTI at 2247ab9c identifies CWeaponSMG, not the Magnum used above.
        prepare_engaged(4,0);f.put(equipped.weapon,0,0x2247ab9cu);
        f.put(f.actor,0x6cc,0x7001u);f.actor[0x3e0]=1;
        f.put(f.actor,0x3ec,7);f.put(f.actor,0x884,7);
        const auto attack_schedule=find(91);
        assert(attack_schedule&&state->install(f.actor,sizeof(f.actor),attack_schedule,70.f,&sb,&sr));
        f.put(f.actor,0x79c,global+191);
        for(int i=0;i<4;++i)assert(state->advance(f.actor,sizeof(f.actor),&sb,&sr));
        assert(state->inspect(f.actor,sizeof(f.actor),&sb,&sr)&&sr.task.id==67);
        const float prior_stamp=stamp;stamp=0;
        auto verify_tick=[&](float now){
            assert(run_engaged(now)&&cycle.completed&&cycle.outer_gather.gather.completed&&cycle.preschedule_called&&cycle.maintenance_called);
            assert(!cycle.scheduler.gather_calls&&!req_run.requests.requests&&!equipped.queries);
            assert(cycle.outer_gather.gather.base.post.combat.attack_called&&outer_line.attack.conditions[0]==20&&outer_weapon.primary.clip_positive);
            assert(!inner_weapon.primary.completed&&!inner_line.attack.line_calls);
            assert(req_run.requests.scheduler.maintenance.state.schedule==attack_schedule);
        };
        verify_tick(70.f);
        assert(stamp==70.f&&req_run.requests.scheduler.maintenance.state.index==4);
        int burst_count=0;std::memcpy(&burst_count,f.actor+0xc98,4);assert(burst_count==4);
        verify_tick(70.5f);assert(stamp==70.f);
        verify_tick(71.f);assert(stamp==0.f);
        verify_tick(72.f);assert(req_run.requests.scheduler.maintenance.state.index==5&&equipped.traces==4);
        // The animation dispatcher already supplies these original events.
        // Replay them here between actual scheduler ticks on the same actor/weapon.
        auto get_parent=reinterpret_cast<GetServerNpcParentWeaponEventApi>(GetProcAddress(s,server_npc_parent_weapon_event_export));
        auto get_reload=reinterpret_cast<GetServerNpcReloadServicesApi>(GetProcAddress(s,server_npc_reload_services_export));
        auto get_cond=reinterpret_cast<GetServerNpcConditionApi>(GetProcAddress(s,server_npc_condition_export));
        assert(get_parent&&get_reload&&get_cond);
        const auto* parent=get_parent(1,sizeof(ServerNpcParentWeaponEventApi));
        const auto* reload=get_reload(1,sizeof(ServerNpcReloadServicesApi));
        const auto* conditions=get_cond(1,sizeof(ServerNpcConditionApi));
        assert(parent&&reload&&conditions);
        ServerNpcReloadServicesBindings event_bind{};
        event_bind.scripts=scripts;event_bind.conditions=sb.conditions;
        auto& event_weapon=event_bind.original.weapon;event_weapon.context=&equipped;
        event_weapon.extent=extent;event_weapon.weapons=selection.state.entities;
        event_weapon.sound.context=&equipped;
        event_weapon.sound.emit=[](void* p,void* w,int id,float) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);assert(w==q.weapon&&(id==2||id==9));++q.sounds;return 1;
        };
        event_weapon.prepare=[](void* p,void* w,void* a,unsigned kind,const ServerAnimationEvent* e) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);assert(w==q.weapon&&a==q.f->actor&&kind==WeaponActivitySmg&&e->event==3002);++q.prepares;return 1;
        };
        event_weapon.primary_effect=[](void* p,void* w,void* a,unsigned kind,const ServerAnimationEvent*) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);assert(w==q.weapon&&a==q.f->actor&&kind==WeaponActivitySmg&&q.prepares==q.shots+1&&q.sounds==q.shots+1);++q.shots;return 1;
        };
        event_weapon.flash=[](void* p,void* a) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);assert(a==q.f->actor&&q.shots==q.flashes+1);++q.flashes;return 1;
        };
        prepare_engaged(4,0);f.put(equipped.weapon,0,0x2247ab9cu);
        f.put(f.actor,0x6cc,0x7001u);f.actor[0x3e0]=1;f.put(f.actor,0x3ec,7);f.put(f.actor,0x884,7);
        assert(state->install(f.actor,sizeof(f.actor),attack_schedule,73.f,&sb,&sr));
        f.put(f.actor,0x79c,global+191);
        for(int i=0;i<4;++i)assert(state->advance(f.actor,sizeof(f.actor),&sb,&sr));
        stamp=0;equipped.weapon[0x1f0]=1;
        ServerNpcReloadSession event_session{};
        assert(reload->prepare(f.actor,sizeof(f.actor),&event_bind,&event_session));
        for(int i=0;i<4;++i){
            const float now=73.f+i*.1f;
            verify_tick(now);
            ServerAnimationEvent shot{3002,"",0.f,now,0,equipped.weapon};
            ServerNpcParentWeaponEventResult er{};
            assert(parent->handle(f.actor,sizeof(f.actor),&shot,now,&event_session.bound,&er)==1&&er.forwarded&&er.weapon.primary_called&&er.weapon.clip_changed&&er.weapon.flash_called);
            int remaining=-1;std::memcpy(&remaining,equipped.weapon+0x4ec,4);
            assert(remaining==3-i&&equipped.shots==static_cast<unsigned>(i+1)&&equipped.weapon[0x200]);
        }
        assert(equipped.prepares==4&&equipped.sounds==4&&equipped.flashes==4);
        // No clip or owner reserve remains: next real gather publishes no-ammo.
        // Stop at the deliberately unavailable state query during reselection.
        assert(!run_engaged(73.5f)&&cycle.outer_gather.gather.completed&&outer_line.attack.conditions[0]==4&&!outer_weapon.primary.available);
        ServerNpcConditionResult condition{};
        assert(conditions->test(f.actor,sizeof(f.actor),4,NpcConditionCurrent,&sb.conditions,&condition)&&condition.value);
        ServerNpcReloadResult reset{};
        assert(reload->fixed_reset(&event_session,&reset)&&reset.live&&reset.primary==8&&reset.secondary==1&&reset.primary_changed&&reset.conditions_cleared==3);
        assert(conditions->test(f.actor,sizeof(f.actor),4,NpcConditionCurrent,&sb.conditions,&condition)&&!condition.value);
        assert(equipped.sounds==5&&event_session.script_lookups==2);
        // Reinstall the attack descriptor (selection itself is tested separately).
        assert(state->install(f.actor,sizeof(f.actor),attack_schedule,74.f,&sb,&sr));
        f.put(f.actor,0x79c,global+191);f.actor[0x3e0]=1;
        for(int i=0;i<4;++i)assert(state->advance(f.actor,sizeof(f.actor),&sb,&sr));
        stamp=0;equipped.queries=0;verify_tick(74.f);
        assert(outer_weapon.primary.clip==8);
        // Real animation traversal in the same exported active update.
        // Existing schedule dictionary private IDs remain stable; register the
        // remaining Grunt activity names and both event names for this fixture.
        auto get_step=reinterpret_cast<GetServerNpcActiveWeaponStepApi>(GetProcAddress(s,server_npc_active_weapon_step_export));
        auto get_reg=reinterpret_cast<GetServerGruntRegistrationApi>(GetProcAddress(s,server_grunt_registration_export));
        auto get_events=reinterpret_cast<GetServerEventDictionaryApi>(GetProcAddress(s,server_event_dictionary_export));
        assert(get_step&&get_reg&&get_events);
        const auto* step=get_step(1,sizeof(ServerNpcActiveWeaponStepApi));
        const auto* reg=get_reg(1,sizeof(ServerGruntRegistrationApi));
        const auto* ed=get_events(1,sizeof(ServerEventDictionaryApi));assert(step&&reg&&ed);
        ServerEventDictionaryEntry event_names[32]{};
        ServerEventDictionary event_dictionary{event_names,32,0,0,0};
        const auto prior_ids=ids;
        assert(ed->seed(&event_dictionary)&&reg->grunt(&activities,&event_dictionary,nullptr,&ids));
        assert(ids.activities[2]==prior_ids.activities[2]&&ids.activities[3]==prior_ids.activities[3]&&ids.activities[5]==prior_ids.activities[5]);
        struct FrameMeta {
            EquippedThinkFixture* q;
            ServerStudioEvent reload{.04f,2,0,"reload",0},shot{.04f,3002,0,"fire",0};
            bool reloading=false,suppress=false,scripted=false;
            ServerStudioEvent script_events[2]{{.02f,1001,0,"",0},{.04f,1002,0,"",0}};
        } meta{&equipped};
        ServerNpcScriptWeaponEventFrameBindings frame{};
        frame.frame.events=event_bind.original;
        // The owner frame gets its registry, script table and condition map
        // from the scheduler-side binding, not from duplicated test data.
        auto& gf=frame.frame.frame;gf.ids=&ids;
        auto& rt=gf.runtime;rt.npc.context=&equipped;
        auto& as=rt.npc.transition.assignment;
        as.translation.actor_class=NpcActivityGrunt;as.translation.activities=&activities;as.translation.events=&event_dictionary;
        as.translation.metadata=[](void*,void*,const ServerActivityCompositeView** view) noexcept -> int{*view=nullptr;return 0;};
        as.dictionary.dictionary=&event_dictionary;
        rt.npc.frame.activities=&activities;rt.npc.frame.frame.dictionary=as.dictionary;
        auto& nf=rt.npc.frame.frame.sequence.frame;nf.context=&meta;
        nf.model_available=[](void*,void*) noexcept -> int{return 1;};
        auto& adv=nf.advance;adv.base.context=&meta;
        adv.base.duration=[](void* p,void* a,int,double* d) noexcept -> int{auto& q=*static_cast<FrameMeta*>(p)->q;*d=a==q.f->actor?2.:4.;return 1;};
        adv.base.movement_distance=[](void*,void*,int,double* d) noexcept -> int{*d=0;return 1;};
        adv.base.finish_threshold=[](void*,void*,int,double* d) noexcept -> int{*d=.9;return 1;};
        adv.base.invalidate=[](void*,void*,unsigned) noexcept -> int{return 1;};
        adv.base.cache=[](void*,void*,std::uint32_t,float) noexcept -> int{return 1;};
        adv.layers.vectors.storage=[](void*,std::uint32_t,void**,unsigned*) noexcept -> int{return 0;};
        adv.layers.vectors.resize=[](void*,std::uint32_t,unsigned,void**,unsigned*) noexcept -> int{return 0;};
        adv.base.vectors=adv.layers.vectors;
        adv.layers.cycle_rate=[](void*,void*,int,double* d) noexcept -> int{*d=1.;return 1;};
        nf.events.context=&meta;nf.events.storage=adv.layers.vectors.storage;
        nf.events.sequence=[](void* p,void* a,int,ServerAnimationEventSequence* seq) noexcept -> int{
            auto& q=*static_cast<FrameMeta*>(p);
            const bool npc=a==q.q->f->actor;assert(npc||a==q.q->weapon);
            if(!q.suppress&&npc&&q.scripted)*seq={q.script_events,2,1,0};
            else if(!q.suppress&&npc==q.reloading)*seq={npc?&q.reload:&q.shot,1,1,0};
            else *seq={nullptr,0,1,0};
            return 1;
        };
        nf.events.cycle_rate=[](void* p,void* a,int,double* d) noexcept -> int{auto& q=*static_cast<FrameMeta*>(p)->q;*d=a==q.f->actor?.5:.25;return 1;};
        rt.npc.storage=[](void* p,std::uint32_t token,const void** out,unsigned* n) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);
            if(token==604){*out=q.nav;*n=sizeof(q.nav);return 1;}
            if(token==605){*out=q.path;*n=sizeof(q.path);return 1;}return 0;
        };
        auto& wf=rt.weapon;wf.context=&equipped;wf.extent=extent;
        wf.animation.activities=&activities;wf.animation.events=&event_dictionary;wf.animation.dictionary=as.dictionary;
        wf.animation.metadata=as.translation.metadata;
        // Native primary-vtable owner cast and player predicate.
        wf.frame.cycle=adv.base;wf.events=nf.events;
        ServerNpcActiveWeaponStepBindings step_bind{};
        step_bind.image=&f.image;step_bind.schedule=&runtime;step_bind.full=&full;step_bind.attack=&ab;step_bind.facing=&fb;
        step_bind.target=&tb;step_bind.movement=&mb;step_bind.cue=&no_legacy_cue;step_bind.observe=&ob;
        step_bind.sound_cover=&scb;step_bind.tactical=&tcb;step_bind.think=&active;step_bind.policy=&policy;step_bind.frame=&frame;
        static ServerNpcActiveWeaponStepResult step_result;
        auto seed_anim=[&](float now){
            f.put(f.actor,0x878,16);f.put(f.actor,0x880,16);f.put(f.actor,0x884,7);f.put(f.actor,0x3ec,7);
            f.put(equipped.weapon,0x3ec,1);f.put(equipped.weapon,0x4a0,0xffffffffu);
            for(void* entity:{static_cast<void*>(f.actor),static_cast<void*>(equipped.weapon)}){
                f.put(entity,0x28,now-.25f);f.put(entity,0x2c,now-.125f);
                f.put(entity,0x3e8,0.f);f.put(entity,0x398,0.f);f.put(entity,0x3e1,std::uint8_t{1});
                f.put(entity,0x3c0,entity==f.actor?1.f:2.f);f.put(entity,0x3e0,static_cast<std::uint8_t>(entity==f.actor));
            }
        };
        prepare_engaged(2,0);f.put(equipped.weapon,0,0x2247ab9cu);f.put(f.actor,0x6cc,0x7001u);
        assert(state->install(f.actor,sizeof(f.actor),attack_schedule,75.f,&sb,&sr));
        f.put(f.actor,0x79c,global+191);
        for(int i=0;i<4;++i)assert(state->advance(f.actor,sizeof(f.actor),&sb,&sr));
        stamp=0;equipped.shots=equipped.sounds=equipped.prepares=equipped.flashes=0;
        for(int i=0;i<2;++i){
            const float now=75.f+i*.125f;
            seed_anim(now);f.put(f.actor,0x864,now);
            const ServerNpcRunAnimationInput input{now,false,false};
            assert(step->run(f.actor,sizeof(f.actor),&input,&step_bind,&step_result)&&step_result.completed&&step_result.think_completed&&step_result.animation_called);
            assert(step_result.outer_weapon.primary.clip==2-i&&step_result.outer_line.attack.conditions[0]==20);
            assert(step_result.animation.frame.frame.weapon_events==1&&step_result.animation.frame.frame.handled==1&&step_result.animation.last.event.parent.weapon.clip_changed);
            int left=-1;std::memcpy(&left,equipped.weapon+0x4ec,4);assert(left==1-i);
        }
        assert(equipped.shots==2&&equipped.prepares==2&&equipped.sounds==2&&equipped.flashes==2);
        // A due update sees the empty clip before any new animation event.
        seed_anim(75.25f);f.put(f.actor,0x864,75.25f);
        const ServerNpcRunAnimationInput depleted{75.25f,false,false};
        assert(!step->run(f.actor,sizeof(f.actor),&depleted,&step_bind,&step_result)&&step_result.think_called&&!step_result.animation_called);
        assert(step_result.outer_line.attack.conditions[0]==4&&!step_result.outer_weapon.primary.available&&equipped.shots==2);
        // Future deadline permits native event2 even though reselection is not
        // presently available. Animation route itself restores clip/conditions.
        meta.reloading=true;seed_anim(75.375f);f.put(f.actor,0x864,80.f);f.actor[0x7b8]=f.actor[0x7b9]=0;
        const ServerNpcRunAnimationInput reloaded{75.375f,false,false};
        assert(step->run(f.actor,sizeof(f.actor),&reloaded,&step_bind,&step_result)&&!step_result.think_called&&step_result.animation.fixed_resets==1&&step_result.animation.frame.frame.npc_events==1&&step_result.completed);
        int loaded=-1;std::memcpy(&loaded,equipped.weapon+0x4ec,4);assert(loaded==8&&equipped.sounds==3);
        assert(conditions->test(f.actor,sizeof(f.actor),4,NpcConditionCurrent,&sb.conditions,&condition)&&!condition.value);
        // Use the composed outer entry, and let its real cadence control
        // subsequent scheduler eligibility. Only the first deadline is seeded.
        auto get_outer=reinterpret_cast<GetServerNpcOuterUpdateApi>(GetProcAddress(s,server_npc_outer_update_export));
        assert(get_outer&&!GetProcAddress(c,server_npc_outer_update_export));
        const auto* outer_api=get_outer(1,sizeof(ServerNpcOuterUpdateApi));assert(outer_api);
        const auto get_outer_scripts=reinterpret_cast<GetServerNpcOuterScriptedUpdateApi>(GetProcAddress(s,server_npc_outer_scripts_export));
        assert(get_outer_scripts&&!GetProcAddress(c,server_npc_outer_scripts_export)&&!get_outer_scripts(0,sizeof(ServerNpcOuterScriptedUpdateApi))&&!get_outer_scripts(1,0));
        const auto* outer_scripts=get_outer_scripts(1,sizeof(ServerNpcOuterScriptedUpdateApi));assert(outer_scripts);
        ServerNpcActivePostAnimationBindings post_bind{};post_bind.context=&equipped;post_bind.resolve=storage;
        post_bind.navigation_update=[](void*,void*,std::uint32_t token,float dt) noexcept -> int{
            assert(token==604u&&std::isfinite(dt));return 1;
        };
        post_bind.prepare_pose=[](void*,void*) noexcept -> int{return 1;};
        post_bind.finish_pose=post_bind.prepare_pose;
        post_bind.motor_finish=[](void*,void*,std::uint32_t token) noexcept -> int{assert(token==88u);return 1;};
        ServerNpcOuterUpdateBindings outer_bind{};outer_bind.post=&post_bind;outer_bind.context=&equipped;outer_bind.active_scene=true;
        outer_bind.admission.enabled=true;
        outer_bind.cadence.notify_think_flags=[](void*,void*) noexcept -> int{return 1;};
        outer_bind.pvs=[](void*,std::uint32_t,bool* visible) noexcept -> int{*visible=false;return 1;};
        ServerNpcOuterUpdateResult outer_result{};ServerNpcActivePostAnimationResult tail_result{};
        prepare_engaged(3,0);f.put(equipped.weapon,0,0x2247ab9cu);f.put(f.actor,0x6cc,0x7001u);
        assert(state->install(f.actor,sizeof(f.actor),attack_schedule,75.5f,&sb,&sr));
        f.put(f.actor,0x79c,global+191);
        for(int i=0;i<4;++i)assert(state->advance(f.actor,sizeof(f.actor),&sb,&sr));
        stamp=0;meta.reloading=false;equipped.shots=equipped.sounds=equipped.prepares=equipped.flashes=0;
        f.put(f.actor,0x864,75.5f);f.put(f.actor,0x910,0x7f7fffffu);f.put(f.actor,0x74,1510);
        for(int i=0;i<3;++i){
            // Middle frame is strictly before the first AI deadline; final
            // frame is exactly at the deadline written by the outer update.
            const float now=i==0?75.5f:i==1?75.55f:75.6f;
            seed_anim(now);
            const ServerNpcRunAnimationInput input{now,false,false};
            script_stats.starts=99;script_stats.runs=99;
            assert(outer_scripts->ground(f.actor,sizeof(f.actor),&input,.05f,&step_bind,&outer_bind,&step_result,&tail_result,&outer_result,&script_services)&&outer_result.completed&&outer_result.active_called&&tail_result.completed);
            assert(!script_stats.starts&&!script_stats.runs);
            assert(bool(outer_result.think_called)==(i!=1)&&bool(step_result.think_completed)==(i!=1));
            assert(step_result.animation.frame.frame.weapon_events==1&&step_result.animation.last.event.parent.weapon.clip_changed);
            assert(outer_result.finish.deadline==(i==2?static_cast<float>(double(now)+double(.1f)):75.6f)&&outer_result.finish.tick==1512+i);
            int left=-1;std::memcpy(&left,equipped.weapon+0x4ec,4);assert(left==2-i);
            if(i==1)assert(!step_result.outer_weapon.primary.completed);
            else assert(step_result.outer_weapon.primary.clip==3-i&&step_result.outer_line.attack.conditions[0]==20);
        }
        assert(equipped.shots==3&&equipped.sounds==3&&equipped.prepares==3&&equipped.flashes==3);
        // Loaded full NPC + weapon animation tick: script studio events
        // coexist with the existing weapon event without duplicate emission.
        {
            const auto scripted_frame=reinterpret_cast<RunServerNpcWeaponFrameWithScriptedEvents>(GetProcAddress(s,server_npc_scripted_weapon_frame_export));
            assert(scripted_frame&&!GetProcAddress(c,server_npc_scripted_weapon_frame_export));
            struct SequenceFixture {
                F* f;alignas(4) unsigned char seq[nm::scripted_sequence_extent]{},other[nm::scripted_sequence_extent]{},partner[0x1088]{},begin_action[0x1c]{},end_action[0x1c]{};
                const ServerNpcScheduleStateApi* state=nullptr;
                const ServerNpcScheduleBindings* schedule=nullptr;
                unsigned restored=0,cleared=0,reads=0,lookups=0,assignments=0,begins=0,ends=0,aims=0,motion_ticks=0;
            } q{&f};
            f.put(q.seq,0,nm::scripted_sequence_vtable);f.put(q.seq,0x378,0x9002u);
            const auto prior_slot=equipped_entries[2];const auto prior_count=selection.state.entities.count;
            equipped_entries[2].serial=9;equipped_entries[2].entity=reinterpret_cast<CBaseEntity*>(q.seq);
            selection.state.entities.count=3;
            nm::ScriptedSequenceControlBindings control{};control.context=&q;
            control.extent=[](void* p,void* entity,unsigned* n) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);*n=entity==q.seq||entity==q.other?sizeof(q.seq):entity==q.partner?sizeof(q.partner):entity==q.f->actor?sizeof(q.f->actor):0;return *n!=0;
            };
            nm::ScriptedSequenceInterruptBindings interrupt{&control,nullptr};
            nm::ScriptedNpcAnimationEventBindings events{&interrupt,nullptr};
            ServerNpcScriptedWeaponFrameResult seq_out{};
            auto bound=frame;bound.frame.frame.runtime.inherited.weapons=selection.state.entities;
            bound.scripts=*policy.ammo->scripts;bound.conditions=sb.conditions;
            prepare_engaged(1,0);f.put(equipped.weapon,0,0x2247ab9cu);f.put(f.actor,0x6cc,0x7001u);
            f.put(f.actor,0,0x224954a4u);f.put(f.actor,0x970,0x9002u);f.put(q.seq,0x3d0,0x3000u);
            stamp=0;equipped.shots=equipped.sounds=equipped.prepares=equipped.flashes=0;
            meta.reloading=false;meta.scripted=true;seed_anim(76.f);
            const ServerNpcRunAnimationInput input{76.f,false,false};
            ServerNpcScriptWeaponEventFrameResult full_frame{};
            assert(scripted_frame(f.actor,sizeof(f.actor),&input,&bound,&full_frame,&events,&seq_out)&&seq_out.handled==2&&!seq_out.failed&&seq_out.last.event==1002&&q.seq[0x3cc]);
            assert(full_frame.frame.frame.npc_events==2&&full_frame.frame.frame.weapon_events==1&&full_frame.frame.frame.handled==3&&equipped.shots==1&&!control.entries&&!control.now);
            // The same native event route must also work inside the outer
            // update before its next AI deadline, without running a task.
            const auto get_combined=reinterpret_cast<GetServerNpcOuterScriptedEventsUpdateApi>(GetProcAddress(s,server_npc_outer_scripts_events_export));
            assert(get_combined&&!GetProcAddress(c,server_npc_outer_scripts_events_export)&&!get_combined(0,sizeof(ServerNpcOuterScriptedEventsUpdateApi))&&!get_combined(1,0));
            const auto* combined=get_combined(1,sizeof(ServerNpcOuterScriptedEventsUpdateApi));assert(combined);
            auto anim=script_animation;anim.control=&control;
            auto finish=script_finish;finish.animation=&anim;
            auto task=script_run;task.finish=&finish;
            auto start=script_start;start.task=&task;
            auto services=script_services;services.run=&task;services.start=&start;
            prepare_engaged(1,0);f.put(equipped.weapon,0,0x2247ab9cu);f.put(f.actor,0x6cc,0x7001u);
            f.put(f.actor,0x970,0x9002u);f.put(f.actor,0x864,80.f);f.put(f.actor,0x910,0x7f7fffffu);f.put(f.actor,0x74,1520);
            stamp=0;equipped.shots=equipped.sounds=equipped.prepares=equipped.flashes=0;
            seed_anim(76.125f);q.seq[0x3cc]=0;
            const ServerNpcRunAnimationInput next{76.125f,false,false};
            assert(combined->ground(f.actor,sizeof(f.actor),&next,.05f,&step_bind,&outer_bind,&step_result,&tail_result,&outer_result,&services,&events,&seq_out)&&outer_result.completed&&outer_result.active_called&&!outer_result.think_called&&tail_result.completed);
            assert(seq_out.handled==2&&!seq_out.failed&&q.seq[0x3cc]&&equipped.shots==1&&!script_stats.runs&&!script_stats.starts);
            // Execute the authenticated inherited WAIT descriptor, not a
            // fabricated one-task row. Start at its actual post-idle cursor.
            const auto bind_chain=reinterpret_cast<BindServerNpcScriptedScheduleChain>(GetProcAddress(s,server_npc_scripted_schedule_chain_export));
            assert(bind_chain);
            ServerScheduleImage inherited{},derived{};ServerScheduleImageResult ir{};
            assert(scripts_image->build(f.actor,NpcSubclassGround,8000,selection.head,&b,&inherited,&ir));
            assert(f.api->build(f.actor,NpcSubclassGround,9000,inherited.head,&b,&derived,&ir));
            ServerNpcScriptedScheduleChain chain{};
            assert(bind_chain(f.actor,&inherited,&derived,&runtime,&chain));
            q.state=state;q.schedule=&chain.bound.scheduler.maintenance.selection.grunt.schedule;
            equipped.scripted_entity=q.seq;
            auto scripted_step=step_bind;scripted_step.image=&derived;scripted_step.schedule=&chain.bound;
            auto native_pos=script_possession;native_pos.context=&q;native_pos.extent=control.extent;
            finish.possession=&native_pos;finish.context=&q;
            finish.current_schedule=[](void* p,void* actor,bool* present,int* type) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(actor==q.f->actor);
                ServerNpcScheduleStateResult s{};if(!q.state->inspect(actor,sizeof(q.f->actor),q.schedule,&s))return 0;
                *present=s.has_task!=0;*type=s.task.id;++q.reads;return 1;
            };
            finish.clear_schedule=[](void* p,void* actor) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(actor==q.f->actor&&q.restored==1);
                ++q.cleared;return q.state->clear(actor,sizeof(q.f->actor));
            };
            nm::ScriptedNpcCleanupBindings cleanup{&control,nullptr,&q};
            cleanup.effect_changed=[](void* p,void* actor) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(actor==q.f->actor);++q.restored;return 1;
            };
            nm::ScriptedNpcCleanupResult cleaned{};
            services.cleanup=&cleanup;services.cleaned=&cleaned;
            prepare_engaged(1,0);f.put(equipped.weapon,0,0x2247ab9cu);f.put(f.actor,0x6cc,0x7001u);
            f.put(f.actor,0x850,4);f.put(f.actor,0x858,4);f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0x8a8,0xffffffffu);
            f.put(f.actor,0x970,0x9002u);f.put(f.actor,0x378,0x3000u);f.put(f.actor,0x9c,100);f.put(f.actor,0x78,1u);
            f.put(q.seq,0xb4,4u);f.put(q.seq,0x3d0,0x3000u);f.put(q.seq,0x3d4,0xffffffffu);f.put(q.seq,0x3c4,2u);
            f.put(f.actor,0x864,77.f);f.put(f.actor,0x910,0x7f7fffffu);f.put(f.actor,0x74,1540);
            assert(state->install(f.actor,sizeof(f.actor),8012,77.f,q.schedule,&sr)&&sr.global==global+158);
            f.put(f.actor,0x79c,global+158);
            for(unsigned i=0;i<4;++i)assert(state->advance(f.actor,sizeof(f.actor),q.schedule,&sr));
            assert(state->inspect(f.actor,sizeof(f.actor),q.schedule,&sr)&&sr.index==4&&sr.task.id==109);
            meta.scripted=false;meta.suppress=true;stamp=0;
            seed_anim(77.f);f.actor[0x3e0]=0;
            const ServerNpcRunAnimationInput wait{77.f,false,false};
            const int waiting=combined->ground(f.actor,sizeof(f.actor),&wait,.05f,&scripted_step,&outer_bind,&step_result,&tail_result,&outer_result,&services,&events,&seq_out);
            if(!waiting)std::fprintf(stderr,"script wait outer=%u think=%u/%u task=%d start=%u run=%u anim=%u\n",outer_result.active_called,step_result.think_called,step_result.think_completed,step_result.think.requests.scheduler.maintenance.state.task.id,script_stats.native_starts,script_stats.native_runs,step_result.animation_called);
            assert(waiting&&outer_result.completed&&outer_result.think_called&&tail_result.completed&&script_stats.native_starts==1&&script_stats.native_runs==1&&script_stats.last_run.script.waiting&&!cleaned.completed&&!q.cleared);
            const float done_time=outer_result.finish.deadline;assert(done_time>77.f);
            seed_anim(done_time);f.actor[0x3e0]=1;
            const ServerNpcRunAnimationInput done{done_time,false,false};
            const int finished=combined->ground(f.actor,sizeof(f.actor),&done,.05f,&scripted_step,&outer_bind,&step_result,&tail_result,&outer_result,&services,&events,&seq_out);
            if(!finished)std::fprintf(stderr,"script finish outer=%u think=%u/%u task=%d start=%u run=%u cleaned=%u anim=%u\n",outer_result.active_called,step_result.think_called,step_result.think_completed,step_result.think.requests.scheduler.maintenance.state.task.id,script_stats.native_starts,script_stats.native_runs,cleaned.completed,step_result.animation_called);
            assert(finished&&outer_result.completed&&outer_result.think_called&&tail_result.completed&&!script_stats.native_starts&&script_stats.native_runs==1&&script_stats.last_run.script.terminal.completed&&cleaned.completed&&cleaned.handles_cleared==3&&q.restored==1&&q.reads==2&&q.cleared==1);
            assert(state->inspect(f.actor,sizeof(f.actor),q.schedule,&sr)&&!sr.schedule);
            unsigned script_handle=0;std::memcpy(&script_handle,f.actor+0x970,4);assert(script_handle==0xffffffffu);
            // Begin a fresh original WAIT at index zero. No test-side
            // cursor advance: task completion and outer cadence drive it.
            std::memset(q.seq,0,sizeof(q.seq));f.put(q.seq,0,nm::scripted_sequence_vtable);
            f.put(q.seq,0x378,0x9002u);f.put(q.seq,0x3d0,0x3000u);f.put(q.seq,0x3d4,0xffffffffu);
            // Two named sequences already registered with the barrier.
            f.put(q.seq,0xc,0x2242ee1cu);f.put(q.seq,0xc0,23u);f.put(q.seq,0x3b8,2u);
            f.put(q.other,0,nm::scripted_sequence_vtable);f.put(q.other,0xc,0x2242ee1cu);f.put(q.other,0xc0,23u);
            f.put(q.other,0x378,0xa003u);f.put(q.other,0x3b8,2u);
            equipped_entries[3].serial=10;equipped_entries[3].entity=reinterpret_cast<CBaseEntity*>(q.other);
            equipped_entries[2].next=&equipped_entries[3];equipped_entries[3].previous=&equipped_entries[2];
            equipped.scripted_other=q.other;chain.bound.scheduler.maintenance.selection.state.entities.count=4;
            ServerEntitySearchBindings group{};group.context=&q;group.head=&equipped_entries[2];group.extent=control.extent;
            group.string_text=[](void*,unsigned token,const char** text,unsigned* n) noexcept ->int{
                *text=token==23?"group":token==0x2242ee1cu?"scripted_sequence":token==0?"":nullptr;
                if(!*text)return 0;*n=static_cast<unsigned>(std::strlen(*text)+1);return 1;
            };
            native_pos.search=&group;
            prepare_engaged(1,0);f.put(equipped.weapon,0,0x2247ab9cu);f.put(f.actor,0x6cc,0x7001u);
            f.put(f.actor,0x850,4);f.put(f.actor,0x858,4);f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0x8a8,0xffffffffu);
            f.put(f.actor,0x970,0x9002u);f.put(f.actor,0x378,0x3000u);f.put(f.actor,0x9c,100);f.put(f.actor,0x78,1u);
            f.put(q.seq,0xb4,4u);f.put(q.seq,0x3c4,2u);
            f.put(equipped.nav,0xc,0);std::memset(equipped.path,0,sizeof(equipped.path));
            auto prefix_move=mb;prefix_move.context=&equipped;
            prefix_move.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept ->int{
                auto& q=*static_cast<EquippedThinkFixture*>(p);
                if(token==604){*out=q.nav;*n=sizeof(q.nav);return 1;}
                if(token==605){*out=q.path;*n=sizeof(q.path);return 1;}return 0;
            };
            prefix_move.clear_path=[](void* p,void* nav) noexcept ->int{
                auto& q=*static_cast<EquippedThinkFixture*>(p);assert(nav==q.nav);
                ++q.f->clear_paths;q.f->put(q.path,0,0u);q.f->put(q.path,0x44,0u);return 1;
            };
            auto prefix_step=scripted_step;prefix_step.movement=&prefix_move;
            float prefix_time=78.f;f.put(f.actor,0x864,prefix_time);f.put(f.actor,0x910,0x7f7fffffu);f.put(f.actor,0x74,1560);
            assert(state->install(f.actor,sizeof(f.actor),8012,prefix_time,q.schedule,&sr)&&sr.index==0);
            f.put(f.actor,0x79c,global+158);
            const unsigned prior_clears=f.clear_paths;
            for(unsigned index=0;index<3;++index){
                seed_anim(prefix_time);
                const ServerNpcRunAnimationInput input{prefix_time,false,false};
                const int ok=combined->ground(f.actor,sizeof(f.actor),&input,.05f,&prefix_step,&outer_bind,&step_result,&tail_result,&outer_result,&services,&events,&seq_out);
                if(!ok)std::fprintf(stderr,"script prefix expected=%u task=%d start=%u run=%u anim=%u\n",index,step_result.think.requests.scheduler.maintenance.state.task.id,script_stats.native_starts,script_stats.native_runs,step_result.animation_called);
                assert(ok&&outer_result.completed&&outer_result.think_called&&tail_result.completed);
                assert(state->inspect(f.actor,sizeof(f.actor),q.schedule,&sr)&&sr.schedule==8012&&sr.index==index);
                if(index==0){assert(sr.task.id==116&&sr.status==4&&f.clear_paths==prior_clears+1&&!script_stats.starts);}
                if(index==1){assert(sr.task.id==110&&sr.status==4&&script_stats.native_starts==1&&script_stats.native_runs==1&&script_stats.last_run.script.synchronized&&script_stats.last_run.script.group_updates==2);}
                if(index==2){assert(sr.task.id==106&&sr.status!=4&&script_stats.native_starts==1&&script_stats.native_runs==1&&script_stats.last_run.script.waiting);}
                assert(outer_result.finish.deadline>prefix_time);prefix_time=outer_result.finish.deadline;
            }
            unsigned sync_count=0,other_count=0;std::memcpy(&sync_count,q.seq+0x3b8,4);std::memcpy(&other_count,q.other+0x3b8,4);assert(sync_count==1&&other_count==1);
            // Without a sequence release, another due frame must stay on 106.
            seed_anim(prefix_time);
            const ServerNpcRunAnimationInput held{prefix_time,false,false};
            assert(combined->ground(f.actor,sizeof(f.actor),&held,.05f,&prefix_step,&outer_bind,&step_result,&tail_result,&outer_result,&services,&events,&seq_out)&&outer_result.completed&&outer_result.think_called);
            assert(state->inspect(f.actor,sizeof(f.actor),q.schedule,&sr)&&sr.index==2&&sr.task.id==106&&sr.status!=4&&!script_stats.native_starts&&script_stats.native_runs==1&&script_stats.last_run.script.waiting);
            // The partner's actual native RunTask110 releases both counters.
            // Keep the first actor on its existing descriptor and live cursor.
            const auto run_task=reinterpret_cast<nm::RunScriptedNpcTask>(GetProcAddress(s,nm::scripted_npc_task_export));assert(run_task);
            f.put(q.partner,0,0x2249aaa4u);f.put(q.partner,0x378,0xb004u);f.put(q.partner,0x970,0xa003u);
            f.put(q.partner,0x7a4,1);f.put(q.other,0x3d0,0xb004u);
            equipped_entries[4].serial=11;equipped_entries[4].entity=reinterpret_cast<CBaseEntity*>(q.partner);
            const float release_time=prefix_time+.025f;
            auto partner_control=control;partner_control.entries=equipped_entries;partner_control.entry_count=5;partner_control.now=&release_time;
            auto partner_anim=anim;partner_anim.control=&partner_control;
            auto partner_pos=native_pos;partner_pos.entries=equipped_entries;partner_pos.entry_count=5;partner_pos.now=&release_time;
            auto partner_finish=finish;partner_finish.animation=&partner_anim;partner_finish.possession=&partner_pos;
            auto partner_life=scheduler.grunt.lifecycle;partner_life.conditions=sb.conditions;
            auto partner_task=task;partner_task.finish=&partner_finish;partner_task.lifecycle=&partner_life;
            nm::ScriptedNpcTaskResult released{};
            assert(run_task(q.partner,sizeof(q.partner),110,&partner_task,&released)&&released.synchronized&&released.group_updates==2&&released.task_completed);
            std::memcpy(&sync_count,q.seq+0x3b8,4);std::memcpy(&other_count,q.other+0x3b8,4);assert(!sync_count&&!other_count);
            float sync_stamp=0,other_stamp=0;std::memcpy(&sync_stamp,q.seq+0x3bc,4);std::memcpy(&other_stamp,q.other+0x3bc,4);assert(sync_stamp==release_time&&other_stamp==release_time);
            assert(state->inspect(f.actor,sizeof(f.actor),q.schedule,&sr)&&sr.index==2&&sr.task.id==106&&sr.status!=4);
            // Real native activity assignment, using only fixture metadata
            // and engine notification leaves. OnBegin fires before lookup.
            f.put(q.seq,0x398,31u);f.put(q.seq,0x39c,32u);
            f.put(q.seq,0x3e0+0x14,1u);f.put(q.seq,0x3f8+0x14,2u);
            for(unsigned char* a:{q.begin_action,q.end_action}){
                f.put(a,0,41u);f.put(a,4,42u);f.put(a,0x10,0xffffffffu);
            }
            ServerEntityVoidOutputBindings fire{};fire.context=&q;
            fire.resolve=[](void* p,unsigned token,void** row,unsigned* n) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);*row=token==1?q.begin_action:token==2?q.end_action:nullptr;
                *n=*row?0x1c:0;return *row!=nullptr;
            };
            fire.enqueue=[](void* p,void* activator,void* caller,const ServerEntityVoidOutputAction* action) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(caller==q.seq&&action->target==41&&action->input==42);
                if(activator){assert(activator==q.seq&&!q.lookups);++q.begins;}else{assert(q.begins==1);++q.ends;}return 1;
            };
            nm::ScriptedSequenceEventBindings script_outputs{&fire,nullptr};
            ServerNpcActivityAssignmentBindings play_assign{};play_assign.context=&q;play_assign.translation.actor_class=NpcActivityGrunt;
            play_assign.reset.context=&q;play_assign.dictionary.dictionary=&event_dictionary;
            play_assign.reset.duration=[](void*,void*,int,double* d) noexcept ->int{*d=2.;return 1;};
            play_assign.reset.distance=[](void*,void*,int,double* d) noexcept ->int{*d=0.;return 1;};
            play_assign.reset.flags=[](void*,void*,int,unsigned* flags) noexcept ->int{*flags=0;return 1;};
            play_assign.dictionary.descriptor=[](void*,void*,int,void**,unsigned*,unsigned*) noexcept ->int{return 0;};
            play_assign.capabilities=[](void*,void*,unsigned* caps) noexcept ->int{*caps=0;return 1;};
            play_assign.weapon_notify=[](void* p,void* a,unsigned h,int activity,float duration) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(a==q.f->actor&&h==0x7001u&&activity==16&&duration==2.f);return 1;
            };
            play_assign.component_notify=[](void* p,void* a,unsigned component) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(a==q.f->actor&&component==88u&&q.seq[0x3cd]);++q.assignments;return 1;
            };
            anim.events=&script_outputs;anim.assignment=&play_assign;anim.context=&q;
            anim.lookup=[](void* p,void* a,unsigned token,int* index) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(a==q.f->actor&&q.begins==1&&(token==31||token==32));
                ++q.lookups;*index=token==31?2:3;return 1;
            };
            anim.activity=[](void* p,void* a,int index,int* activity) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(a==q.f->actor&&(index==2||index==3));*activity=16;return 1;
            };
            prefix_time=outer_result.finish.deadline;assert(prefix_time>release_time);
            seed_anim(prefix_time);
            const ServerNpcRunAnimationInput play{prefix_time,false,false};
            const int playing=combined->ground(f.actor,sizeof(f.actor),&play,.05f,&prefix_step,&outer_bind,&step_result,&tail_result,&outer_result,&services,&events,&seq_out);
            if(!playing)std::fprintf(stderr,"script release/play think=%u/%u begin=%u assigned=%u anim=%u\n",step_result.think_called,step_result.think_completed,q.begins,q.assignments,step_result.animation_called);
            assert(playing&&outer_result.completed&&outer_result.think_called&&tail_result.completed&&!script_stats.native_starts&&script_stats.native_runs==1);
            assert(script_stats.last_run.script.task_completed&&script_stats.last_run.script.begin_output&&script_stats.last_run.script.begin_queued==1&&script_stats.last_run.script.animation.assigned);
            assert(state->inspect(f.actor,sizeof(f.actor),q.schedule,&sr)&&sr.index==2&&sr.task.id==106&&sr.status==4);
            int play_sequence=-1;std::memcpy(&play_sequence,f.actor+0x3ec,4);assert(play_sequence==2&&q.begins==1&&!q.ends&&q.lookups==1&&q.assignments==1&&!cleaned.completed);
            // Complete the same schedule with nonlooping animation metadata.
            // From here no seed_anim, cursor writes, or finished-byte writes:
            // the recovered base cycle and original outer cadence drive it.
            q.restored=q.cleared=q.reads=0;
            finish.sequence_has_motion=[](void* p,void* a,int seq,bool* value) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(a==q.f->actor&&seq==2);*value=false;return 1;
            };
            task.context=&q;
            task.update_aim=[](void* p,void* a,int x,int y) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(a==q.f->actor&&!x&&!y);++q.aims;return 1;
            };
            nm::ScriptedNpcStartMotionBindings motion{};motion.base=&start;motion.context=&q;
            motion.duration=[](void* p,void* a,int seq,double* duration) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(a==q.f->actor&&seq==2);*duration=2.;return 1;
            };
            motion.advance_motion=[](void* p,void* a,float elapsed,int x,int y) noexcept ->int{
                auto& q=*static_cast<SequenceFixture*>(p);assert(a==q.f->actor&&elapsed>0.f&&elapsed<=.15f&&!x&&!y);++q.motion_ticks;return 1;
            };
            ServerNpcScriptedMotionSchedulerStats motion_stats{};services.motion=&motion;services.motion_stats=&motion_stats;
            unsigned play_ticks=0,post_ticks=0,play_starts=0,post_starts=0,play_completions=0;
            bool terminated=false;
            for(unsigned tick=0;tick<64;++tick){
                const float now=outer_result.finish.deadline;assert(now>prefix_time);prefix_time=now;
                const ServerNpcRunAnimationInput input{now,false,false};
                const int ok=combined->ground(f.actor,sizeof(f.actor),&input,.05f,&prefix_step,&outer_bind,&step_result,&tail_result,&outer_result,&services,&events,&seq_out);
                if(!ok)std::fprintf(stderr,"script continuous tick=%u task=%d status=%d starts=%u/%u runs=%u assigned=%u end=%u cleaned=%u anim=%u\n",tick,step_result.think.requests.scheduler.maintenance.state.task.id,step_result.think.requests.scheduler.maintenance.state.status,script_stats.native_starts,motion_stats.native_starts,script_stats.native_runs,q.assignments,q.ends,cleaned.completed,step_result.animation_called);
                assert(ok&&outer_result.completed&&outer_result.think_called&&tail_result.completed);
                if(motion_stats.native_starts){
                    assert(motion_stats.native_starts==1&&motion_stats.last.task==108&&motion_stats.last.state_written&&motion_stats.last.duration_called);
                    ++play_starts;
                }
                if(script_stats.native_starts){
                    assert(script_stats.native_starts==1&&script_stats.last_start.script.task==109&&script_stats.last_start.script.state_written);
                    ++post_starts;
                }
                assert(script_stats.native_runs==1);
                const auto& result=script_stats.last_run.script;
                if(result.task==108){
                    ++play_ticks;assert(result.aim_called);
                    if(result.task_completed){
                        ++play_completions;assert(result.animation.post_idle_started&&result.animation.output_called&&result.animation.queued==1&&q.ends==1&&q.assignments==2);
                        int current=-1;std::memcpy(&current,f.actor+0x3ec,4);assert(current==3&&!f.actor[0x3e1]);
                    }else assert(result.waiting);
                }else{
                    assert(result.task==109);++post_ticks;
                    if(result.terminal.completed){
                        assert(cleaned.completed&&cleaned.handles_cleared==3&&q.restored==1&&q.cleared==1&&q.reads==2);
                        assert(state->inspect(f.actor,sizeof(f.actor),q.schedule,&sr)&&!sr.schedule);
                        unsigned script=0;std::memcpy(&script,f.actor+0x970,4);assert(script==0xffffffffu);
                        terminated=true;break;
                    }
                    assert(result.waiting&&!cleaned.completed);
                }
                assert(state->inspect(f.actor,sizeof(f.actor),q.schedule,&sr)&&sr.schedule==8012&&(sr.index==3||sr.index==4));
            }
            assert(terminated&&play_starts==1&&post_starts==1&&play_completions==1&&play_ticks>1&&post_ticks>1&&q.motion_ticks==1&&q.aims==play_ticks&&q.begins==1&&q.ends==1&&q.assignments==2&&q.lookups==2);
            // Fresh FACE descriptor: the original cursor enters Start112
            // after STOP_MOVING. Use the same native outer entry and clock.
            std::memset(q.seq,0,sizeof(q.seq));f.put(q.seq,0,nm::scripted_sequence_vtable);
            f.put(q.seq,0x378,0x9002u);f.put(q.seq,0x3d0,0x3000u);f.put(q.seq,0x3d4,0xffffffffu);
            f.put(q.seq,0x370,90.f);f.put(q.seq,0xb4,4u);
            prepare_engaged(1,0);f.put(equipped.weapon,0,0x2247ab9cu);f.put(f.actor,0x6cc,0x7001u);
            f.put(f.actor,0x850,4);f.put(f.actor,0x858,4);f.put(f.actor,0x8a0,0xffffffffu);f.put(f.actor,0x8a8,0x9002u);
            f.put(f.actor,0x970,0x9002u);f.put(f.actor,0x378,0x3000u);f.put(f.actor,0x9c,100);
            f.put(f.motor,0x14,0.f);f.put(equipped.nav,0xc,0);std::memset(equipped.path,0,sizeof(equipped.path));
            auto face_move=prefix_move;
            face_move.stop_path=[](void* p,void* nav,bool finished) noexcept ->int{
                auto& q=*static_cast<EquippedThinkFixture*>(p);assert(nav==q.nav&&finished);++q.f->stop_paths;return 1;
            };
            auto face_step=prefix_step;face_step.movement=&face_move;
            const unsigned turns=f.begins,steps=f.steps,stops=f.stop_paths;
            const bool saved_align=f.align_on_step;f.align_on_step=false;
            float face_time=prefix_time+1.f;f.put(f.actor,0x864,face_time);f.put(f.actor,0x910,0x7f7fffffu);f.put(f.actor,0x74,1700);
            assert(state->install(f.actor,sizeof(f.actor),8015,face_time,q.schedule,&sr)&&sr.index==0&&sr.task.id==116);
            f.put(f.actor,0x79c,global+159);
            for(unsigned tick=0;tick<3;++tick){
                if(tick==2)f.align_on_step=true;
                seed_anim(face_time);
                const ServerNpcRunAnimationInput input{face_time,false,false};
                const int ok=combined->ground(f.actor,sizeof(f.actor),&input,.05f,&face_step,&outer_bind,&step_result,&tail_result,&outer_result,&services,&events,&seq_out);
                if(!ok)std::fprintf(stderr,"script face tick=%u task=%d status=%d turns=%u steps=%u stops=%u\n",tick,step_result.think.requests.scheduler.maintenance.state.task.id,step_result.think.requests.scheduler.maintenance.state.status,f.begins-turns,f.steps-steps,f.stop_paths-stops);
                assert(ok&&outer_result.completed&&outer_result.think_called&&tail_result.completed);
                assert(state->inspect(f.actor,sizeof(f.actor),q.schedule,&sr)&&sr.schedule==8015&&sr.index==(tick?1u:0u));
                if(tick==0)assert(sr.status==4&&sr.task.id==116);
                else{
                    float yaw=0;std::memcpy(&yaw,f.motor+0x14,4);assert(yaw==90.f&&sr.task.id==112);
                    assert(f.begins==turns+1&&f.stop_paths==stops+1&&f.steps==steps+tick);
                    assert((sr.status==4)==(tick==2));
                }
                assert(outer_result.finish.deadline>face_time);face_time=outer_result.finish.deadline;
            }
            f.align_on_step=saved_align;
            equipped_entries[4]={};
            meta.suppress=false;meta.scripted=false;equipped.scripted_entity=nullptr;equipped.scripted_other=nullptr;equipped_entries[3]={};equipped_entries[2]=prior_slot;selection.state.entities.count=prior_count;
        }
        // Real due-think wake route, using the original proximity branch
        // with PVS false to retain the already verified combat fixture.
        auto get_wake_outer=reinterpret_cast<GetServerNpcOuterWakeUpdateApi>(GetProcAddress(s,server_npc_outer_wake_update_export));
        assert(get_wake_outer&&!GetProcAddress(c,server_npc_outer_wake_update_export));
        const auto* wake_outer=get_wake_outer(1,sizeof(ServerNpcOuterWakeUpdateApi));assert(wake_outer);
        ServerNpcOuterWakeUpdateBindings wake_bind{};wake_bind.outer=outer_bind;
        wake_bind.policy.context=&equipped;wake_bind.policy.max_clients=1;
        wake_bind.policy.local_player=[](void* p,void** e) noexcept -> int{*e=static_cast<EquippedThinkFixture*>(p)->enemy;return 1;};
        wake_bind.policy.extent=extent;wake_bind.policy.origin=center;
        wake_bind.transition.context=&equipped;wake_bind.transition.extent=extent;
        unsigned wake_outputs=0;
        wake_bind.transition.context=&wake_outputs;
        wake_bind.transition.output=[](void* p,void* a,unsigned off) noexcept -> int{
            int sleep=-1;std::memcpy(&sleep,static_cast<unsigned char*>(a)+0x868,4);
            assert(!sleep&&off==0xbc8u);++*static_cast<unsigned*>(p);return 1;
        };
        wake_bind.transition.extent=nullptr; // no linked or squad members in this fixture
        ServerNpcOuterWakeUpdateResult wake_result{};
        for(bool prior_sleep_tick:{false,true}){
            const float start=prior_sleep_tick?75.8f:75.7f;
            prepare_engaged(2,0);f.put(equipped.weapon,0,0x2247ab9cu);f.put(f.actor,0x6cc,0x7001u);
            f.put(equipped.enemy,0xbc,0u);
            assert(state->install(f.actor,sizeof(f.actor),attack_schedule,start,&sb,&sr));
            f.put(f.actor,0x79c,global+191);
            for(int i=0;i<4;++i)assert(state->advance(f.actor,sizeof(f.actor),&sb,&sr));
            stamp=0;meta.reloading=false;meta.suppress=false;wake_outputs=0;
            equipped.shots=equipped.sounds=equipped.prepares=equipped.flashes=0;
            f.put(f.actor,0x868,2);f.put(f.actor,0x78,0x20u);f.put(f.actor,0x85c,0);
            f.put(f.actor,0x864,start);f.put(f.actor,0x910,0x7f7fffffu);f.put(f.actor,0x74,1514);
            auto update_wake=[&](float now){
                seed_anim(now);
                const ServerNpcRunAnimationInput input{now,false,false};
                const int ok=wake_outer->ground(f.actor,sizeof(f.actor),&input,.05f,&step_bind,&wake_bind,&step_result,&tail_result,&wake_result);
                if(!ok)std::fprintf(stderr,"due wake now=%f policy=%u transition=%u bypass=%u admitted=%u active=%u think=%u animation=%u tail=%u finish=%u\n",now,wake_result.policy.completed,wake_result.transition.completed,wake_result.outer.sleep_bypassed,wake_result.outer.admission.admitted,wake_result.outer.active_called,step_result.think_completed,step_result.animation_called,tail_result.completed,wake_result.outer.finish.completed);
                assert(ok&&wake_result.outer.completed);
            };
            if(prior_sleep_tick){
                // Original 22043328 gate runs after wake policy. Sleeping
                // cadence first establishes mode4, which survives the wake.
                f.put(f.actor,0x86c,0.f);update_wake(start);
                assert(wake_result.outer.wake_called&&!wake_result.policy.wake_calls&&wake_result.outer.sleep_bypassed&&wake_result.outer.finish.mode==4);
                assert(!wake_result.outer.active_called&&!step_result.think_called&&!equipped.shots&&!wake_outputs);
            }
            f.put(f.actor,0x86c,100.f);
            const float wake_now=prior_sleep_tick?wake_result.outer.finish.next_time:start;
            update_wake(wake_now);
            assert(wake_result.outer.wake_called&&wake_result.policy.reason==NpcWakeProximity&&wake_result.policy.distance_squared==10000.f);
            assert(wake_result.transition.completed&&wake_result.transition.transitions==1&&wake_result.transition.flags_changed==1&&wake_outputs==1);
            assert(wake_result.outer.finish.mode==0);
            if(prior_sleep_tick){
                assert(wake_result.outer.sleep_bypassed&&!wake_result.outer.active_called&&!step_result.think_called&&!equipped.shots);
                const float next=wake_result.outer.finish.next_time;update_wake(next);
                assert(!wake_result.outer.wake_called&&!wake_result.transition.transitions&&wake_outputs==1);
            }
            assert(!wake_result.outer.sleep_bypassed&&wake_result.outer.active_called&&wake_result.outer.think_called&&step_result.think_completed&&tail_result.completed);
            assert(step_result.outer_weapon.primary.clip==2&&step_result.outer_line.attack.conditions[0]==20);
            assert(step_result.animation.frame.frame.weapon_events==1&&step_result.animation.last.event.parent.weapon.clip_changed);
            int remaining=-1;std::memcpy(&remaining,equipped.weapon+0x4ec,4);
            assert(remaining==1&&equipped.shots==1&&equipped.prepares==1&&equipped.sounds==1&&equipped.flashes==1);
        }
        // Exhausted engaged weapon: native idle state query sees condition10,
        // returns combat state3 and the native selector must choose 48.
        prepare_engaged(0,0);f.put(equipped.weapon,0,0x2247ab9cu);
        assert(state->install(f.actor,sizeof(f.actor),attack_schedule,76.f,&sb,&sr));
        f.put(f.actor,0x79c,global+191);
        for(int i=0;i<4;++i)assert(state->advance(f.actor,sizeof(f.actor),&sb,&sr));
        const auto prior_changed=selection.state.state_changed;
        selection.state.state_query=nullptr;selection.state.state_changed=nullptr;
        ServerNpcGruntPointPolicyBindings grenade{};
        ServerNpcGruntSchedulePolicyServices reload_policy{};
        reload_policy.enemy_memory=&vis.memory;reload_policy.grenade=&grenade;
        unsigned reload_rolls=0;reload_policy.context=&reload_rolls;
        reload_policy.probability=[](void* p,float chance,bool* out) noexcept -> int{
            assert(chance==20.f);++*static_cast<unsigned*>(p);*out=true;return 1;
        };
        auto reload_scene=req_scene;reload_scene.policy=&reload_policy;
        auto reload_line=engaged_line;reload_line.scene=&reload_scene;
        auto reload_cone=engaged_cone;reload_cone.line=&reload_line;
        auto reload_combat=engaged_combat;reload_combat.line=&reload_line;reload_combat.cone=&reload_cone;
        active.cone=&reload_cone;active.combat_line=&reload_combat;
        assert(run_engaged(76.f)&&cycle.completed&&cycle.scheduler.gather_calls==1&&!equipped.queries);
        assert(req_run.requests.scheduler.maintenance.ideal.ideal==3&&req_run.requests.scheduler.maintenance.applied.changed);
        assert(!outer_weapon.primary.available&&!inner_weapon.primary.available&&outer_line.attack.conditions[0]==4);
        assert(req_run.requests.requests==1&&req_run.requests.combat.schedule==48&&req_run.requests.request.requested==48);
        assert(req_run.requests.request.acquisition.translated==84&&reload_rolls==1&&req_run.requests.scheduler.maintenance.state.schedule==find(84));
        assert(req_run.requests.scheduler.maintenance.started==1&&req_run.requests.scheduler.maintenance.state.task.id==88);
        int reload_fail=-1;std::memcpy(&reload_fail,f.actor+0x7bc,4);assert(reload_fail==49);
        // New descriptor's real first task installs the ordinary reload
        // fallback. No manual installation of schedule84 or forced selector.
        // The other original probability branch must install the inherited
        // ordinary reload descriptor rather than a synthetic fallback token.
        const auto original_image=f.image;
        assert(reload_image->build(f.actor,NpcSubclassGround,original_image.token_base,original_image.fallback_head,&b,&f.image,&result));
        const auto ordinary=find(49),extended_attack=find(91);assert(ordinary&&extended_attack);
        reload_policy.probability=[](void* p,float chance,bool* out) noexcept -> int{
            assert(chance==20.f);++*static_cast<unsigned*>(p);*out=false;return 1;
        };
        gather.classname_matches=[](void*,void*,const char* name,bool* yes) noexcept -> int{
            assert(!std::strcmp(name,"npc_gib_hopper"));*yes=false;return 1;
        };
        // Combat state3 always enters the original perception route even
        // outside PVS. Supply bounded empty scene buckets, no active sounds,
        // and live existing enemy memory; do not replace the perception tick.
        gather.base.gate.pre_perception=[](void*,void*) noexcept -> int{return 1;};
        auto& perception=gather.base.perception;
        perception.visual.senses.storage.context=&equipped;perception.visual.senses.storage.storage=storage;
        perception.context=&equipped;
        perception.rebuild_category=[](void* p,void* senses,unsigned category,int radius) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);assert(senses==q.senses&&category<3&&radius==512);return 1;
        };
        perception.sound.active_head=[](void*,int* index) noexcept -> int{*index=-1;return 1;};
        perception.maintenance.storage=vis.memory.storage;perception.maintenance.context=&equipped;
        perception.maintenance.extent=extent;perception.maintenance.origin=center;
        perception.maintenance.npc_state=[](void* p,void* e,bool* npc,int* state) noexcept -> int{
            assert(e==static_cast<EquippedThinkFixture*>(p)->enemy);*npc=true;*state=3;return 1;
        };
        perception.enemy_switch.best.memory=vis.memory;
        // First gather after acquisition still sees the previous attack
        // interrupt mask. Retain the original current-enemy relation and
        // validity path rather than clearing those bits to skip reconsideration.
        auto& choice=perception.enemy_switch.gate;
        choice.context=&equipped;choice.extent=extent;
        choice.relation.context=&equipped;choice.relation.storage=vis.memory.storage;
        choice.relation.as_npc=[](void* p,void* e,void** out) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);assert(e==q.enemy);*out=e;return 1;
        };
        choice.relation.npc_identity=[](void* p,void* e,std::uint32_t* vt,unsigned* bytes) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);assert(e==q.enemy);*vt=0x2249aaa4u;*bytes=sizeof(q.enemy);return 1;
        };
        post.search.weapon_caps=[](void*,void*,unsigned* caps) noexcept -> int{*caps=0x10000000u;return 1;};
        auto reload_move=mb;reload_move.context=&equipped;reload_move.storage=storage;
        reload_move.clear_path=[](void* p,void* nav) noexcept -> int{
            auto& q=*static_cast<EquippedThinkFixture*>(p);assert(nav==q.nav);q.f->put(q.path,0,0u);q.f->put(q.path,0x44,0u);return 1;
        };
        step_bind.movement=&reload_move;
        prepare_engaged(0,0);f.put(equipped.weapon,0,0x2247ab9cu);f.put(f.actor,0x6cc,0x7001u);
        f.put(f.actor,0x898,606u);f.put(equipped.senses,8,512.f);
        for(unsigned i=0;i<3;++i)f.put(equipped.senses,0x54+4*i,607u+i);
        f.put(equipped.memory,0x1c,5.f);f.put(equipped.memory,0x20,60.f);
        unsigned counter=0;std::memcpy(&counter,equipped.memory+0x3c,4);f.put(f.actor,0x8c0,counter);
        f.put(f.actor,0x638,610u);f.put(f.actor,0x644,1);
        f.put(equipped.relationship,0,0x8002u);f.put(equipped.relationship,8,1);
        f.put(equipped.enemy,0,0x2249aaa4u);f.put(equipped.enemy,0x868,0);
        f.put(f.actor,0x9b0,std::uint16_t{2});f.put(f.actor,0x9b2,std::uint16_t{3});
        assert(state->install(f.actor,sizeof(f.actor),extended_attack,76.25f,&sb,&sr));
        f.put(f.actor,0x79c,global+191);
        for(int i=0;i<4;++i)assert(state->advance(f.actor,sizeof(f.actor),&sb,&sr));
        stamp=0;equipped.shots=equipped.sounds=equipped.prepares=equipped.flashes=0;
        meta.reloading=true;meta.suppress=true;
        f.put(f.actor,0x864,76.25f);f.put(f.actor,0x910,0x7f7fffffu);f.put(f.actor,0x74,1525);
        auto update_reload=[&](float now,int activity,bool finished){
            seed_anim(now);f.put(f.actor,0x878,activity);f.put(f.actor,0x880,activity);f.actor[0x3e0]=finished;
            const ServerNpcRunAnimationInput input{now,false,false};
            const int ok=outer_api->ground(f.actor,sizeof(f.actor),&input,.05f,&step_bind,&outer_bind,&step_result,&tail_result,&outer_result);
            const auto& mt=step_result.think.requests.scheduler.maintenance;
            if(!ok)std::fprintf(stderr,"reload phase now=%f admitted=%u think=%u complete=%u selected=%d schedule=%u task=%d status=%d anim=%u event=%d tail=%u\n",now,outer_result.admission.admitted,step_result.think_called,step_result.think_completed,step_result.think.requests.request.acquisition.translated,mt.state.schedule,mt.state.task.id,mt.state.status,step_result.animation_called,step_result.animation.frame.frame.failed_event,tail_result.completed);
            if(!ok){
                const auto& t=step_result.think.think.think.think.squad.cycle.preschedule.think;
                const auto& g=t.outer_gather.gather;
                const auto& e=g.base.perception.enemy_switch;
                std::fprintf(stderr,"gather=%u gate=%u perceive=%u perception=%u categories=%u memory=%u refreshed=%u removed=%u enemy=%u allowed=%u/%u deferred=%u choice=%u reason=%u tests=%u post=%u pre=%u maintain=%u\n",g.completed,g.base.gate.pre_called,g.base.gate.perceive,g.base.perception.completed,g.base.perception.categories,g.base.perception.maintenance.visited,g.base.perception.maintenance.position_refreshed,g.base.perception.maintenance.removed,e.had_enemy,e.new_allowed,e.lost_allowed,e.deferred,e.gate.choose,e.gate.reason,e.gate.condition_tests,g.base.post.completed,t.preschedule_called,t.maintenance_called);
            }
            assert(ok&&outer_result.completed&&outer_result.think_called&&step_result.think_completed&&tail_result.completed);
        };
        update_reload(76.25f,16,false);
        const auto& maintenance=step_result.think.requests.scheduler.maintenance;
        assert(step_result.think.requests.request.requested==48&&step_result.think.requests.request.acquisition.translated==49&&reload_rolls==2);
        assert(maintenance.state.schedule==ordinary&&maintenance.state.task.id==116&&maintenance.state.status==4&&!step_result.animation.frame.frame.handled);
        assert(f.last_pending==1);
        // The ordinary reload task starts activity66. The synthetic studio
        // descriptor delivers the original event2, not a direct clip write.
        meta.suppress=false;
        const float reload_now=outer_result.finish.deadline;
        update_reload(reload_now,66,false);
        assert(maintenance.state.schedule==ordinary&&maintenance.state.task.id==71&&maintenance.state.status!=4&&f.last_pending==66);
        assert(step_result.animation.fixed_resets==1&&step_result.animation.frame.frame.npc_events==1&&!step_result.animation.frame.frame.weapon_events);
        int reloaded_clip=-1;std::memcpy(&reloaded_clip,equipped.weapon+0x4ec,4);assert(reloaded_clip==8&&equipped.sounds==1&&!equipped.shots);
        assert(conditions->test(f.actor,sizeof(f.actor),4,NpcConditionCurrent,&sb.conditions,&condition)&&!condition.value);
        // A later due update observes restored ammo and completes task71
        // when the synthetic animation reports its terminal sequence flag.
        meta.suppress=true;
        update_reload(outer_result.finish.deadline,66,true);
        assert(step_result.outer_weapon.primary.available&&step_result.outer_weapon.primary.clip==8&&maintenance.state.schedule==ordinary&&maintenance.state.task.id==71&&maintenance.state.status==4);
        assert(equipped.sounds==1&&!equipped.shots);
        f.image=original_image;step_bind.movement=&mb;
        selection.state.state_changed=prior_changed;
        ids=prior_ids;
        stamp=prior_stamp;
        selection.state.entities=previous_entities;selection.state.state_query=previous_query;selection.state.context=previous_context;
        req_schedule.admission=previous_admission;
    }

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
    // Actual ground schedule41 high-capability path through outer think, not a
    // direct Cone API call. Distinct serial-resolved actor, weapon and player target.
    struct ConeThinkFixture {
        F* f;unsigned char weapon[0x498]{},enemy[0x9b0]{},blocker[0x9b0]{},memory[0x40]{};
        unsigned traces=0,spreads=0,players=0;int disposition=1;
    } high{&f};
    f.put(high.weapon,0,0x224776acu);f.put(high.weapon,0x494,0x3000u);
    f.put(high.enemy,0,0x224aec3cu);f.put(high.enemy,0x360,300.f);f.put(high.enemy,0x2bc,300.f);
    f.put(high.memory,0,0x22068d70u);f.put(high.memory,0x18,0x22068b40u);high.memory[0x10]=255;
    sin_independent::EntityHandleEntry high_entries[4]{};
    high_entries[0].serial=3;high_entries[0].entity=reinterpret_cast<CBaseEntity*>(f.actor);
    high_entries[1].serial=7;high_entries[1].entity=reinterpret_cast<CBaseEntity*>(high.weapon);
    high_entries[2].serial=8;high_entries[2].entity=reinterpret_cast<CBaseEntity*>(high.enemy);
    high_entries[3].serial=9;high_entries[3].entity=reinterpret_cast<CBaseEntity*>(high.blocker);
    auto high_old_entities=selection.state.entities;
    selection.state.entities.entries=high_entries;selection.state.entities.count=4;
    req_schedule.admission.context=&high;
    req_schedule.admission.extent=[](void* p,void* e,unsigned* n) noexcept -> int{
        auto& q=*static_cast<ConeThinkFixture*>(p);
        if(e==q.f->actor)*n=sizeof(q.f->actor);else if(e==q.weapon)*n=sizeof(q.weapon);
        else if(e==q.enemy)*n=sizeof(q.enemy);else if(e==q.blocker)*n=sizeof(q.blocker);else return 0;return 1;
    };
    req_line.context=&high;
    req_line.random_float=[](void*,float lo,float hi,float* out) noexcept -> int{assert(lo==.7f&&hi==1.f);*out=1.f;return 1;};
    req_line.shoot_position=[](void* p,void* a,float* out) noexcept -> int{
        auto& q=*static_cast<ConeThinkFixture*>(p);assert(a==q.f->actor&&!q.f->actor[0xc64]);out[0]=out[1]=out[2]=0;return 1;
    };
    req_line.line_trace=[](void* p,void* a,const float* start,const float* end,unsigned mask,int group,float* fraction,void** hit) noexcept -> int{
        auto& q=*static_cast<ConeThinkFixture*>(p);assert(a==q.f->actor&&start[0]==0&&end[0]==300&&mask==0x46004003u&&group==6);
        ++q.traces;*fraction=1.f;*hit=nullptr;return 1;
    };
    req_line.disposition=[](void* p,void* a,void* e,int* out) noexcept -> int{
        auto& q=*static_cast<ConeThinkFixture*>(p);assert(a==q.f->actor&&e==q.blocker);*out=q.disposition;return 1;
    };
    req_line.center=[](void* p,void* e,float* out) noexcept -> int{
        auto& q=*static_cast<ConeThinkFixture*>(p);assert(e==q.blocker);out[0]=100;out[1]=out[2]=0;return 1;
    };
    ServerNpcGruntAdaptiveSpreadBindings high_spread{};high_spread.context=&high;high_spread.memory.storage.context=&high;
    high_spread.memory.storage.storage=[](void* p,std::uint32_t token,void** out,unsigned* n) noexcept -> int{
        if(token!=601)return 0;auto& q=*static_cast<ConeThinkFixture*>(p);*out=q.memory;*n=sizeof(q.memory);return 1;
    };
    high_spread.base_spread=[](void* p,void* a,void* w,void* target,float* out) noexcept -> int{
        auto& q=*static_cast<ConeThinkFixture*>(p);assert(a==q.f->actor&&w==q.weapon&&!target);++q.spreads;out[0]=out[1]=out[2]=.25f;return 1; // exercises reconstructed sine table, not clamp
    };
    req_cone.spread=&high_spread;req_cone.context=&high;
    req_cone.player_count=[](void*,int* count) noexcept -> int{*count=1;return 1;};
    req_cone.player_at=[](void* p,int index,void** out) noexcept -> int{assert(index==1);auto& q=*static_cast<ConeThinkFixture*>(p);++q.players;*out=q.blocker;return 1;};
    req_cone.normalize=[](void*,float* v,float* length) noexcept -> int{
        *length=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(*length)for(unsigned i=0;i<3;++i)v[i]/=*length;return 1;
    };
    req_arbitration.shared_select=[](void*,void*,int* id) noexcept -> int{*id=41;return 1;};
    auto high_run=[&](float now,bool legacy){
        std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,7);f.put(f.actor,0x858,7);f.put(f.actor,0xccc,-1);bind_nav();
        f.put(f.actor,0x78c,0x7001u);f.put(f.actor,0x8a0,0x8002u);f.put(f.actor,0x8ec,0x10000000u);
        f.put(f.actor,0x984,601u);f.put(f.actor,0xc90,100.f);f.actor[0xc64]=f.actor[0xc66]=1;
        return legacy?request_think->run(f.actor,sizeof(f.actor),now,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&req_think,&req_run):
            cone_think->run(f.actor,sizeof(f.actor),now,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&cone_bind,&req_run);
    };
    assert(high_run(69.497f,false)&&req_run.requests.requests==1&&req_run.requests.request.acquisition.translated==91&&
        req_run.requests.scheduler.maintenance.state.schedule==find(91)&&!f.actor[0xc64]&&high.traces==1&&high.spreads==1&&high.players==1);
    high.disposition=0;
    assert(high_run(69.498f,false)&&req_run.requests.request.acquisition.translated==91&&f.actor[0xc64]&&high.traces==2&&high.spreads==2&&high.players==2);
    assert(!high_run(69.499f,true)&&!req_run.requests.request.acquisition.completed&&!f.actor[0xc64]&&high.traces==3&&high.spreads==2&&high.players==2);
    req_cone.spread=nullptr;
    assert(!high_run(69.4995f,false)&&!req_run.requests.request.acquisition.completed&&!f.actor[0xc64]&&high.traces==4&&high.spreads==2&&high.players==2);
    // Restore the pre-existing test services and registry; no fixture pointers escape.
    req_cone={};req_cone.line=&req_line;req_line={};req_line.scene=&req_scene;
    req_schedule.admission.context=nullptr;req_schedule.admission.extent=nullptr;
    selection.state.entities=high_old_entities;
    req_arbitration.shared_select=[](void*,void*,int* id) noexcept -> int{*id=26;return 1;};
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
    const auto jet_combat_getter=reinterpret_cast<GetServerNpcNativeJetCombatLineThinkApi>(GetProcAddress(s,server_npc_native_jet_combat_line_think_export));
    assert(jet_combat_getter&&!GetProcAddress(c,server_npc_native_jet_combat_line_think_export)&&!jet_combat_getter(0,sizeof(ServerNpcNativeJetCombatLineThinkApi))&&!jet_combat_getter(1,0));
    const auto* jet_combat_think=jet_combat_getter(1,sizeof(ServerNpcNativeJetCombatLineThinkApi));assert(jet_combat_think);
    const auto jet_weapon_getter=reinterpret_cast<GetServerNpcNativeJetWeaponPolicyThinkApi>(GetProcAddress(s,server_npc_native_jet_weapon_policy_think_export));
    assert(jet_weapon_getter&&!GetProcAddress(c,server_npc_native_jet_weapon_policy_think_export)&&!jet_weapon_getter(0,sizeof(ServerNpcNativeJetWeaponPolicyThinkApi))&&!jet_weapon_getter(1,0));
    const auto* jet_weapon_think=jet_weapon_getter(1,sizeof(ServerNpcNativeJetWeaponPolicyThinkApi));assert(jet_weapon_think);
    ServerNpcNativeJetCombatLineThinkBindings jet_combat_bind{};jet_combat_bind.think=jet_bind.think;jet_combat_bind.selector=&jet_request;jet_combat_bind.cone=&req_cone;jet_combat_bind.combat_line=&think_line;
    req_arbitration.shared_select=[](void*,void*,int* id) noexcept -> int{*id=41;return 1;};
    selection.translate=nullptr;selection.fail_select=nullptr;selection.special_select=nullptr;
    std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,7);f.put(f.actor,0x858,7);f.put(f.actor,0xccc,-1);f.put(f.actor,0x218,321u);ready=true;
    assert(jet_weapon_think->run(f.actor,sizeof(f.actor),83,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&jet_combat_bind,&jet_run,&outer_line,&inner_line,&weapon_policy,&outer_weapon,&inner_weapon)&&!outer_line.attack.line_calls&&!inner_line.attack.line_calls&&!outer_weapon.primary.completed&&!inner_weapon.primary.completed);
    const auto& jet_cycle=jet_run.think.think.think.squad.cycle.preschedule;
    assert(jet_cycle.jet_queried&&jet_cycle.jet_called&&jet_cycle.think.completed);
    assert(jet_run.requests.base.requests==1&&jet_run.requests.base.request.requested==41&&jet_run.requests.base.request.acquisition.translated==122&&jet_run.requests.base.scheduler.maintenance.state.schedule==find(122));
    int jet_fail=0;std::memcpy(&jet_fail,f.actor+0x7bc,4);assert(jet_fail==119);
    // Retained original chase schedule, Jet custom task160 writes movement mode.
    install_at(119,1);f.put(f.actor,0x850,7);f.put(f.actor,0x858,7);f.put(f.actor,0xccc,-1);f.put(f.actor,0x218,321u);ready=false;
    assert(jet_think->run(f.actor,sizeof(f.actor),84,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&jet_bind,&jet_run));
    assert(!jet_run.requests.base.requests&&jet_cycle.jet_queried&&!jet_cycle.jet_called&&jet_cycle.think.completed&&jet_run.requests.base.scheduler.jetpack.lifecycle.completed);
    int mode=0;std::memcpy(&mode,f.actor+0xfec,4);assert(mode==2);
    // Due Jetpack outer update: keep the original attachment tick and
    // actual native descriptor acquisition, then enter animation.
    {
        auto get_step=reinterpret_cast<GetServerNpcActiveJetWeaponStepApi>(GetProcAddress(s,server_npc_active_jet_weapon_step_export));
        auto get_reg=reinterpret_cast<GetServerGruntRegistrationApi>(GetProcAddress(s,server_grunt_registration_export));
        auto get_events=reinterpret_cast<GetServerEventDictionaryApi>(GetProcAddress(s,server_event_dictionary_export));
        assert(get_step&&get_reg&&get_events);
        const auto* step=get_step(1,sizeof(ServerNpcActiveJetWeaponStepApi));
        const auto* reg=get_reg(1,sizeof(ServerGruntRegistrationApi));const auto* ed=get_events(1,sizeof(ServerEventDictionaryApi));
        assert(step&&reg&&ed);
        ServerEventDictionaryEntry event_names[32]{};ServerEventDictionary events{event_names,32,0,0,0};const auto old_ids=ids;
        assert(ed->seed(&events)&&reg->grunt(&activities,&events,nullptr,&ids));
        ServerNpcScriptWeaponEventFrameBindings frame{};auto& gf=frame.frame.frame;gf.ids=&ids;
        auto& rt=gf.runtime;rt.npc.context=&f;
        auto& as=rt.npc.transition.assignment;as.translation.actor_class=NpcActivityJetpack;as.translation.activities=&activities;as.translation.events=&events;
        as.translation.metadata=[](void*,void*,const ServerActivityCompositeView** out) noexcept -> int{*out=nullptr;return 0;};
        as.dictionary.dictionary=&events;rt.npc.frame.activities=&activities;rt.npc.frame.frame.dictionary=as.dictionary;
        // Explicit absent studio model, not a fake successful animation.
        rt.npc.frame.frame.sequence.frame.model_available=[](void*,void*) noexcept -> int{return 0;};
        rt.npc.storage=[](void* p,std::uint32_t token,const void** out,unsigned* n) noexcept -> int{
            auto& q=F::self(p);if(token==100){*out=q.nav;*n=sizeof(q.nav);return 1;}
            if(token==101){*out=q.path;*n=sizeof(q.path);return 1;}return 0;
        };
        rt.weapon.animation.activities=&activities;rt.weapon.animation.events=&events;rt.weapon.animation.dictionary=as.dictionary;
        ServerNpcActiveJetWeaponStepBindings binding{};auto& bs=binding.base;
        bs.image=&f.image;bs.schedule=&runtime;bs.full=&full;bs.attack=&ab;bs.facing=&fb;bs.target=&tb;bs.movement=&mb;
        bs.cue=&no_legacy_cue;bs.observe=&ob;bs.sound_cover=&scb;bs.tactical=&tcb;bs.policy=&weapon_policy;bs.frame=&frame;binding.think=&jet_combat_bind;
        static ServerNpcActiveJetWeaponStepResult out;
        std::memset(f.actor,0,sizeof(f.actor));f.put(f.actor,0x850,7);f.put(f.actor,0x858,7);f.put(f.actor,0xccc,-1);
        f.put(f.actor,0x218,321u);f.put(f.actor,0x78c,0xffffffffu);bind_nav();ready=true;
        f.put(f.actor,0x864,85.f);const ServerNpcRunAnimationInput input{85.f,false,false};
        assert(step->run(f.actor,sizeof(f.actor),&input,&binding,&out)&&out.completed&&out.think_completed&&out.animation_called);
        assert(out.think.think.think.think.squad.cycle.preschedule.jet_called&&out.think.requests.base.request.acquisition.translated==122);
        assert(!out.animation.frame.frame.runtime.tick.animation.frame.frame.model_available&&!out.animation.frame.frame.handled);
        ids=old_ids;
    }
    std::memcpy(before,f.actor,sizeof(before));
    assert(!jet_think->run(f.actor,0x1084,85,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&jet_bind,&jet_run)&&!std::memcmp(before,f.actor,sizeof(before)));
    assert(!request_think->run(f.actor,sizeof(f.actor),85,&f.image,&runtime,&full,&ab,&fb,&tb,&mb,&no_legacy_cue,&ob,&scb,&tcb,&req_think,&req_run)&&!std::memcmp(before,f.actor,sizeof(before)));
    FreeLibrary(s);FreeLibrary(c);std::puts("PASS original schedule descriptor materialization and real task cursor");
}
