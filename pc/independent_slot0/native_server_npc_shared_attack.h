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
    // Optional legacy +684 refresh. Null uses the native weapon parameters
    // when no behavior component is active; an active component needs its notification service.
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
inline constexpr char server_npc_attack_parameter_export[]="SiN_GetServerNpcAttackParameterApi";
struct ServerNpcAttackParameterResult {
    unsigned weapon_class,lower,upper,component_notified;
    float short_delay,long_low,long_high;
    std::uint32_t component;
};
struct ServerNpcAttackParameterApi {
    std::uint32_t version,size;
    // Original 22030930: active weapon interval, burst bounds, long-delay bounds.
    int (__cdecl *refresh)(void*,unsigned,const ServerWeaponActivityBindings*,void*,int (__cdecl *)(void*,std::uint32_t) noexcept,ServerNpcAttackParameterResult*) noexcept;
};
using GetServerNpcAttackParameterApi=const ServerNpcAttackParameterApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
