#pragma once
#include "native_server_grunt_task_dispatch.h"
#include "native_server_npc_schedule_resolution.h"
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
struct ServerNpcSharedMovementStats {ServerNpcSharedMovementResult last;unsigned calls,native,components;};
struct ServerNpcSharedTargetAnimationStats {ServerNpcSharedTargetAnimationResult last;unsigned calls,native,components;};
struct ServerNpcSharedFacingStats {ServerNpcSharedFacingResult last;unsigned calls,native,components;};
struct ServerNpcSharedTaskStats {ServerNpcSharedTaskResult last;unsigned calls,native,component_handled,special,parent;};
// Per-maintenance call only; no callback or actor memory retains the session.
struct ServerNpcSharedAttackBindings;
struct ServerNpcSharedAttackStats;
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
};
int bind_shared_parent_internal(void*,unsigned,float,const ServerGruntTaskBindings*,const ServerNpcScheduleResolutionBindings*,const ServerNpcSharedTaskBindings*,ServerNpcSharedTaskStats*,ServerNpcSharedParentSession*,ServerGruntTaskBindings*,const ServerNpcSharedAttackBindings* = nullptr,ServerNpcSharedAttackStats* = nullptr,const ServerNpcSharedFacingBindings* = nullptr,ServerNpcSharedFacingStats* = nullptr,const ServerNpcSharedTargetAnimationBindings* = nullptr,ServerNpcSharedTargetAnimationStats* = nullptr,const ServerNpcSharedMovementBindings* = nullptr,ServerNpcSharedMovementStats* = nullptr) noexcept;
}
