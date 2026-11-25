#pragma once
#include "native_server_npc_shared_task.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_shared_attack_export[]="SiN_GetServerNpcSharedAttackApi";
struct ServerNpcSharedAttackBindings {
    ServerNpcSharedTaskBindings shared;
    ServerWeaponActivityBindings weapons;
    void* context;
    // Original 22031780 / +304 / 2206bff0 or 2206c710.
    int (__cdecl *aim_vector)(void*,void*,float[3]) noexcept;
    int (__cdecl *aim_aligned)(void*,void*,const float*,bool*) noexcept;
    int (__cdecl *aim_apply)(void*,void*,const float*,bool stored_yaw) noexcept;
    // Original +684 weapon-dependent timer parameter refresh.
    int (__cdecl *refresh_parameters)(void*,void*) noexcept;
    int (__cdecl *dirty)(void*,void*,std::uint32_t) noexcept;
};
struct ServerNpcSharedAttackResult {
    unsigned special,component_called,component_handled,capabilities,aim_queries,aim_updates,turned,timer_advanced,timer_reset,parameters_refreshed,deadline_changed;
    int task,activity,failed_code;
    ServerNpcTaskLifecycleResult lifecycle;
    ServerNpcTransitionResult transition;
};
struct ServerNpcSharedAttackApi {
    std::uint32_t version,size;
    int (__cdecl *supports)(int) noexcept;
    int (__cdecl *advance_timer)(void*,unsigned,float,const ServerNpcTaskLifecycleBindings*,unsigned*) noexcept;
    int (__cdecl *execute)(void*,unsigned,const ServerNpcScheduleTask*,bool,float,const ServerNpcSharedAttackBindings*,ServerNpcSharedAttackResult*) noexcept;
};
using GetServerNpcSharedAttackApi=const ServerNpcSharedAttackApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct ServerNpcSharedAttackStats {ServerNpcSharedAttackResult last;unsigned calls,components,special;};
}
