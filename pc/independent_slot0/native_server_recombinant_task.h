#pragma once
#include "native_server_recombinant_decision.h"
#include "native_server_npc_task_lifecycle.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_task_export[]="SiN_GetServerRecombinantTaskApi";
struct ServerRecombinantTaskGoal {
    int kind;float position[3];int field10,activity,arrival_activity,arrival_sequence;
    float distance,tolerance;unsigned flags,owner;
};
static_assert(sizeof(ServerRecombinantTaskGoal)==48);
struct ServerRecombinantTaskBindings {
    const ServerRecombinantDecisionBindings* decision;
    ServerNpcTaskLifecycleBindings lifecycle;
    void* context;
    unsigned goal_owner; // live original 225372b0 equivalent
    int (__cdecl *parent)(void*,void*,const ServerNpcScheduleTask*,bool) noexcept;
    int (__cdecl *activity)(void*,void*,int) noexcept; // 22049c20
    int (__cdecl *enemy)(void*,void*,void**) noexcept;
    int (__cdecl *submit_goal)(void*,unsigned,const ServerRecombinantTaskGoal*,int,bool*) noexcept;
    int (__cdecl *goal_enemy)(void*,unsigned,void*) noexcept; // 22074000
    int (__cdecl *failure_token)(void*,const char*,int*) noexcept;
};
struct ServerRecombinantTaskResult {
    unsigned completed,custom,delegated,schedule_tests,condition_writes,random_calls,activity_calls,
        goal_calls,goal_accepted,enemy_calls,goal_enemy_calls,handle_cleared,latch_cleared;
    int task,activity,failure;
    float draw,deadline,probability;
    ServerRecombinantTaskGoal goal;
    ServerNpcTaskLifecycleResult lifecycle;
};
struct ServerRecombinantTaskApi {
    unsigned version,size;
    int (__cdecl *supports_major)(int,bool) noexcept;
    int (__cdecl *major)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerRecombinantTaskBindings*,ServerRecombinantTaskResult*) noexcept;
};
using GetServerRecombinantTaskApi=const ServerRecombinantTaskApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
