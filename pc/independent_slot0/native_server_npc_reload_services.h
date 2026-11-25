#pragma once
#include "native_server_weapon_script_view.h"
#include "native_server_npc_condition.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_reload_services_export[]="SiN_GetServerNpcReloadServicesApi";
struct ServerNpcReloadServicesBindings {
    ServerNpcParentWeaponEventBindings original;
    ServerWeaponScriptView scripts;
    ServerNpcConditionMapBindings conditions;
};
// Caller-owned stable stack session; bound callbacks must not escape it.
struct ServerNpcReloadSession {
    void* actor;
    unsigned bytes;
    ServerNpcReloadServicesBindings host;
    ServerNpcParentWeaponEventBindings bound;
    unsigned script_lookups,condition_calls,sound_intervals,burst_queries;
};
struct ServerNpcReloadResult {
    ServerWeaponEventSoundResult sound;
    unsigned live,primary_changed,secondary_changed,conditions_cleared;
    int primary,secondary;
};
struct ServerNpcReloadServicesApi {
    std::uint32_t version,size;
    int (__cdecl *prepare)(void*,unsigned,const ServerNpcReloadServicesBindings*,ServerNpcReloadSession*) noexcept;
    // 2235e963: Grunt event2. Clears conditions even without a live weapon;
    // caller publishes +fc8 only after successful return.
    int (__cdecl *fixed_reset)(ServerNpcReloadSession*,ServerNpcReloadResult*) noexcept;
};
using GetServerNpcReloadServicesApi=const ServerNpcReloadServicesApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
