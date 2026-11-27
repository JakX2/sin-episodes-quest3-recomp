#pragma once
#include "native_server_recombinant_events.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_attack_gate_export[]="SiN_GetServerRecombinantAttackGateApi";
struct ServerRecombinantAttackTrace {
    float start[3],end[3],mins[3],maxs[3];
    unsigned mask;int group;
};
struct ServerRecombinantAttackGateBindings {
    const ServerRecombinantEventBindings* event;
    void* context;
    int (__cdecl *origin)(void*,void*,float[3]) noexcept; // 220092d0
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *enemy_cast)(void*,void*,bool*) noexcept; // 223a5bab
    int (__cdecl *hull_mins)(void*,unsigned,float[3]) noexcept; // 22066210
    int (__cdecl *hull_maxs)(void*,unsigned,float[3]) noexcept; // 22066220
    int (__cdecl *trace)(void*,void*,const ServerRecombinantAttackTrace*,void**) noexcept;
    int (__cdecl *held_object)(void*,void*,bool*) noexcept; // 22256b00
};
struct ServerRecombinantAttackGateResult {
    unsigned completed,enemy_queries,condition_tests,origin_calls,refreshes,target_queries,
        casts,hull_calls,traces,held_calls,accepted,grounded;
    int condition;
    float height,deadline;
    ServerRecombinantAttackTrace trace;
};
struct ServerRecombinantAttackGateApi {
    unsigned version,size;
    int (__cdecl *leap)(void*,unsigned,float,float,const ServerRecombinantAttackGateBindings*,ServerRecombinantAttackGateResult*) noexcept;
    int (__cdecl *spit)(void*,unsigned,float,float,const ServerRecombinantAttackGateBindings*,ServerRecombinantAttackGateResult*) noexcept;
};
using GetServerRecombinantAttackGateApi=const ServerRecombinantAttackGateApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
