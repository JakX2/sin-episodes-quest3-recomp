#pragma once
#include "native_weapon_datatables.h"
namespace sin_port::native_weapon_tables {
inline constexpr char generic_actor_export[]="SiN_GetGenericActorApi";
inline constexpr unsigned generic_actor_bytes=0x1014;
enum class ActorFieldKind:unsigned {Key,Save,Input};
struct ActorField {const char* name;ActorFieldKind kind;unsigned type,offset;};
struct ActorSchema {const char* classname;unsigned bytes,count;const ActorField* fields;};
struct ActorValue {unsigned type,word;};
struct ActorBindings {
    void* context;
    // Original virtual +1b4 before changed health. May update initial
    // health; native routine reads the field again after the callback.
    int (__cdecl *health_dirty)(void*,void* npc) noexcept;
    // The original forwards the SAME mutable CTakeDamageInfo object.
    int (__cdecl *behavior_damage)(void*,std::uint32_t behavior,PlayerDamageInfo*,int*) noexcept;
    int (__cdecl *base_damage)(void*,void* npc,PlayerDamageInfo*,int*) noexcept;
    // Original behavior virtual +bc. Host validates live behavior and
    // observation object; no copy of its polymorphic storage.
    int (__cdecl *behavior_observe)(void*,std::uint32_t behavior,const void* observation) noexcept;
};
struct GenericActorApi {
    std::uint32_t version,size;
    const ActorSchema* (__cdecl *schema)() noexcept;
    int (__cdecl *key)(HologramControl*,const char*,const ActorValue*) noexcept;
    // Bounded original spawn scalar slice only; host owns the preceding
    // base spawn and subsequent attachment/animation initialization.
    int (__cdecl *spawn_profile)(HologramControl*,const ActorBindings*) noexcept;
    int (__cdecl *take_damage)(HologramControl*,const ActorBindings*,PlayerDamageInfo*,int*) noexcept;
    int (__cdecl *observe)(HologramControl*,const ActorBindings*,const void*) noexcept;
    int (__cdecl *uses_speech_semaphore)(HologramControl*,unsigned*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetGenericActorApi=const GenericActorApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
