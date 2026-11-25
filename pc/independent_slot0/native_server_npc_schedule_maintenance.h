#pragma once
#include "native_server_npc_schedule_resolution.h"
#include "native_server_npc_schedule_validity.h"
#include "native_server_npc_weapon_runtime.h"
#include "native_server_grunt_task_dispatch.h"
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
}
