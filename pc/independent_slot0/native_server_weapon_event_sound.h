#pragma once
#include "native_server_weapon_activity.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_weapon_event_sound_export[]="SiN_GetServerWeaponEventSoundApi";
struct ServerWeaponEventSoundBindings {
    void* context;
    // Resolved script sound and engine emission, original 222ad8a0.
    int (__cdecl *emit)(void*,void* weapon,int sound,float time) noexcept;
    // +4f8: burst count, NOT magazine capacity. Legacy member name retained.
    int (__cdecl *max_clip)(void*,void* weapon,int*) noexcept;
    // +3ac: assault sound interval, resolved at each emitted sound.
    int (__cdecl *interval)(void*,void* weapon,double*) noexcept;
};
struct ServerWeaponEventSoundResult {unsigned emitted;int last_sound;float last_time;};
struct ServerWeaponEventSoundApi {
    std::uint32_t version,size;
    // Verified +3c4, 2230b240 special gate for SMG and chaingun.
    int (__cdecl *play)(void*,unsigned,unsigned,int,float,const ServerWeaponEventSoundBindings*,ServerWeaponEventSoundResult*) noexcept;
    // 2230b090: at most two scheduled sound emissions; preserves +580.
    int (__cdecl *assault_tick)(void*,unsigned,float,const ServerWeaponEventSoundBindings*,ServerWeaponEventSoundResult*) noexcept;
};
using GetServerWeaponEventSoundApi=const ServerWeaponEventSoundApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
