#pragma once
#include <cstdint>
#include "native_server_npc_weapon_scene.h"
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
}
