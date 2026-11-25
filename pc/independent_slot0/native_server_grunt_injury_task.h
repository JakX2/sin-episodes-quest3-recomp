#pragma once
#include "native_server_grunt_combat_task.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_grunt_injury_task_export[]="SiN_GetServerGruntInjuryTaskApi";
struct ServerGruntInjuryTaskBindings {
    ServerGruntCombatTaskBindings combat;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *is_player)(void*,void*,bool*) noexcept;
    int (__cdecl *npc_caps)(void*,void*,bool*,unsigned*) noexcept;
    int (__cdecl *squad_flag)(void*,std::uint32_t,unsigned) noexcept;
    int (__cdecl *cue_finish)(void*,void*,int,int) noexcept; // +588
    int (__cdecl *damage)(void*,void* actor,void* attacker,float amount,int type,float min_force,float max_force) noexcept;
    int (__cdecl *path_gate)(void*,void*,void*,bool*) noexcept; // +620
    int (__cdecl *target_path)(void*,void*,void*,bool*) noexcept; // original route build + submit
    int (__cdecl *failure_token)(void*,const char*,int*) noexcept;
    int (__cdecl *face_helper)(void*,void*,void*) noexcept; // original yaw component write
    int (__cdecl *face_grenade)(void*,void*) noexcept; // 2235cd48 target-facing leaf
    int (__cdecl *yaw_error)(void*,void*,double*) noexcept; // 2206d1b0
    int (__cdecl *dodge_probe)(void*,void*,void*,bool*) noexcept;
    int (__cdecl *dodge_path)(void*,void*,bool*) noexcept;
    int (__cdecl *cover_gate)(void*,void*,bool*) noexcept; // 22054d40(5)
    int (__cdecl *cover_candidate)(void*,void*,bool*,double*) noexcept; // +504 and signed facing dot
};
struct ServerGruntInjuryTaskResult {
    ServerGruntCombatTaskResult combat;
    unsigned recovered,enemy_live,attacker_live,cue_called,damage_called,path_called,forced_completion;
    int failure,activity;
};
struct ServerGruntInjuryTaskApi {
    std::uint32_t version,size;
    int (__cdecl *supports)(int,bool) noexcept;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerGruntInjuryTaskBindings*,ServerGruntInjuryTaskResult*) noexcept;
};
using GetServerGruntInjuryTaskApi=const ServerGruntInjuryTaskApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
