#pragma once
#include "native_server_npc_schedule_resolution.h"
#include "native_server_npc_schedule_validity.h"
#include "native_server_npc_weapon_runtime.h"
#include "native_server_grunt_task_dispatch.h"
#include "native_server_elite_task.h"
#include "native_server_jetpack_task.h"
#include "native_server_npc_state_reconsider.h"
#include "native_server_map_entity.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_schedule_maintenance_export[]="SiN_GetServerNpcScheduleMaintenanceApi";
struct ServerNpcScheduleMaintenanceBindings {
    ServerNpcScheduleResolutionBindings selection;
    ServerNpcScheduleValidityBindings validity;
    void* context;
    bool check_inner_budget;
    int (__cdecl *invalidated)(void*,void*) noexcept; // +42c
    int (__cdecl *before_selection)(void*,void*) noexcept; // +420
    int (__cdecl *start_task)(void*,void*,const ServerNpcScheduleTask*) noexcept; // +444
    int (__cdecl *run_task)(void*,void*,const ServerNpcScheduleTask*) noexcept; // +44c
    int (__cdecl *after_start)(void*,void*) noexcept; // 2204e2c0
    int (__cdecl *motion_gate)(void*,void*,bool*) noexcept; // +610
    int (__cdecl *after_run)(void*,void*) noexcept; // 2204e400
    int (__cdecl *budget)(void*,void*,bool*) noexcept; // 2204f490, true yields
    // If supplied, native composed animation activity transition replaces
    // the hosted 22049da0 / +4dc services below.
    const ServerNpcWeaponRuntimeBindings* animation;
    int (__cdecl *transition)(void*,void*) noexcept;
    int (__cdecl *idle)(void*,void*) noexcept;
};
struct ServerNpcScheduleMaintenanceResult {
    ServerNpcScheduleStateResult state;
    ServerNpcScheduleValidityResult validity;
    ServerNpcScheduleResolutionResult selection;
    ServerNpcStateResult ideal,applied;
    ServerNpcWeaponRuntimeResult animation;
    unsigned iterations,advanced,invalidated,selected,failed_selected,started,runs,transitions;
    unsigned budget_yield,missing_schedule,iteration_limit,inner_limit;
};
struct ServerNpcScheduleMaintenanceApi {
    std::uint32_t version,size;
    // Original 220530d0 maintenance control flow, without retail RDTSC profiling.
    int (__cdecl *run)(void*,unsigned,float,const ServerNpcScheduleMaintenanceBindings*,ServerNpcScheduleMaintenanceResult*) noexcept;
};
using GetServerNpcScheduleMaintenanceApi=const ServerNpcScheduleMaintenanceApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_grunt_scheduler_export[]="SiN_GetServerGruntSchedulerApi";
struct ServerGruntSchedulerBindings {
    ServerNpcScheduleMaintenanceBindings maintenance;
    ServerGruntTaskBindings tasks;
};
struct ServerGruntSchedulerResult {
    ServerNpcScheduleMaintenanceResult maintenance;
    ServerGruntTaskResult last;
    unsigned native_tasks,parent_tasks,special_tasks,task_failures;
};
struct ServerGruntSchedulerApi {
    std::uint32_t version,size;
    // Base ground Grunt only; custom subclass overrides remain explicit.
    int (__cdecl *run)(void*,unsigned,float,const ServerGruntSchedulerBindings*,ServerGruntSchedulerResult*) noexcept;
};
using GetServerGruntSchedulerApi=const ServerGruntSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_subclass_scheduler_export[]="SiN_GetServerNpcSubclassSchedulerApi";
enum ServerNpcSubclass : unsigned {NpcSubclassGround,NpcSubclassElite,NpcSubclassJetpack};
struct ServerNpcSubclassSchedulerBindings {
    ServerNpcScheduleMaintenanceBindings maintenance;
    ServerGruntTaskBindings grunt;
    ServerEliteTaskBindings elite;
    ServerJetpackTaskBindings jetpack;
    ServerJetpackSelectorBindings jet_selector;
    // Explicit original primary table identity, not a process pointer.
    std::uint32_t original_vtable;
};
struct ServerNpcSubclassSchedulerResult {
    ServerNpcScheduleMaintenanceResult maintenance;
    ServerGruntTaskResult grunt;
    ServerEliteTaskResult elite;
    ServerJetpackTaskResult jetpack;
    ServerJetpackSelectorResult jet_selector;
    unsigned subclass,task_calls,delegated,task_failures;
};
struct ServerNpcSubclassSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *classify)(std::uint32_t,unsigned*) noexcept;
    int (__cdecl *run)(void*,unsigned,float,const ServerNpcSubclassSchedulerBindings*,ServerNpcSubclassSchedulerResult*) noexcept;
};
using GetServerNpcSubclassSchedulerApi=const ServerNpcSubclassSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Standalone opt-in composed scripted scheduler. Original v1
// maintenance, subclass and shared-task binding sizes remain unchanged.
struct ServerNpcSharedTaskBindings;
struct ServerNpcScriptedSchedulerStats;
struct ServerNpcScriptedSchedulerBindings {
    const ServerNpcSubclassSchedulerBindings* scheduler;
    const ServerNpcSharedTaskBindings* shared;
    const native_map::ScriptedNpcTaskBindings* run;
    const native_map::ScriptedNpcStartTaskBindings* start;
};
inline constexpr char server_npc_scripted_scheduler_export[]="SiN_RunServerNpcScriptedScheduler";
using RunServerNpcScriptedScheduler=int (__cdecl *)(void*,unsigned,float,const ServerNpcScriptedSchedulerBindings*,
    ServerNpcSubclassSchedulerResult*,ServerNpcScriptedSchedulerStats*) noexcept;
struct ServerNpcScriptedMotionSchedulerStats;
inline constexpr char server_npc_scripted_motion_scheduler_export[]="SiN_RunServerNpcScriptedSchedulerWithMotion";
using RunServerNpcScriptedSchedulerWithMotion=int (__cdecl *)(void*,unsigned,float,const ServerNpcScriptedSchedulerBindings*,
    ServerNpcSubclassSchedulerResult*,ServerNpcScriptedSchedulerStats*,const native_map::ScriptedNpcStartMotionBindings*,
    ServerNpcScriptedMotionSchedulerStats*) noexcept;
inline constexpr char server_npc_scripted_cleanup_scheduler_export[]="SiN_RunServerNpcScriptedSchedulerWithCleanup";
using RunServerNpcScriptedSchedulerWithCleanup=int (__cdecl *)(void*,unsigned,float,const ServerNpcScriptedSchedulerBindings*,
    ServerNpcSubclassSchedulerResult*,ServerNpcScriptedSchedulerStats*,const native_map::ScriptedNpcStartMotionBindings*,
    ServerNpcScriptedMotionSchedulerStats*,const native_map::ScriptedNpcCleanupBindings*,native_map::ScriptedNpcCleanupResult*) noexcept;
// Opt-in explicit navigation sidecar; no previous binding layout changes.
struct ServerNpcSharedFacingBindings;
struct ServerNpcSharedFacingStats;
struct ServerNpcSharedMovementBindings;
struct ServerNpcSharedMovementStats;
struct ServerNpcScriptedNavigationStats;
struct ServerNpcScriptedNavigationServices {
    const native_map::ScriptedNpcMoveBindings* move;
    const native_map::ScriptedNpcArrivalBindings* arrival;
    const ServerNpcSharedFacingBindings* facing;
    const ServerNpcSharedMovementBindings* movement;
    ServerNpcSharedFacingStats* facing_stats;
    ServerNpcSharedMovementStats* movement_stats;
    ServerNpcScriptedNavigationStats* stats;
};
inline constexpr char server_npc_scripted_navigation_scheduler_export[]="SiN_RunServerNpcScriptedSchedulerWithNavigation";
using RunServerNpcScriptedSchedulerWithNavigation=int (__cdecl *)(void*,unsigned,float,const ServerNpcScriptedSchedulerBindings*,
    ServerNpcSubclassSchedulerResult*,ServerNpcScriptedSchedulerStats*,const native_map::ScriptedNpcStartMotionBindings*,
    ServerNpcScriptedMotionSchedulerStats*,const native_map::ScriptedNpcCleanupBindings*,native_map::ScriptedNpcCleanupResult*,
    const ServerNpcScriptedNavigationServices*) noexcept;
// New ground-Grunt path connecting original +494/+498 requests to actual task maintenance.
inline constexpr char server_npc_native_request_scheduler_export[]="SiN_GetServerNpcNativeRequestSchedulerApi";
struct ServerGruntHelperTaskBindings;
struct ServerGruntCombatSchedulerStats;
struct ServerGruntFullTaskStats;
struct ServerNpcNativeRequestSchedulerBindings {
    ServerNpcSubclassSchedulerBindings scheduler;
    const ServerNpcGruntSpecialCombatBindings* selector;
    const ServerNpcGruntScheduleLineServices* line;
    const ServerGruntHelperTaskBindings* full_tasks; // optional composed 26/11 custom task routes
};
struct ServerNpcNativeRequestSchedulerResult {
    ServerNpcSubclassSchedulerResult scheduler;
    ServerNpcGruntScheduleRequestResult request;
    ServerNpcGruntSelectorTailResult tail;
    ServerNpcGruntCombatSelectorResult combat;
    ServerNpcGruntSpecialCombatResult special;
    unsigned requests,failure_requests;
};
struct ServerNpcNativeRequestSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerNpcNativeRequestSchedulerBindings*,ServerNpcNativeRequestSchedulerResult*,ServerGruntCombatSchedulerStats*,ServerGruntFullTaskStats*) noexcept;
};
using GetServerNpcNativeRequestSchedulerApi=const ServerNpcNativeRequestSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_native_jet_request_scheduler_export[]="SiN_GetServerNpcNativeJetRequestSchedulerApi";
struct ServerNpcNativeJetRequestSchedulerBindings {
    ServerNpcSubclassSchedulerBindings scheduler;
    const ServerNpcGruntJetSelectorBindings* selector;
    const ServerNpcGruntScheduleLineServices* line;
    const ServerGruntHelperTaskBindings* full_tasks;
};
struct ServerNpcNativeJetRequestSchedulerResult {
    ServerNpcNativeRequestSchedulerResult base;
    ServerNpcGruntJetSelectorResult tail,special;
};
struct ServerNpcNativeJetRequestSchedulerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerNpcNativeJetRequestSchedulerBindings*,ServerNpcNativeJetRequestSchedulerResult*,ServerGruntCombatSchedulerStats*,ServerGruntFullTaskStats*) noexcept;
};
using GetServerNpcNativeJetRequestSchedulerApi=const ServerNpcNativeJetRequestSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_gather_scheduler_export[]="SiN_GetServerNpcGatherSchedulerApi";
struct ServerNpcGatherSchedulerBindings {
    ServerNpcSubclassSchedulerBindings scheduler;
    ServerNpcGruntGatherConditionsBindings gather;
    ServerNpcGruntPointPolicyBindings point;
};
struct ServerNpcGatherSchedulerResult {
    ServerNpcSubclassSchedulerResult scheduler;
    ServerNpcGruntGatherConditionsResult gather;
    ServerNpcGruntPointPolicyResult point;
    unsigned gather_calls;
};
struct ServerNpcGatherSchedulerApi {
    std::uint32_t version,size;
    // Original 22053254 +420 call site; no unconditional outer-think call.
    int (__cdecl *run)(void*,unsigned,float,const ServerNpcGatherSchedulerBindings*,ServerNpcGatherSchedulerResult*) noexcept;
};
using GetServerNpcGatherSchedulerApi=const ServerNpcGatherSchedulerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// Internal synchronous route; no change to legacy scheduler binding layouts.
struct ServerNpcSharedNativeSpeechStats;
// Explicit, synchronous sidecar for the complete combat-think route.
struct ServerNpcScriptedThinkServices {
    const native_map::ScriptedNpcTaskBindings* run;
    const native_map::ScriptedNpcStartTaskBindings* start;
    const native_map::ScriptedNpcStartMotionBindings* motion;
    const native_map::ScriptedNpcCleanupBindings* cleanup;
    ServerNpcScriptedSchedulerStats* stats;
    ServerNpcScriptedMotionSchedulerStats* motion_stats;
    native_map::ScriptedNpcCleanupResult* cleaned;
};
inline bool valid_scripted_think_services(const ServerNpcScriptedThinkServices* s) noexcept {
    return s&&s->run&&s->start&&s->start->task==s->run&&s->stats&&s->run->finish&&
        s->run->finish->animation&&s->run->finish->animation->control&&s->run->finish->possession&&
        bool(s->motion)==bool(s->motion_stats)&&(!s->motion||s->motion->base==s->start)&&
        bool(s->cleanup)==bool(s->cleaned)&&(!s->cleanup||s->cleanup->control==s->run->finish->animation->control);
}
struct ServerNpcNativeGatherRoute {
    const ServerNpcGruntGatherNativeBindings* bindings;
    ServerNpcGruntGatherNativeResult* result;
    unsigned* calls;
    const ServerNpcGruntPerceptionSpeechBindings* dialogue=nullptr;
    ServerNpcGruntPerceptionSpeechResult* dialogue_result=nullptr;
    const ServerNpcGruntPerceptionSlotsBindings* slots=nullptr;
    ServerNpcGruntPerceptionSlotsResult* slots_result=nullptr;
    ServerGruntNativeSlotSelectorResult* slot_selection=nullptr;
    unsigned* slot_selections=nullptr;
    const ServerNpcGruntSquadReportBindings* reports=nullptr;
    ServerNpcGruntSquadReportResult* report_result=nullptr;
    const ServerNpcGruntNativeStateBindings* native_state=nullptr;
    ServerNpcGruntNativeStateResult* choose_result=nullptr;
    ServerNpcGruntNativeStateResult* apply_result=nullptr;
    unsigned* choose_calls=nullptr;
    unsigned* apply_calls=nullptr;
    const ServerNpcGruntEventSpeechBindings* speech_tasks=nullptr;
    ServerNpcSharedNativeSpeechStats* speech_stats=nullptr;
    const ServerNpcGruntSpecialCombatBindings* request_selector=nullptr;
    const ServerNpcGruntScheduleLineServices* request_line=nullptr;
    ServerNpcNativeRequestSchedulerResult* request_result=nullptr;
    const ServerNpcGruntJetSelectorBindings* jet_request_selector=nullptr;
    ServerNpcNativeJetRequestSchedulerResult* jet_request_result=nullptr;
    const ServerNpcGruntScheduleConeServices* request_cone=nullptr;
    const ServerNpcGruntCombatLineBindings* combat_line=nullptr;
    ServerNpcGruntNativeLineAttackResult* combat_line_result=nullptr;
    const ServerNpcWeaponPolicyBindings* weapon_policy=nullptr;
    ServerNpcWeaponPolicyResult* weapon_policy_result=nullptr;
    const ServerNpcScriptedThinkServices* scripted=nullptr;
};
}
