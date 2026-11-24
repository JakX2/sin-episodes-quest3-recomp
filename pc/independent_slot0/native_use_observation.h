#pragma once
#include "native_action_monitor.h"
namespace sin_port::native_weapon_tables {
inline constexpr char use_observation_export[]="SiN_GetUseObservationApi";
struct UseObservationBindings {
    ActionMonitorBindings monitor;
    // Original use event emitter, 222b61d0. Host owns event manager,
    // entity handles, and event creation/dispatch.
    int (__cdecl *event)(void*,int descriptor,void* owner,void* subject,const char* subobject) noexcept;
};
struct NpcObservationBindings {
    void* context;
    int (__cdecl *max_clients)(void*,int*) noexcept;
    // Existing single-player lookup 22288a40, with serial-valid live
    // player storage and full extent. Null player is successful absence.
    int (__cdecl *single_player)(void*,void** player,unsigned* bytes) noexcept;
    // Original NPC virtual +46c. Synchronously receives the borrowed
    // player action monitor; host dispatches the concrete NPC handler.
    int (__cdecl *observe)(void*,void* npc,const void* monitor,unsigned bytes) noexcept;
};
struct UseObservationApi {
    std::uint32_t version,size;
    int (__cdecl *capture)(HologramControl*,const UseObservationBindings*,int descriptor,void* subject) noexcept;
    // Original NPC think slice, after pre-think and before remaining
    // AI work. No new NPC scheduler or physics implementation.
    int (__cdecl *npc_tick)(void* npc,const NpcObservationBindings*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetUseObservationApi=const UseObservationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
