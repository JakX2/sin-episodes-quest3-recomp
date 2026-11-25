#pragma once
#include <cstdint>
#include <cstddef>
namespace sin_port::native_weapon_tables {
inline constexpr char client_overlay_events_export[]="SiN_GetClientOverlayEventsApi";
// Original sequence-relative event records: 0x50 bytes, fixed options
// at +0c. The last word is not consumed by 240c1f30.
struct ClientOverlayEvent {
    float cycle;
    int event;
    std::uint32_t type;
    char options[64];
    std::uint32_t unknown4c;
};
static_assert(sizeof(ClientOverlayEvent)==0x50&&offsetof(ClientOverlayEvent,options)==0xc);
struct ClientOverlayEventView {
    const ClientOverlayEvent* events;
    unsigned count;
};
struct ClientOverlayEventBindings {
    void* context;
    // Original sequence metadata is borrowed, stable throughout dispatch.
    int (__cdecl *sequence)(void*,void* actor,int sequence,ClientOverlayEventView*) noexcept;
    // Original actor virtual +250 event dispatch. Options are bounded to
    // the original 64-byte field; caller owns engine-side effect delivery.
    void (__cdecl *emit)(void*,void* actor,unsigned layer,const ClientOverlayEvent*) noexcept;
};
struct ClientOverlayEventResult {
    unsigned visited,emitted,wrapped;
    int aborted_slot,failed_slot;
};
struct ClientOverlayEventsApi {
    std::uint32_t version,size;
    // Overlay-specific part of 240c1f30, after base events and sequence
    // history repair. Does not acquire the original engine/model lock.
    int (__cdecl *dispatch)(void* actor,unsigned bytes,float frame_time,int sequence_count,const ClientOverlayEventBindings*,ClientOverlayEventResult*) noexcept;
};
using GetClientOverlayEventsApi=const ClientOverlayEventsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
