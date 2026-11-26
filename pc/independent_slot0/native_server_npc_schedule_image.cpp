#include "native_server_npc_schedule_image.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleCatalogApi* __cdecl SiN_GetServerNpcScheduleCatalogApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleNamespaceApi* __cdecl SiN_GetServerNpcScheduleNamespaceApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityDictionaryApi* __cdecl SiN_GetServerActivityDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleServicesApi* __cdecl SiN_GetServerNpcScheduleServicesApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSubclassSchedulerApi* __cdecl SiN_GetServerNpcSubclassSchedulerApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntGatherNativeApi* __cdecl SiN_GetServerNpcGruntGatherNativeApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntPrescheduleApi* __cdecl SiN_GetServerNpcGruntPrescheduleApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerEliteTaskApi* __cdecl SiN_GetServerEliteTaskApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSpeechQueueApi* __cdecl SiN_GetServerNpcGruntSpeechQueueApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntPerceptionSpeechApi* __cdecl SiN_GetServerNpcGruntPerceptionSpeechApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntPerceptionSlotsApi* __cdecl SiN_GetServerNpcGruntPerceptionSlotsApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcGruntSquadReportApi* __cdecl SiN_GetServerNpcGruntSquadReportApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
bool private_activity(const ServerGruntRegistration* r,int index,float& value) noexcept{
    const auto* api=SiN_GetServerActivityDictionaryApi(1,sizeof(ServerActivityDictionaryApi));
    constexpr const char* names[]{"ACT_COMBINE_THROW_GRENADE","ACT_COMBINE_LAUNCH_GRENADE","ACT_COMBINE_BUGBAIT","ACT_COMBINE_AR2_ALTFIRE","ACT_MELEE_KICK","ACT_GRUNT_SHOT_IN_HELMET"};
    if(!api||!r||!r->activities_ready||!r->activity_owner||r->activity_generation!=r->activity_owner->names.generation||index<0||index>=6)return false;
    int id=-1;if(!api->lookup(r->activity_owner,names[index],&id)||id<0||id!=r->activities[index]||id>16777216)return false;
    value=static_cast<float>(id);return true;
}
int __cdecl build(void* actor,unsigned family,std::uint32_t base,std::uint32_t fallback,const ServerScheduleImageBindings* b,ServerScheduleImage* out,ServerScheduleImageResult* result) noexcept{
    if(!result)return 0;*result={};result->failed_schedule=result->failed_task=result->failed_condition=-1;
    if(!actor||family>NpcSubclassJetpack||!base||base>0xffffffffu-137u||(fallback>=base&&fallback-base<138u)||!b||!out)return 0;
    const auto* cat=SiN_GetServerNpcScheduleCatalogApi(1,sizeof(ServerNpcScheduleCatalogApi));
    const auto* ns=SiN_GetServerNpcScheduleNamespaceApi(1,sizeof(ServerNpcScheduleNamespaceApi));
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    const ServerScheduleCatalogEntry* entries=nullptr;unsigned count=0;
    if(!cat||!ns||!cond||!cat->all(&entries,&count)||count!=46)return 0;
    ServerScheduleImage staged{};staged.family=family;staged.token_base=base;staged.head=staged.fallback_head=fallback;
    // Base initializes first, then subclass. Parser prepends new descriptors.
    for(unsigned pass=0;pass<(family==NpcSubclassGround?1u:2u);++pass){
        const unsigned wanted=pass?family:static_cast<unsigned>(NpcSubclassGround);
        for(unsigned i=0;i<count;++i){
            const auto& e=entries[i];if(e.family!=wanted)continue;
            result->failed_schedule=e.local_schedule;result->failed_task=result->failed_condition=-1;
            if(staged.row_count>=46||e.task_count>270-staged.task_count)return 0;
            int global=-1;if(!ns->to_global(actor,e.local_schedule,&b->namespace_map,&global)||global<npc_global_condition_base)return 0;
            for(unsigned j=0;j<staged.row_count;++j)if(staged.rows[j].descriptor[0]==static_cast<std::uint32_t>(global))return 0;
            auto& row=staged.rows[staged.row_count];row.source=&e;row.task_offset=staged.task_count;
            const auto token=base+3u*staged.row_count;
            row.descriptor[0]=static_cast<std::uint32_t>(global);row.descriptor[1]=token+1;row.descriptor[2]=e.task_count;
            row.descriptor[11]=token+2;row.descriptor[12]=staged.head;
            for(unsigned j=0;j<e.task_count;++j){
                result->failed_task=static_cast<int>(j);const auto& task=e.tasks[j];
                float parameter=task.argument;
                if(task.argument_kind==ScheduleArgumentPrivateActivity){
                    if(!private_activity(b->registration,static_cast<int>(parameter),parameter))return 0;
                    staged.activity_owner=b->registration->activity_owner;staged.activity_generation=b->registration->activity_generation;++result->private_activities;
                }else if(task.argument_kind==ScheduleArgumentSchedule){
                    int mapped=-1,local=-1;
                    if(!ns->to_global(actor,static_cast<int>(parameter),&b->namespace_map,&mapped)||mapped<npc_global_condition_base||
                       !ns->to_local(actor,mapped,&b->namespace_map,&local)||local!=static_cast<int>(parameter))return 0;
                }else if(task.argument_kind==ScheduleArgumentDamageToken){
                    // 2209f1b9 -> 223a5c8b: bare nonnumeric token takes
                    // the CRT no-conversion path and stores positive zero.
                    parameter=0.f;
                    if(b->damage_token){
                        float witness=0.f;
                        if(!b->damage_token(b->context,"DMG_BURN",&witness)||witness!=0.f||std::signbit(witness))return 0;
                    }
                    ++result->damage_tokens;
                }else if(task.argument_kind>ScheduleArgumentDamageToken)return 0;
                if(!std::isfinite(parameter))return 0;
                staged.tasks[staged.task_count++]={task.local_task,parameter};++result->tasks;
            }
            result->failed_task=-1;
            for(unsigned j=0;j<e.condition_count;++j){
                result->failed_condition=static_cast<int>(j);ServerNpcConditionResult cr{};
                if(!cond->map(actor,e.conditions[j],&b->namespace_map,&cr)||!cr.mapped||cr.bit<0||cr.bit>=256)return 0;
                row.descriptor[3+cr.bit/32]|=1u<<(cr.bit%32);++result->interrupts;
            }
            staged.head=token;++staged.row_count;++result->rows;
        }
    }
    if(staged.activity_owner&&staged.activity_owner->names.generation!=staged.activity_generation)return 0;
    result->failed_schedule=result->failed_task=result->failed_condition=-1;
    *out=staged;return 1;
}
int __cdecl storage(const ServerScheduleImage* image,std::uint32_t token,const void** out,unsigned* bytes) noexcept{
    if(!out||!bytes)return 0;*out=nullptr;*bytes=0;
    if(!image||!image->token_base||image->row_count>46||image->task_count>270||token<image->token_base||
       (image->activity_owner&&image->activity_owner->names.generation!=image->activity_generation))return 0;
    const auto offset=token-image->token_base,index=offset/3,kind=offset%3;
    if(index>=image->row_count)return 0;const auto& row=image->rows[index];
    if(!row.source||row.task_offset>image->task_count||row.descriptor[2]>image->task_count-row.task_offset)return 0;
    if(kind==0){*out=row.descriptor;*bytes=sizeof(row.descriptor);}
    else if(kind==1){*out=image->tasks+row.task_offset;*bytes=row.descriptor[2]*sizeof(ServerNpcScheduleTask);}
    else{*out=row.source->name;*bytes=static_cast<unsigned>(std::strlen(row.source->name)+1);}
    return 1;
}
template<class T>T think_read(const void* p,unsigned off) noexcept {T v{};std::memcpy(&v,static_cast<const unsigned char*>(p)+off,sizeof(v));return v;}
struct PrescheduleRoute {
    const ServerNpcNativePrescheduleThinkBindings* bindings;
    ServerNpcNativePrescheduleThinkResult* result;
    const ServerNpcGruntSpeechQueueBindings* speech=nullptr;
    ServerNpcGruntSpeechQueueResult* speech_result=nullptr;
};
struct ThinkRoute {
    const ServerNpcThinkCatalogBindings* bindings;
    ServerNpcThinkCatalogResult* result;
    const PrescheduleRoute* native_pre=nullptr;
    const ServerNpcGruntPerceptionSpeechBindings* dialogue=nullptr;
    ServerNpcGruntPerceptionSpeechResult* dialogue_result=nullptr;
    const ServerNpcGruntPerceptionSlotsBindings* slots=nullptr;
    ServerNpcGruntPerceptionSlotsResult* slots_result=nullptr;
    const ServerNpcGruntSquadReportBindings* reports=nullptr;
    ServerNpcGruntSquadReportResult* report_result=nullptr;
    const ServerNpcGruntCombatLineBindings* combat_line=nullptr;
    ServerNpcGruntNativeLineAttackResult* combat_line_result=nullptr;
};
bool native_preschedule(void* actor,unsigned bytes,float now,const ServerNpcSubclassSchedulerBindings& scheduler,const PrescheduleRoute& route) noexcept{
    const auto& b=*route.bindings;auto& out=*route.result;
    const auto& selection=scheduler.maintenance.selection;const auto& reg=selection.state.entities;
    const auto* classify=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    const auto* api=SiN_GetServerNpcGruntPrescheduleApi(1,sizeof(ServerNpcGruntPrescheduleApi));
    unsigned family=0;
    if(!classify||!api||!classify->classify(scheduler.original_vtable,&family)||reg.count>sin_independent::kEntityHandleCapacity||(reg.count&&!reg.entries))return false;
    const auto registry=sin_independent::EntityHandleView(reg.entries,reg.count);
    if(family==NpcSubclassElite){
        void* linked=registry.Resolve(think_read<unsigned>(actor,0xfdc));
        if(linked){
            auto eb=scheduler.elite;eb.grunt.entities=reg;
            const auto& wb=eb.reload.original.weapon;unsigned n=0;
            const auto* elite=SiN_GetServerEliteTaskApi(1,sizeof(ServerEliteTaskApi));
            out.elite_called=1;
            if(!elite||!wb.extent||!wb.extent(wb.context,linked,&n)||!elite->spin_update(linked,n,now,&eb,&out.elite))return false;
        }
    }else if(family==NpcSubclassJetpack){
        const auto attachment=think_read<unsigned>(actor,0x218);bool ready=false;
        out.jet_queried=1;
        if(!attachment||!b.jet_ready||!b.jet_ready(b.jet_context,attachment,&ready))return false;
        if(ready){out.jet_called=1;if(!b.jet_tick||!b.jet_tick(b.jet_context,attachment))return false;}
    }
    auto bound=b.preschedule;bound.entries=reg.entries;bound.count=reg.count;bound.original_vtable=scheduler.original_vtable;bound.conditions=selection.grunt.schedule.conditions;
    if(route.speech){
        const auto* queue=SiN_GetServerNpcGruntSpeechQueueApi(1,sizeof(ServerNpcGruntSpeechQueueApi));
        return queue&&queue->preschedule(actor,bytes,now,&bound,route.speech,&out.preschedule,route.speech_result);
    }
    return api->run(actor,bytes,now,&bound,&out.preschedule)!=0;
}
bool think_before(void* actor,unsigned bytes,float now,const ServerNpcSubclassSchedulerBindings& scheduler,const ThinkRoute& route) noexcept{
    const auto& b=*route.bindings;auto& out=*route.result;
    const auto& selection=scheduler.maintenance.selection;
    const auto& sb=selection.grunt.schedule;
    auto gather=b.gather;gather.gather.base.gate.original_vtable=scheduler.original_vtable;
    gather.gather.base.post.schedule=sb;
    gather.gather.base.perception.enemy_switch.best.memory.entries=selection.state.entities.entries;
    gather.gather.base.perception.enemy_switch.best.memory.count=selection.state.entities.count;
    const auto* api=SiN_GetServerNpcGruntGatherNativeApi(1,sizeof(ServerNpcGruntGatherNativeApi));
    if(!api||(!route.native_pre&&!b.preschedule)||!std::isfinite(now))return false;
    auto* a=static_cast<unsigned char*>(actor);
    // 2204147c: reset both per-think bytes before virtual +420.
    a[0x84d]=a[0x84e]=0;
    out.outer_called=1;
    if(route.combat_line){
        if(!route.reports||!native_line_squad_report_gather(actor,bytes,now,&gather,route.reports,&out.outer_gather,route.report_result,route.combat_line,route.combat_line_result))return false;
    }else if(route.reports){
        const auto* reports=SiN_GetServerNpcGruntSquadReportApi(1,sizeof(ServerNpcGruntSquadReportApi));
        if(!reports||!reports->gather(actor,bytes,now,&gather,route.reports,&out.outer_gather,route.report_result))return false;
    }else if(route.slots){
        const auto* slots=SiN_GetServerNpcGruntPerceptionSlotsApi(1,sizeof(ServerNpcGruntPerceptionSlotsApi));
        if(!slots||!slots->gather(actor,bytes,now,&gather,route.slots,&out.outer_gather,route.slots_result))return false;
    }else if(route.dialogue){
        const auto* dialogue=SiN_GetServerNpcGruntPerceptionSpeechApi(1,sizeof(ServerNpcGruntPerceptionSpeechApi));
        if(!dialogue||!dialogue->gather(actor,bytes,now,&gather,route.dialogue,&out.outer_gather,route.dialogue_result))return false;
    }else if(!api->gather(actor,bytes,now,&gather,&out.outer_gather))return false;
    if(!a[0x84d])a[0x84d]=1;
    // 220414c8: single-player context notification, delegated only if component exists.
    if(gather.gather.max_clients<=1){
        const void* player=nullptr;unsigned n=0;
        if(!b.player_context||!b.player_context(b.context,&player,&n))return false;
        if(player){
            if(n<0x5e9)return false;
            const auto component=think_read<unsigned>(actor,0xc18);
            if((think_read<unsigned char>(player,0x300)||think_read<unsigned char>(player,0x5e8))&&component){
                out.player_called=1;
                if(!b.component_player||!b.component_player(b.context,component,player))return false;
            }
        }
    }
    // Preserve gate of 2203a330; active geometry/entity correction remains hosted.
    const auto token=think_read<unsigned>(actor,0x798);
    if(a[0xbe0]&&token){
        const void* row=nullptr;unsigned n=0;
        if(!sb.storage||!sb.storage(sb.context,token,&row,&n)||!row||n<4)return false;
        if(think_read<int>(row,0)!=0x41){
            out.housekeeping_called=1;if(!b.housekeeping||!b.housekeeping(b.context,actor))return false;
        }
    }
    out.preschedule_called=1;
    return route.native_pre?native_preschedule(actor,bytes,now,scheduler,*route.native_pre):b.preschedule(b.context,actor,scheduler.original_vtable,now)!=0;
}
bool think_after(void* actor,unsigned bytes,const ServerNpcConditionMapBindings& map,ServerNpcThinkCatalogResult& out) noexcept{
    // +428 points to 22044230 (ret) for all eight authenticated classes.
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    if(!cond)return false;
    constexpr int ids[]{0x10,0x11,0x12,0x3e};
    for(int id:ids){ServerNpcConditionResult cr{};
        if(!cond->store(actor,bytes,id,NpcConditionCurrent,false,&map,&cr))return false;
        out.conditions_cleared+=cr.changed;
    }
    out.completed=1;return true;
}
struct CatalogCall {
    const ServerScheduleImage* image;
    const ServerNpcScheduleBindings* original;
    ServerNpcCatalogSchedulerResult* result;
};
int run_catalog_impl(void* actor,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,ServerNpcCatalogSchedulerResult* out,const ServerGruntCombatTaskBindings* combat=nullptr,ServerGruntCombatSchedulerStats* stats=nullptr,const ServerGruntHelperTaskBindings* full=nullptr,ServerGruntFullTaskStats* full_stats=nullptr,const ServerNpcSharedTaskBindings* shared=nullptr,ServerNpcSharedTaskStats* shared_stats=nullptr,const ServerNpcSharedAttackBindings* attack=nullptr,ServerNpcSharedAttackStats* attack_stats=nullptr,const ServerNpcSharedFacingBindings* facing=nullptr,ServerNpcSharedFacingStats* facing_stats=nullptr,const ServerNpcSharedTargetAnimationBindings* target_animation=nullptr,ServerNpcSharedTargetAnimationStats* target_animation_stats=nullptr,const ServerNpcSharedMovementBindings* movement=nullptr,ServerNpcSharedMovementStats* movement_stats=nullptr,const ServerNpcSharedCueBindings* cue=nullptr,ServerNpcSharedCueStats* cue_stats=nullptr,const ServerNpcSharedObservationBindings* observe=nullptr,ServerNpcSharedObservationStats* observe_stats=nullptr,const ServerNpcSharedSoundCoverBindings* sound_cover=nullptr,ServerNpcSharedSoundCoverStats* sound_cover_stats=nullptr,const ServerNpcSharedTacticalCoverBindings* tactical=nullptr,ServerNpcSharedTacticalCoverStats* tactical_stats=nullptr,const ServerNpcNativeGatherRoute* native_gather=nullptr,const ThinkRoute* think=nullptr) noexcept {
    if(!out)return 0;*out={};
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    const auto* runtime=SiN_GetServerNpcScheduleServicesApi(1,sizeof(ServerNpcScheduleServicesApi));
    if(!actor||!image||!b||!cls||!runtime||!image->row_count||image->row_count>46||!image->token_base||
       image->token_base>0xffffffffu-137u||image->head!=image->token_base+3u*(image->row_count-1)||
       (image->activity_owner&&image->activity_owner->names.generation!=image->activity_generation))return 0;
    unsigned family=0;
    if(!cls->classify(b->scheduler.original_vtable,&family)||family!=image->family)return 0;
    auto bound=*b;
    auto& selection=bound.scheduler.maintenance.selection;
    const auto& original=b->scheduler.maintenance.selection.grunt.schedule;
    if(selection.head!=image->fallback_head)return 0;
    const auto* reg=b->scheduler.grunt.registration;
    if(image->activity_owner&&(!reg||!reg->activities_ready||reg->activity_owner!=image->activity_owner||
       reg->activity_generation!=image->activity_generation))return 0;
    // Validate namespace compatibility before publishing any actor state.
    const auto* ns=SiN_GetServerNpcScheduleNamespaceApi(1,sizeof(ServerNpcScheduleNamespaceApi));
    const auto* cond=SiN_GetServerNpcConditionApi(1,sizeof(ServerNpcConditionApi));
    if(!ns||!cond)return 0;
    for(unsigned i=0;i<image->row_count;++i){
        const auto& row=image->rows[i];int global=-1;
        if(!row.source||!ns->to_global(actor,row.source->local_schedule,&original.conditions,&global)||
           static_cast<std::uint32_t>(global)!=row.descriptor[0])return 0;
        std::uint32_t mask[8]{};
        for(unsigned j=0;j<row.source->condition_count;++j){
            ServerNpcConditionResult cr{};
            if(!cond->map(actor,row.source->conditions[j],&original.conditions,&cr)||!cr.mapped||cr.bit<0||cr.bit>=256)return 0;
            mask[cr.bit/32]|=1u<<(cr.bit%32);
        }
        if(std::memcmp(mask,row.descriptor+3,sizeof(mask)))return 0;
    }
    CatalogCall q{image,&original,out};
    selection.head=image->head;
    auto& sb=selection.grunt.schedule;sb.context=&q;
    sb.storage=[](void* p,std::uint32_t token,const void** data,unsigned* n) noexcept -> int{
        auto& q=*static_cast<CatalogCall*>(p);
        if(token>=q.image->token_base&&token-q.image->token_base<138u){
            ++q.result->native_reads;return storage(q.image,token,data,n);
        }
        ++q.result->inherited_reads;
        return q.original->storage&&q.original->storage(q.original->context,token,data,n);
    };
    sb.reset_movement=[](void* p,void* actor,std::uint32_t token) noexcept -> int{
        const auto& original=*static_cast<CatalogCall*>(p)->original;
        return original.reset_movement&&original.reset_movement(original.context,actor,token);
    };
    if(think){
        if(!combat||!native_gather||bytes<(family==NpcSubclassJetpack?0x1085u:family==NpcSubclassElite?0xfe0u:0xfccu)||!think_before(actor,bytes,now,bound.scheduler,*think))return 0;
        think->result->maintenance_called=1;
    }
    if(combat){
        if(!run_schedule_services_combat_internal(actor,bytes,now,&bound,&out->runtime,combat,stats,full,full_stats,shared,shared_stats,attack,attack_stats,facing,facing_stats,target_animation,target_animation_stats,movement,movement_stats,cue,cue_stats,observe,observe_stats,sound_cover,sound_cover_stats,tactical,tactical_stats,native_gather))return 0;
        return !think||think_after(actor,bytes,sb.conditions,*think->result);
    }
    return runtime->run(actor,bytes,now,&bound,&out->runtime);
}
int __cdecl run_catalog(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,ServerNpcCatalogSchedulerResult* out) noexcept{
    return run_catalog_impl(a,bytes,now,image,b,out);
}
int __cdecl run_combat(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntCombatTaskBindings* combat,ServerNpcCombatCatalogSchedulerResult* out) noexcept{
    if(!out)return 0;*out={};if(!combat)return 0;
    return run_catalog_impl(a,bytes,now,image,b,&out->catalog,combat,&out->combat);
}
int __cdecl run_full(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,ServerNpcFullCatalogSchedulerResult* out) noexcept{
    if(!out)return 0;*out={};if(!full)return 0;
    return run_catalog_impl(a,bytes,now,image,b,&out->combat.catalog,&full->injury.combat,&out->combat.combat,full,&out->full);
}
int __cdecl run_shared(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedTaskBindings* shared,ServerNpcSharedCatalogSchedulerResult* out) noexcept{
    if(!out)return 0;*out={};if(!full||!shared)return 0;
    return run_catalog_impl(a,bytes,now,image,b,&out->full.combat.catalog,&full->injury.combat,&out->full.combat.combat,full,&out->full.full,shared,&out->shared);
}
int __cdecl run_attack(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,ServerNpcAttackCatalogSchedulerResult* out) noexcept{
    if(!out)return 0;*out={};if(!full||!attack)return 0;
    auto& sh=out->shared;
    return run_catalog_impl(a,bytes,now,image,b,&sh.full.combat.catalog,&full->injury.combat,&sh.full.combat.combat,full,&sh.full.full,&attack->shared,&sh.shared,attack,&out->attack);
}
int __cdecl run_facing(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,ServerNpcFacingCatalogSchedulerResult* out) noexcept{
    if(!out)return 0;*out={};if(!full||!attack||!facing)return 0;
    auto& sh=out->attack.shared;
    return run_catalog_impl(a,bytes,now,image,b,&sh.full.combat.catalog,&full->injury.combat,&sh.full.combat.combat,full,&sh.full.full,&attack->shared,&sh.shared,attack,&out->attack.attack,facing,&out->facing);
}
int __cdecl run_animation(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,ServerNpcAnimationCatalogSchedulerResult* out) noexcept{
    if(!out)return 0;*out={};if(!full||!attack||!facing||!target)return 0;
    auto& sh=out->facing.attack.shared;
    return run_catalog_impl(a,bytes,now,image,b,&sh.full.combat.catalog,&full->injury.combat,&sh.full.combat.combat,full,&sh.full.full,&attack->shared,&sh.shared,attack,&out->facing.attack.attack,facing,&out->facing.facing,target,&out->target_animation);
}
int __cdecl run_movement(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,ServerNpcMovementCatalogSchedulerResult* out) noexcept{
    if(!out)return 0;*out={};if(!full||!attack||!facing||!target||!movement)return 0;
    auto& sh=out->animation.facing.attack.shared;
    return run_catalog_impl(a,bytes,now,image,b,&sh.full.combat.catalog,&full->injury.combat,&sh.full.combat.combat,full,&sh.full.full,&attack->shared,&sh.shared,attack,&out->animation.facing.attack.attack,facing,&out->animation.facing.facing,target,&out->animation.target_animation,movement,&out->movement);
}
int __cdecl run_cue(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,ServerNpcCueCatalogSchedulerResult* out) noexcept{
    if(!out)return 0;*out={};if(!full||!attack||!facing||!target||!movement||!cue)return 0;
    auto& sh=out->movement.animation.facing.attack.shared;
    return run_catalog_impl(a,bytes,now,image,b,&sh.full.combat.catalog,&full->injury.combat,&sh.full.combat.combat,full,&sh.full.full,&attack->shared,&sh.shared,attack,&out->movement.animation.facing.attack.attack,facing,&out->movement.animation.facing.facing,target,&out->movement.animation.target_animation,movement,&out->movement.movement,cue,&out->cue);
}
int __cdecl run_observation(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,ServerNpcObservationCatalogSchedulerResult* out) noexcept{
    if(!out)return 0;*out={};if(!full||!attack||!facing||!target||!movement||!cue||!observe)return 0;
    auto& sh=out->cue.movement.animation.facing.attack.shared;
    return run_catalog_impl(a,bytes,now,image,b,&sh.full.combat.catalog,&full->injury.combat,&sh.full.combat.combat,full,&sh.full.full,&attack->shared,&sh.shared,attack,&out->cue.movement.animation.facing.attack.attack,facing,&out->cue.movement.animation.facing.facing,target,&out->cue.movement.animation.target_animation,movement,&out->cue.movement.movement,cue,&out->cue.cue,observe,&out->observation);
}
int __cdecl run_sound_cover(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,ServerNpcSoundCoverCatalogSchedulerResult* out) noexcept{
    if(!out)return 0;*out={};if(!full||!attack||!facing||!target||!movement||!cue||!observe||!sound_cover)return 0;
    auto& sh=out->observation.cue.movement.animation.facing.attack.shared;
    auto& o=out->observation;
    return run_catalog_impl(a,bytes,now,image,b,&sh.full.combat.catalog,&full->injury.combat,&sh.full.combat.combat,full,&sh.full.full,&attack->shared,&sh.shared,attack,&o.cue.movement.animation.facing.attack.attack,facing,&o.cue.movement.animation.facing.facing,target,&o.cue.movement.animation.target_animation,movement,&o.cue.movement.movement,cue,&o.cue.cue,observe,&o.observation,sound_cover,&out->sound_cover);
}
int __cdecl run_tactical_cover(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,ServerNpcTacticalCoverCatalogSchedulerResult* out) noexcept{
    if(!out)return 0;
    // MSVC x86 C1054: initialize the existing ABI members independently.
    out->sound_cover={};out->tactical={};
    if(!full||!attack||!facing||!target||!movement||!cue||!observe||!sound_cover||!tactical)return 0;
    auto& o=out->sound_cover.observation;auto& sh=o.cue.movement.animation.facing.attack.shared;
    return run_catalog_impl(a,bytes,now,image,b,&sh.full.combat.catalog,&full->injury.combat,&sh.full.combat.combat,full,&sh.full.full,&attack->shared,&sh.shared,attack,&o.cue.movement.animation.facing.attack.attack,facing,&o.cue.movement.animation.facing.facing,target,&o.cue.movement.animation.target_animation,movement,&o.cue.movement.movement,cue,&o.cue.cue,observe,&o.observation,sound_cover,&out->sound_cover.sound_cover,tactical,&out->tactical);
}
int __cdecl run_native_gather_catalog(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcGruntGatherNativeBindings* gather,ServerNpcNativeGatherCatalogSchedulerResult* result) noexcept{
    if(!result)return 0;
    result->gather={};result->gather.object.selected_handle=0xffffffffu;result->gather_calls=0;
    auto* out=&result->scheduler;
    // MSVC x86 C1054: initialize the existing ABI members independently.
    out->sound_cover={};out->tactical={};
    if(!full||!attack||!facing||!target||!movement||!cue||!observe||!sound_cover||!tactical||!gather)return 0;
    const ServerNpcNativeGatherRoute route{gather,&result->gather,&result->gather_calls};
    auto& o=out->sound_cover.observation;auto& sh=o.cue.movement.animation.facing.attack.shared;
    return run_catalog_impl(a,bytes,now,image,b,&sh.full.combat.catalog,&full->injury.combat,&sh.full.combat.combat,full,&sh.full.full,&attack->shared,&sh.shared,attack,&o.cue.movement.animation.facing.attack.attack,facing,&o.cue.movement.animation.facing.facing,target,&o.cue.movement.animation.target_animation,movement,&o.cue.movement.movement,cue,&o.cue.cue,observe,&o.observation,sound_cover,&out->sound_cover.sound_cover,tactical,&out->tactical,&route);
}
int run_think_impl(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcThinkCatalogBindings* think,ServerNpcThinkCatalogResult* result,const PrescheduleRoute* native_pre,const ServerNpcGruntPerceptionSpeechBindings* dialogue=nullptr,ServerNpcGruntPerceptionSpeechResult* outer_dialogue=nullptr,ServerNpcGruntPerceptionSpeechResult* inner_dialogue=nullptr,const ServerNpcGruntPerceptionSlotsBindings* slots=nullptr,ServerNpcNativeSquadThinkResult* squad=nullptr,const ServerNpcGruntSquadReportBindings* reports=nullptr,ServerNpcNativeSquadReportThinkResult* report_result=nullptr,const ServerNpcGruntNativeStateBindings* native_state=nullptr,ServerNpcNativeSquadStateThinkResult* state_result=nullptr,const ServerNpcGruntEventSpeechBindings* speech_tasks=nullptr,ServerNpcSharedNativeSpeechStats* speech_stats=nullptr,const ServerNpcGruntSpecialCombatBindings* request_selector=nullptr,const ServerNpcGruntScheduleLineServices* request_line=nullptr,ServerNpcNativeRequestSchedulerResult* request_result=nullptr,const ServerNpcGruntJetSelectorBindings* jet_selector=nullptr,ServerNpcNativeJetRequestSchedulerResult* jet_result=nullptr,const ServerNpcGruntScheduleConeServices* request_cone=nullptr,const ServerNpcGruntCombatLineBindings* combat_line=nullptr,ServerNpcGruntNativeLineAttackResult* outer_line=nullptr,ServerNpcGruntNativeLineAttackResult* inner_line=nullptr) noexcept{
    if(!result)return 0;
    result->outer_gather={};result->outer_gather.object.selected_handle=0xffffffffu;
    result->outer_called=result->player_called=result->housekeeping_called=result->preschedule_called=result->maintenance_called=result->conditions_cleared=result->completed=0;
    auto& inner=result->scheduler;inner.gather={};inner.gather.object.selected_handle=0xffffffffu;inner.gather_calls=0;
    auto* out=&inner.scheduler;
    // MSVC x86 C1054: initialize the existing ABI members independently.
    out->sound_cover={};out->tactical={};
    if(!full||!attack||!facing||!target||!movement||!cue||!observe||!sound_cover||!tactical||!think)return 0;
    const ServerNpcNativeGatherRoute route{&think->gather,&inner.gather,&inner.gather_calls,dialogue,inner_dialogue,slots,squad?&squad->maintenance:nullptr,squad?&squad->selection:nullptr,squad?&squad->selections:nullptr,reports,report_result?&report_result->maintenance_reports:nullptr,native_state,state_result?&state_result->choose:nullptr,state_result?&state_result->apply:nullptr,state_result?&state_result->choose_calls:nullptr,state_result?&state_result->apply_calls:nullptr,speech_tasks,speech_stats,request_selector,request_line,request_result,jet_selector,jet_result,request_cone,combat_line,inner_line};
    const ThinkRoute outer{think,result,native_pre,dialogue,outer_dialogue,slots,squad?&squad->outer:nullptr,reports,report_result?&report_result->outer_reports:nullptr,combat_line,outer_line};
    auto& o=out->sound_cover.observation;auto& sh=o.cue.movement.animation.facing.attack.shared;
    return run_catalog_impl(a,bytes,now,image,b,&sh.full.combat.catalog,&full->injury.combat,&sh.full.combat.combat,full,&sh.full.full,&attack->shared,&sh.shared,attack,&o.cue.movement.animation.facing.attack.attack,facing,&o.cue.movement.animation.facing.facing,target,&o.cue.movement.animation.target_animation,movement,&o.cue.movement.movement,cue,&o.cue.cue,observe,&o.observation,sound_cover,&out->sound_cover.sound_cover,tactical,&out->tactical,&route,&outer);
}
int __cdecl run_think_catalog(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcThinkCatalogBindings* think,ServerNpcThinkCatalogResult* result) noexcept{
    return run_think_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,think,result,nullptr);
}
int run_native_preschedule_impl(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativePrescheduleThinkBindings* bindings,ServerNpcNativePrescheduleThinkResult* result,const ServerNpcGruntSpeechQueueBindings* speech,ServerNpcGruntSpeechQueueResult* speech_result,const ServerNpcGruntPerceptionSpeechBindings* dialogue=nullptr,ServerNpcGruntPerceptionSpeechResult* outer_dialogue=nullptr,ServerNpcGruntPerceptionSpeechResult* inner_dialogue=nullptr,const ServerNpcGruntPerceptionSlotsBindings* slots=nullptr,ServerNpcNativeSquadThinkResult* squad=nullptr,const ServerNpcGruntSquadReportBindings* reports=nullptr,ServerNpcNativeSquadReportThinkResult* report_result=nullptr,const ServerNpcGruntNativeStateBindings* native_state=nullptr,ServerNpcNativeSquadStateThinkResult* state_result=nullptr,const ServerNpcGruntEventSpeechBindings* speech_tasks=nullptr,ServerNpcSharedNativeSpeechStats* speech_stats=nullptr,const ServerNpcGruntSpecialCombatBindings* request_selector=nullptr,const ServerNpcGruntScheduleLineServices* request_line=nullptr,ServerNpcNativeRequestSchedulerResult* request_result=nullptr,const ServerNpcGruntJetSelectorBindings* jet_selector=nullptr,ServerNpcNativeJetRequestSchedulerResult* jet_result=nullptr,const ServerNpcGruntScheduleConeServices* request_cone=nullptr,const ServerNpcGruntCombatLineBindings* combat_line=nullptr,ServerNpcGruntNativeLineAttackResult* outer_line=nullptr,ServerNpcGruntNativeLineAttackResult* inner_line=nullptr) noexcept{
    if(!result)return 0;
    result->preschedule={};result->preschedule.target_handle=0xffffffffu;result->elite={};
    result->elite_called=result->jet_queried=result->jet_called=0;
    if(!bindings)return 0;
    const PrescheduleRoute route{bindings,result,speech,speech_result};
    return run_think_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->think,&result->think,&route,dialogue,outer_dialogue,inner_dialogue,slots,squad,reports,report_result,native_state,state_result,speech_tasks,speech_stats,request_selector,request_line,request_result,jet_selector,jet_result,request_cone,combat_line,outer_line,inner_line);
}
int __cdecl run_native_preschedule_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativePrescheduleThinkBindings* bindings,ServerNpcNativePrescheduleThinkResult* result) noexcept{
    return run_native_preschedule_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,bindings,result,nullptr,nullptr);
}
int __cdecl run_native_speech_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeSpeechThinkBindings* bindings,ServerNpcNativeSpeechThinkResult* result) noexcept{
    if(!result)return 0;result->speech={};result->speech.index=-1;
    if(!bindings)return 0;
    return run_native_preschedule_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->preschedule,&result->preschedule,&bindings->speech,&result->speech);
}
int __cdecl run_native_dialogue_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeDialogueThinkBindings* bindings,ServerNpcNativeDialogueThinkResult* result) noexcept{
    if(!result)return 0;
    result->cycle.speech={};result->cycle.speech.index=-1;result->outer_dialogue={};result->maintenance_dialogue={};
    if(!bindings)return 0;
    const auto& dialogue=bindings->dialogue;
    return run_native_preschedule_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->preschedule,&result->cycle.preschedule,&dialogue.sentence.speech,&result->cycle.speech,&dialogue,&result->outer_dialogue,&result->maintenance_dialogue);
}
int __cdecl run_native_squad_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeSquadThinkBindings* bindings,ServerNpcNativeSquadThinkResult* result) noexcept{
    if(!result)return 0;
    result->cycle.speech={};result->cycle.speech.index=-1;result->outer={};result->maintenance={};result->selection={};result->selections=0;
    if(!bindings)return 0;
    const auto& p=bindings->perception;
    return run_native_preschedule_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->preschedule,&result->cycle.preschedule,&p.dialogue.sentence.speech,&result->cycle.speech,nullptr,nullptr,nullptr,&p,result);
}
int __cdecl run_native_squad_report_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeSquadReportThinkBindings* bindings,ServerNpcNativeSquadReportThinkResult* result) noexcept{
    if(!result)return 0;
    auto& q=result->squad;q.cycle.speech={};q.cycle.speech.index=-1;q.outer={};q.maintenance={};q.selection={};q.selections=0;
    result->outer_reports={};result->maintenance_reports={};
    if(!bindings)return 0;
    const auto& reports=bindings->reports;const auto& p=reports.perception;
    const int ok=run_native_preschedule_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->preschedule,&q.cycle.preschedule,&p.dialogue.sentence.speech,&q.cycle.speech,nullptr,nullptr,nullptr,&p,&q,&reports,result);
    q.outer=result->outer_reports.perception;q.maintenance=result->maintenance_reports.perception;return ok;
}
int __cdecl run_native_squad_state_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeSquadStateThinkBindings* bindings,ServerNpcNativeSquadStateThinkResult* result) noexcept{
    if(!result)return 0;
    result->choose={};result->apply={};result->choose_calls=result->apply_calls=0;
    auto& report=result->think;auto& q=report.squad;
    q.cycle.speech={};q.cycle.speech.index=-1;q.outer={};q.maintenance={};q.selection={};q.selections=0;
    report.outer_reports={};report.maintenance_reports={};
    if(!bindings)return 0;
    const auto& reports=bindings->think.reports;const auto& p=reports.perception;
    auto state=bindings->state;state.squad.slots=p.slots;state.sentence=p.dialogue.sentence;state.request=p.dialogue.request;
    const int ok=run_native_preschedule_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->think.preschedule,&q.cycle.preschedule,&p.dialogue.sentence.speech,&q.cycle.speech,nullptr,nullptr,nullptr,&p,&q,&reports,&report,&state,result);
    q.outer=report.outer_reports.perception;q.maintenance=report.maintenance_reports.perception;return ok;
}
int run_native_squad_task_impl(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeSquadTaskThinkBindings* bindings,ServerNpcNativeSquadTaskThinkResult* result,const ServerNpcGruntSpecialCombatBindings* request_selector=nullptr,const ServerNpcGruntScheduleLineServices* request_line=nullptr,ServerNpcNativeRequestSchedulerResult* request_result=nullptr,const ServerNpcGruntJetSelectorBindings* jet_selector=nullptr,ServerNpcNativeJetRequestSchedulerResult* jet_result=nullptr,const ServerNpcGruntScheduleConeServices* request_cone=nullptr,const ServerNpcGruntCombatLineBindings* combat_line=nullptr,ServerNpcGruntNativeLineAttackResult* outer_line=nullptr,ServerNpcGruntNativeLineAttackResult* inner_line=nullptr) noexcept{
    if(!result)return 0;
    auto& st=result->think;st.choose={};st.apply={};st.choose_calls=st.apply_calls=0;result->speech_tasks={};
    auto& report=st.think;auto& q=report.squad;
    q.cycle.speech={};q.cycle.speech.index=-1;q.outer={};q.maintenance={};q.selection={};q.selections=0;
    report.outer_reports={};report.maintenance_reports={};
    if(!bindings)return 0;
    const auto& root=bindings->think;const auto& reports=root.think.reports;const auto& p=reports.perception;
    auto state=root.state;state.squad.slots=p.slots;state.sentence=p.dialogue.sentence;state.request=p.dialogue.request;
    auto tasks=bindings->speech_tasks;tasks.speech.sentence=p.dialogue.sentence;tasks.speech.request=p.dialogue.request;
    const int ok=run_native_preschedule_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&root.think.preschedule,&q.cycle.preschedule,&p.dialogue.sentence.speech,&q.cycle.speech,nullptr,nullptr,nullptr,&p,&q,&reports,&report,&state,&st,&tasks,&result->speech_tasks,request_selector,request_line,request_result,jet_selector,jet_result,request_cone,combat_line,outer_line,inner_line);
    q.outer=report.outer_reports.perception;q.maintenance=report.maintenance_reports.perception;return ok;
}
int __cdecl run_native_squad_task_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeSquadTaskThinkBindings* bindings,ServerNpcNativeSquadTaskThinkResult* result) noexcept{
    return run_native_squad_task_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,bindings,result);
}
int __cdecl run_native_request_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeRequestThinkBindings* bindings,ServerNpcNativeRequestThinkResult* result) noexcept{
    if(!result)return 0;
    result->requests={};
    // Reject malformed chains, wrong class or a compatibility override before outer think effects.
    if(!a||bytes<0xfd8||!b||!bindings||!bindings->selector||!bindings->line||
       !bindings->selector->combat||!bindings->selector->combat->tail)return 0;
    const auto* req=bindings->selector->combat->tail->request;
    if(!req||!req->acquisition||req->acquisition->special_translate||!req->acquisition->arbitration||
       !req->acquisition->arbitration->activation||!req->acquisition->arbitration->activation->schedule)return 0;
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    unsigned family=0;if(!cls||!cls->classify(b->scheduler.original_vtable,&family)||family==NpcSubclassJetpack||bytes<(family==NpcSubclassElite?0xfe0u:0xfd8u))return 0;
    return run_native_squad_task_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->think,&result->think,bindings->selector,bindings->line,&result->requests);
}
const ServerNpcNativeRequestThinkApi native_request_think_api{1,sizeof(ServerNpcNativeRequestThinkApi),&run_native_request_think};
int __cdecl run_native_cone_request_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeConeRequestThinkBindings* bindings,ServerNpcNativeRequestThinkResult* result) noexcept{
    if(!result)return 0;
    result->requests={};
    // Reject malformed chains, wrong class or a compatibility override before outer think effects.
    if(!a||bytes<0xfd8||!b||!bindings||!bindings->selector||!bindings->cone||!bindings->cone->line||
       !bindings->selector->combat||!bindings->selector->combat->tail)return 0;
    const auto* req=bindings->selector->combat->tail->request;
    if(!req||!req->acquisition||req->acquisition->special_translate||!req->acquisition->arbitration||
       !req->acquisition->arbitration->activation||!req->acquisition->arbitration->activation->schedule)return 0;
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    unsigned family=0;if(!cls||!cls->classify(b->scheduler.original_vtable,&family)||family==NpcSubclassJetpack||bytes<(family==NpcSubclassElite?0xfe0u:0xfd8u))return 0;
    return run_native_squad_task_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->think,&result->think,bindings->selector,bindings->cone->line,&result->requests,nullptr,nullptr,bindings->cone);
}
const ServerNpcNativeConeRequestThinkApi native_cone_request_think_api{1,sizeof(ServerNpcNativeConeRequestThinkApi),&run_native_cone_request_think};
int __cdecl run_native_combat_line_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeCombatLineThinkBindings* bindings,ServerNpcNativeRequestThinkResult* result,ServerNpcGruntNativeLineAttackResult* outer_line,ServerNpcGruntNativeLineAttackResult* inner_line) noexcept{
    if(!result||!outer_line||!inner_line||outer_line==inner_line)return 0;
    *outer_line={};*inner_line={};
    result->requests={};
    // Reject malformed chains, wrong class or a compatibility override before outer think effects.
    if(!a||bytes<0xfd8||!b||!bindings||!bindings->selector||!bindings->cone||!bindings->cone->line||!bindings->combat_line||!bindings->combat_line->admission||
       bindings->combat_line->line!=bindings->cone->line||bindings->combat_line->cone!=bindings->cone||
       !bindings->selector->combat||!bindings->selector->combat->tail)return 0;
    const auto* req=bindings->selector->combat->tail->request;
    if(!req||!req->acquisition||req->acquisition->special_translate||!req->acquisition->arbitration||
       !req->acquisition->arbitration->activation||!req->acquisition->arbitration->activation->schedule)return 0;
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    unsigned family=0;if(!cls||!cls->classify(b->scheduler.original_vtable,&family)||family==NpcSubclassJetpack||bytes<(family==NpcSubclassElite?0xfe0u:0xfd8u))return 0;
    return run_native_squad_task_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->think,&result->think,bindings->selector,bindings->cone->line,&result->requests,nullptr,nullptr,bindings->cone,bindings->combat_line,outer_line,inner_line);
}
const ServerNpcNativeCombatLineThinkApi native_combat_line_think_api{1,sizeof(ServerNpcNativeCombatLineThinkApi),&run_native_combat_line_think};
int __cdecl run_native_jet_request_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeJetRequestThinkBindings* bindings,ServerNpcNativeJetRequestThinkResult* result) noexcept{
    if(!result)return 0;
    result->requests={};
    // Reject malformed chains, wrong class or a compatibility override before outer think effects.
    if(!a||bytes<0x1085||!b||!bindings||!bindings->selector||!bindings->line||
       !bindings->selector->combat||!bindings->selector->combat->tail)return 0;
    const auto* req=bindings->selector->combat->tail->request;
    if(!req||!req->acquisition||req->acquisition->special_translate||!req->acquisition->arbitration||
       !req->acquisition->arbitration->activation||!req->acquisition->arbitration->activation->schedule)return 0;
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    unsigned family=0;if(!cls||!cls->classify(b->scheduler.original_vtable,&family)||family!=NpcSubclassJetpack)return 0;
    return run_native_squad_task_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->think,&result->think,nullptr,bindings->line,&result->requests.base,bindings->selector,&result->requests);
}
const ServerNpcNativeJetRequestThinkApi native_jet_request_think_api{1,sizeof(ServerNpcNativeJetRequestThinkApi),&run_native_jet_request_think};
int __cdecl run_native_jet_cone_request_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeJetConeRequestThinkBindings* bindings,ServerNpcNativeJetRequestThinkResult* result) noexcept{
    if(!result)return 0;
    result->requests={};
    // Reject malformed chains, wrong class or a compatibility override before outer think effects.
    if(!a||bytes<0x1085||!b||!bindings||!bindings->selector||!bindings->cone||!bindings->cone->line||
       !bindings->selector->combat||!bindings->selector->combat->tail)return 0;
    const auto* req=bindings->selector->combat->tail->request;
    if(!req||!req->acquisition||req->acquisition->special_translate||!req->acquisition->arbitration||
       !req->acquisition->arbitration->activation||!req->acquisition->arbitration->activation->schedule)return 0;
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    unsigned family=0;if(!cls||!cls->classify(b->scheduler.original_vtable,&family)||family!=NpcSubclassJetpack)return 0;
    return run_native_squad_task_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->think,&result->think,nullptr,bindings->cone->line,&result->requests.base,bindings->selector,&result->requests,bindings->cone);
}
const ServerNpcNativeJetConeRequestThinkApi native_jet_cone_request_think_api{1,sizeof(ServerNpcNativeJetConeRequestThinkApi),&run_native_jet_cone_request_think};
int __cdecl run_native_jet_combat_line_think(void* a,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,const ServerGruntHelperTaskBindings* full,const ServerNpcSharedAttackBindings* attack,const ServerNpcSharedFacingBindings* facing,const ServerNpcSharedTargetAnimationBindings* target,const ServerNpcSharedMovementBindings* movement,const ServerNpcSharedCueBindings* cue,const ServerNpcSharedObservationBindings* observe,const ServerNpcSharedSoundCoverBindings* sound_cover,const ServerNpcSharedTacticalCoverBindings* tactical,const ServerNpcNativeJetCombatLineThinkBindings* bindings,ServerNpcNativeJetRequestThinkResult* result,ServerNpcGruntNativeLineAttackResult* outer_line,ServerNpcGruntNativeLineAttackResult* inner_line) noexcept{
    if(!result||!outer_line||!inner_line||outer_line==inner_line)return 0;
    *outer_line={};*inner_line={};
    result->requests={};
    // Reject malformed chains, wrong class or a compatibility override before outer think effects.
    if(!a||bytes<0x1085||!b||!bindings||!bindings->selector||!bindings->cone||!bindings->cone->line||!bindings->combat_line||!bindings->combat_line->admission||
       bindings->combat_line->line!=bindings->cone->line||bindings->combat_line->cone!=bindings->cone||
       !bindings->selector->combat||!bindings->selector->combat->tail)return 0;
    const auto* req=bindings->selector->combat->tail->request;
    if(!req||!req->acquisition||req->acquisition->special_translate||!req->acquisition->arbitration||
       !req->acquisition->arbitration->activation||!req->acquisition->arbitration->activation->schedule)return 0;
    const auto* cls=SiN_GetServerNpcSubclassSchedulerApi(1,sizeof(ServerNpcSubclassSchedulerApi));
    unsigned family=0;if(!cls||!cls->classify(b->scheduler.original_vtable,&family)||family!=NpcSubclassJetpack)return 0;
    return run_native_squad_task_impl(a,bytes,now,image,b,full,attack,facing,target,movement,cue,observe,sound_cover,tactical,&bindings->think,&result->think,nullptr,bindings->cone->line,&result->requests.base,bindings->selector,&result->requests,bindings->cone,bindings->combat_line,outer_line,inner_line);
}
const ServerNpcNativeJetCombatLineThinkApi native_jet_combat_line_think_api{1,sizeof(ServerNpcNativeJetCombatLineThinkApi),&run_native_jet_combat_line_think};
const ServerNpcNativeSquadTaskThinkApi native_squad_task_think_api{1,sizeof(ServerNpcNativeSquadTaskThinkApi),&run_native_squad_task_think};
const ServerNpcNativeSquadStateThinkApi native_squad_state_think_api{1,sizeof(ServerNpcNativeSquadStateThinkApi),&run_native_squad_state_think};
const ServerNpcNativeSquadReportThinkApi native_squad_report_think_api{1,sizeof(ServerNpcNativeSquadReportThinkApi),&run_native_squad_report_think};
const ServerNpcNativeSquadThinkApi native_squad_think_api{1,sizeof(ServerNpcNativeSquadThinkApi),&run_native_squad_think};
const ServerNpcNativeDialogueThinkApi native_dialogue_think_api{1,sizeof(ServerNpcNativeDialogueThinkApi),&run_native_dialogue_think};
const ServerNpcNativeSpeechThinkApi native_speech_think_api{1,sizeof(ServerNpcNativeSpeechThinkApi),&run_native_speech_think};
const ServerNpcNativePrescheduleThinkApi native_preschedule_think_api{1,sizeof(ServerNpcNativePrescheduleThinkApi),&run_native_preschedule_think};
const ServerNpcThinkCatalogApi think_catalog_api{1,sizeof(ServerNpcThinkCatalogApi),&run_think_catalog};
const ServerNpcNativeGatherCatalogSchedulerApi native_gather_catalog_api{1,sizeof(ServerNpcNativeGatherCatalogSchedulerApi),&run_native_gather_catalog};
const ServerNpcTacticalCoverCatalogSchedulerApi tactical_cover_catalog_api{1,sizeof(ServerNpcTacticalCoverCatalogSchedulerApi),&run_tactical_cover};
const ServerNpcSoundCoverCatalogSchedulerApi sound_cover_catalog_api{1,sizeof(ServerNpcSoundCoverCatalogSchedulerApi),&run_sound_cover};
const ServerNpcObservationCatalogSchedulerApi observation_catalog_api{1,sizeof(ServerNpcObservationCatalogSchedulerApi),&run_observation};
const ServerNpcCueCatalogSchedulerApi cue_catalog_api{1,sizeof(ServerNpcCueCatalogSchedulerApi),&run_cue};
const ServerNpcMovementCatalogSchedulerApi movement_catalog_api{1,sizeof(ServerNpcMovementCatalogSchedulerApi),&run_movement};
const ServerNpcAnimationCatalogSchedulerApi animation_catalog_api{1,sizeof(ServerNpcAnimationCatalogSchedulerApi),&run_animation};
const ServerNpcFacingCatalogSchedulerApi facing_catalog_api{1,sizeof(ServerNpcFacingCatalogSchedulerApi),&run_facing};
const ServerNpcAttackCatalogSchedulerApi attack_catalog_api{1,sizeof(ServerNpcAttackCatalogSchedulerApi),&run_attack};
const ServerNpcSharedCatalogSchedulerApi shared_catalog_api{1,sizeof(ServerNpcSharedCatalogSchedulerApi),&run_shared};
const ServerNpcFullCatalogSchedulerApi full_catalog_api{1,sizeof(ServerNpcFullCatalogSchedulerApi),&run_full};
const ServerNpcCombatCatalogSchedulerApi combat_catalog_api{1,sizeof(ServerNpcCombatCatalogSchedulerApi),&run_combat};
const ServerNpcCatalogSchedulerApi catalog_scheduler_api{1,sizeof(ServerNpcCatalogSchedulerApi),&run_catalog};
const ServerNpcScheduleImageApi api{1,sizeof(ServerNpcScheduleImageApi),&build,&storage};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetServerNpcScheduleImageApi=_SiN_GetServerNpcScheduleImageApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleImageApi* __cdecl SiN_GetServerNpcScheduleImageApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcScheduleImageApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcCatalogSchedulerApi=_SiN_GetServerNpcCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcCatalogSchedulerApi* __cdecl SiN_GetServerNpcCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcCatalogSchedulerApi)?&catalog_scheduler_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcCombatCatalogSchedulerApi=_SiN_GetServerNpcCombatCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcCombatCatalogSchedulerApi* __cdecl SiN_GetServerNpcCombatCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcCombatCatalogSchedulerApi)?&combat_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcFullCatalogSchedulerApi=_SiN_GetServerNpcFullCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcFullCatalogSchedulerApi* __cdecl SiN_GetServerNpcFullCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcFullCatalogSchedulerApi)?&full_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSharedCatalogSchedulerApi=_SiN_GetServerNpcSharedCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSharedCatalogSchedulerApi* __cdecl SiN_GetServerNpcSharedCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSharedCatalogSchedulerApi)?&shared_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcAttackCatalogSchedulerApi=_SiN_GetServerNpcAttackCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcAttackCatalogSchedulerApi* __cdecl SiN_GetServerNpcAttackCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcAttackCatalogSchedulerApi)?&attack_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcFacingCatalogSchedulerApi=_SiN_GetServerNpcFacingCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcFacingCatalogSchedulerApi* __cdecl SiN_GetServerNpcFacingCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcFacingCatalogSchedulerApi)?&facing_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcAnimationCatalogSchedulerApi=_SiN_GetServerNpcAnimationCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcAnimationCatalogSchedulerApi* __cdecl SiN_GetServerNpcAnimationCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcAnimationCatalogSchedulerApi)?&animation_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcMovementCatalogSchedulerApi=_SiN_GetServerNpcMovementCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcMovementCatalogSchedulerApi* __cdecl SiN_GetServerNpcMovementCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcMovementCatalogSchedulerApi)?&movement_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcCueCatalogSchedulerApi=_SiN_GetServerNpcCueCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcCueCatalogSchedulerApi* __cdecl SiN_GetServerNpcCueCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcCueCatalogSchedulerApi)?&cue_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcObservationCatalogSchedulerApi=_SiN_GetServerNpcObservationCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcObservationCatalogSchedulerApi* __cdecl SiN_GetServerNpcObservationCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcObservationCatalogSchedulerApi)?&observation_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcSoundCoverCatalogSchedulerApi=_SiN_GetServerNpcSoundCoverCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSoundCoverCatalogSchedulerApi* __cdecl SiN_GetServerNpcSoundCoverCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcSoundCoverCatalogSchedulerApi)?&sound_cover_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcTacticalCoverCatalogSchedulerApi=_SiN_GetServerNpcTacticalCoverCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcTacticalCoverCatalogSchedulerApi* __cdecl SiN_GetServerNpcTacticalCoverCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcTacticalCoverCatalogSchedulerApi)?&tactical_cover_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeGatherCatalogSchedulerApi=_SiN_GetServerNpcNativeGatherCatalogSchedulerApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeGatherCatalogSchedulerApi* __cdecl SiN_GetServerNpcNativeGatherCatalogSchedulerApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeGatherCatalogSchedulerApi)?&native_gather_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcThinkCatalogApi=_SiN_GetServerNpcThinkCatalogApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcThinkCatalogApi* __cdecl SiN_GetServerNpcThinkCatalogApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcThinkCatalogApi)?&think_catalog_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativePrescheduleThinkApi=_SiN_GetServerNpcNativePrescheduleThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativePrescheduleThinkApi* __cdecl SiN_GetServerNpcNativePrescheduleThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativePrescheduleThinkApi)?&native_preschedule_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeSpeechThinkApi=_SiN_GetServerNpcNativeSpeechThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeSpeechThinkApi* __cdecl SiN_GetServerNpcNativeSpeechThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeSpeechThinkApi)?&native_speech_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeDialogueThinkApi=_SiN_GetServerNpcNativeDialogueThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeDialogueThinkApi* __cdecl SiN_GetServerNpcNativeDialogueThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeDialogueThinkApi)?&native_dialogue_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeSquadThinkApi=_SiN_GetServerNpcNativeSquadThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeSquadThinkApi* __cdecl SiN_GetServerNpcNativeSquadThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeSquadThinkApi)?&native_squad_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeSquadReportThinkApi=_SiN_GetServerNpcNativeSquadReportThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeSquadReportThinkApi* __cdecl SiN_GetServerNpcNativeSquadReportThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeSquadReportThinkApi)?&native_squad_report_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeSquadStateThinkApi=_SiN_GetServerNpcNativeSquadStateThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeSquadStateThinkApi* __cdecl SiN_GetServerNpcNativeSquadStateThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeSquadStateThinkApi)?&native_squad_state_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeSquadTaskThinkApi=_SiN_GetServerNpcNativeSquadTaskThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeSquadTaskThinkApi* __cdecl SiN_GetServerNpcNativeSquadTaskThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeSquadTaskThinkApi)?&native_squad_task_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeRequestThinkApi=_SiN_GetServerNpcNativeRequestThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeRequestThinkApi* __cdecl SiN_GetServerNpcNativeRequestThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeRequestThinkApi)?&native_request_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeConeRequestThinkApi=_SiN_GetServerNpcNativeConeRequestThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeConeRequestThinkApi* __cdecl SiN_GetServerNpcNativeConeRequestThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeConeRequestThinkApi)?&native_cone_request_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeCombatLineThinkApi=_SiN_GetServerNpcNativeCombatLineThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeCombatLineThinkApi* __cdecl SiN_GetServerNpcNativeCombatLineThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeCombatLineThinkApi)?&native_combat_line_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeJetRequestThinkApi=_SiN_GetServerNpcNativeJetRequestThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeJetRequestThinkApi* __cdecl SiN_GetServerNpcNativeJetRequestThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeJetRequestThinkApi)?&native_jet_request_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeJetConeRequestThinkApi=_SiN_GetServerNpcNativeJetConeRequestThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeJetConeRequestThinkApi* __cdecl SiN_GetServerNpcNativeJetConeRequestThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeJetConeRequestThinkApi)?&native_jet_cone_request_think_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetServerNpcNativeJetCombatLineThinkApi=_SiN_GetServerNpcNativeJetCombatLineThinkApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcNativeJetCombatLineThinkApi* __cdecl SiN_GetServerNpcNativeJetCombatLineThinkApi(std::uint32_t version,std::uint32_t size) noexcept{
    using namespace sin_port::native_weapon_tables;return version==1&&size==sizeof(ServerNpcNativeJetCombatLineThinkApi)?&native_jet_combat_line_think_api:nullptr;
}
