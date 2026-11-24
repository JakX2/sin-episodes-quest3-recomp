#pragma once
#include "native_jessica_script.h"
#include "native_count_interrogation.h"
#include "native_npc_map.h"
#include "native_npc_scene_playback.h"
namespace sin_port::native_weapon_tables {
inline constexpr char npc_dialogue_export[]="SiN_GetNpcDialogueApi";
struct JessicaDialogueBindings {
    void* context;
    int (__cdecl *clock)(void*,float*) noexcept;
    // Host invokes original record criteria producer (record,buffer,time).
    // Return a bounded, terminated copy of its returned modifier string.
    int (__cdecl *criteria)(void*,const void* record,float now,CountCriteria*) noexcept;
    CompanionTalkBindings talk;
    const TalkCooldownState* cooldown;
    const SpeechSemaphorePair* semaphore;
};
struct CountDialogueBindings {
    void* context;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *expeditor)(void*,void* npc,void**,unsigned* bytes) noexcept;
    // Original Count uses the 512-byte buffer itself as modifiers.
    int (__cdecl *criteria)(void*,const void* record,float now,CountCriteria*) noexcept;
    ResponseSpeakBindings speak;
};
struct NpcDialogueApi {
    std::uint32_t version,size;
    // Compose existing locked observation routines with original speech paths.
    // Caller owns stable observation and host bindings through each invocation.
    int (__cdecl *jessica)(HologramControl*,JessicaAttentionState*,const JessicaDialogueBindings*,const void* observation,unsigned bytes) noexcept;
    int (__cdecl *count)(HologramControl*,const CountDialogueBindings*,const void* observation,unsigned bytes) noexcept;
};
using GetNpcDialogueApi=const NpcDialogueApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char npc_map_dialogue_export[]="SiN_GetNpcMapDialogueApi";
struct NpcMapDialogueBindings {JessicaDialogueBindings jessica;CountDialogueBindings count;};
struct NpcMapDialogueApi {
    std::uint32_t version,size;
    // Preserves the original map-level active behavior routing for Count.
    // GenericActor/JC continue through the existing NpcMapApi.
    int (__cdecl *observe)(NpcMapKind,HologramControl*,NpcMapSession*,const NpcMapBindings*,const NpcMapDialogueBindings*,const void*,unsigned bytes) noexcept;
};
using GetNpcMapDialogueApi=const NpcMapDialogueApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char managed_jessica_observation_export[]="SiN_GetManagedJessicaObservationApi";
struct ManagedJessicaObservationApi {
    std::uint32_t version,size;
    // Reuses the shared attention state and locked observation callback.
    // The detached cooldown pointer in JessicaDialogueBindings is ignored.
    int (__cdecl *observe)(HologramControl*,JessicaAttentionState*,const JessicaDialogueBindings*,const TalkManagerRuntimeBindings*,const void*,unsigned) noexcept;
};
using GetManagedJessicaObservationApi=const ManagedJessicaObservationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char integrated_npc_map_dialogue_export[]="SiN_GetIntegratedNpcMapDialogueApi";
struct IntegratedNpcMapDialogueBindings {
    ResponseChoreographyBindings response;
    SceneEntityLifecycleBindings lifecycle;
};
struct IntegratedNpcMapDialogueApi {
    std::uint32_t version,size;
    int (__cdecl *observe)(NpcMapKind,HologramControl*,NpcMapSession*,const NpcMapBindings*,const NpcMapDialogueBindings*,
        const TalkManagerRuntimeBindings*,const IntegratedNpcMapDialogueBindings*,const void*,unsigned) noexcept;
};
using GetIntegratedNpcMapDialogueApi=const IntegratedNpcMapDialogueApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_generated_map_dialogue_export[]="SiN_GetNativeGeneratedMapDialogueApi";
struct NativeGeneratedMapDialogueApi {
    std::uint32_t version,size;
    // Non-raw Speak uses native generation; named VCD retains integrated playback.
    int (__cdecl *observe)(NpcMapKind,HologramControl*,NpcMapSession*,const NpcMapBindings*,const NpcMapDialogueBindings*,
        const TalkManagerRuntimeBindings*,const NativeGeneratedResponseBindings*,const void*,unsigned) noexcept;
};
using GetNativeGeneratedMapDialogueApi=const NativeGeneratedMapDialogueApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;


}
