#pragma once
#include <cstdint>
#include <cstddef>
namespace sin_port::native_weapon_tables {
inline constexpr char client_overlay_history_export[]="SiN_GetClientOverlayHistoryApi";
// Raw PE32 history node at watcher+30. Node data starts at +4, links
// at +24/+28. The backwards link is not read by the selected routines.
struct ClientOverlaySample {
    int sequence;
    float previous_cycle,weight;
    int order;
    std::uint32_t unknown10;
    float cycle;
    std::uint32_t unknown18,unknown1c;
};
struct ClientOverlayHistoryNode {
    float time;
    ClientOverlaySample sample;
    std::uint32_t previous,next;
};
static_assert(sizeof(ClientOverlaySample)==0x20&&offsetof(ClientOverlaySample,cycle)==0x14);
static_assert(sizeof(ClientOverlayHistoryNode)==0x2c&&offsetof(ClientOverlayHistoryNode,next)==0x28);
struct ClientOverlayHistorySelection {
    ClientOverlayHistoryNode *earlier,*older,*newer;
    float fraction;
    int hermite;
};
struct ClientOverlayHistoryBindings {
    void* context;
    // Validate a nonzero PE32 address word and return its writable node.
    // All selected storage remains live until correct returns.
    ClientOverlayHistoryNode* (__cdecl *resolve)(void*,std::uint32_t) noexcept;
    // Original sequence metadata and watcher virtual +10 service.
    // Callbacks must not relocate or change selected history storage.
    bool (__cdecl *looping)(void*,void* actor,int sequence) noexcept;
    void (__cdecl *interpolate)(void*,void* watcher,float time) noexcept;
};
struct ClientOverlayHistoryApi {
    std::uint32_t version,size;
    int (__cdecl *select)(std::uint32_t head,int type,float time,float delay,const ClientOverlayHistoryBindings*,ClientOverlayHistorySelection*) noexcept;
    int (__cdecl *loop_lerp)(float fraction,float from,float to,float*) noexcept;
    int (__cdecl *correct)(void* actor,unsigned bytes,float time,const ClientOverlayHistoryBindings*,unsigned* changed) noexcept;
};
using GetClientOverlayHistoryApi=const ClientOverlayHistoryApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
