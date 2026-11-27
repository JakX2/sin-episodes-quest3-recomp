#pragma once
#include "native_server_recombinant_melee.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_major_event_session_export[]="SiN_GetServerRecombinantMajorEventSessionApi";
struct ServerRecombinantMajorEventSessionBindings {
    const ServerRecombinantMajorEventBindings* events;
    // Optional native +774 override. When absent, original owner callback
    // remains responsible for the complete hit query.
    const ServerRecombinantMeleeBindings* melee;
};
struct ServerRecombinantMajorEventSessionResult {
    unsigned completed,melee_invocations,delegated_melee;
    ServerRecombinantMajorEventResult event;
    ServerRecombinantMeleeResult melee;
};
struct ServerRecombinantMajorEventSessionApi {
    unsigned version,size;
    int (__cdecl *execute)(void*,unsigned,const void*,unsigned,
        const ServerRecombinantMajorEventSessionBindings*,ServerRecombinantMajorEventSessionResult*) noexcept;
};
using GetServerRecombinantMajorEventSessionApi=const ServerRecombinantMajorEventSessionApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
