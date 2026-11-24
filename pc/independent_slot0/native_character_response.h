#pragma once
#include "native_weapon_datatables.h"
namespace sin_port::native_weapon_tables {
inline constexpr char character_response_export[]="SiN_GetCharacterResponseApi";
struct CountRegionQuery {float direction[3];int hitbox;float distance;};
struct CountRegionBindings {
    void* context;
    // Existing engine angle/forward-vector and hitbox services. The
    // returned bone name is stable through the synchronous call.
    int (__cdecl *forward_xy)(void*,void* npc,float xy[2]) noexcept;
    int (__cdecl *hit_identity)(void*,void* npc,int hitbox,int* group,const char** bone_name) noexcept;
};
struct JessicaFilterBindings {
    void* context;
    int (__cdecl *base_filter)(void*,void* npc,const PlayerDamageInfo*,unsigned*) noexcept;
};
struct CharacterResponseApi {
    std::uint32_t version,size;
    // Null name is the original out-of-range/unrecognized group result.
    // Names point to static module strings.
    int (__cdecl *count_region)(HologramControl*,const CountRegionBindings*,const CountRegionQuery*,const char**) noexcept;
    // Complete object pointer for Count/JC; original secondary this
    // at +dfc is adjusted back to spawnflag byte +b6.
    int (__cdecl *uses_speech_semaphore)(HologramControl*,unsigned*) noexcept;
    int (__cdecl *jessica_damage_filter)(HologramControl*,const JessicaFilterBindings*,const PlayerDamageInfo*,unsigned*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetCharacterResponseApi=const CharacterResponseApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
