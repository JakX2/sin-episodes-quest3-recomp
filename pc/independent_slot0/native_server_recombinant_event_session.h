#pragma once
#include "native_server_recombinant_pounce.h"
#include "native_server_recombinant_spit.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_event_session_export[]="SiN_GetServerRecombinantEventSessionApi";
// The session lives for a single call. Original engine services remain owned
// by their providers; it only composes original Minor virtual event overrides.
struct ServerRecombinantEventSessionBindings {
    const ServerRecombinantEventBindings* events;
    const ServerRecombinantAirborneBindings* airborne;
    const ServerRecombinantSpitBindings* spit;
    // Opt-in native pounce: otherwise retain the original effect callback.
    const ServerRecombinantPounceBindings* pounce;
};
struct ServerRecombinantEventSessionResult {
    unsigned completed,leap_invocations,spit_invocations,pounce_off_invocations,pounce_slash_invocations,delegated_effects;
    ServerRecombinantEventResult event;
    ServerRecombinantAirborneResult airborne;
    ServerRecombinantSpitResult spit;
    ServerRecombinantPounceResult pounce;
};
struct ServerRecombinantEventSessionApi {
    unsigned version,size;
    int (__cdecl *execute)(void*,unsigned,const void*,unsigned,float,
        const ServerRecombinantEventSessionBindings*,ServerRecombinantEventSessionResult*) noexcept;
};
using GetServerRecombinantEventSessionApi=const ServerRecombinantEventSessionApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
