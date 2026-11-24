#pragma once
#include "native_vehicle_seat_events.h"
#include "native_vehicle_part_events.h"
#include "native_vehicle_think.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_nested_export[]="SiN_GetVehicleNestedApi";
// These are synchronous host callbacks, not general entry points.
// Caller must use the SAME persistent control as the parent map input
// or Think, on the owning engine thread, while its specific callback
// is executing. The original standalone APIs still reject busy controls.
// No object, vector, or control lifetime may end during a callback.
struct VehicleNestedApi {
    std::uint32_t version,size;
    int (__cdecl *entered)(HologramControl*,const VehicleSeatEventBindings*,int seat,void* actor) noexcept;
    int (__cdecl *exiting)(HologramControl*,const VehicleSeatEventBindings*,int seat) noexcept;
    int (__cdecl *parts)(HologramControl*,const VehiclePartEventBindings*) noexcept;
};
using GetVehicleNestedApi=const VehicleNestedApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
