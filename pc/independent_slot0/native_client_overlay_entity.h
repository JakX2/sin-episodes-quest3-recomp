#pragma once
#include "native_client_overlay_pool.h"
#include "native_client_overlay_interpolate.h"
namespace sin_port::native_weapon_tables {
inline constexpr char client_overlay_entity_frame_export[]="SiN_GetClientOverlayEntityFrameApi";
// Port-owned stable per-watcher storage; this is not the original shared
// pool ABI. Each live vector index has its own disjoint pool and word range.
struct ClientOverlayEntitySlot {
    ClientOverlayPool* pool;
    ClientOverlayPoolStorage storage;
};
struct ClientOverlayEntityBindings {
    const ClientOverlayEntitySlot* slots;
    unsigned count;
    void* context;
    bool (__cdecl *looping)(void*,void* actor,int sequence) noexcept;
};
struct ClientOverlayEntityResult {
    unsigned processed,writes,repairs,raw_copies,recycled;
    int failed_slot; // -1 if no slot-specific failure
};
struct ClientOverlayEntityFrameApi {
    std::uint32_t version,size;
    int (__cdecl *capture)(void* actor,unsigned bytes,float sample_time,float current_time,float network_time,
        const ClientOverlayEntityBindings*,ClientOverlayEntityResult*) noexcept;
    int (__cdecl *reset)(void* actor,unsigned bytes,float current_time,const ClientOverlayEntityBindings*,ClientOverlayEntityResult*) noexcept;
    int (__cdecl *render)(void* actor,unsigned bytes,float time,const ClientOverlayInterpolationClock*,
        const ClientOverlayEntityBindings*,ClientOverlayEntityResult*) noexcept;
    int (__cdecl *restore)(void* actor,unsigned bytes,const ClientOverlayEntityBindings*,ClientOverlayEntityResult*) noexcept;
};
using GetClientOverlayEntityFrameApi=const ClientOverlayEntityFrameApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
