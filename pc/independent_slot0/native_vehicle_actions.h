#pragma once
#include "native_vehicle_use.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_actions_export[]="SiN_GetVehicleActionsApi";
struct VehicleActionBindings {
    void* context;
    // Original integer output helper 221f62d0 at vehicle+7cc; this
    // precedes every branch, even unimplemented or invalid indices.
    int (__cdecl *output)(void*,void* vehicle,void* actor,unsigned offset,int value) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *participant)(void*,void* actor,unsigned*) noexcept;
    // Host-owned passenger interface +0xc/+0x44 and observed seat field.
    int (__cdecl *occupied_seat)(void*,void* vehicle,void* actor,int*) noexcept;
    int (__cdecl *candidate_seat)(void*,void* vehicle,void* actor,int*) noexcept;
    int (__cdecl *seat_type)(void*,void* vehicle,int seat,int*) noexcept;
    int (__cdecl *passenger_enter)(void*,void* vehicle,void* actor,int requested) noexcept;
    int (__cdecl *passenger_exit)(void*,void* vehicle,void* actor,unsigned argument) noexcept;
    // Existing sound dispatch helper 2232fe20 and sequence helper
    // 2232e240. These are engine service boundaries, not named map I/O.
    int (__cdecl *sound)(void*,void* vehicle,const char* group,const char* action,int index) noexcept;
    int (__cdecl *sequence)(void*,void* vehicle,const char* name) noexcept;
    int (__cdecl *radio_next)(void*,void* radio) noexcept;
    int (__cdecl *unimplemented)(void*,int part) noexcept;
};
struct VehicleActionsApi {
    std::uint32_t version,size;
    // Invoke original action after admission. Do not call while the
    // same persistent control is busy in VehicleUseApi; that API's
    // callback may instead invoke apply_leaf using its borrowed object.
    int (__cdecl *apply)(HologramControl*,const VehicleActionBindings*,void* actor,unsigned actor_bytes,int part) noexcept;
    int (__cdecl *apply_leaf)(void* vehicle,unsigned bytes,const VehicleActionBindings*,void* actor,unsigned actor_bytes,int part) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetVehicleActionsApi=const VehicleActionsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
