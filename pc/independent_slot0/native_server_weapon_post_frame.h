#pragma once
#include "native_server_weapon_animation.h"
#include "native_server_base_animation_frame.h"
#include "native_server_animation_events.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_weapon_post_frame_export[]="SiN_GetServerWeaponPostFrameApi";
struct ServerWeaponPostFrameBindings {
    ServerWeaponAnimationBindings animation;
    ServerBaseAnimationFrameBindings frame;
    ServerAnimationEventBindings events;
    ServerWeaponActivityBindings weapons;
    void* context;
    // Original 222ad370: serial owner +494 then virtual +fc.
    // Return 0 absent/noncombat, 1 resolved, negative failure.
    int (__cdecl *combat_owner)(void*,void* entity,void**) noexcept; // optional for authenticated primary vtables
    int (__cdecl *is_player)(void*,void* owner,bool*) noexcept; // optional for authenticated primary vtables
    int (__cdecl *extent)(void*,void* entity,unsigned*) noexcept;
    // Original weapon +2c0 forwards viewmodel event through +4c0
    // only after fresh combat-owner and player checks.
    int (__cdecl *weapon_event)(void*,void* weapon,void* player,const ServerAnimationEvent*) noexcept;
    ServerBaseAnimationFrameBindings view_frame;
    ServerAnimationEventBindings view_events; // emit replaced by native forwarding
};
struct ServerWeaponPostFrameResult {
    unsigned advanced,reselected,sequence_set,owner_live,player_owner,viewmodel_live,viewmodel_advanced,viewmodel_forwarded;
    int selected;
    ServerSequenceDictionaryResult reset;
    ServerAnimationEventResult events,view_events;
};
struct ServerWeaponPostFrameApi {
    std::uint32_t version,size;
    // 222aaec0. receiver is caller NPC, not necessarily weapon owner.
    int (__cdecl *run)(void* weapon,unsigned,void* receiver,float,const ServerWeaponPostFrameBindings*,ServerWeaponPostFrameResult*) noexcept;
    // 222a54e0. Serial-checked active weapon dispatch.
    int (__cdecl *notify)(void* actor,unsigned,float,const ServerWeaponPostFrameBindings*,ServerWeaponPostFrameResult*,unsigned* live) noexcept;
};
using GetServerWeaponPostFrameApi=const ServerWeaponPostFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
