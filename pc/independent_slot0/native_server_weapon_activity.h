#pragma once
#include "server_entity_handle_core.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_weapon_activity_export[]="SiN_GetServerWeaponActivityApi";
enum ServerWeaponActivityClass : unsigned {
    WeaponActivityBase=0,WeaponActivityAssault,WeaponActivityChaingun,WeaponActivityGrenade,
    WeaponActivityMagnum,WeaponActivityMelee,WeaponActivityNpcPistol,WeaponActivityScatter,WeaponActivitySmg
};
struct ServerWeaponActivityRow {int input,output;bool required;};
struct ServerWeaponActivityBindings {
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    void* context;
    // Identify the actual independent weapon class, not a retail address.
    int (__cdecl *classify)(void*,void* weapon,unsigned*) noexcept;
    // Base weapon capability uses script-info +720, not a constant.
    int (__cdecl *base_capabilities)(void*,void* weapon,unsigned*) noexcept;
};
struct ServerWeaponActivityApi {
    std::uint32_t version,size;
    int (__cdecl *table)(unsigned,const ServerWeaponActivityRow**,unsigned*) noexcept;
    // 222ac480: no match leaves optional required output untouched.
    int (__cdecl *translate)(unsigned,int,bool*,int*) noexcept;
    // 222a53e0: missing/stale handle clears required output.
    int (__cdecl *actor_translate)(void*,unsigned,int,const ServerWeaponActivityBindings*,bool*,int*) noexcept;
    // 22030360: raw actor +8ec OR active weapon +4a0 result.
    int (__cdecl *capabilities)(void*,unsigned,const ServerWeaponActivityBindings*,unsigned*) noexcept;
};
using GetServerWeaponActivityApi=const ServerWeaponActivityApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
