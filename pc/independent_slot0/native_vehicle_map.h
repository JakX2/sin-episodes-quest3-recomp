#pragma once
#include "native_vehicle_views.h"
#include "native_music_radio.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_map_export[]="SiN_GetVehicleMapApi";
enum class VehicleFieldKind:unsigned {Key,Input,Output};
struct VehicleField {
    const char* name;
    VehicleFieldKind kind;
    unsigned type,offset,original_handler;
};
struct VehicleMapSchema {
    const char* classname;
    unsigned field_count;
    const VehicleField* fields;
};
struct VehicleMapBindings {
    void* context;
    VehicleViewBindings views;
    MusicRadioBindings radio;
    int (__cdecl *enter)(void*,void* vehicle,void* activator,int role) noexcept;
    int (__cdecl *occupant)(void*,void* vehicle,int role,void**) noexcept;
    int (__cdecl *exit)(void*,void* vehicle,void* occupant,unsigned argument) noexcept;
    int (__cdecl *sound)(void*,void* vehicle,const char*,const char*,int) noexcept;
    int (__cdecl *dirty_edict)(void*,void* vehicle,std::uint32_t edict_word) noexcept;
    // Existing Source variant string conversion and pooled-word
    // normalization; empty or null ->0. Engine owns string lifetime.
    int (__cdecl *camera_value)(void*,unsigned type,std::uint32_t word,std::uint32_t*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *unlink_camera)(void*,void* camera) noexcept;
    // A distinct persistent control for a live linked env_radio.
    int (__cdecl *radio_control)(void*,void* radio,HologramControl**) noexcept;
};
struct VehicleMapApi {
    std::uint32_t version,size;
    const VehicleMapSchema* (__cdecl *schema)() noexcept;
    int (__cdecl *key)(HologramControl*,const char*,unsigned type,std::uint32_t word) noexcept;
    int (__cdecl *input)(HologramControl*,const VehicleMapBindings*,const char*,unsigned type,std::uint32_t word,void* activator) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetVehicleMapApi=const VehicleMapApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
