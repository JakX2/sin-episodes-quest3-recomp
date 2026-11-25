#pragma once
#include "native_client_overlay_entity.h"
#include "native_client_overlay_events.h"
namespace sin_port::native_weapon_tables {
inline constexpr char client_overlay_event_frame_export[]="SiN_GetClientOverlayEventFrameApi";
struct ClientOverlayEventFrameBindings {
    void* context;
    // 240b2580: 1 model available, 0 no model, negative failure.
    int (__cdecl *resolve_model)(void*,void* actor,void** model) noexcept;
    void (__cdecl *lock)(void*,void* model) noexcept;
    void (__cdecl *unlock)(void*,void* model) noexcept;
    int (__cdecl *sequence_count)(void*,void* model,int* count) noexcept;
    // Original 240b7830 base animation event boundary.
    void (__cdecl *base_events)(void*,void* actor) noexcept;
    ClientOverlayEntityBindings entity;
    ClientOverlayEventBindings events;
};
enum ClientOverlayEventFrameSkip : unsigned {EventFrameRan=0,EventFrameZeroTime=1,EventFrameNoModel=2};
struct ClientOverlayEventFrameResult {
    unsigned skipped;
    ClientOverlayEntityResult repair;
    ClientOverlayEventResult events;
};
struct ClientOverlayEventFrameApi {
    std::uint32_t version,size;
    // Original event wrapper ordering. General interpolation is not
    // repeated here: only changed sequences invoke native interpolation.
    int (__cdecl *run)(void* actor,unsigned bytes,float frame_time,float current_time,
        const ClientOverlayInterpolationClock*,const ClientOverlayEventFrameBindings*,ClientOverlayEventFrameResult*) noexcept;
};
using GetClientOverlayEventFrameApi=const ClientOverlayEventFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
