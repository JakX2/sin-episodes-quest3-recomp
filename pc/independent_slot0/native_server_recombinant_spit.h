#pragma once
#include "native_server_recombinant_events.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_spit_export[]="SiN_GetServerRecombinantSpitApi";
struct ServerRecombinantSpitRequest {
    float muzzle[3],velocity[3],angular[3],damage;
    bool alternate;
};
struct ServerRecombinantSpitEffect {
    float muzzle[3],ballistic[3];
    unsigned first_token,second_token;
};
struct ServerRecombinantSpitBindings {
    const ServerRecombinantEventBindings* event;
    void* context;
    float gravity,damage;bool alternate;
    int (__cdecl *attachment)(void*,void*,const char*,float[3]) noexcept;
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *aim_point)(void*,void*,const float[3],int,float[3]) noexcept; // target +1d4
    int (__cdecl *ballistic)(void*,const float[3],const float[3],float,float,float[3]) noexcept; // 2204c850
    int (__cdecl *normalize)(void*,float[3]) noexcept;
    int (__cdecl *projectile)(void*,void*,const ServerRecombinantSpitRequest*) noexcept;
    // Original post-projectile particle sequence 2237c59b..2237c7bc,
    // kept as an owner effect boundary; the original two tokens are passed.
    int (__cdecl *effects)(void*,void*,const ServerRecombinantSpitEffect*) noexcept;
};
struct ServerRecombinantSpitResult {
    unsigned completed,enemy_queries,enemy_live,attachment_calls,origin_refreshes,aim_calls,
        ballistic_calls,normalizations,random_calls,projectiles,effects,deadline_written,probability_written;
    float aim[3],target[3],direction[3],ballistic[3],draws[4],gravity,deadline,probability;
    ServerRecombinantSpitRequest request;
};
struct ServerRecombinantSpitApi {
    unsigned version,size;
    int (__cdecl *execute)(void*,unsigned,float,const ServerRecombinantSpitBindings*,ServerRecombinantSpitResult*) noexcept;
};
using GetServerRecombinantSpitApi=const ServerRecombinantSpitApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
