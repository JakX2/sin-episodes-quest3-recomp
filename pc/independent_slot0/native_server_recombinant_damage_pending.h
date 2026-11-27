#pragma once
#include "native_server_recombinant_damage.h"
#include "native_server_recombinant_activity_transition.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_damage_pending_export[]="SiN_GetServerRecombinantDamagePendingApi";
// Separate from the original damage ABI to avoid a transitive include cycle.
// This call-local bridge connects Major damage/ignition +4dc requests to
// the native pending-activity lifecycle without advancing animation itself.
struct ServerRecombinantDamagePendingBindings {
    const ServerRecombinantDamageBindings* damage;
    const ServerRecombinantPendingActivityBindings* pending;
};
struct ServerRecombinantDamagePendingResult {
    ServerRecombinantDamageResult damage;
    ServerRecombinantIgniteResult ignition;
    ServerRecombinantPendingActivityResult pending;
    unsigned pending_calls;
    int requested_activity;
};
struct ServerRecombinantDamagePendingApi {
    unsigned version,size;
    int (__cdecl *damage_major)(void*,unsigned,const void*,unsigned,float,
        const ServerRecombinantDamagePendingBindings*,ServerRecombinantDamagePendingResult*) noexcept;
    int (__cdecl *ignite_major)(void*,unsigned,const ServerRecombinantIgniteRequest*,
        const ServerRecombinantDamagePendingBindings*,ServerRecombinantDamagePendingResult*) noexcept;
};
using GetServerRecombinantDamagePendingApi=const ServerRecombinantDamagePendingApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
