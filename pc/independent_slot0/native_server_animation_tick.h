#pragma once
#include "native_server_activity_transition.h"
#include "native_server_npc_run_animation.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_animation_tick_export[]="SiN_GetServerNpcAnimationTickApi";
struct ServerNpcAnimationTickBindings {
    ServerNpcTransitionBindings transition;
    ServerNpcActivityFrameBindings frame;
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,const void**,unsigned*) noexcept;
    // 220754e0 may replace current path. promoted conveys original return.
    int (__cdecl *promote_path)(void*,void*,std::uint32_t,bool*) noexcept;
    // Serial checked active weapon resolver, 0 absent, 1 available.
    int (__cdecl *weapon)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *weapon_post)(void*,void*,void*) noexcept;
};
struct ServerNpcAnimationTickResult {
    ServerNpcTransitionResult movement;
    ServerNpcActivityFrameResult animation;
    unsigned path_suppressed,movement_checked,promoted,movement_requested,weapon_posted;
    int movement_activity;
};
struct ServerNpcAnimationTickApi {
    std::uint32_t version,size;
    // 22030040, verified Grunt/Jetpack vtables only.
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcAnimationTickBindings*,ServerNpcAnimationTickResult*) noexcept;
};
using GetServerNpcAnimationTickApi=const ServerNpcAnimationTickApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
