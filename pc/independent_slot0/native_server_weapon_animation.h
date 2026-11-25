#pragma once
#include "native_server_weapon_activity.h"
#include "native_server_activity_metadata.h"
#include "native_server_sequence_reset.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_weapon_animation_export[]="SiN_GetServerWeaponAnimationApi";
struct ServerWeaponAnimationBindings {
    void* context;
    // Resolve current weapon's studio metadata. 0 absent, 1 resolved,
    // negative service failure. Null model preserves original choice 0.
    int (__cdecl *metadata)(void*,void*,const ServerActivityCompositeView**) noexcept;
    ServerActivityDictionary* activities;
    ServerEventDictionary* events;
    ServerSequenceChoiceBindings random;
    ServerSequenceResetBindings reset;
    ServerSequenceDictionaryBindings dictionary;
};
struct ServerWeaponAnimationResult {
    int sequence;
    unsigned selections,fallback,assigned,sequence_changed,cycle_changed;
    float playback;
    ServerSequenceDictionaryResult reset;
};
struct ServerWeaponAnimationDispatchBindings {
    ServerWeaponActivityBindings weapons;
    ServerWeaponAnimationBindings animation;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
};
struct ServerWeaponAnimationDispatchResult {
    unsigned live;
    ServerWeaponAnimationResult animation;
};
struct ServerWeaponAnimationApi {
    std::uint32_t version,size;
    // 222ae770: direct sequence write, cycle zero, unconditional reset;
    // the previous looping flag does not gate reset.
    int (__cdecl *assign)(void*,unsigned,int,float,const ServerWeaponAnimationBindings*,ServerWeaponAnimationResult*) noexcept;
    // 222a5470: serial-checked +78c, then original +3d8.
    int (__cdecl *notify)(void*,unsigned,int,float,const ServerWeaponAnimationDispatchBindings*,ServerWeaponAnimationDispatchResult*) noexcept;
};
using GetServerWeaponAnimationApi=const ServerWeaponAnimationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
