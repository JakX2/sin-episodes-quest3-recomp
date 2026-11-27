#pragma once
#include "native_server_recombinant_task.h"
#include "native_server_recombinant_minor_touch.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_minor_task_export[]="SiN_GetServerRecombinantMinorTaskApi";
struct ServerRecombinantMinorTaskBindings {
    const ServerRecombinantTaskBindings* task;
    const ServerRecombinantMinorTouchBindings* touch;
    void* context;
    unsigned goal_owner; // live 22537780
    int arrival_activity,custom_activity; // live 2259d44c,2259d450
    int (__cdecl *world_center)(void*,void*,float[3]) noexcept;
    int (__cdecl *origin)(void*,void*,float[3]) noexcept;
    int (__cdecl *face)(void*,unsigned,const float[3],float,bool) noexcept; // 2206c670 / 2206c710
    int (__cdecl *aim_reset)(void*,void*,int,int) noexcept; // 2204d350
    int (__cdecl *leap)(void*,void*,float,int) noexcept; // 220325c0
    int (__cdecl *target_gate)(void*,void*,void*,bool*) noexcept; // +620
    int (__cdecl *goal_adjust)(void*,void*,void*,ServerRecombinantTaskGoal*) noexcept; // +6f8
    int (__cdecl *set_arrival)(void*,unsigned,int) noexcept; // 22073e00
    int (__cdecl *diagnostic)(void*,int,const char*) noexcept;
    int (__cdecl *special_gate)(void*,void*,bool*) noexcept; // +298
    int (__cdecl *support_probe)(void*,void*,bool*) noexcept; // 2237b550
};
enum RecombinantMinorTaskRoute:unsigned {MinorTaskInherited=0,MinorTaskNative=1,MinorTaskUnrecovered=2};
struct ServerRecombinantMinorTaskResult {
    unsigned completed,route,delegated,enemy_calls,target_calls,face_calls,leap_calls,
        movement_calls,goal_calls,goal_adjusted,goal_accepted,arrival_calls,activity_calls,
        reset_calls,schedule_tests,ground_live,grounded,velocity_refresh,special_calls,probe_calls,diagnostic_calls;
    int task,activity,failure;
    float vector[3];
    ServerRecombinantTaskGoal goal;
    ServerNpcTaskLifecycleResult lifecycle;
};
struct ServerRecombinantMinorTaskApi {
    unsigned version,size;
    // 0 inherited, 1 native, 2 original override not yet recovered.
    unsigned (__cdecl *route)(int,bool) noexcept;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerRecombinantMinorTaskBindings*,ServerRecombinantMinorTaskResult*) noexcept;
};
using GetServerRecombinantMinorTaskApi=const ServerRecombinantMinorTaskApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
