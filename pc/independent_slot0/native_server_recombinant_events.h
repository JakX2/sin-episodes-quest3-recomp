#pragma once
#include "native_server_recombinant_minor.h"
#include "native_server_event_dictionary.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_events_export[]="SiN_GetServerRecombinantEventsApi";
struct ServerRecombinantEventDefinition {const char* name;unsigned slot,store;};
struct ServerRecombinantEventRegistration {
    const ServerEventDictionary* owner;unsigned generation,count,ready;int ids[5];
};
struct ServerRecombinantEventBindings {
    const ServerRecombinantPolicyBindings* policy;
    const ServerRecombinantEventRegistration* registration;
    void* context;
    int (__cdecl *enemy)(void*,void*,void**) noexcept;
    int (__cdecl *special)(void*,void*,bool*) noexcept; // +f4
    int (__cdecl *target_point)(void*,void*,float[3]) noexcept; // +1c4
    int (__cdecl *center)(void*,void*,float[3]) noexcept; // +214
    int (__cdecl *player_probe)(void*,void*) noexcept; // +120, result ignored
    int (__cdecl *launch)(void*,void*,int,const float[3],int) noexcept; // +76c
    int (__cdecl *pounce_slash)(void*,void*,void*) noexcept; // +774
    int (__cdecl *angular_effect)(void*,void*,const float[3]) noexcept; // 22139c10
    int (__cdecl *pounce_off)(void*,void*) noexcept; // +770
    int (__cdecl *spit)(void*,void*) noexcept; // 2237c360
    int (__cdecl *random)(void*,float,float,float*) noexcept; // original imported RNG
    int (__cdecl *parent)(void*,void*,const void*,unsigned) noexcept;
};
struct ServerRecombinantEventResult {
    unsigned completed,custom,delegated,enemy_queries,target_live,cache_written,cache_consumed,
        attached_blocked,launch_calls,random_calls,slash_calls,angular_calls,spit_calls,off_calls,player_probes;
    int event,route;float point[3],angles[3],probability;
};
struct ServerRecombinantEventsApi {
    unsigned version,size;
    int (__cdecl *definitions)(const ServerRecombinantEventDefinition**,unsigned*) noexcept;
    int (__cdecl *register_minor)(ServerEventDictionary*,ServerRecombinantEventRegistration*) noexcept;
    int (__cdecl *valid)(const ServerRecombinantEventRegistration*) noexcept;
    int (__cdecl *minor)(void*,unsigned,const void*,unsigned,const ServerRecombinantEventBindings*,ServerRecombinantEventResult*) noexcept;
};
using GetServerRecombinantEventsApi=const ServerRecombinantEventsApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
