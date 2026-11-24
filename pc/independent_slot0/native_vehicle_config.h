#pragma once
#include "native_vehicle_views.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_config_export[]="SiN_GetVehicleConfigApi";
struct VehicleConfigEntry {
    const char* name;
    const char* text;
    int integer;
    float real;
};
struct VehicleConfigBindings {
    void* context;
    // Existing engine-owned, already parsed configuration. Do not open
    // model files, parse physics data or replace Source KeyValues.
    // Missing block reports present=0 at index 0. Duplicate sibling keys
    // must remain ordered. Strings borrowed until next callback.
    int (__cdecl *entry)(void*,const char* block,unsigned index,unsigned* present,VehicleConfigEntry*) noexcept;
    // Host allocator appends one original 100-byte record, updates
    // +7fc/+808. Attachment lookup uses existing Source model services.
    int (__cdecl *append_view)(void*,void* vehicle) noexcept;
    int (__cdecl *attachment_id)(void*,void* vehicle,const char* name,int*) noexcept;
};
struct VehicleConfigApi {
    std::uint32_t version,size;
    // Exact gameplay view construction after successful original config
    // load; roots then links then temporary views, then ordered overrides.
    int (__cdecl *views)(HologramControl*,const VehicleConfigBindings*) noexcept;
    // Optional damage_mod block, first matching key per damage category.
    int (__cdecl *damage)(HologramControl*,const VehicleConfigBindings*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetVehicleConfigApi=const VehicleConfigApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
