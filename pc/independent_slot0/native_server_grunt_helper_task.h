#pragma once
#include "native_server_grunt_injury_task.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_grunt_helper_task_export[]="SiN_GetServerGruntHelperTaskApi";
struct ServerGruntHelperCandidate {
    void* actor;unsigned bytes;
    std::uint32_t token; // resolved raw relationship token, not a serial handle
};
struct ServerGruntHelperTaskBindings {
    ServerGruntInjuryTaskBindings injury;
    void* context;
    int (__cdecl *origin)(void*,void*,float[3]) noexcept; // +214
    int (__cdecl *distance)(void*,void*,void*,double*) noexcept; // 220adad0
    int (__cdecl *path_start)(void*,void*,void*,bool*) noexcept; // original route mode8
    int (__cdecl *path_fallback)(void*,void*,void*) noexcept;
    int (__cdecl *movement_activity)(void*,void*,int*) noexcept; // translate activity10
    int (__cdecl *path_activity)(void*,void*,int) noexcept;
    int (__cdecl *path_status)(void*,void*,int*) noexcept; // 22074060
    int (__cdecl *path_stop)(void*,void*,bool) noexcept;
    int (__cdecl *path_follow)(void*,void*,bool*) noexcept; // 220740a0
    int (__cdecl *path_follow_update)(void*,void*,bool) noexcept; // original +60c or pending
    int (__cdecl *path_tolerance)(void*,void*,float) noexcept;
    int (__cdecl *path_rebuild)(void*,void*,bool*) noexcept; // 2207a5c0(1,0)
    int (__cdecl *squad_candidates)(void*,std::uint32_t,const ServerGruntHelperCandidate**,unsigned*) noexcept;
    int (__cdecl *nearest)(void*,std::uint32_t,void*,ServerGruntHelperCandidate*) noexcept;
    int (__cdecl *self_identity)(void*,void*,std::uint32_t*,std::uint32_t*) noexcept; // token and EHANDLE
};
struct ServerGruntHelperTaskResult {
    ServerGruntInjuryTaskResult injury;
    unsigned recovered,enemy_live,path_started,path_rebuilt,path_stopped,origin_saved,helper_assigned,candidates;
    int failure;
};
struct ServerGruntHelperTaskApi {
    std::uint32_t version,size;
    int (__cdecl *supports)(int,bool) noexcept;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerGruntHelperTaskBindings*,ServerGruntHelperTaskResult*) noexcept;
};
using GetServerGruntHelperTaskApi=const ServerGruntHelperTaskApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct ServerGruntFullTaskStats {
    ServerGruntHelperTaskResult last;
    unsigned calls,injury_calls,helper_calls;
};
}
