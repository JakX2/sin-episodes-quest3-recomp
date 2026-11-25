#include "native_server_npc_schedule_image.h"
#include <cmath>
#include <cstring>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleCatalogApi* __cdecl SiN_GetServerNpcScheduleCatalogApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleNamespaceApi* __cdecl SiN_GetServerNpcScheduleNamespaceApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcConditionApi* __cdecl SiN_GetServerNpcConditionApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerActivityDictionaryApi* __cdecl SiN_GetServerActivityDictionaryApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcScheduleServicesApi* __cdecl SiN_GetServerNpcScheduleServicesApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ServerNpcSubclassSchedulerApi* __cdecl SiN_GetServerNpcSubclassSchedulerApi(std::uint32_t,std::uint32_t) noexcept;
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
struct CatalogCall {
    const ServerScheduleImage* image;
    const ServerNpcScheduleBindings* original;
    ServerNpcCatalogSchedulerResult* result;
};
int run_catalog_impl(void* actor,unsigned bytes,float now,const ServerScheduleImage* image,const ServerNpcScheduleServicesBindings* b,ServerNpcCatalogSchedulerResult* out,const ServerGruntCombatTaskBindings* combat=nullptr,ServerGruntCombatSchedulerStats* stats=nullptr,const ServerGruntHelperTaskBindings* full=nullptr,ServerGruntFullTaskStats* full_stats=nullptr,const ServerNpcSharedTaskBindings* shared=nullptr,ServerNpcSharedTaskStats* shared_stats=nullptr,const ServerNpcSharedAttackBindings* attack=nullptr,ServerNpcSharedAttackStats* attack_stats=nullptr,const ServerNpcSharedFacingBindings* facing=nullptr,ServerNpcSharedFacingStats* facing_stats=nullptr,const ServerNpcSharedTargetAnimationBindings* target_animation=nullptr,ServerNpcSharedTargetAnimationStats* target_animation_stats=nullptr,const ServerNpcSharedMovementBindings* movement=nullptr,ServerNpcSharedMovementStats* movement_stats=nullptr,const ServerNpcSharedCueBindings* cue=nullptr,ServerNpcSharedCueStats* cue_stats=nullptr,const ServerNpcSharedObservationBindings* observe=nullptr,ServerNpcSharedObservationStats* observe_stats=nullptr,const ServerNpcSharedSoundCoverBindings* sound_cover=nullptr,ServerNpcSharedSoundCoverStats* sound_cover_stats=nullptr,const ServerNpcSharedTacticalCoverBindings* tactical=nullptr,ServerNpcSharedTacticalCoverStats* tactical_stats=nullptr) noexcept {
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
    if(combat)return run_schedule_services_combat_internal(actor,bytes,now,&bound,&out->runtime,combat,stats,full,full_stats,shared,shared_stats,attack,attack_stats,facing,facing_stats,target_animation,target_animation_stats,movement,movement_stats,cue,cue_stats,observe,observe_stats,sound_cover,sound_cover_stats,tactical,tactical_stats);
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
