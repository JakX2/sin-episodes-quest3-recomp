#pragma once
#include "native_server_recombinant_spit.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_grenade_spit_detonation_export[]="SiN_GetServerGrenadeSpitDetonationApi";
// Original exported CGrenadeSpitMutant/CGrenadeSpitPoison::Detonate.
// The host retains state +1ac, sound, effect and entity-removal services.
struct ServerGrenadeSpitPoisonEffect {
    float origin[3];
    float arg0,arg1,arg2,arg3;
    int mode;
};
struct ServerGrenadeSpitDetonationBindings {
    void* context;
    int (__cdecl *clear_state)(void*,void*) noexcept; // original virtual +1ac
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *sound)(void*,void*,const char*,int,int) noexcept;
    int (__cdecl *refresh_origin)(void*,void*) noexcept; // Source 2213fa60
    int (__cdecl *poison_effect)(void*,void*,const ServerGrenadeSpitPoisonEffect*) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept; // original 22286bf0
};
struct ServerGrenadeSpitDetonationResult {
    unsigned completed,variant,state_cleared,random_calls,sounds,refreshes,effects,removed;
    float draw;
    ServerGrenadeSpitPoisonEffect effect;
};
struct ServerGrenadeSpitDetonationApi {
    unsigned version,size;
    int (__cdecl *mutant)(void*,unsigned,const ServerGrenadeSpitDetonationBindings*,ServerGrenadeSpitDetonationResult*) noexcept;
    int (__cdecl *poison)(void*,unsigned,const ServerGrenadeSpitDetonationBindings*,ServerGrenadeSpitDetonationResult*) noexcept;
};
using GetServerGrenadeSpitDetonationApi=const ServerGrenadeSpitDetonationApi* (__cdecl *)(unsigned,unsigned) noexcept;
}
