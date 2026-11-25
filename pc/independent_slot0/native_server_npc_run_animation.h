#pragma once
#include "native_server_animation_scene.h"
#include "native_server_animation_events.h"
#include "native_server_sequence_reset.h"
#include "native_server_sequence_choice.h"
#include "native_server_activity_metadata.h"
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
inline constexpr char server_npc_native_sequence_frame_export[]="SiN_GetServerNpcNativeSequenceFrameApi";
struct ServerNpcNativeSequenceBindings {
    ServerNpcRunAnimationBindings frame;
    void* context;
    // 1 resolved (stable borrowed entries), 0 null studio metadata,
    // negative failure. Activity registration must already be complete.
    int (__cdecl *choices)(void*,void* actor,const ServerSequenceChoiceEntry**,unsigned*) noexcept;
    ServerSequenceChoiceBindings choice;
    ServerSequenceResetBindings reset;
};
struct ServerNpcNativeSequenceFrameResult {
    ServerNpcRunAnimationResult frame;
    ServerSequenceResetResult reset;
};
struct ServerNpcNativeSequenceFrameApi {
    std::uint32_t version,size;
    // Does not use frame.select_sequence or frame.set_sequence.
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcNativeSequenceBindings*,ServerNpcNativeSequenceFrameResult*) noexcept;
};
using GetServerNpcNativeSequenceFrameApi=const ServerNpcNativeSequenceFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_dictionary_frame_export[]="SiN_GetServerNpcDictionaryFrameApi";
struct ServerNpcDictionaryFrameBindings {
    ServerNpcNativeSequenceBindings sequence;
    ServerSequenceDictionaryBindings dictionary;
};
struct ServerNpcDictionaryFrameResult {
    ServerNpcRunAnimationResult frame;
    ServerSequenceDictionaryResult sequence;
};
struct ServerNpcDictionaryFrameApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcDictionaryFrameBindings*,ServerNpcDictionaryFrameResult*) noexcept;
};
using GetServerNpcDictionaryFrameApi=const ServerNpcDictionaryFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_activity_frame_export[]="SiN_GetServerNpcActivityFrameApi";
struct ServerNpcActivityFrameBindings {
    ServerNpcDictionaryFrameBindings frame;
    ServerActivityDictionary* activities;
    void* context;
    // Resolve the current actor's global descriptor order and group words.
    // 1 available, 0 null metadata, negative failure. View must stay stable
    // through selection; the reset descriptor is resolved separately.
    int (__cdecl *metadata)(void*,void* actor,const ServerActivityCompositeView**) noexcept;
};
struct ServerNpcActivityFrameResult {
    ServerNpcDictionaryFrameResult frame;
    ServerActivityChoiceResult choice;
};
struct ServerNpcActivityFrameApi {
    std::uint32_t version,size;
    // No legacy choices/select_sequence/set_sequence/register_events.
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcActivityFrameBindings*,ServerNpcActivityFrameResult*) noexcept;
};
using GetServerNpcActivityFrameApi=const ServerNpcActivityFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
