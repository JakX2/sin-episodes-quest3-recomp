#pragma once
#include "native_server_npc_task_lifecycle.h"
#include "native_server_activity_transition.h"
#include "native_server_npc_schedule_state.h"
#include "native_server_weapon_activity.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_grunt_task_dispatch_export[]="SiN_GetServerGruntTaskDispatchApi";
enum ServerGruntTaskRoute : unsigned {GruntTaskParent,GruntTaskNative,GruntTaskHostedSpecial};
struct ServerGruntTaskPeer {void* actor;unsigned bytes;};
struct ServerGruntTaskBindings {
    ServerNpcTaskLifecycleBindings lifecycle;
    ServerWeaponActivityBindings entities;
    const ServerNpcTransitionBindings* transition;
    const ServerGruntRegistration* registration;
    void* context;
    int (__cdecl *parent)(void*,void*,const ServerNpcScheduleTask*,bool) noexcept;
    // Only original custom cases whose remaining bodies are unrecovered.
    int (__cdecl *special)(void*,void*,const ServerNpcScheduleTask*,bool) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *peers)(void*,std::uint32_t,const ServerGruntTaskPeer**,unsigned*) noexcept;
    int (__cdecl *cue)(void*,void*,unsigned virtual_slot,int,int) noexcept;
    int (__cdecl *set_enemy)(void*,void*,void*,bool) noexcept;
    int (__cdecl *finish_hook)(void*,void*) noexcept; // +84c
    int (__cdecl *turn_update)(void*,void*) noexcept; // 2204d350(0,0)
    int (__cdecl *combat_target)(void*,void* entity,void**) noexcept; // +fc
    int (__cdecl *target_gate)(void*,void* target,void* actor,bool*) noexcept; // +300
    int (__cdecl *path_refresh)(void*,void*,void*) noexcept; // run149 leaf
};
struct ServerGruntTaskResult {
    unsigned route,peer_writes,helper_writes,condition_writes,activity_requested,parent_called,special_called;
    int task,activity;
    ServerNpcTaskLifecycleResult lifecycle;
    ServerNpcTransitionResult transition;
};
struct ServerGruntTaskDispatchApi {
    std::uint32_t version,size;
    // 2235bfd0 / 2235cce0 ground base, no subclass overrides.
    int (__cdecl *route)(int,bool,unsigned*) noexcept;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerGruntTaskBindings*,ServerGruntTaskResult*) noexcept;
};
using GetServerGruntTaskDispatchApi=const ServerGruntTaskDispatchApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
