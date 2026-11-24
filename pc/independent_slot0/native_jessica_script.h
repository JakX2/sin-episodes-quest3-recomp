#pragma once
#include "native_weapon_datatables.h"
namespace sin_port::native_weapon_tables {
inline constexpr char jessica_export[]="SiN_GetJessicaScriptApi";
enum class JessicaInput:unsigned {ThrowItem,PrimaryAmmo,SecondaryAmmo,Holster};
enum class JessicaCondition:unsigned {Set,Test,Clear,Interrupt};
enum class JessicaEventResult:unsigned {Forwarded,NoSpawn,Thrown};
struct JessicaValue {unsigned type;std::uint32_t word;};
struct JessicaAttachment {float origin[3],angles[3];};
struct JessicaBindings {
    void* context;
    int (__cdecl *condition)(void*,void* npc,JessicaCondition,unsigned id,unsigned* present) noexcept;
    int (__cdecl *base_schedule)(void*,void* npc,int*) noexcept;
    int (__cdecl *base_interrupts)(void*,void* npc) noexcept;
    // The original holster lookup uses npc+0x78c (m_hActiveWeapon), not a global local-player lookup.
    // Resolve the packed handle with serial validation; absent => null.
    int (__cdecl *resolve_target)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *holster)(void*,void* target,int argument) noexcept;
    int (__cdecl *base_event)(void*,void* npc,const void* event) noexcept;
    int (__cdecl *max_clients)(void*,int*) noexcept;
    int (__cdecl *single_player)(void*,void**) noexcept;
    int (__cdecl *attachment)(void*,void* npc,const char*,JessicaAttachment*) noexcept;
    // The host converts a zero string_t to the original empty classname.
    int (__cdecl *spawn)(void*,void* npc,std::uint32_t classname,const JessicaAttachment*,void**) noexcept;
    // Return an adjusted CBaseCombatWeapon pointer and writable span, or null.
    int (__cdecl *weapon_cast)(void*,void* item,void** weapon,unsigned* bytes) noexcept;
    // Original DispatchSpawn-like call, engine result is not consumed by caller.
    int (__cdecl *post_spawn)(void*,void* item) noexcept;
    // Existing Source target-center, live gravity, ballistic helper and
    // additive impulse services. No replacement physics implementation.
    int (__cdecl *toss)(void*,void* item,void* player,const float origin[3],float extra_height) noexcept;
};
struct JessicaApi {
    std::uint32_t version,size;
    int (__cdecl *spawn_state)(HologramControl*) noexcept;
    int (__cdecl *input)(HologramControl*,const JessicaBindings*,JessicaInput,const JessicaValue*) noexcept;
    int (__cdecl *select_schedule)(HologramControl*,const JessicaBindings*,int*) noexcept;
    int (__cdecl *build_interrupts)(HologramControl*,const JessicaBindings*) noexcept;
    // Registered event ID comes from the live animation registry, not schedule ID.
    int (__cdecl *animation)(HologramControl*,const JessicaBindings*,const void* event,int id,int registered_id,JessicaEventResult*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetJessicaApi=const JessicaApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char jessica_combat_export[]="SiN_GetJessicaCombatApi";
struct JessicaCombatBindings {
    void* context;
    // TLK response-system dispatch, original two trailing arguments zero.
    // Engine response result is ignored; return transport success only.
    int (__cdecl *speak)(void*,void* npc,const char* concept_name,int,int) noexcept;
    int (__cdecl *base_damage)(void*,void* npc,const PlayerDamageInfo*,int*) noexcept;
    // Scene interface vslot5; return its observed boolean separately.
    int (__cdecl *scene_predicate)(void*,void* scene,std::uint32_t argument,unsigned*) noexcept;
    int (__cdecl *base_scene)(void*,void* npc,void* scene,std::uint32_t argument) noexcept;
    int (__cdecl *resolve_weapon)(void*,std::uint32_t packed_handle,void**) noexcept;
    int (__cdecl *holster)(void*,void* weapon,int argument) noexcept;
    // Original weapon slots 0x354 and 0x344; names intentionally provisional.
    int (__cdecl *weapon_query)(void*,void* weapon,unsigned*) noexcept;
    int (__cdecl *weapon_restore)(void*,void* weapon) noexcept;
};
struct JessicaCombatApi {
    std::uint32_t version,size;
    int (__cdecl *take_damage)(HologramControl*,const JessicaCombatBindings*,const PlayerDamageInfo*,int*) noexcept;
    int (__cdecl *scene_enter)(HologramControl*,const JessicaCombatBindings*,void*,std::uint32_t) noexcept;
    int (__cdecl *scene_exit)(HologramControl*,const JessicaCombatBindings*) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetJessicaCombatApi=const JessicaCombatApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

}
