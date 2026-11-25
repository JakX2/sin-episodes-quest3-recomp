#pragma once
#include "native_client_overlay_events.h"
#include <cstdint>
namespace sin_port::native_weapon_tables {
// Same original 0x50-byte studio event layout on the server.
using ServerStudioEvent=ClientOverlayEvent;
struct ServerAnimationEvent {
    int event;
    const char* options;
    float cycle,time;
    std::uint32_t type;
    void* source;
};
static_assert(sizeof(ServerAnimationEvent)==0x18);
struct ServerAnimationEventSequence {
    const ServerStudioEvent* events;
    unsigned count,flags;
    float finish_window; // sequence descriptor +6c
};
struct ServerAnimationEventBindings {
    void* context;
    // 1 available, 0 absent/out of range, -1 service failure.
    int (__cdecl *sequence)(void*,void* metadata_actor,int,ServerAnimationEventSequence*) noexcept;
    int (__cdecl *cycle_rate)(void*,void* metadata_actor,int,double*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Original virtual +2c0, synchronous borrowed event/options. Must not
    // relocate layer/metadata storage or alter actor animation state.
    void (__cdecl *emit)(void*,void* receiver,int layer,const ServerAnimationEvent*) noexcept;
};
struct ServerAnimationEventResult {unsigned layers,emitted;int failed_layer;};
inline constexpr char server_animation_events_export[]="SiN_GetServerAnimationEventsApi";
struct ServerAnimationEventsApi {
    std::uint32_t version,size;
    // 220eaa10: next matching original studio record, returns next index
    // (one based) or 0 for exhaustion, -1 for invalid inputs.
    int (__cdecl *next)(const ServerAnimationEventSequence*,float start,float end,unsigned index,float now,ServerAnimationEvent*) noexcept;
    // 220f39e0: base first, then active overlay rows in raw index order.
    int (__cdecl *dispatch)(void* actor,unsigned bytes,void* receiver,float now,const ServerAnimationEventBindings*,ServerAnimationEventResult*) noexcept;
};
using GetServerAnimationEventsApi=const ServerAnimationEventsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_base_animation_events_export[]="SiN_GetServerBaseAnimationEventsApi";
struct ServerBaseAnimationEventsApi {
    std::uint32_t version,size;
    // 220eefe0, base only. Does not read +494 or other overlay fields.
    int (__cdecl *dispatch)(void*,unsigned,void*,float,const ServerAnimationEventBindings*,ServerAnimationEventResult*) noexcept;
};
using GetServerBaseAnimationEventsApi=const ServerBaseAnimationEventsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
