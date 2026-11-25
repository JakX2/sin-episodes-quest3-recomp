#pragma once
#include "native_server_native_activity_services.h"
#include "native_server_animation_tick.h"
#include "native_server_weapon_post_frame.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_weapon_runtime_export[]="SiN_GetServerNpcWeaponRuntimeApi";
struct ServerNpcWeaponRuntimeBindings {
    ServerNpcAnimationTickBindings npc;
    ServerInheritedActivityBindings inherited;
    ServerWeaponPostFrameBindings weapon;
};
struct ServerNpcWeaponRuntimeResult {
    ServerNpcActivityAssignmentResult request;
    ServerNpcTransitionResult transition;
    ServerNpcAnimationTickResult tick;
    ServerWeaponAnimationDispatchResult weapon_assignment;
    ServerWeaponPostFrameResult weapon_frame;
    unsigned weapon_notifications,weapon_frames;
};
struct ServerNpcWeaponRuntimeApi {
    std::uint32_t version,size;
    // All three operations use one activity dictionary, event dictionary,
    // and serial registry. Callback adapters live only for this call.
    int (__cdecl *request)(void*,unsigned,int,const ServerNpcWeaponRuntimeBindings*,ServerNpcWeaponRuntimeResult*) noexcept;
    int (__cdecl *update)(void*,unsigned,const ServerNpcWeaponRuntimeBindings*,ServerNpcWeaponRuntimeResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcWeaponRuntimeBindings*,ServerNpcWeaponRuntimeResult*) noexcept;
};
using GetServerNpcWeaponRuntimeApi=const ServerNpcWeaponRuntimeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
