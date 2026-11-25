#pragma once
#include "native_server_animation_scene.h"
#include "native_server_animation_events.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_run_animation_export[]="SiN_GetServerNpcRunAnimationApi";
struct ServerNpcRunAnimationBindings {
    ServerComposedAnimationFrameBindings advance;
    ServerAnimationEventBindings events;
    void* context;
    int (__cdecl *model_available)(void*,void* actor) noexcept; // 220eb8a0
    // Optional override for the +298 virtual; null is verified Grunt 22035530.
    int (__cdecl *finished)(void*,void* actor,unsigned*) noexcept;
    int (__cdecl *select_sequence)(void*,void* actor,int activity,int current,bool looping,int*) noexcept;
    int (__cdecl *set_sequence)(void*,void* actor,int sequence) noexcept; // 220f2300
    void (__cdecl *debug_update)(void*,void* actor,std::uint32_t component) noexcept; // 220740a0
};
struct ServerNpcRunAnimationInput {
    float now;
    bool clear_npc_flags; // true only for verified 22028280 +294 override
    bool debug_enabled; // original global 22546d5c bit 1
};
struct ServerNpcRunAnimationResult {
    unsigned model_available,advanced,selection_attempted,sequence_set,events_dispatched;
    int selected;
    ServerAnimationEventResult events;
};
struct ServerNpcRunAnimationApi {
    std::uint32_t version,size;
    // CAI_BaseNPC_RunAnimation 2202ff60. Not the full AI think scheduler.
    int (__cdecl *run)(void* actor,unsigned bytes,const ServerNpcRunAnimationInput*,const ServerNpcRunAnimationBindings*,ServerNpcRunAnimationResult*) noexcept;
};
using GetServerNpcRunAnimationApi=const ServerNpcRunAnimationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
