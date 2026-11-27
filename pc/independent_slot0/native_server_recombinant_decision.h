#pragma once
#include "native_server_recombinant_policy.h"
#include "native_server_npc_schedule_state.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_decision_export[]="SiN_GetServerRecombinantDecisionApi";
struct ServerRecombinantDecisionBindings {
    const ServerRecombinantPolicyBindings* policy;
    const ServerNpcScheduleBindings* schedule;
    void* context;
    float major_far_distance; // original live cvar 2259d330+28
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *parent_select)(void*,void*,int*) noexcept;
    int (__cdecl *parent_gather)(void*,void*) noexcept;
    int (__cdecl *parent_interrupts)(void*,void*) noexcept;
    int (__cdecl *parent_invalidated)(void*,void*) noexcept;
    int (__cdecl *flinch_activity)(void*,void*,int,int,int*) noexcept; // +728
    int (__cdecl *model_available)(void*,void*,bool*) noexcept; // 220eb8a0
    int (__cdecl *sequence_for_activity)(void*,void*,int,int*) noexcept; // 220eebd0
    int (__cdecl *small_flinch)(void*,void*,int,bool*) noexcept; // 2202e600
    int (__cdecl *play_flinch)(void*,void*) noexcept; // inherited +4a0
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
};
struct ServerRecombinantDecisionResult {
    unsigned completed,kind,condition_tests,condition_writes,changed,delegated,goal_live,enemy_live,flinch_calls,
        activity_calls,model_calls,sequence_calls,random_calls,schedule_tests,latch_cleared,deadline_changed;
    int selected,activity,sequence,prior;
    float distance,draws[2],deadline;
};
struct ServerRecombinantDecisionApi {
    unsigned version,size;
    int (__cdecl *select)(void*,unsigned,const ServerRecombinantDecisionBindings*,ServerRecombinantDecisionResult*) noexcept;
    int (__cdecl *gather_major)(void*,unsigned,const ServerRecombinantDecisionBindings*,ServerRecombinantDecisionResult*) noexcept;
    int (__cdecl *interrupts)(void*,unsigned,const ServerRecombinantDecisionBindings*,ServerRecombinantDecisionResult*) noexcept;
    int (__cdecl *invalidated_major)(void*,unsigned,float,const ServerRecombinantDecisionBindings*,ServerRecombinantDecisionResult*) noexcept;
    int (__cdecl *flinch_maintenance_major)(void*,unsigned,float,const ServerRecombinantDecisionBindings*,ServerRecombinantDecisionResult*) noexcept;
};
using GetServerRecombinantDecisionApi=const ServerRecombinantDecisionApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
