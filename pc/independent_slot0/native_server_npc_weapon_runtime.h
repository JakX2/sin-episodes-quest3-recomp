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
inline constexpr char server_npc_weapon_grunt_frame_export[]="SiN_GetServerNpcWeaponGruntFrameApi";
struct ServerNpcWeaponGruntFrameBindings {
    ServerNpcWeaponRuntimeBindings runtime;
    const ServerGruntRegistration* ids;
    ServerGruntAnimationBindings handler;
};
struct ServerNpcWeaponGruntFrameResult {
    ServerNpcWeaponRuntimeResult runtime;
    ServerGruntAnimationResult last;
    unsigned npc_events,weapon_events,handled;
    int failed_event,failed_status;
    void* failed_source;
};
struct ServerNpcWeaponGruntFrameApi {
    std::uint32_t version,size;
    // Same original +2c0 target on all eight Grunt/Jetpack classes.
    // Binds both event traversal receivers to the registered native handler.
    // Unrecovered routes fail explicitly; previous writes are not rolled back.
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcWeaponGruntFrameBindings*,ServerNpcWeaponGruntFrameResult*) noexcept;
};
using GetServerNpcWeaponGruntFrameApi=const ServerNpcWeaponGruntFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_native_weapon_event_frame_export[]="SiN_GetServerNpcNativeWeaponEventFrameApi";
struct ServerNpcNativeWeaponEventFrameBindings {
    ServerNpcWeaponGruntFrameBindings frame;
    ServerNpcParentWeaponEventBindings events;
};
struct ServerNpcNativeWeaponEventFrameResult {
    ServerNpcWeaponGruntFrameResult frame;
    ServerGruntNativeWeaponEventResult last;
    unsigned native_altfire,native_parent;
};
struct ServerNpcNativeWeaponEventFrameApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcNativeWeaponEventFrameBindings*,ServerNpcNativeWeaponEventFrameResult*) noexcept;
};
using GetServerNpcNativeWeaponEventFrameApi=const ServerNpcNativeWeaponEventFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_script_weapon_event_frame_export[]="SiN_GetServerNpcScriptWeaponEventFrameApi";
struct ServerNpcScriptWeaponEventFrameBindings {
    ServerNpcNativeWeaponEventFrameBindings frame;
    ServerWeaponScriptView scripts;
    ServerNpcConditionMapBindings conditions;
};
struct ServerNpcScriptWeaponEventFrameResult {
    ServerNpcNativeWeaponEventFrameResult frame;
    ServerGruntScriptWeaponEventResult last;
    unsigned fixed_resets,script_lookups,condition_calls,sound_intervals,burst_queries;
};
struct ServerNpcScriptWeaponEventFrameApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcScriptWeaponEventFrameBindings*,ServerNpcScriptWeaponEventFrameResult*) noexcept;
};
using GetServerNpcScriptWeaponEventFrameApi=const ServerNpcScriptWeaponEventFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
