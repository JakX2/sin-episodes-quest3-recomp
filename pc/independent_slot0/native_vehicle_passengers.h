#pragma once
#include "native_vehicle_config.h"
namespace sin_port::native_weapon_tables {
inline constexpr char vehicle_passengers_export[]="SiN_GetVehiclePassengersApi";
struct VehiclePassengerBindings {
    void* context;
    int (__cdecl *occupant)(void*,void* vehicle,int seat,void**) noexcept;
    int (__cdecl *global_damage_scale)(void*,float*) noexcept;
    int (__cdecl *participant)(void*,void* occupant,unsigned*) noexcept;
    int (__cdecl *view_index)(void*,void* player,int*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    // Original occupant virtual +fc returns the combat character.
    int (__cdecl *combat_character)(void*,void* occupant,void**) noexcept;
    int (__cdecl *base_player_cast)(void*,void* combat,unsigned*) noexcept;
    int (__cdecl *ammo_index)(void*,const char*,int*) noexcept;
    int (__cdecl *ammo_allowed)(void*,void* combat,int index,unsigned*) noexcept;
    int (__cdecl *ammo_count)(void*,void* combat,int index,int*) noexcept;
    int (__cdecl *ammo_max)(void*,int index,int*) noexcept;
    // Original GiveAmmo virtual +334, maximum count, index, zero.
    // The original ignores the number actually granted.
    int (__cdecl *give_ammo)(void*,void* combat,int amount,int index,unsigned argument) noexcept;
};
struct VehiclePassengersApi {
    std::uint32_t version,size;
    // Constructor leaves directly related to this API only.
    int (__cdecl *defaults)(HologramControl*) noexcept;
    // This returns the multiplier, not a modified damage record.
    int (__cdecl *damage_factor)(HologramControl*,const VehiclePassengerBindings*,int seat,unsigned damage_flags,float*) noexcept;
    int (__cdecl *enable_ammo)(HologramControl*,unsigned type,std::uint32_t word) noexcept;
    int (__cdecl *refill)(HologramControl*,const VehiclePassengerBindings*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetVehiclePassengersApi=const VehiclePassengersApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
