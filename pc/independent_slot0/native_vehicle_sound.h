#pragma once
#include "native_vehicle_actions.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_sound_export[]="SiN_GetVehicleSoundApi";
struct VehicleSoundBindings {
    void* context;
    // Original vehicle controller +610 -> float +16c. Observe the
    // existing engine value without recreating vehicle physics.
    int (__cdecl *controller_scalar)(void*,void* vehicle,float*) noexcept;
    int (__cdecl *emit)(void*,void* vehicle,const char* sound,unsigned arg1,unsigned arg2) noexcept;
    int (__cdecl *lookup_sequence)(void*,void* vehicle,const char*,int*) noexcept;
    int (__cdecl *set_sequence)(void*,void* vehicle,int) noexcept;
    int (__cdecl *reset_sequence)(void*,void* vehicle) noexcept;
};
struct VehicleSoundApi {
    std::uint32_t version,size;
    // Borrowed record table from the original configuration loader:
    // pointer +838, signed count +844, stride128, key/sound 64 bytes.
    int (__cdecl *find)(const void* vehicle,unsigned bytes,const char* group,const char* action,int index,int* found) noexcept;
    int (__cdecl *sound)(void* vehicle,unsigned bytes,const VehicleSoundBindings*,const char* group,const char* action,int index) noexcept;
    int (__cdecl *sequence)(void* vehicle,unsigned bytes,const VehicleSoundBindings*,const char* name) noexcept;
};
using GetVehicleSoundApi=const VehicleSoundApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
