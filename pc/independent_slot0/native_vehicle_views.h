#pragma once
#include "native_weapon_datatables.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_views_export[]="SiN_GetVehicleViewsApi";
struct VehicleViewBindings {
    void* context;
    // Host validates the original cast to the player subclass and the
    // complete player extent before lending its persistent control.
    int (__cdecl *cast_player)(void*,void* activator,HologramControl**) noexcept;
    int (__cdecl *dirty_edict)(void*,void* player,std::uint32_t edict_word) noexcept;
    int (__cdecl *output)(void*,void* vehicle,void* player,unsigned offset,int index) noexcept;
    // Existing engine attachment service. Missing attachment is a
    // normal outcome. Do not implement camera, model or geometry here.
    int (__cdecl *attachment)(void*,void* vehicle,int index,float position[3],unsigned* found) noexcept;
    int (__cdecl *set_position)(void*,void* player,const float position[3]) noexcept;
};
struct VehicleViewsApi {
    std::uint32_t version,size;
    int (__cdecl *set_view)(HologramControl* vehicle,const VehicleViewBindings*,void* activator,unsigned type,int value) noexcept;
    // Post-entry scalar slice, after base passenger services, before hint.
    int (__cdecl *seat_default)(HologramControl* vehicle,HologramControl* player,const VehicleViewBindings*,int seat) noexcept;
    // Tail of original automatic directional transition; host already
    // selected the candidate and performed prerequisite door/window work.
    int (__cdecl *directional)(HologramControl* vehicle,HologramControl* player,const VehicleViewBindings*,int candidate,const float position[3]) noexcept;
    // Original return chain from current view following +40 while +54.
    // Host has already checked input and the runtime enable cvar.
    int (__cdecl *return_view)(HologramControl* vehicle,HologramControl* player,const VehicleViewBindings*) noexcept;
};
using GetVehicleViewsApi=const VehicleViewsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
