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
}
