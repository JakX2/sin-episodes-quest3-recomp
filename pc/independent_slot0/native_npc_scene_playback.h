#pragma once
#include "native_jessica_script.h"
namespace sin_port::native_weapon_tables {
inline constexpr char npc_scene_playback_export[]="SiN_GetNpcScenePlaybackApi";
struct SceneDurationBindings {
    void* context;
    // Host resolves each event from the original parsed scene vector.
    int (__cdecl *event)(void*,void* parsed,unsigned index,void** event,unsigned* bytes) noexcept;
};
struct NpcScenePlaybackBindings {
    void* context;
    // Engine creates and owns instanced_scripted_scene. Full object storage.
    int (__cdecl *create)(void*,const char* classname,void** entity,unsigned* bytes) noexcept;
    // Original 221b52f0 model-dependent scene name resolution.
    int (__cdecl *resolve_name)(void*,void* npc,const char*,char* dest,unsigned capacity) noexcept;
    int (__cdecl *actor_handle)(void*,void* npc,std::uint32_t*) noexcept;
    int (__cdecl *set_delay)(void*,void* scene,float delay) noexcept;
    int (__cdecl *register_scene)(void*,void* scene) noexcept;
    int (__cdecl *spawn_sound)(void*,void* scene) noexcept;
    int (__cdecl *activate)(void*,void* scene) noexcept;
    int (__cdecl *set_parsed_flag)(void*,std::uint32_t parsed,int argument) noexcept;
    int (__cdecl *start)(void*,void* scene) noexcept;
    int (__cdecl *missing)(void*,const char*,unsigned generated) noexcept;
    int (__cdecl *resolve_parsed)(void*,std::uint32_t,void**,unsigned*) noexcept;
    SceneDurationBindings duration;
};
struct NpcScenePlaybackApi {
    std::uint32_t version,size;
    // Original 22113c10 duration of parsed choreography; event end at
    // +188 unless bit-exact -1 sentinel, then event start at +184.
    int (__cdecl *duration)(void* parsed,unsigned bytes,const SceneDurationBindings*,float*) noexcept;
    // Fixed 220315e0 -> 221a4330 NPC path (no output handle, flag zero).
    int (__cdecl *scene)(void* npc,const char* name,float delay,const NpcScenePlaybackBindings*,float*) noexcept;
    // Fixed 22031600 -> 221a5710 NPC sound scene, no output handle.
    // The sound string is borrowed by the engine entity, as in original.
    int (__cdecl *sound)(void* npc,const char* name,const NpcScenePlaybackBindings*,float*) noexcept;
};
using GetNpcScenePlaybackApi=const NpcScenePlaybackApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
