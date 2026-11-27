#pragma once
#include "native_server_recombinant_minor_touch.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_airborne_export[]="SiN_GetServerRecombinantAirborneApi";
inline constexpr unsigned recombinant_airborne_think_marker=0x2237c2c0u;
struct ServerRecombinantAirborneBindings {
    const ServerRecombinantMinorTouchBindings* touch;
    void* context;
    float gravity;
    int (__cdecl *forward)(void*,const float[3],float[3]) noexcept;
    int (__cdecl *ballistic)(void*,const float[3],const float[3],float,float,float[3]) noexcept;
    int (__cdecl *clear_ground)(void*,void*) noexcept;
    int (__cdecl *relocate)(void*,void*,const float[3],int) noexcept;
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *ai_update)(void*,void*) noexcept; // original +418
};
struct ServerRecombinantAirborneResult {
    unsigned completed,forward_calls,ballistic_calls,condition_writes,ground_cleared,relocated,
        origin_refreshes,velocity_set,think_installed,think_scheduled,ai_updates,grounded,attached;
    unsigned contact_marker,think_marker;
    float gravity,velocity[3],relocation[3],snapshot[3],contact_deadline,ai_deadline,next_think;
};
struct ServerRecombinantAirborneApi {
    unsigned version,size;
    int (__cdecl *leap)(void*,unsigned,float,bool,const float[3],const ServerRecombinantAirborneBindings*,ServerRecombinantAirborneResult*) noexcept;
    int (__cdecl *launch_velocity)(void*,unsigned,float,const float[3],const ServerRecombinantAirborneBindings*,ServerRecombinantAirborneResult*) noexcept;
    int (__cdecl *think)(void*,unsigned,float,const ServerRecombinantAirborneBindings*,ServerRecombinantAirborneResult*) noexcept;
};
using GetServerRecombinantAirborneApi=const ServerRecombinantAirborneApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
