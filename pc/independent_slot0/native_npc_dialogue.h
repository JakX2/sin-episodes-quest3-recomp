#pragma once
#include "native_jessica_script.h"
#include "native_count_interrogation.h"
#include "native_npc_map.h"
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

}
