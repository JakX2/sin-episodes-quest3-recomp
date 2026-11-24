#pragma once
#include "native_vehicle_map.h"
#include "native_vehicle_lifecycle.h"
#include "native_vehicle_hint.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_seat_events_export[]="SiN_GetVehicleSeatEventsApi";
struct VehicleSeatEventBindings {
    void* context;
    VehicleHintBindings hint;
    VehicleViewBindings views;
    int (__cdecl *base_enter)(void*,void* vehicle,int seat,void* actor) noexcept;
    int (__cdecl *base_exit)(void*,void* vehicle,int seat) noexcept;
    int (__cdecl *participant)(void*,void* actor,unsigned*) noexcept;
    int (__cdecl *sound)(void*,void* vehicle,const char*,const char*,int) noexcept;
    // Original 22330d40 and existing controller predicate +3c.
    int (__cdecl *refresh_environment)(void*,void* vehicle) noexcept;
    int (__cdecl *controller_predicate)(void*,void* vehicle,unsigned*) noexcept;
    // Existing interface lookup +130, cast and 2229ee80. No physics.
    int (__cdecl *controller_followup)(void*,void* vehicle) noexcept;
    int (__cdecl *dirty_edict)(void*,void* vehicle,std::uint32_t) noexcept;
};
struct VehicleSeatEventsApi {
    std::uint32_t version,size;
    int (__cdecl *entered)(HologramControl*,const VehicleSeatEventBindings*,int seat,void* actor) noexcept;
    int (__cdecl *exiting)(HologramControl*,const VehicleSeatEventBindings*,int seat) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetVehicleSeatEventsApi=const VehicleSeatEventsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
