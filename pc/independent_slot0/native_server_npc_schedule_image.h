#pragma once
#include "native_server_npc_schedule_catalog.h"
#include "native_server_npc_schedule_services.h"
#include "native_server_grunt_helper_task.h"
#include "native_server_npc_shared_attack.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_schedule_image_export[]="SiN_GetServerNpcScheduleImageApi";
struct ServerScheduleImageRow {
    std::uint32_t descriptor[13]; // original +0 global,+4 tasks,+8 count,+c mask,+2c name,+30 next
    unsigned task_offset;
    const ServerScheduleCatalogEntry* source;
};
struct ServerScheduleImage {
    std::uint32_t token_base,head,fallback_head;
    unsigned family,row_count,task_count;
    const ServerActivityDictionary* activity_owner;
    unsigned activity_generation;
    ServerScheduleImageRow rows[46];
    ServerNpcScheduleTask tasks[270];
};
struct ServerScheduleImageBindings {
    ServerNpcConditionMapBindings namespace_map;
    const ServerGruntRegistration* registration;
    void* context;
    // Optional legacy witness callback. Retail bare DMG_BURN converts to
    // +0.0f; if supplied, the callback must agree. Not a damage flag lookup.
    int (__cdecl *damage_token)(void*,const char*,float*) noexcept;
};
struct ServerScheduleImageResult {unsigned rows,tasks,interrupts,private_activities,damage_tokens;int failed_schedule,failed_task,failed_condition;};
struct ServerNpcScheduleImageApi {
    std::uint32_t version,size;
    // Caller owns nonoverlapping token interval [base,base+137].
    // Build is transactional; old output remains unchanged on failure.
    int (__cdecl *build)(void*,unsigned,std::uint32_t,std::uint32_t,const ServerScheduleImageBindings*,ServerScheduleImage*,ServerScheduleImageResult*) noexcept;
    int (__cdecl *storage)(const ServerScheduleImage*,std::uint32_t,const void**,unsigned*) noexcept;
};
using GetServerNpcScheduleImageApi=const ServerNpcScheduleImageApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_catalog_scheduler_export[]="SiN_GetServerNpcCatalogSchedulerApi";
struct ServerNpcCatalogSchedulerResult {
    ServerNpcScheduleServicesResult runtime;
    unsigned native_reads,inherited_reads;
};
struct ServerNpcCatalogSchedulerApi {
    std::uint32_t version,size;
    // Image must remain stable while any actor retains its descriptor tokens.
    // Build with the same class namespace and registered activities as runtime.
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,ServerNpcCatalogSchedulerResult*) noexcept;
};
using GetServerNpcCatalogSchedulerApi=const ServerNpcCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_combat_catalog_scheduler_export[]="SiN_GetServerNpcCombatCatalogSchedulerApi";
struct ServerNpcCombatCatalogSchedulerResult {
    ServerNpcCatalogSchedulerResult catalog;
    ServerGruntCombatSchedulerStats combat;
};
struct ServerNpcCombatCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntCombatTaskBindings*,ServerNpcCombatCatalogSchedulerResult*) noexcept;
};
using GetServerNpcCombatCatalogSchedulerApi=const ServerNpcCombatCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_full_catalog_scheduler_export[]="SiN_GetServerNpcFullCatalogSchedulerApi";
struct ServerNpcFullCatalogSchedulerResult {
    ServerNpcCombatCatalogSchedulerResult combat;
    ServerGruntFullTaskStats full;
};
struct ServerNpcFullCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,ServerNpcFullCatalogSchedulerResult*) noexcept;
};
using GetServerNpcFullCatalogSchedulerApi=const ServerNpcFullCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_shared_catalog_scheduler_export[]="SiN_GetServerNpcSharedCatalogSchedulerApi";
struct ServerNpcSharedCatalogSchedulerResult {
    ServerNpcFullCatalogSchedulerResult full;
    ServerNpcSharedTaskStats shared;
};
struct ServerNpcSharedCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedTaskBindings*,ServerNpcSharedCatalogSchedulerResult*) noexcept;
};
using GetServerNpcSharedCatalogSchedulerApi=const ServerNpcSharedCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_attack_catalog_scheduler_export[]="SiN_GetServerNpcAttackCatalogSchedulerApi";
struct ServerNpcAttackCatalogSchedulerResult {
    ServerNpcSharedCatalogSchedulerResult shared;
    ServerNpcSharedAttackStats attack;
};
struct ServerNpcAttackCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,ServerNpcAttackCatalogSchedulerResult*) noexcept;
};
using GetServerNpcAttackCatalogSchedulerApi=const ServerNpcAttackCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_facing_catalog_scheduler_export[]="SiN_GetServerNpcFacingCatalogSchedulerApi";
struct ServerNpcFacingCatalogSchedulerResult {
    ServerNpcAttackCatalogSchedulerResult attack;
    ServerNpcSharedFacingStats facing;
};
struct ServerNpcFacingCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,ServerNpcFacingCatalogSchedulerResult*) noexcept;
};
using GetServerNpcFacingCatalogSchedulerApi=const ServerNpcFacingCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// Gameplay extensions are opt-in, keep older catalog entry points intact.
inline constexpr char server_npc_animation_catalog_scheduler_export[]="SiN_GetServerNpcAnimationCatalogSchedulerApi";
struct ServerNpcAnimationCatalogSchedulerResult {
    ServerNpcFacingCatalogSchedulerResult facing;
    ServerNpcSharedTargetAnimationStats target_animation;
};
struct ServerNpcAnimationCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,ServerNpcAnimationCatalogSchedulerResult*) noexcept;
};
using GetServerNpcAnimationCatalogSchedulerApi=const ServerNpcAnimationCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_movement_catalog_scheduler_export[]="SiN_GetServerNpcMovementCatalogSchedulerApi";
struct ServerNpcMovementCatalogSchedulerResult {
    ServerNpcAnimationCatalogSchedulerResult animation;
    ServerNpcSharedMovementStats movement;
};
struct ServerNpcMovementCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,ServerNpcMovementCatalogSchedulerResult*) noexcept;
};
using GetServerNpcMovementCatalogSchedulerApi=const ServerNpcMovementCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_cue_catalog_scheduler_export[]="SiN_GetServerNpcCueCatalogSchedulerApi";
struct ServerNpcCueCatalogSchedulerResult {
    ServerNpcMovementCatalogSchedulerResult movement;
    ServerNpcSharedCueStats cue;
};
struct ServerNpcCueCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,ServerNpcCueCatalogSchedulerResult*) noexcept;
};
using GetServerNpcCueCatalogSchedulerApi=const ServerNpcCueCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_observation_catalog_scheduler_export[]="SiN_GetServerNpcObservationCatalogSchedulerApi";
struct ServerNpcObservationCatalogSchedulerResult {
    ServerNpcCueCatalogSchedulerResult cue;
    ServerNpcSharedObservationStats observation;
};
struct ServerNpcObservationCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,const ServerNpcSharedObservationBindings*,ServerNpcObservationCatalogSchedulerResult*) noexcept;
};
using GetServerNpcObservationCatalogSchedulerApi=const ServerNpcObservationCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_sound_cover_catalog_scheduler_export[]="SiN_GetServerNpcSoundCoverCatalogSchedulerApi";
struct ServerNpcSoundCoverCatalogSchedulerResult {
    ServerNpcObservationCatalogSchedulerResult observation;
    ServerNpcSharedSoundCoverStats sound_cover;
};
struct ServerNpcSoundCoverCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,const ServerNpcSharedObservationBindings*,const ServerNpcSharedSoundCoverBindings*,ServerNpcSoundCoverCatalogSchedulerResult*) noexcept;
};
using GetServerNpcSoundCoverCatalogSchedulerApi=const ServerNpcSoundCoverCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_tactical_cover_catalog_scheduler_export[]="SiN_GetServerNpcTacticalCoverCatalogSchedulerApi";
struct ServerNpcTacticalCoverCatalogSchedulerResult {
    ServerNpcSoundCoverCatalogSchedulerResult sound_cover;
    ServerNpcSharedTacticalCoverStats tactical;
};
struct ServerNpcTacticalCoverCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,const ServerNpcSharedObservationBindings*,const ServerNpcSharedSoundCoverBindings*,const ServerNpcSharedTacticalCoverBindings*,ServerNpcTacticalCoverCatalogSchedulerResult*) noexcept;
};
using GetServerNpcTacticalCoverCatalogSchedulerApi=const ServerNpcTacticalCoverCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_native_gather_catalog_scheduler_export[]="SiN_GetServerNpcNativeGatherCatalogSchedulerApi";
struct ServerNpcNativeGatherCatalogSchedulerResult {
    ServerNpcTacticalCoverCatalogSchedulerResult scheduler;
    ServerNpcGruntGatherNativeResult gather;
    unsigned gather_calls;
};
struct ServerNpcNativeGatherCatalogSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,const ServerNpcSharedObservationBindings*,const ServerNpcSharedSoundCoverBindings*,const ServerNpcSharedTacticalCoverBindings*,const ServerNpcGruntGatherNativeBindings*,ServerNpcNativeGatherCatalogSchedulerResult*) noexcept;
};
using GetServerNpcNativeGatherCatalogSchedulerApi=const ServerNpcNativeGatherCatalogSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// Original +41c routine; debug drawing/profiling omitted, engine housekeeping hosted.
inline constexpr char server_npc_think_catalog_export[]="SiN_GetServerNpcThinkCatalogApi";
struct ServerNpcThinkCatalogBindings {
    ServerNpcGruntGatherNativeBindings gather;
    void* context;
    // Return nullptr for absent player, else the original player+0xc40 context.
    int (__cdecl *player_context)(void*,const void**,unsigned*) noexcept;
    int (__cdecl *component_player)(void*,std::uint32_t,const void*) noexcept; // component +bc
    int (__cdecl *housekeeping)(void*,void*) noexcept; // 2203a330 (engine trace)
    int (__cdecl *preschedule)(void*,void*,std::uint32_t,float) noexcept; // +424, original class
};
struct ServerNpcThinkCatalogResult {
    ServerNpcNativeGatherCatalogSchedulerResult scheduler;
    ServerNpcGruntGatherNativeResult outer_gather;
    unsigned outer_called,player_called,housekeeping_called,preschedule_called,maintenance_called,conditions_cleared,completed;
};
struct ServerNpcThinkCatalogApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,const ServerNpcSharedObservationBindings*,const ServerNpcSharedSoundCoverBindings*,const ServerNpcSharedTacticalCoverBindings*,const ServerNpcThinkCatalogBindings*,ServerNpcThinkCatalogResult*) noexcept;
};
using GetServerNpcThinkCatalogApi=const ServerNpcThinkCatalogApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_native_preschedule_think_export[]="SiN_GetServerNpcNativePrescheduleThinkApi";
struct ServerNpcNativePrescheduleThinkBindings {
    ServerNpcThinkCatalogBindings think;
    ServerNpcGruntPrescheduleBindings preschedule;
    void* jet_context;
    // Jetpack 223634e0 attachment vtable +10 and +8, not the geometry implementation.
    int (__cdecl *jet_ready)(void*,std::uint32_t,bool*) noexcept;
    int (__cdecl *jet_tick)(void*,std::uint32_t) noexcept;
};
struct ServerNpcNativePrescheduleThinkResult {
    ServerNpcThinkCatalogResult think;
    ServerNpcGruntPrescheduleResult preschedule;
    ServerEliteTaskResult elite;
    unsigned elite_called,jet_queried,jet_called;
};
struct ServerNpcNativePrescheduleThinkApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,const ServerNpcSharedObservationBindings*,const ServerNpcSharedSoundCoverBindings*,const ServerNpcSharedTacticalCoverBindings*,const ServerNpcNativePrescheduleThinkBindings*,ServerNpcNativePrescheduleThinkResult*) noexcept;
};
using GetServerNpcNativePrescheduleThinkApi=const ServerNpcNativePrescheduleThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_native_speech_think_export[]="SiN_GetServerNpcNativeSpeechThinkApi";
struct ServerNpcNativeSpeechThinkBindings {
    ServerNpcNativePrescheduleThinkBindings preschedule;
    ServerNpcGruntSpeechQueueBindings speech;
};
struct ServerNpcNativeSpeechThinkResult {
    ServerNpcNativePrescheduleThinkResult preschedule;
    ServerNpcGruntSpeechQueueResult speech;
};
struct ServerNpcNativeSpeechThinkApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,const ServerNpcSharedObservationBindings*,const ServerNpcSharedSoundCoverBindings*,const ServerNpcSharedTacticalCoverBindings*,const ServerNpcNativeSpeechThinkBindings*,ServerNpcNativeSpeechThinkResult*) noexcept;
};
using GetServerNpcNativeSpeechThinkApi=const ServerNpcNativeSpeechThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_native_dialogue_think_export[]="SiN_GetServerNpcNativeDialogueThinkApi";
struct ServerNpcNativeDialogueThinkBindings {
    ServerNpcNativePrescheduleThinkBindings preschedule;
    ServerNpcGruntPerceptionSpeechBindings dialogue;
};
struct ServerNpcNativeDialogueThinkResult {
    ServerNpcNativeSpeechThinkResult cycle;
    ServerNpcGruntPerceptionSpeechResult outer_dialogue,maintenance_dialogue;
};
struct ServerNpcNativeDialogueThinkApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,const ServerNpcSharedObservationBindings*,const ServerNpcSharedSoundCoverBindings*,const ServerNpcSharedTacticalCoverBindings*,const ServerNpcNativeDialogueThinkBindings*,ServerNpcNativeDialogueThinkResult*) noexcept;
};
using GetServerNpcNativeDialogueThinkApi=const ServerNpcNativeDialogueThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_native_squad_think_export[]="SiN_GetServerNpcNativeSquadThinkApi";
struct ServerNpcNativeSquadThinkBindings {
    ServerNpcNativePrescheduleThinkBindings preschedule;
    ServerNpcGruntPerceptionSlotsBindings perception;
};
struct ServerNpcNativeSquadThinkResult {
    ServerNpcNativeSpeechThinkResult cycle;
    ServerNpcGruntPerceptionSlotsResult outer,maintenance;
    ServerGruntNativeSlotSelectorResult selection;
    unsigned selections;
};
struct ServerNpcNativeSquadThinkApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,const ServerNpcSharedObservationBindings*,const ServerNpcSharedSoundCoverBindings*,const ServerNpcSharedTacticalCoverBindings*,const ServerNpcNativeSquadThinkBindings*,ServerNpcNativeSquadThinkResult*) noexcept;
};
using GetServerNpcNativeSquadThinkApi=const ServerNpcNativeSquadThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_native_squad_report_think_export[]="SiN_GetServerNpcNativeSquadReportThinkApi";
struct ServerNpcNativeSquadReportThinkBindings {
    ServerNpcNativePrescheduleThinkBindings preschedule;
    ServerNpcGruntSquadReportBindings reports;
};
struct ServerNpcNativeSquadReportThinkResult {
    ServerNpcNativeSquadThinkResult squad;
    ServerNpcGruntSquadReportResult outer_reports,maintenance_reports;
};
struct ServerNpcNativeSquadReportThinkApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,const ServerNpcSharedObservationBindings*,const ServerNpcSharedSoundCoverBindings*,const ServerNpcSharedTacticalCoverBindings*,const ServerNpcNativeSquadReportThinkBindings*,ServerNpcNativeSquadReportThinkResult*) noexcept;
};
using GetServerNpcNativeSquadReportThinkApi=const ServerNpcNativeSquadReportThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_native_squad_state_think_export[]="SiN_GetServerNpcNativeSquadStateThinkApi";
struct ServerNpcNativeSquadStateThinkBindings {
    ServerNpcNativeSquadReportThinkBindings think;
    ServerNpcGruntNativeStateBindings state;
};
struct ServerNpcNativeSquadStateThinkResult {
    ServerNpcNativeSquadReportThinkResult think;
    ServerNpcGruntNativeStateResult choose,apply;
    unsigned choose_calls,apply_calls;
};
struct ServerNpcNativeSquadStateThinkApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerScheduleImage*,const ServerNpcScheduleServicesBindings*,const ServerGruntHelperTaskBindings*,const ServerNpcSharedAttackBindings*,const ServerNpcSharedFacingBindings*,const ServerNpcSharedTargetAnimationBindings*,const ServerNpcSharedMovementBindings*,const ServerNpcSharedCueBindings*,const ServerNpcSharedObservationBindings*,const ServerNpcSharedSoundCoverBindings*,const ServerNpcSharedTacticalCoverBindings*,const ServerNpcNativeSquadStateThinkBindings*,ServerNpcNativeSquadStateThinkResult*) noexcept;
};
using GetServerNpcNativeSquadStateThinkApi=const ServerNpcNativeSquadStateThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
