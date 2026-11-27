#pragma once
#include "native_server_grunt_task_dispatch.h"
#include "native_server_npc_schedule_resolution.h"
#include "native_server_map_entity.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_shared_task_export[]="SiN_GetServerNpcSharedTaskApi";
struct ServerNpcSharedTaskBindings {
    ServerNpcTaskLifecycleBindings lifecycle;
    const ServerNpcTransitionBindings* transition;
    const ServerNpcScheduleResolutionBindings* resolution;
    void* context;
    int (__cdecl *random)(void*,int,int,int*) noexcept;
    int (__cdecl *component_storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *component_task)(void*,std::uint32_t,const ServerNpcScheduleTask*,bool) noexcept;
    int (__cdecl *special67)(void*,void*,const ServerNpcScheduleTask*,bool) noexcept;
    int (__cdecl *parent)(void*,void*,const ServerNpcScheduleTask*,bool) noexcept; // shared dispatcher unresolved task
    int (__cdecl *pending)(void*,void*,int) noexcept;
    int (__cdecl *aim_query)(void*,void*,bool*) noexcept;
    int (__cdecl *aim_update)(void*,void*) noexcept;
    int (__cdecl *turn)(void*,void*) noexcept;
};
struct ServerNpcSharedTaskResult {
    unsigned native,component_called,component_handled,special_called,parent_called,timer_written,aim_called,turn_called,schedule_set;
    int task,activity,ticks,failed_code;
    ServerNpcTaskLifecycleResult lifecycle;
    ServerNpcTransitionResult transition;
    ServerNpcScheduleResolutionResult resolution;
    ServerNpcScheduleStateResult schedule;
};
struct ServerNpcSharedTaskApi {
    std::uint32_t version,size;
    // Original 220325c0/220326c0/220326d0/220326f0.
    int (__cdecl *wait)(void*,unsigned,float,float,float,const ServerNpcSharedTaskBindings*,int*) noexcept;
    int (__cdecl *cancel_wait)(void*,unsigned) noexcept;
    int (__cdecl *wait_state)(const void*,unsigned,float,unsigned*,unsigned*) noexcept;
    int (__cdecl *supports)(int,bool) noexcept;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerNpcSharedTaskBindings*,ServerNpcSharedTaskResult*) noexcept;
};
using GetServerNpcSharedTaskApi=const ServerNpcSharedTaskApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// Opt-in scripted NPC task dispatcher retains the original component
// delegation gate. Other shared tasks still use the ordinary dispatcher.
struct ServerNpcSharedScriptedTaskResult {
    ServerNpcSharedTaskResult shared;
    native_map::ScriptedNpcTaskResult script;
};
inline constexpr char server_npc_shared_scripted_task_export[]="SiN_ExecuteServerNpcSharedScriptedTask";
using ExecuteServerNpcSharedScriptedTask=int (__cdecl *)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,
    const ServerNpcSharedTaskBindings*,const native_map::ScriptedNpcTaskBindings*,ServerNpcSharedScriptedTaskResult*) noexcept;
struct ServerNpcSharedScriptedStartResult {
    ServerNpcSharedTaskResult shared;
    native_map::ScriptedNpcStartTaskResult script;
};
inline constexpr char server_npc_shared_scripted_start_export[]="SiN_ExecuteServerNpcSharedScriptedStartTask";
using ExecuteServerNpcSharedScriptedStartTask=int (__cdecl *)(void*,unsigned,const ServerNpcScheduleTask*,float,
    const ServerNpcSharedTaskBindings*,const native_map::ScriptedNpcStartTaskBindings*,ServerNpcSharedScriptedStartResult*) noexcept;
struct ServerNpcScriptedSchedulerStats {
    unsigned starts,runs,native_starts,native_runs,component_handled;
    ServerNpcSharedScriptedStartResult last_start;
    ServerNpcSharedScriptedTaskResult last_run;
};
struct ServerNpcScriptedMotionSchedulerStats {
    unsigned starts,native_starts,component_handled;
    ServerNpcSharedTaskResult shared;
    native_map::ScriptedNpcStartMotionResult last;
};
// Facing service extends the shared dispatcher without altering the v1 ABI.
inline constexpr char server_npc_shared_facing_export[]="SiN_GetServerNpcSharedFacingApi";
struct ServerNpcSharedFacingBindings {
    void* context;
    int (__cdecl *motor_storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *aim_vector)(void*,void*,float[3]) noexcept; // 22031780
    int (__cdecl *aligned)(void*,void*,const float*,bool*) noexcept; // actor +304
    int (__cdecl *vector_yaw)(void*,void*,const float*,float*) noexcept; // actor +5dc
    int (__cdecl *desired_yaw)(void*,void*,bool,double*) noexcept; // actor +754, mode false for task57
    int (__cdecl *begin_turn)(void*,void*) noexcept; // actor +71c
    int (__cdecl *step_turn)(void*,void*,int) noexcept; // motor +48
};
struct ServerNpcSharedFacingResult {
    ServerNpcSharedTaskResult shared;
    unsigned queried,vector_written,yaw_written,begun,stepped,completed;
    float current_quantized;
    double error;
};
struct ServerNpcSharedFacingApi {
    std::uint32_t version,size;
    // 2206d1b0 / 2226c630. Signed error is desired minus quantized actual yaw.
    int (__cdecl *yaw_error)(float actual,float desired,float*,double*) noexcept;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerNpcSharedTaskBindings*,const ServerNpcSharedFacingBindings*,ServerNpcSharedFacingResult*) noexcept;
};
using GetServerNpcSharedFacingApi=const ServerNpcSharedFacingApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_shared_target_animation_export[]="SiN_GetServerNpcSharedTargetAnimationApi";
struct ServerNpcSharedTargetAnimationBindings {
    void* context;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    int (__cdecl *current_target)(void*,void*,void**) noexcept; // +154; null is valid
    int (__cdecl *absolute_position)(void*,void*,float[3]) noexcept; // 220092d0 incl. dirty transform
    int (__cdecl *motor_storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *step_turn)(void*,void*,int) noexcept; // +48(-1)
};
struct ServerNpcSharedTargetAnimationResult {
    ServerNpcSharedTaskResult shared;
    unsigned target_live,target_handle,motor_written,stepped,animation_finished;
    float direction[3],yaw;
};
struct ServerNpcSharedTargetAnimationApi {
    std::uint32_t version,size;
    // Run94/95/96 (22053937), Start remains in the shared v1 API.
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,float,const ServerNpcSharedTaskBindings*,const ServerNpcSharedTargetAnimationBindings*,ServerNpcSharedTargetAnimationResult*) noexcept;
};
using GetServerNpcSharedTargetAnimationApi=const ServerNpcSharedTargetAnimationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_shared_movement_export[]="SiN_GetServerNpcSharedMovementApi";
struct ServerNpcSharedMovementBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *clear_path)(void*,void* navigator) noexcept; // 22077e60
    int (__cdecl *stop_path)(void*,void* navigator,bool finished) noexcept; // 22079250
    int (__cdecl *try_stopping_path)(void*,void* navigator,bool*) noexcept; // 220754e0
    int (__cdecl *pose_lookup)(void*,void*,const char*,int*) noexcept; // 220ecf20
    int (__cdecl *pose_set)(void*,void*,const char*,float) noexcept; // 220f0b40
    int (__cdecl *velocity)(void*,void*,float[3]) noexcept; // +1e4
    int (__cdecl *movement_update)(void*,void*) noexcept; // +60c
    int (__cdecl *task_fail)(void*,void*,int) noexcept; // +47c
};
struct ServerNpcSharedMovementResult {
    ServerNpcSharedTaskResult shared,nested;
    unsigned nested_calls,clear_calls,stop_calls,try_calls,pose_calls,move_calls,mode_cleared,landed,stuck,timed_out;
    int navigator_mode,idle_activity;
    double speed;
};
struct ServerNpcSharedMovementApi {
    std::uint32_t version,size;
    // Original Start43/44/89/90/116/121/122 and Run116/121/122; engine effects remain bound.
    // Start43/44 require transition->assignment.translation (original 22049870).
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerNpcSharedTaskBindings*,const ServerNpcSharedMovementBindings*,ServerNpcSharedMovementResult*) noexcept;
};
using GetServerNpcSharedMovementApi=const ServerNpcSharedMovementApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_difficulty_wait_export[]="SiN_GetServerNpcDifficultyWaitApi";
struct ServerNpcDifficultyWaitApi {
    std::uint32_t version,size;
    // Task138 uses the caller's current metric11 (including adaptive values).
    // Metric is ignored on Run; fixed campaign integration is optional.
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,float,const ServerNpcSharedTaskBindings*,ServerNpcSharedTaskResult*) noexcept;
};
using GetServerNpcDifficultyWaitApi=const ServerNpcDifficultyWaitApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_shared_cue_export[]="SiN_GetServerNpcSharedCueApi";
struct ServerNpcSharedCueBindings {
    void* context;
    std::uint32_t original_vtable;
    int (__cdecl *random)(void*,int,int,int*) noexcept;
    int (__cdecl *target)(void*,void*,void**) noexcept; // +154
    int (__cdecl *as_npc)(void*,void*,void**) noexcept; // target +fc
    int (__cdecl *squad_gate)(void*,std::uint32_t,int,bool*) noexcept; // 220a7cd0
    int (__cdecl *center)(void*,void*,float[3]) noexcept; // +214
    int (__cdecl *target_gate)(void*,void*,void*,bool*) noexcept; // target +300(actor)
    int (__cdecl *visible)(void*,void*,void*,unsigned,bool*) noexcept; // +1f0(target,0x4081,null)
    int (__cdecl *path_distance)(void*,std::uint32_t,float*) noexcept; // navigator +24 / 2209d410
    int (__cdecl *cue)(void*,void*,const char*,int,int) noexcept; // 220a35c0 on +cc0
};
struct ServerNpcSharedCueResult {
    ServerNpcSharedTaskResult shared;
    int mode,draw;
    unsigned gate,live,within_range,target_allowed,visible,emitted;
    double distance_squared;
    float path_distance;
};
struct ServerNpcSharedCueApi {
    std::uint32_t version,size;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,float,const ServerNpcSharedTaskBindings*,const ServerNpcSharedCueBindings*,ServerNpcSharedCueResult*) noexcept;
};
using GetServerNpcSharedCueApi=const ServerNpcSharedCueApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_shared_native_speech_export[]="SiN_GetServerNpcSharedNativeSpeechApi";
struct ServerNpcSharedNativeSpeechResult {
    ServerNpcSharedTaskResult shared;
    ServerNpcGruntSentenceResult idle;
    ServerNpcGruntSentenceRequestResult idle_request;
    ServerNpcGruntRequestSpeechResult alert;
    ServerNpcGruntReactionSpeechResult reaction;
    ServerNpcGruntEventSpeechResult event;
    int mode;
};
struct ServerNpcSharedNativeSpeechApi {
    std::uint32_t version,size;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,float,const ServerNpcSharedTaskBindings*,const ServerNpcGruntEventSpeechBindings*,ServerNpcSharedNativeSpeechResult*) noexcept;
};
using GetServerNpcSharedNativeSpeechApi=const ServerNpcSharedNativeSpeechApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct ServerNpcSharedNativeSpeechStats {ServerNpcSharedNativeSpeechResult last;unsigned calls,native,components;};
inline constexpr char server_npc_shared_observation_export[]="SiN_GetServerNpcSharedObservationApi";
struct ServerNpcSharedObservationBindings {
    void* context;
    std::uint32_t original_vtable;
    // Component +c0; shared component_storage resolves the same token.
    int (__cdecl *component_gate)(void*,std::uint32_t,bool*) noexcept;
    int (__cdecl *pvs_query)(void*,std::uint32_t,bool*) noexcept; // 222895a0
    int (__cdecl *target)(void*,void*,void**) noexcept; // +154
    // 2206b1a0 lookup with fallback enabled; record +1c if found.
    int (__cdecl *memory_record)(void*,std::uint32_t,void*,bool*,float*) noexcept;
};
struct ServerNpcSharedObservationResult {
    ServerNpcSharedTaskResult shared;
    unsigned gate_component,gate_handled,flag_gate,pvs_called,pvs,target_calls,memory_called,memory_found;
    float last_seen;
    double elapsed;
};
struct ServerNpcSharedObservationApi {
    std::uint32_t version,size;
    // Original Run6; Start6 is the deferred shared state route.
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,float,const ServerNpcSharedTaskBindings*,const ServerNpcSharedObservationBindings*,ServerNpcSharedObservationResult*) noexcept;
};
using GetServerNpcSharedObservationApi=const ServerNpcSharedObservationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_shared_sound_cover_export[]="SiN_GetServerNpcSharedSoundCoverApi";
// Original navigator goal constructor 2204c660, fixed PE32 data layout.
struct ServerNpcSoundCoverGoal {
    int type;float position[3];int target,activity,aux1,aux2;
    float tolerance,range;unsigned flags,context_token;
};
static_assert(sizeof(ServerNpcSoundCoverGoal)==0x30);
struct ServerNpcSharedSoundCoverBindings {
    void* context;
    std::uint32_t original_vtable;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *best_sound)(void*,void*,unsigned,const void**,unsigned*) noexcept; // +504
    int (__cdecl *cover_position)(void*,void*,const void*,float[3],bool*) noexcept; // +698
    int (__cdecl *prepare_navigation)(void*,std::uint32_t) noexcept; // 22074440
    int (__cdecl *sound_cleared)(void*,void*) noexcept; // +4f4
    int (__cdecl *goal_context)(void*,std::uint32_t*) noexcept; // original process-wide 224de6f8
    int (__cdecl *submit_goal)(void*,std::uint32_t,const ServerNpcSoundCoverGoal*,bool*) noexcept; // nav +14, false argument
};
struct ServerNpcSharedSoundCoverResult {
    ServerNpcSharedTaskResult shared;
    int stage;
    unsigned sound_queries,position_found,prepared,cache_cleared,cache_copied,advanced,submitted,accepted,deadline_written;
    ServerNpcSoundCoverGoal goal;
};
struct ServerNpcSharedSoundCoverApi {
    std::uint32_t version,size;
    // Original Run97. Start97 is the existing shared deferred route.
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,float,const ServerNpcSharedTaskBindings*,const ServerNpcSharedSoundCoverBindings*,ServerNpcSharedSoundCoverResult*) noexcept;
};
using GetServerNpcSharedSoundCoverApi=const ServerNpcSharedSoundCoverApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_shared_tactical_cover_export[]="SiN_GetServerNpcSharedTacticalCoverApi";
struct ServerNpcSharedTacticalCoverBindings {
    void* context;
    std::uint32_t original_vtable;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *default_range)(void*,void*,float*) noexcept; // +6b4
    int (__cdecl *absolute_origin)(void*,void*,float[3]) noexcept; // dirty transform / +2bc
    int (__cdecl *viewpoint)(void*,void*,float[3]) noexcept; // +1c4
    int (__cdecl *search)(void*,std::uint32_t,const float*,const float*,float,float,float[3],bool*) noexcept; // 220acb10
    int (__cdecl *special_cover)(void*,void*,void*,float[3],bool*) noexcept; // +69c
    int (__cdecl *goal_context)(void*,std::uint32_t*) noexcept;
    int (__cdecl *submit_goal)(void*,std::uint32_t,const ServerNpcSoundCoverGoal*,bool*) noexcept;
    int (__cdecl *hint_activity)(void*,void*,void*,int*) noexcept; // +714
    int (__cdecl *hint_direction)(void*,void*,float[3]) noexcept; // 22065050 then normalize
};
struct ServerNpcSharedTacticalCoverResult {
    ServerNpcSharedTaskResult shared;
    unsigned search_called,special_called,hint_released,hint_cleared,hint_applied,position_found,submitted,accepted,deadline_written;
    float minimum,maximum;
    ServerNpcSoundCoverGoal goal;
};
struct ServerNpcSharedTacticalCoverApi {
    std::uint32_t version,size;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,float,const ServerNpcSharedTaskBindings*,const ServerNpcSharedTacticalCoverBindings*,ServerNpcSharedTacticalCoverResult*) noexcept;
};
using GetServerNpcSharedTacticalCoverApi=const ServerNpcSharedTacticalCoverApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_shared_enemy_refresh_export[]="SiN_GetServerNpcSharedEnemyRefreshApi";
struct ServerNpcSharedEnemyRefreshBindings {
    void* context;
    std::uint32_t original_vtable;
    ServerNpcEnemyStateBindings enemy;
    int (__cdecl *memory_record)(void*,std::uint32_t,void*,bool*,float*) noexcept;
    // +518 routes to component +9c or original candidate scorer 2203d3e0.
    int (__cdecl *select_enemy)(void*,void*,std::uint32_t,void**) noexcept;
};
struct ServerNpcSharedEnemyRefreshResult {
    ServerNpcSharedTaskResult shared;
    ServerNpcEnemyStateResult enemy;
    unsigned current_live,memory_found,selected,replaced,cleared,extra_notification,state_changed;
    float last_seen;
};
struct ServerNpcSharedEnemyRefreshApi {
    std::uint32_t version,size;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,float,const ServerNpcSharedTaskBindings*,const ServerNpcSharedEnemyRefreshBindings*,ServerNpcSharedEnemyRefreshResult*) noexcept;
};
using GetServerNpcSharedEnemyRefreshApi=const ServerNpcSharedEnemyRefreshApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct ServerNpcSharedTacticalCoverStats {ServerNpcSharedTacticalCoverResult last;unsigned calls,native,components;};
struct ServerNpcSharedSoundCoverStats {ServerNpcSharedSoundCoverResult last;unsigned calls,native,components;};
struct ServerNpcSharedObservationStats {ServerNpcSharedObservationResult last;unsigned calls,native,components;};
struct ServerNpcSharedCueStats {ServerNpcSharedCueResult last;unsigned calls,native,components;};
struct ServerNpcSharedMovementStats {ServerNpcSharedMovementResult last;unsigned calls,native,components;};
struct ServerNpcSharedTargetAnimationStats {ServerNpcSharedTargetAnimationResult last;unsigned calls,native,components;};
struct ServerNpcSharedFacingStats {ServerNpcSharedFacingResult last;unsigned calls,native,components;};
struct ServerNpcSharedTaskStats {ServerNpcSharedTaskResult last;unsigned calls,native,component_handled,special,parent;};
// Per-maintenance call only; no callback or actor memory retains the session.
struct ServerNpcSharedAttackBindings;
struct ServerNpcSharedAttackStats;
struct ServerNpcGruntScheduleConeServices;
struct ServerNpcSharedParentSession {
    void* actor;unsigned bytes;float now;
    ServerGruntTaskBindings original;
    ServerNpcSharedTaskBindings shared;
    ServerNpcSharedTaskStats* stats;
    const ServerNpcSharedAttackBindings* attack;
    ServerNpcSharedAttackStats* attack_stats;
    const ServerNpcSharedFacingBindings* facing;
    ServerNpcSharedFacingStats* facing_stats;
    const ServerNpcSharedTargetAnimationBindings* target_animation;
    ServerNpcSharedTargetAnimationStats* target_animation_stats;
    const ServerNpcSharedMovementBindings* movement;
    ServerNpcSharedMovementStats* movement_stats;
    const ServerNpcSharedCueBindings* cue;
    ServerNpcSharedCueStats* cue_stats;
    const ServerNpcSharedObservationBindings* observe;
    ServerNpcSharedObservationStats* observe_stats;
    const ServerNpcSharedSoundCoverBindings* sound_cover;
    ServerNpcSharedSoundCoverStats* sound_cover_stats;
    const ServerNpcSharedTacticalCoverBindings* tactical;
    ServerNpcSharedTacticalCoverStats* tactical_stats;
    const ServerNpcGruntEventSpeechBindings* native_speech=nullptr;
    ServerNpcSharedNativeSpeechStats* native_speech_stats=nullptr;
    const ServerNpcGruntScheduleAcquireBindings* native_acquire=nullptr;
    const ServerNpcGruntScheduleLineServices* native_line=nullptr;
    const ServerNpcGruntScheduleConeServices* native_cone=nullptr;
    const native_map::ScriptedNpcTaskBindings* script_run=nullptr;
    const native_map::ScriptedNpcStartTaskBindings* script_start=nullptr;
    ServerNpcScriptedSchedulerStats* script_stats=nullptr;
    const native_map::ScriptedNpcStartMotionBindings* script_motion=nullptr;
    ServerNpcScriptedMotionSchedulerStats* script_motion_stats=nullptr;
};
int bind_shared_parent_internal(void*,unsigned,float,const ServerGruntTaskBindings*,const ServerNpcScheduleResolutionBindings*,const ServerNpcSharedTaskBindings*,ServerNpcSharedTaskStats*,ServerNpcSharedParentSession*,ServerGruntTaskBindings*,const ServerNpcSharedAttackBindings* = nullptr,ServerNpcSharedAttackStats* = nullptr,const ServerNpcSharedFacingBindings* = nullptr,ServerNpcSharedFacingStats* = nullptr,const ServerNpcSharedTargetAnimationBindings* = nullptr,ServerNpcSharedTargetAnimationStats* = nullptr,const ServerNpcSharedMovementBindings* = nullptr,ServerNpcSharedMovementStats* = nullptr,const ServerNpcSharedCueBindings* = nullptr,ServerNpcSharedCueStats* = nullptr,const ServerNpcSharedObservationBindings* = nullptr,ServerNpcSharedObservationStats* = nullptr,const ServerNpcSharedSoundCoverBindings* = nullptr,ServerNpcSharedSoundCoverStats* = nullptr,const ServerNpcSharedTacticalCoverBindings* = nullptr,ServerNpcSharedTacticalCoverStats* = nullptr) noexcept;
}
