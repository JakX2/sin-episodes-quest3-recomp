#pragma once
#include "native_server_recombinant_policy.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_damage_export[]="SiN_GetServerRecombinantDamageApi";
// Exact original six-word +2d4 argument shape; only the lifetime and
// boolean mode have established types. Remaining words stay opaque.
struct ServerRecombinantIgniteRequest {
    float lifetime;unsigned mode,arg2,arg3,arg4,arg5;
};
static_assert(sizeof(ServerRecombinantIgniteRequest)==24);
struct ServerRecombinantDamageBindings {
    const ServerRecombinantPolicyBindings* policy;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *classification)(void*,void*,int*) noexcept; // original +d0
    int (__cdecl *color_fade)(void*,void*,int,int) noexcept; // 220f2900
    int (__cdecl *parent_damage)(void*,void*,const void*,unsigned,int*) noexcept; // 22342cb0
    int (__cdecl *parent_ignite)(void*,void*,const ServerRecombinantIgniteRequest*) noexcept; // 220f2210
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *health_changed)(void*,void*) noexcept; // +1b4, before write
    int (__cdecl *pending_activity)(void*,void*,int) noexcept; // +4dc
};
struct ServerRecombinantIgniteResult {
    unsigned completed,parent_called,random_calls,health_written,pending_called;
    int old_health,new_health,activity;
    float draws[2];
};
struct ServerRecombinantDamageResult {
    unsigned completed,source_live,immune,color_called,accumulated,threshold_reached,ignited,parent_called;
    int source_class,damage_result;
    float accumulated_damage,deadline;
    ServerRecombinantIgniteResult ignition;
};
struct ServerRecombinantDamageExpiryResult {unsigned completed,cleared;float deadline,prior;};
struct ServerRecombinantDamageApi {
    unsigned version,size;
    int (__cdecl *damage_major)(void*,unsigned,const void*,unsigned,float,const ServerRecombinantDamageBindings*,ServerRecombinantDamageResult*) noexcept;
    int (__cdecl *ignite_major)(void*,unsigned,const ServerRecombinantIgniteRequest*,const ServerRecombinantDamageBindings*,ServerRecombinantIgniteResult*) noexcept;
    // Original preschedule prefix only; later custom object manipulation
    // remains separate, rather than pretending to implement the full method.
    int (__cdecl *expire_major)(void*,unsigned,float,ServerRecombinantDamageExpiryResult*) noexcept;
};
using GetServerRecombinantDamageApi=const ServerRecombinantDamageApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
