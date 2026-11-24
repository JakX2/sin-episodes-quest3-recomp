#pragma once
#include "native_vehicle_lifecycle.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_part_events_export[]="SiN_GetVehiclePartEventsApi";
struct VehiclePartEventBindings {
    void* context;
    // Existing engine pose parameter services. Index is the observed
    // original record's +40, not a guessed model attachment index.
    int (__cdecl *pose)(void*,void* vehicle,int parameter,float*) noexcept;
    int (__cdecl *frame_time)(void*,float*) noexcept;
    int (__cdecl *set_pose)(void*,void* vehicle,int parameter,float) noexcept;
    // FIELD_INTEGER variant, original offset, index, activator/caller
    // both vehicle, delay zero. Host owns actual output dispatch.
    int (__cdecl *output)(void*,void* vehicle,unsigned offset,int index) noexcept;
    int (__cdecl *sound)(void*,void* vehicle,const char*,const char*,int) noexcept;
};
struct VehiclePartEventsApi {
    std::uint32_t version,size;
    // Only door/window gameplay completion slice of 22330e40. Host
    // retains wiper/monitor pose work and model/animation services.
    int (__cdecl *tick)(HologramControl*,const VehiclePartEventBindings*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetVehiclePartEventsApi=const VehiclePartEventsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
