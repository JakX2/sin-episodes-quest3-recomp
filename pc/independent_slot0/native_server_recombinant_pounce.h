#pragma once
#include "native_server_recombinant_airborne.h"
#include "native_server_recombinant_events.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_pounce_export[]="SiN_GetServerRecombinantPounceApi";
struct ServerRecombinantPounceBindings {
    const ServerRecombinantAirborneBindings* airborne;
    const ServerRecombinantEventBindings* event;
    void* context;
    float slash_damage;
    int (__cdecl *normalize)(void*,float[3]) noexcept;
    int (__cdecl *yaw)(void*,const float[3],float*) noexcept;
    int (__cdecl *motor_yaw)(void*,unsigned,float,float) noexcept;
    // Shared damage-info construction and force/application, not generic
    // combat reimplementation. Both original inflictor/attacker are actor.
    int (__cdecl *damage_info)(void*,void*,float,unsigned,void*,unsigned) noexcept;
    int (__cdecl *force)(void*,void*,unsigned,void*,const float[3],const float[3],float) noexcept;
    int (__cdecl *apply_damage)(void*,void*,const void*,unsigned) noexcept;
};
struct ServerRecombinantPounceResult {
    unsigned completed,enemy_live,movement_set,random_calls,normalized,yaw_calls,motor_calls,ground_cleared,
        origin_refreshes,velocity_refreshes,velocity_set,think_installed,think_scheduled,damage_prepared,force_calls,damage_calls;
    float direction[3],velocity[3],snapshot[3],draws[2],yaw,deadline,damage,origin[3];
    unsigned damage_type;
    ServerRecombinantAirborneResult fallback;
};
struct ServerRecombinantPounceApi {
    unsigned version,size;
    int (__cdecl *off)(void*,unsigned,float,const ServerRecombinantPounceBindings*,ServerRecombinantPounceResult*) noexcept;
    int (__cdecl *slash)(void*,unsigned,void*,const ServerRecombinantPounceBindings*,ServerRecombinantPounceResult*) noexcept;
};
using GetServerRecombinantPounceApi=const ServerRecombinantPounceApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
