#pragma once
#include "native_use_observation.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_use_export[]="SiN_GetVehicleUseApi";
struct VehicleUseBindings {
    void* context;
    // Existing engine hitbox-set/ray query 2232eac0. A miss is index -1.
    int (__cdecl *pick_part)(void*,void* vehicle,void* actor,int*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t handle,void**) noexcept;
    int (__cdecl *participant)(void*,void* actor,unsigned*) noexcept;
    // Existing passenger interface query, not a new geometry implementation.
    int (__cdecl *outside_seat)(void*,void* vehicle,void* actor,int*) noexcept;
    // Original vehicle vslot +314. The host supplies actual part behavior.
    int (__cdecl *apply_part)(void*,void* vehicle,void* actor,int part) noexcept;
};
struct VehicleUseApi {
    std::uint32_t version,size;
    const char* (__cdecl *part_name)(int) noexcept;
    int (__cdecl *can_use)(HologramControl*,const VehicleUseBindings*,void* actor,unsigned actor_bytes,unsigned* allowed) noexcept;
    // Optional use record at use-data+10; original sets subobject+2c0.
    // Executed is distinct from admission: only use type 3 dispatches.
    int (__cdecl *use)(HologramControl*,const VehicleUseBindings*,void* actor,unsigned actor_bytes,int type,void* use_record,unsigned record_bytes,unsigned* executed) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetVehicleUseApi=const VehicleUseApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
