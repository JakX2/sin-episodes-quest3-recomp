#pragma once
#include <cstdint>
#include "native_server_npc_weapon_scene.h"
#include "native_server_cached_facial_think.h"
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
inline constexpr char server_animation_scene_frame_export[]="SiN_GetServerAnimationSceneFrameApi";
struct ServerAnimationSceneFrameBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Optional override for 2210e450. Null selects the native absolute-tag warp.
    int (__cdecl *gesture_cycle)(void*,const void* event,float,float*) noexcept;
};
struct ServerAnimationSceneFrameInput {
    float scene_time,now,intensity;
    bool suppressed,first_sequence_frame;
    unsigned sequence_flags;
    double sequence_duration;
};
struct ServerAnimationSceneFrameApi {
    std::uint32_t version,size;
    // Post-initialization numeric stage only. Suppression and event intensity
    // are already resolved. No priority, NPC keepalive or layer allocation.
    int (__cdecl *update)(void* actor,unsigned,void* record,unsigned,const void* event,unsigned,
        const ServerAnimationSceneFrameInput*,const ServerAnimationSceneFrameBindings*,unsigned*) noexcept;
};
using GetServerAnimationSceneFrameApi=const ServerAnimationSceneFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_gesture_tag_warp_export[]="SiN_GetServerGestureTagWarpApi";
struct ServerGestureTagWarpApi {
    std::uint32_t version,size;
    // Original 2210e450, bank0->bank1, valid finite sorted source tags.
    int (__cdecl *map)(const void* event,unsigned,float fraction,const ServerAnimationSceneFrameBindings*,float*) noexcept;
};
using GetServerGestureTagWarpApi=const ServerGestureTagWarpApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_gesture_metadata_export[]="SiN_GetServerGestureMetadataApi";
struct ServerGestureMetadata {
    // Sequence metadata was available, even if faceposer subsection absent.
    unsigned available;
    const char* type;      // null = absent
    const char* startloop; // null = default "loop", empty = explicit empty name
    const char* endloop;   // null = default "end"
};
struct ServerGestureMetadataApi {
    std::uint32_t version,size;
    // Metadata portion of 220f7db0 only, after successful repeat lookup.
    // Does not allocate layers or load model/sequence metadata.
    int (__cdecl *apply)(void* record,unsigned,void* event,unsigned,const ServerGestureMetadata*,unsigned suppressed,
        const ServerAnimationSceneFrameBindings*) noexcept;
};
using GetServerGestureMetadataApi=const ServerGestureMetadataApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_animation_record_export[]="SiN_GetServerNpcAnimationRecordApi";
struct ServerNpcAnimationRecordBindings {
    ServerNpcSceneRecordBindings npc;
    ServerAnimationSceneBindings animation;
};
struct ServerNpcAnimationRecordApi {
    std::uint32_t version,size;
    int (__cdecl *adapt)(const ServerNpcAnimationRecordBindings*,ActorEventRecordBindings*) noexcept;
};
using GetServerNpcAnimationRecordApi=const ServerNpcAnimationRecordApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_animation_priority_export[]="SiN_GetServerAnimationPriorityApi";
struct ServerAnimationPriorityApi {
    std::uint32_t version,size;
    // 22107170: channel index in choreography actor, -1 if absent.
    int (__cdecl *channel)(const void* choreo_actor,unsigned,std::uint32_t channel,
        const ServerAnimationSceneFrameBindings*,int*) noexcept;
    // 220f7cc0: preceding attached scenes' +34 count, stopping at current.
    int (__cdecl *prefix)(const void* actor,unsigned,std::uint32_t scene,
        const ServerAnimationSceneFrameBindings*,std::uint32_t*) noexcept;
    // 220f3a60: priority insertion into active layer order.
    int (__cdecl *reorder)(void* actor,unsigned,int layer,int priority,float now,
        const ServerAnimationSceneFrameBindings*) noexcept;
    // Recompute from record+14 and scene prefix when actor+600 is enabled.
    int (__cdecl *refresh)(void* actor,unsigned,const void* record,unsigned,float now,
        const ServerAnimationSceneFrameBindings*) noexcept;
};
using GetServerAnimationPriorityApi=const ServerAnimationPriorityApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_animation_scene_init_export[]="SiN_GetServerAnimationSceneInitApi";
struct ServerAnimationSceneInitBindings {
    ServerAnimationSceneBindings admission;
    ServerAnimationSceneFrameBindings frame;
    void* context;
    int (__cdecl *metadata)(void*,void*,int sequence,ServerGestureMetadata*) noexcept;
    int (__cdecl *sequence_gate)(void*,void*,unsigned*) noexcept;
    int (__cdecl *suppressed)(void*,void*,unsigned*) noexcept;
    // Engine allocation/initialization of active row, 220f5120.
    int (__cdecl *allocate_layer)(void*,void*,int sequence,int priority,int*) noexcept;
    int (__cdecl *sequence_info)(void*,void*,int sequence,unsigned* flags,double* duration) noexcept;
};
struct ServerAnimationSceneInitApi {
    std::uint32_t version,size;
    // 1 handled (accepted original initializer AL), 0 unrelated, -1 failure.
    // Only consumes deferred layer sentinel -2. No post-init frame or keepalive.
    int (__cdecl *initialize)(void* actor,unsigned,void* record,unsigned,void* event,unsigned,float scene_time,float now,
        const ServerAnimationSceneInitBindings*,unsigned*) noexcept;
};
using GetServerAnimationSceneInitApi=const ServerAnimationSceneInitApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_animation_think_export[]="SiN_GetServerNpcAnimationThinkApi";
struct ServerNpcAnimationThinkBindings {
    ServerCachedFacialThinkBindings facial; // optional paired bank/file cache
    ServerAnimationSceneInitBindings animation;
    void* context;
    // Resolve original NPC identity and condition 0x3c/1, activity +878.
    int (__cdecl *npc_state)(void*,void* actor,void** npc,unsigned* condition,int* activity) noexcept;
    int (__cdecl *keepalive)(void*,void* npc,float duration) noexcept;
    int (__cdecl *interrupted)(void*,void* actor) noexcept; // optional warning
};
struct ServerNpcAnimationThinkApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,float,const ServerNpcAnimationThinkBindings*) noexcept;
};
using GetServerNpcAnimationThinkApi=const ServerNpcAnimationThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
