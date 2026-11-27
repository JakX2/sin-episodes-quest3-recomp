#pragma once
#include "native_server_recombinant_major_events.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_recombinant_melee_export[]="SiN_GetServerRecombinantMeleeApi";
struct ServerRecombinantMeleeTrace {
    float reach,mins[3],maxs[3],scale;
    int damage,type,mode;
};
struct ServerRecombinantMeleeBindings {
    const ServerRecombinantMajorEventBindings* event;
    void* context;
    int (__cdecl *throw_object)(void*,void*,void*) noexcept; // original 223791a0
    int (__cdecl *hull_mins)(void*,unsigned,float[3]) noexcept;
    int (__cdecl *hull_maxs)(void*,unsigned,float[3]) noexcept;
    int (__cdecl *hit_query)(void*,void*,const ServerRecombinantMeleeTrace*,void**,unsigned*) noexcept;
    int (__cdecl *player_view)(void*,void*,const float[3]) noexcept;
    int (__cdecl *player_impulse)(void*,void*,const float[3]) noexcept;
    int (__cdecl *effect_token)(void*,void*,unsigned*) noexcept; // virtual +ec
    int (__cdecl *effect_eligible)(void*,unsigned,bool*) noexcept; // 2228be90
    int (__cdecl *attachment)(void*,void*,const char*,float[3],bool*) noexcept;
    int (__cdecl *blood)(void*,const float[3],unsigned,float) noexcept; // 22149eb0
};
struct ServerRecombinantMeleeResult {
    unsigned completed,enemy_live,goal_live,throw_calls,hull_calls,hit_calls,sound_calls,player_calls,
        player_protected,view_calls,impulse_calls,effect_queries,effect_eligible,attachment_calls,blood_calls;
    void* hit;
    float blood_amount;
    ServerRecombinantMeleeTrace trace;
};
struct ServerRecombinantMeleeApi {
    unsigned version,size;
    int (__cdecl *execute)(void*,unsigned,const ServerRecombinantMajorMeleeRequest*,const ServerRecombinantMeleeBindings*,ServerRecombinantMeleeResult*) noexcept;
};
using GetServerRecombinantMeleeApi=const ServerRecombinantMeleeApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
