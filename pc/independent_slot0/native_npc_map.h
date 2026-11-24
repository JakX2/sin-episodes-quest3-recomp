#pragma once
#include "native_generic_actor.h"
#include "native_jessica_script.h"
#include "native_count_scheduler.h"
namespace sin_port::native_weapon_tables {
inline constexpr char npc_map_export[]="SiN_GetNpcMapApi";
enum class NpcMapKind:unsigned {Jessica,Count,GenericActor,Jc};
enum class NpcMapFieldKind:unsigned {Key,Input};
struct NpcMapField {const char* name;NpcMapFieldKind kind;unsigned type;unsigned original_handler;};
struct NpcMapSchema {const char* classname;unsigned bytes,registration,count;const NpcMapField* fields;};
struct NpcMapBindings {
    JessicaBindings jessica;
    JessicaAttentionBindings attention;
    CountSchedulerBindings scheduler;
    CountBindings count;
    ActorBindings actor;
};
// These are original module-wide state, not separate per-entity timers.
struct NpcMapSession {JessicaAttentionState attention{};CountSchedulerState scheduler{};};
struct NpcMapApi {
    std::uint32_t version,size;
    const NpcMapSchema* (__cdecl *schema)(NpcMapKind) noexcept;
    int (__cdecl *key)(NpcMapKind,HologramControl*,const char*,unsigned type,std::uint32_t word) noexcept;
    int (__cdecl *input)(NpcMapKind,HologramControl*,const NpcMapBindings*,const char*,unsigned type,std::uint32_t word) noexcept;
    int (__cdecl *observe)(NpcMapKind,HologramControl*,NpcMapSession*,const NpcMapBindings*,const void*,unsigned bytes) noexcept;
    int (__cdecl *select_count)(HologramControl*,NpcMapSession*,const NpcMapBindings*,int*) noexcept;
};
using GetNpcMapApi=const NpcMapApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
