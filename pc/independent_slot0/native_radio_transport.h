#pragma once
#include "native_radio_hud.h"
#include "native_radio_message.h"
namespace sin_port::native_weapon_tables {
inline constexpr char radio_delivery_export[]="SiN_GetRadioDeliveryApi";
inline constexpr char radio_route_export[]="SiN_GetRadioClientRouteApi";
struct RadioEdictObservation {
    unsigned present,flags;
    void* unknown;
};
struct RadioDeliveryBindings {
    void* context;
    int (__cdecl *max_clients)(void*,int*) noexcept;
    int (__cdecl *edict)(void*,unsigned index,RadioEdictObservation*) noexcept;
    int (__cdecl *player_from_unknown)(void*,void*,void**) noexcept;
    // Transport owns the actual recipient filter and message manager.
    // Reliable flag and exact byte-aligned payload are recovered leaves.
    int (__cdecl *send)(void*,void* player,unsigned reliable,const char* name,const unsigned char*,unsigned bytes) noexcept;
    int (__cdecl *register_message)(void*,const char*,int bytes) noexcept;
};
struct RadioDeliveryApi {
    std::uint32_t version,size;
    int (__cdecl *register_message)(const RadioDeliveryBindings*) noexcept;
    // No valid recipient is a successful no-op, not a transport error.
    int (__cdecl *notify)(const RadioDeliveryBindings*,unsigned active,int priority,const char* speaker) noexcept;
};
using GetRadioDeliveryApi=const RadioDeliveryApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct RadioClientRouteBindings {
    void* context;
    int (__cdecl *register_message)(void*,const char*,int bytes) noexcept;
    // Existing client HUD lookup (complete object identity).
    int (__cdecl *find_hud)(void*,const char*,HologramControl**) noexcept;
    RadioHudBindings hud;
};
struct RadioClientRouteApi {
    std::uint32_t version,size;
    int (__cdecl *register_message)(const RadioClientRouteBindings*) noexcept;
    // Hook entry is installed by the owner of the client message manager.
    int (__cdecl *receive)(const RadioClientRouteBindings*,const unsigned char*,unsigned bytes) noexcept;
};
using GetRadioClientRouteApi=const RadioClientRouteApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
