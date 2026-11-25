#pragma once
#include "native_server_animation_events.h"
#include "native_server_activity_dictionary.h"
#include "native_server_weapon_event_route.h"
#include "native_server_npc_reload_services.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_grunt_animation_export[]="SiN_GetServerGruntAnimationApi";
struct ServerGruntAnimationBindings {
    void* context;
    int begin_altfire,altfire; // registered IDs, not guessed literals
    // Exact original virtual/service boundaries, stable synchronous objects.
    void (__cdecl *debug)(void*,const char*) noexcept;
    int (__cdecl *weapon)(void*,void* grunt,std::uint32_t handle,void**) noexcept;
    void (__cdecl *weapon_mode)(void*,void* weapon,int,int) noexcept; // +3c4
    int (__cdecl *weapon_value)(void*,void* weapon,unsigned slot) noexcept; // +404/+408
    void (__cdecl *weapon_store)(void*,void* weapon,unsigned offset,int value) noexcept; // +4ec/+4f0
    void (__cdecl *clear_condition)(void*,void* grunt,int) noexcept; // 220398a0
    void (__cdecl *weapon_event)(void*,void* weapon,int event,void* grunt) noexcept; // +4c0
    // For nonnull +9c4, enumerate the original qualifying squad members;
    // bounded stable array of writable peers, each with >=0xc94 bytes.
    int (__cdecl *peers)(void*,void* grunt,std::uint32_t squad,void***,unsigned*) noexcept;
    int (__cdecl *activity)(void*,void* grunt) noexcept; // +7f8
    void (__cdecl *activity_request)(void*,void* grunt,int activity,int,int) noexcept; // 220a35c0 on +cc0
    void (__cdecl *parent)(void*,void* grunt,const ServerAnimationEvent*) noexcept; // 2204a3b0
};
enum ServerGruntAnimationRoute : unsigned {
    GruntAnimParent=0,GruntAnimBlocking=1,GruntAnimBeginAlt=2,GruntAnimAlt=3,
    GruntAnimWeaponReset=4,GruntAnimMark=5,GruntAnimActivity=6,GruntAnimUnrecovered=7
};
struct ServerGruntAnimationResult {unsigned route;unsigned peers_updated;};
struct ServerGruntAnimationApi {
    std::uint32_t version,size;
    // 2235e8c0, subset of actual +2c0 animation handler. Returns 1 for
    // completed route, 0 for known unrecovered route, -1 for invalid/host failure.
    int (__cdecl *handle)(void* grunt,unsigned bytes,const ServerAnimationEvent*,float now,const ServerGruntAnimationBindings*,ServerGruntAnimationResult*) noexcept;
};
using GetServerGruntAnimationApi=const ServerGruntAnimationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_grunt_registration_export[]="SiN_GetServerGruntRegistrationApi";
struct ServerGruntRegistration {
    const ServerActivityDictionary* activity_owner;
    const ServerEventDictionary* event_owner;
    unsigned activity_generation,event_generation;
    int activities[6],events[2]; // native registration order
    bool activities_ready,events_ready;
};
struct ServerGruntRegistrationBindings {
    void* context;
    // Optional host mirrors of 22022a60/22060530, after native ID assignment.
    void (__cdecl *publish)(void*,bool event,const char* name,int id) noexcept;
};
struct ServerGruntActivityBindings {
    void* context;
    int (__cdecl *has_weapon)(void*,void* actor,const char* classname,int owner_tag,bool*) noexcept;
    int (__cdecl *component_identity)(void*,void* actor,std::uint32_t component,bool*) noexcept;
    int (__cdecl *component_translate)(void*,std::uint32_t component,int activity,int*) noexcept;
    int (__cdecl *parent_translate)(void*,void* actor,int activity,int*) noexcept;
};
struct ServerGruntRegistrationApi {
    std::uint32_t version,size;
    int (__cdecl *grunt)(ServerActivityDictionary*,ServerEventDictionary*,const ServerGruntRegistrationBindings*,ServerGruntRegistration*) noexcept;
    int (__cdecl *jetpack)(ServerEventDictionary*,const ServerGruntRegistrationBindings*,ServerGruntRegistration*) noexcept;
    // 2235a250: native raw Grunt activity remapping, explicit external leaves.
    int (__cdecl *translate)(void*,unsigned,int,const ServerGruntRegistration*,const ServerGruntActivityBindings*,int*) noexcept;
    // Use registered IDs, not caller-assigned numeric altfire values.
    int (__cdecl *handle)(void*,unsigned,const ServerAnimationEvent*,float,const ServerGruntRegistration*,const ServerGruntAnimationBindings*,ServerGruntAnimationResult*) noexcept;
};
using GetServerGruntRegistrationApi=const ServerGruntRegistrationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_jetpack_activity_export[]="SiN_GetServerJetpackActivityApi";
struct ServerJetpackActivityBindings {
    ServerGruntActivityBindings grunt;
    void* context;
    // Resolve original +1024 serial handle. Return stable readable
    // entity bytes, or fail if absent/stale. No physical simulation.
    int (__cdecl *attachment)(void*,std::uint32_t,void**,unsigned*) noexcept;
};
struct ServerJetpackActivityResult {
    int activity;
    float magnitude;
    unsigned delegated;
};
struct ServerJetpackActivityApi {
    std::uint32_t version,size;
    // 22364700: read attachment before any branch; rounded magnitude
    // selects 208/213, reload is 218, otherwise native Grunt translation.
    int (__cdecl *translate)(void*,unsigned,int,const ServerGruntRegistration*,const ServerJetpackActivityBindings*,ServerJetpackActivityResult*) noexcept;
};
using GetServerJetpackActivityApi=const ServerJetpackActivityApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_grunt_native_weapon_event_export[]="SiN_GetServerGruntNativeWeaponEventApi";
struct ServerGruntNativeWeaponEventResult {
    ServerGruntAnimationResult grunt;
    ServerWeaponEventRouteResult altfire;
    ServerNpcParentWeaponEventResult parent;
    unsigned native_altfire,native_parent;
};
struct ServerGruntNativeWeaponEventApi {
    std::uint32_t version,size;
    // Retains original Grunt dispatch precedence; altfire and legacy parent
    // weapon routes use checked native services, no void weapon-event shim.
    int (__cdecl *handle)(void*,unsigned,const ServerAnimationEvent*,float,const ServerGruntRegistration*,const ServerGruntAnimationBindings*,const ServerNpcParentWeaponEventBindings*,ServerGruntNativeWeaponEventResult*) noexcept;
};
using GetServerGruntNativeWeaponEventApi=const ServerGruntNativeWeaponEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_grunt_script_weapon_event_export[]="SiN_GetServerGruntScriptWeaponEventApi";
struct ServerGruntScriptWeaponEventResult {
    ServerGruntNativeWeaponEventResult event;
    ServerNpcReloadResult reload;
    unsigned fixed_reset;
};
struct ServerGruntScriptWeaponEventApi {
    std::uint32_t version,size;
    int (__cdecl *handle)(void*,unsigned,const ServerAnimationEvent*,float,const ServerGruntRegistration*,const ServerGruntAnimationBindings*,ServerNpcReloadSession*,ServerGruntScriptWeaponEventResult*) noexcept;
};
using GetServerGruntScriptWeaponEventApi=const ServerGruntScriptWeaponEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
