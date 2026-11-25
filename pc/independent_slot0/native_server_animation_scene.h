#pragma once
#include <cstdint>
namespace sin_port::native_weapon_tables {
inline constexpr char server_animation_scene_export[]="SiN_GetServerAnimationSceneApi";
struct ServerAnimationSceneBindings {
    void* context;
    int (__cdecl *lookup)(void*,void* actor,const char*,int*) noexcept;
    int (__cdecl *missing)(void*,void* actor,std::uint32_t choreo_actor,unsigned type,const char*) noexcept;
    int (__cdecl *remove)(void*,void* actor,int layer) noexcept;
    int (__cdecl *blend_out)(void*,void* actor,int layer,float duration,float delay) noexcept;
};
struct ServerAnimationSceneApi {
    std::uint32_t version,size;
    // 1 handled (accepted is original AL), 0 unrelated, -1 invalid/host failure.
    int (__cdecl *begin)(void* actor,void* record,unsigned,const void* event,unsigned,std::uint32_t choreo_actor,const ServerAnimationSceneBindings*,unsigned*) noexcept;
    int (__cdecl *stop)(void* actor,const void* record,unsigned,const void* event,unsigned,int immediate,int forced,const ServerAnimationSceneBindings*,unsigned*) noexcept;
    int (__cdecl *check)(const void* event,unsigned,unsigned*) noexcept;
};
using GetServerAnimationSceneApi=const ServerAnimationSceneApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
