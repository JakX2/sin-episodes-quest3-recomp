#pragma once
#include "native_server_animation_scene.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_base_animation_frame_export[]="SiN_GetServerBaseAnimationFrameApi";
struct ServerBaseAnimationFrameBindings {
    ServerBaseAnimationCycleBindings cycle;
    void* context;
    int (__cdecl *width_transition)(void*,void*) noexcept;
};
struct ServerBaseAnimationFrameApi {
    std::uint32_t version,size;
    // Original 220f2780 / 220f17f0, no overlay storage or NPC flag clearing.
    int (__cdecl *run)(void*,unsigned,float,const ServerBaseAnimationFrameBindings*) noexcept;
};
using GetServerBaseAnimationFrameApi=const ServerBaseAnimationFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
