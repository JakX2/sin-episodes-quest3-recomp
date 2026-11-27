#pragma once
#include "native_server_recombinant_minor_task.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_registration_export[]="SiN_GetServerRecombinantRegistrationApi";
struct ServerRecombinantActivityDefinition {const char* name;unsigned original_slot,original_store;};
struct ServerRecombinantActivityRegistration {
    unsigned kind,generation,count,ready;
    const ServerActivityDictionary* owner;
    int ids[10];
};
struct ServerRecombinantRegistrationBindings {
    void* context;
    // Optional name publication; original 22022a60 follows each register call.
    void (__cdecl *publish)(void*,const ServerRecombinantActivityDefinition*,int) noexcept;
};
struct ServerRecombinantRegistrationApi {
    unsigned version,size;
    int (__cdecl *definitions)(unsigned,const ServerRecombinantActivityDefinition**,unsigned*) noexcept;
    int (__cdecl *register_activities)(unsigned,ServerActivityDictionary*,const ServerRecombinantRegistrationBindings*,ServerRecombinantActivityRegistration*) noexcept;
    int (__cdecl *valid)(const ServerRecombinantActivityRegistration*) noexcept;
    // Copy caller's task bindings and bind the two live original private IDs.
    int (__cdecl *bind_minor)(const ServerRecombinantActivityRegistration*,const ServerRecombinantMinorTaskBindings*,ServerRecombinantMinorTaskBindings*) noexcept;
};
using GetServerRecombinantRegistrationApi=const ServerRecombinantRegistrationApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
