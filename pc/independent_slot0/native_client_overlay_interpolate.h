#pragma once
#include "native_client_overlay_history.h"
namespace sin_port::native_weapon_tables {
inline constexpr char client_overlay_interpolate_export[]="SiN_GetClientOverlayInterpolateApi";
struct ClientOverlayInterpolateBindings {
    ClientOverlayHistoryBindings history;
    // Original 240c3230 node-pool reclamation. Must be infallible and
    // preserve the current layer/watcher. Called only after a sample write.
    void (__cdecl *prune)(void*,void* watcher,float cutoff) noexcept;
};
struct ClientOverlayInterpolateApi {
    std::uint32_t version,size;
    // Numerical 240c2ba0/240c2e60 path, single overlay sample.
    int (__cdecl *evaluate)(const ClientOverlayHistorySelection*,bool looping,ClientOverlaySample*) noexcept;
    // 240c29b0 with extrapolation disabled: select, evaluate, write and
    // request history pruning. Explicit row extent and pointer identity.
    int (__cdecl *apply)(void* watcher,unsigned bytes,void* row,unsigned row_bytes,float time,const ClientOverlayInterpolateBindings*,int* wrote) noexcept;
};
using GetClientOverlayInterpolateApi=const ClientOverlayInterpolateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char client_overlay_clocked_interpolate_export[]="SiN_GetClientOverlayClockedInterpolateApi";
struct ClientOverlayInterpolationClock {
    bool allow_extrapolation;
    float last_network_time; // original 24369390
};
struct ClientOverlayClockedInterpolateApi {
    std::uint32_t version,size;
    // 240c29d0: equal-endpoint extrapolation gate. For overlay payloads
    // 240c2b10 copies the selected sample without the ordinary clamps.
    int (__cdecl *apply)(void* watcher,unsigned bytes,void* row,unsigned row_bytes,float time,
        const ClientOverlayInterpolationClock*,const ClientOverlayInterpolateBindings*,int* wrote,int* raw_copy) noexcept;
};
using GetClientOverlayClockedInterpolateApi=const ClientOverlayClockedInterpolateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
