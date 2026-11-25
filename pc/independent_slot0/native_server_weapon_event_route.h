#pragma once
#include "native_server_weapon_event_sound.h"
#include "native_server_animation_events.h"
#include "native_server_weapon_post_frame.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_weapon_event_route_export[]="SiN_GetServerWeaponEventRouteApi";
enum ServerWeaponEventRoute : unsigned {
    WeaponEventDiagnostic=0,WeaponEventSound=1,WeaponEventPrimary=2,
    WeaponEventSecondary=3,WeaponEventSpinStart=4,WeaponEventSpinIdle=5,
    WeaponEventGrenade=6,WeaponEventGrenadeReady=7,WeaponEventMelee=8,
    WeaponEventBaseContact=9
};
struct ServerWeaponEventRouteBindings {
    void* context;
    ServerWeaponEventSoundBindings sound;
    ServerWeaponActivityBindings weapons;
    // Resolve actual combat owner, +fc and +120, not the event receiver.
    int (__cdecl *combat_owner)(void*,void*,void**) noexcept;
    int (__cdecl *is_player)(void*,void*,bool*) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Engine-facing leaves of the recovered event methods.
    int (__cdecl *prepare)(void*,void*,void* receiver,unsigned,const ServerAnimationEvent*) noexcept;
    int (__cdecl *primary_effect)(void*,void*,void* receiver,unsigned,const ServerAnimationEvent*) noexcept;
    int (__cdecl *flash)(void*,void* receiver) noexcept;
    int (__cdecl *secondary)(void*,void*,unsigned) noexcept;
    // Dedicated grenade helper (22315aa0/22315cd0/22315f80) and ammo removal.
    int (__cdecl *grenade)(void*,void*,void* player,int event) noexcept;
    int (__cdecl *consume_grenade)(void*,void* player,int ammo_type) noexcept;
    int (__cdecl *grenade_danger)(void*,void* weapon,void* combat_owner) noexcept;
    int (__cdecl *melee)(void*,void*,void* receiver,const ServerAnimationEvent*) noexcept;
    int (__cdecl *base_contact)(void*,void*,void* player,const ServerAnimationEvent*) noexcept;
    void (__cdecl *diagnostic)(void*,void*,const ServerAnimationEvent*) noexcept;
};
struct ServerWeaponEventRouteResult {
    unsigned route,clip_changed,prepared,primary_called,flash_called,secondary_called;
    unsigned grenade_consumed,deadlines_written,owner_live,player_owner;
    ServerWeaponEventSoundResult sound;
};
struct ServerWeaponEventRouteApi {
    std::uint32_t version,size;
    // Direct weapon +4c0. Known native state decisions; leaves explicit.
    int (__cdecl *handle)(void*,unsigned,unsigned,void*,const ServerAnimationEvent*,float,const ServerWeaponEventRouteBindings*,ServerWeaponEventRouteResult*) noexcept;
    // Original 222aa640 active weapon resolver, then +4c0.
    int (__cdecl *active)(void*,unsigned,const ServerAnimationEvent*,float,const ServerWeaponEventRouteBindings*,ServerWeaponEventRouteResult*) noexcept;
};
using GetServerWeaponEventRouteApi=const ServerWeaponEventRouteApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
