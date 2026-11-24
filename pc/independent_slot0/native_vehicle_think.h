#pragma once
#include "native_vehicle_passengers.h"
#include "native_vehicle_lifecycle.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_think_export[]="SiN_GetVehicleThinkApi";
inline constexpr unsigned camera_link_bytes=0x3a0;
struct VehicleCameraBindings {
    void* context;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *find_named)(void*,std::uint32_t string_word,void**) noexcept;
    int (__cdecl *is_point_camera)(void*,void*,unsigned*) noexcept;
    // Real engine factory constructs info_camera_link and registers its
    // original global list entry. Null means no helper could be created.
    int (__cdecl *create)(void*,const char*,HologramControl**) noexcept;
    int (__cdecl *handle)(void*,void*,std::uint32_t*) noexcept;
    int (__cdecl *spawn)(void*,void* link) noexcept;
    // Actual engine deferred-removal service, not camera destruction.
    int (__cdecl *remove_link)(void*,void* link) noexcept;
    int (__cdecl *diagnostic)(void*,std::uint32_t string_word) noexcept;
};
struct VehicleThinkBindings {
    void* context;
    VehiclePassengerBindings passengers;
    VehicleCameraBindings camera;
    // The host supplies existing base and component update services.
    // No recreation of vehicle physics, geometry or model animation.
    int (__cdecl *base_think)(void*,void*) noexcept;
    int (__cdecl *wiper_update)(void*,void*) noexcept;
    int (__cdecl *driver_update)(void*,void*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *dirty_edict)(void*,void*,std::uint32_t) noexcept;
    int (__cdecl *schedule)(void*,void*,float,unsigned) noexcept;
    int (__cdecl *startup)(void*,void*,unsigned stage) noexcept;
    int (__cdecl *active_predicate)(void*,void*,unsigned*) noexcept;
    int (__cdecl *frame_time)(void*,float*) noexcept;
    int (__cdecl *tail)(void*,void*) noexcept;
};
struct VehicleThinkApi {
    std::uint32_t version,size;
    int (__cdecl *link_defaults)(HologramControl*) noexcept;
    int (__cdecl *refresh_camera)(HologramControl*,const VehicleCameraBindings*) noexcept;
    int (__cdecl *think)(HologramControl*,const VehicleThinkBindings*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetVehicleThinkApi=const VehicleThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
