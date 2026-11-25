#pragma once
#include "native_client_overlay_history.h"
namespace sin_port::native_weapon_tables {
inline constexpr char client_overlay_pool_export[]="SiN_GetClientOverlayPoolApi";
// Mirrors the four words at original pool+10/+14/+18/+1c.
// This API owns one normalized chain, not the original global allocator.
struct ClientOverlayPool {
    std::uint32_t head,tail,free_head,count;
};
static_assert(sizeof(ClientOverlayPool)==16);
struct ClientOverlayPoolStorage {
    ClientOverlayHistoryNode* nodes;
    std::uint32_t capacity,first_word;
};
struct ClientOverlayPoolApi {
    std::uint32_t version,size;
    // Host provides stable storage, never relocated while registered.
    int (__cdecl *initialize)(void* watcher,unsigned bytes,ClientOverlayPool*,const ClientOverlayPoolStorage*) noexcept;
    int (__cdecl *record)(void* watcher,unsigned bytes,ClientOverlayPool*,const ClientOverlayPoolStorage*,float time,const ClientOverlaySample*,bool replace_future) noexcept;
    int (__cdecl *prune)(void* watcher,unsigned bytes,ClientOverlayPool*,const ClientOverlayPoolStorage*,float cutoff,unsigned* removed) noexcept;
    int (__cdecl *clear)(void* watcher,unsigned bytes,ClientOverlayPool*,const ClientOverlayPoolStorage*,unsigned* removed) noexcept;
    // Composes native selection/evaluation with internal pool pruning,
    // rather than requiring a caller-provided history/prune callback.
    int (__cdecl *apply)(void* watcher,unsigned bytes,void* row,unsigned row_bytes,float time,ClientOverlayPool*,const ClientOverlayPoolStorage*,int* wrote,unsigned* removed) noexcept;
};
using GetClientOverlayPoolApi=const ClientOverlayPoolApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// Original capture/reset/restore orchestration for a single overlay watcher.
// Kept as a separate export so the Phase1014 pool API ABI remains unchanged.
inline constexpr char client_overlay_capture_export[]="SiN_GetClientOverlayCaptureApi";
struct ClientOverlayCaptureApi {
    std::uint32_t version,size;
    int (__cdecl *trim)(void* watcher,unsigned bytes,ClientOverlayPool*,const ClientOverlayPoolStorage*,float cutoff,unsigned* removed) noexcept;
    int (__cdecl *capture)(void* watcher,unsigned bytes,const void* row,unsigned row_bytes,float sample_time,float current_time,float network_time,ClientOverlayPool*,const ClientOverlayPoolStorage*,unsigned* removed) noexcept;
    int (__cdecl *reset)(void* watcher,unsigned bytes,const void* row,unsigned row_bytes,float current_time,ClientOverlayPool*,const ClientOverlayPoolStorage*) noexcept;
    int (__cdecl *restore)(void* watcher,unsigned bytes,void* row,unsigned row_bytes) noexcept;
};
using GetClientOverlayCaptureApi=const ClientOverlayCaptureApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
