#pragma once
#include "native_server_animation_events.h"
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
}
