#pragma once
#include "native_server_activity_assignment.h"
#include "native_server_inherited_activity.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_native_activity_services_export[]="SiN_GetServerNativeActivityServicesApi";
// Stable nonmovable caller-owned session. Its bound assignment can be copied
// into pending transitions or animation tick bindings, not beyond session life.
struct ServerNativeActivitySession;
struct ServerNativeGruntForward {
    ServerNativeActivitySession* session;
    ServerGruntActivityBindings original;
};
struct ServerNativeActivitySession {
    void* actor;
    unsigned bytes;
    ServerNpcActivityAssignmentBindings original,bound;
    ServerInheritedActivityBindings inherited;
    ServerNativeGruntForward grunt,jetpack;
    unsigned weapon_calls,parent_calls,capability_calls;
};
struct ServerNativeActivityServicesApi {
    std::uint32_t version,size;
    int (__cdecl *prepare)(void*,unsigned,const ServerNpcActivityAssignmentBindings*,const ServerInheritedActivityBindings*,ServerNativeActivitySession*) noexcept;
};
using GetServerNativeActivityServicesApi=const ServerNativeActivityServicesApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
