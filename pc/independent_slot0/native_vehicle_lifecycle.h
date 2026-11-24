#pragma once
#include "native_vehicle_config.h"
#include "native_music_radio_map.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_lifecycle_export[]="SiN_GetVehicleLifecycleApi";
inline constexpr unsigned vehicle_object_bytes=0x890;
enum class VehicleConfigKind:unsigned {Damage,Sounds,Views};
struct VehicleLifecycleBindings {
    void* context;
    VehicleConfigBindings config;
    MusicRadioMapBindings music;
    int (__cdecl *base_spawn)(void*,void* vehicle) noexcept;
    int (__cdecl *base_activate)(void*,void* vehicle) noexcept;
    int (__cdecl *initialize_subsystem)(void*,void* vehicle,unsigned offset) noexcept;
    // Existing parsed configuration access: each original helper opens
    // independently. absent config is not a transport failure.
    int (__cdecl *open_config)(void*,void* vehicle,VehicleConfigKind,unsigned* present) noexcept;
    int (__cdecl *close_config)(void*,void* vehicle,VehicleConfigKind) noexcept;
    int (__cdecl *append_sound)(void*,void* vehicle) noexcept;
    int (__cdecl *precache)(void*,const char*) noexcept;
    // Existing factory and EHANDLE services. First classname match,
    // NOT targetname. The linked radio must have genuine live storage.
    int (__cdecl *find_radio)(void*,const char* classname,void**) noexcept;
    int (__cdecl *handle)(void*,void*,std::uint32_t*) noexcept;
    int (__cdecl *resolve_radio)(void*,std::uint32_t,HologramControl**) noexcept;
    int (__cdecl *parent_radio)(void*,void* radio,void* vehicle,int attachment) noexcept;
    // Existing engine model/attachment services, no recreation here.
    int (__cdecl *initialize_components)(void*,void* vehicle) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *health_dirty)(void*,void* vehicle) noexcept;
    int (__cdecl *spawn_tail)(void*,void* vehicle,unsigned flags) noexcept;
    int (__cdecl *activate_tail)(void*,void* vehicle,unsigned offset) noexcept;
};
struct VehicleLifecycleApi {
    std::uint32_t version,size;
    // Scalar constructor leaves; actual base, vector/output storage,
    // vtables and destructor belong to the real class factory.
    int (__cdecl *defaults)(HologramControl*) noexcept;
    int (__cdecl *spawn)(HologramControl*,const VehicleLifecycleBindings*) noexcept;
    int (__cdecl *activate)(HologramControl*,const VehicleLifecycleBindings*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetVehicleLifecycleApi=const VehicleLifecycleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
