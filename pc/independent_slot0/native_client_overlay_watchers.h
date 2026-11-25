#pragma once
#include <cstdint>
namespace sin_port::native_weapon_tables {
inline constexpr char client_overlay_watchers_export[]="SiN_GetClientOverlayWatchersApi";
// Host supplies preallocated watcher and mapping storage and the original
// watcher object lifetime/virtual services. All void services are infallible
// after preflight and must not relocate any of the three vectors.
struct ClientOverlayWatcherBindings {
    void* context;
    unsigned char* (__cdecl *grow_layers)(void*,unsigned char*,unsigned) noexcept;
    void (__cdecl *construct)(void*,void* watcher) noexcept;
    void (__cdecl *destroy)(void*,void* watcher) noexcept;
    void (__cdecl *setup)(void*,void* watcher,void* layer,int type) noexcept;
    int (__cdecl *type)(void*,void* watcher) noexcept;
    float (__cdecl *amount)(void*,void* actor,int type) noexcept;
    void (__cdecl *set_amount)(void*,void* watcher,float amount) noexcept;
};
struct ClientOverlayWatchersApi {
    std::uint32_t version,size;
    int (__cdecl *reserve)(void*,unsigned bytes,int target,const ClientOverlayWatcherBindings*) noexcept;
    int (__cdecl *resize)(void*,unsigned bytes,int target,const ClientOverlayWatcherBindings*) noexcept;
};
using GetClientOverlayWatchersApi=const ClientOverlayWatchersApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
