#pragma once
#include "native_companion_reactions.h"
#include "native_count_scheduler.h"
#include "native_jessica_script.h"
namespace sin_port::native_weapon_tables {
inline constexpr char npc_spawn_export[]="SiN_GetNpcSpawnApi";
struct JessicaSpawnBindings {
    void* context;
    int (__cdecl *base_spawn)(void*,void* npc) noexcept;
    int (__cdecl *resolve_parent)(void*,std::uint32_t,void**) noexcept;
    // Existing 22143610(0,0) or 22362550. No collision implementation.
    int (__cdecl *parented_setup)(void*,void* npc,unsigned,unsigned) noexcept;
    int (__cdecl *restore_unparented)(void*,void* npc) noexcept;
    int (__cdecl *health_dirty)(void*,void* npc) noexcept;
    int (__cdecl *npc_initialize)(void*,void* npc) noexcept;
    // Install a real host callback corresponding to 22361e40. Do NOT
    // copy an original executable address into the native object.
    int (__cdecl *install_interaction)(void*,void* npc,unsigned field_offset,unsigned original_thunk) noexcept;
    int (__cdecl *expeditor)(void*,void* npc,void**) noexcept;
    int (__cdecl *seed_concept)(void*,void* expeditor,const char*,unsigned,unsigned) noexcept;
};
struct CountClassInitBindings {
    void* context;
    CountSchedulerBindings behaviors;
    // Original postconstructor 22031b10 -> 2213b140 -> slot+3f8.
    int (__cdecl *base_postconstruct)(void*,void* npc,std::uint32_t classname_word) noexcept;
    int (__cdecl *base_create)(void*,void* npc,unsigned* accepted) noexcept;
    // Slot+7c0 owns allocation/construction of the 0x7c expeditor.
    // Return null on normal absence; retain storage through return.
    int (__cdecl *create_expeditor)(void*,void* npc,void**,unsigned* bytes) noexcept;
};
struct NpcSpawnApi {
    std::uint32_t version,size;
    int (__cdecl *jessica)(HologramControl*,const JessicaSpawnBindings*) noexcept;
    // Original Count postconstructor BEFORE keys and ordinary Spawn.
    // accepted is the original class-create boolean, not transport status.
    int (__cdecl *count_class_init)(HologramControl*,const CountClassInitBindings*,std::uint32_t classname_word,unsigned* accepted) noexcept;
    // Original Count scalar spawn tail after host's base/model setup.
    // Does not append behavior vector again.
    int (__cdecl *count_profile)(HologramControl*,const JcProfileBindings*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetNpcSpawnApi=const NpcSpawnApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
