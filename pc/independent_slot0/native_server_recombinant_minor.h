#pragma once
#include "native_server_recombinant_damage.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_minor_export[]="SiN_GetServerRecombinantMinorApi";
struct ServerRecombinantMinorBindings {
    const ServerRecombinantDamageBindings* damage;
    void* context;
    unsigned max_clients;
    int (__cdecl *local_player)(void*,void**) noexcept;
    int (__cdecl *refresh_velocity)(void*,void*) noexcept;
    int (__cdecl *set_velocity)(void*,void*,const float[3]) noexcept;
    // Original 2214d7b0 with current time and zero context.
    int (__cdecl *schedule_touch_think)(void*,void*,float,int) noexcept;
    int (__cdecl *entity_index)(void*,unsigned,int*) noexcept; // engine +48
    int (__cdecl *set_schedule)(void*,void*,int) noexcept;
    int (__cdecl *parent_gather)(void*,void*) noexcept;
    int (__cdecl *npc_or_special)(void*,void*,bool*) noexcept; // +f4
    int (__cdecl *is_player)(void*,void*,bool*) noexcept; // +120
    int (__cdecl *object_flags)(void*,unsigned,unsigned*) noexcept; // +218 storage, +20
};
struct ServerRecombinantMinorResult {
    unsigned completed,immune,source_live,attacker_live,local_matches,velocity_refreshed,velocity_set,
        detached,condition_writes,think_scheduled,color_called,schedule_tests,schedule_set,ignited,parent_called,
        ground_live,index_called,special_calls,player_calls,object_calls,ground_accepted;
    int source_class,entity_index,damage_result;
    float impulse[3],velocity[3];
    double squared;
};
struct ServerRecombinantMinorApi {
    unsigned version,size;
    int (__cdecl *damage)(void*,unsigned,const void*,unsigned,float,const ServerRecombinantMinorBindings*,ServerRecombinantMinorResult*) noexcept;
    int (__cdecl *detach)(void*,unsigned,float,const ServerRecombinantMinorBindings*,ServerRecombinantMinorResult*) noexcept;
    int (__cdecl *gather)(void*,unsigned,const ServerRecombinantMinorBindings*,ServerRecombinantMinorResult*) noexcept;
};
using GetServerRecombinantMinorApi=const ServerRecombinantMinorApi* (__cdecl *)(unsigned,unsigned) noexcept;

inline constexpr char server_recombinant_minor_metric_gate_export[]="SiN_GetServerRecombinantMinorMetricGateApi";
// Original Minor virtual +0x398 (2237e8c0). The global engine owner
// decides whether to suppress the check; if not, candidate+2c > 10.f.
// The meaning of the candidate's scalar is not yet authenticated.
struct ServerRecombinantMinorMetricGateBindings {
    void* context;
    int (__cdecl *manager_suppresses)(void*,bool*) noexcept; // owner virtual +0x170
};
struct ServerRecombinantMinorMetricGateResult {
    unsigned completed,manager_calls,suppressed,compared,allowed;
    float observed,threshold;
};
struct ServerRecombinantMinorMetricGateApi {
    unsigned version,size;
    int (__cdecl *evaluate)(const void*,unsigned,const void*,unsigned,
        const ServerRecombinantMinorMetricGateBindings*,
        ServerRecombinantMinorMetricGateResult*) noexcept;
};
using GetServerRecombinantMinorMetricGateApi=
    const ServerRecombinantMinorMetricGateApi* (__cdecl *)(unsigned,unsigned) noexcept;


inline constexpr char server_recombinant_minor_origin_point_export[]="SiN_GetServerRecombinantMinorOriginPointApi";
// Original Minor virtual +0x1d4 (2237b420) writes origin +(0,0,6).
// Only the Source-owned dirty-origin refresh remains a host service.
struct ServerRecombinantMinorOriginPointBindings {
    void* context;
    int (__cdecl *refresh_origin)(void*,void*) noexcept; // original 2213fa60
};
struct ServerRecombinantMinorOriginPointResult {
    unsigned completed,refresh_calls;
    float origin[3],point[3];
};
struct ServerRecombinantMinorOriginPointApi {
    unsigned version,size;
    int (__cdecl *point)(void*,unsigned,float[3],
        const ServerRecombinantMinorOriginPointBindings*,
        ServerRecombinantMinorOriginPointResult*) noexcept;
};
using GetServerRecombinantMinorOriginPointApi=
    const ServerRecombinantMinorOriginPointApi* (__cdecl *)(unsigned,unsigned) noexcept;

}
