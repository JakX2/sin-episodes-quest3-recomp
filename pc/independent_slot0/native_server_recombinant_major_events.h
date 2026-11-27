#pragma once
#include "native_server_recombinant_events.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_major_events_export[]="SiN_GetServerRecombinantMajorEventsApi";
struct ServerRecombinantMajorEventRegistration {
    const ServerEventDictionary* owner;unsigned generation,count,ready;int ids[7];
};
struct ServerRecombinantMajorMeleeRequest {
    float reach;int damage;float angular[3],impulse[3];int side;
};
struct ServerRecombinantMajorEventBindings {
    const ServerRecombinantPolicyBindings* policy;
    const ServerRecombinantMajorEventRegistration* registration;
    void* context;
    float normal_damage,heavy_damage; // live owner values 2259d210/2259d258+28
    int (__cdecl *basis)(void*,const float[3],float[3],float[3]) noexcept; // original 22269bc0
    int (__cdecl *forward)(void*,const float[3],float[3]) noexcept; // 22269b30
    int (__cdecl *enemy)(void*,void*,void**) noexcept;
    int (__cdecl *is_player)(void*,void*,bool*) noexcept;
    int (__cdecl *melee)(void*,void*,const ServerRecombinantMajorMeleeRequest*,void**) noexcept; // original +774
    int (__cdecl *parent)(void*,void*,const void*,unsigned) noexcept;
};
struct ServerRecombinantMajorEventResult {
    unsigned completed,custom,delegated,basis_calls,forward_calls,random_calls,enemy_calls,player_calls,melee_calls,hit;
    int event,route,draws[2];
    float angles[3],forward[3],right[3];
    ServerRecombinantMajorMeleeRequest request;
};
struct ServerRecombinantMajorEventsApi {
    unsigned version,size;
    int (__cdecl *definitions)(const ServerRecombinantEventDefinition**,unsigned*) noexcept;
    int (__cdecl *register_major)(ServerEventDictionary*,ServerRecombinantMajorEventRegistration*) noexcept;
    int (__cdecl *valid)(const ServerRecombinantMajorEventRegistration*) noexcept;
    int (__cdecl *major)(void*,unsigned,const void*,unsigned,const ServerRecombinantMajorEventBindings*,ServerRecombinantMajorEventResult*) noexcept;
};
using GetServerRecombinantMajorEventsApi=const ServerRecombinantMajorEventsApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
