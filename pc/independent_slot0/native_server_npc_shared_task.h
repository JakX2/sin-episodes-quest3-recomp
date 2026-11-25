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
    int (__cdecl *desired_yaw)(void*,void*,bool,double*) noexcept; // actor +754
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
};
int bind_shared_parent_internal(void*,unsigned,float,const ServerGruntTaskBindings*,const ServerNpcScheduleResolutionBindings*,const ServerNpcSharedTaskBindings*,ServerNpcSharedTaskStats*,ServerNpcSharedParentSession*,ServerGruntTaskBindings*,const ServerNpcSharedAttackBindings* = nullptr,ServerNpcSharedAttackStats* = nullptr,const ServerNpcSharedFacingBindings* = nullptr,ServerNpcSharedFacingStats* = nullptr) noexcept;
}
