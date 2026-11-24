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


inline constexpr char jessica_attention_export[]="SiN_GetJessicaAttentionApi";
// Two independent original module globals, shared across ALL Jessica
// instances. One state per server session, serialized by its owner.
struct JessicaAttentionState {float looking=0,using_part=0;};
struct JessicaAttentionBindings {
    void* context;
    int (__cdecl *clock)(void*,float*) noexcept;
    // Observe the real record's virtual +0x10 criteria producer, then
    // response system dispatch. Return transport success, not whether
    // a matching response played (the original ignores that result).
    int (__cdecl *speak_criteria)(void*,void* npc,const void* record,float now,const char* concept_name) noexcept;
};
struct JessicaAttentionApi {
    std::uint32_t version,size;
    // The original wrapper reads two records and their enable bytes in
    // sequence. Host owns both and guarantees a stable readable span.
    int (__cdecl *observe)(HologramControl*,JessicaAttentionState*,const JessicaAttentionBindings*,const void* observation,unsigned bytes) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetJessicaAttentionApi=const JessicaAttentionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char jessica_interaction_export[]="SiN_GetJessicaInteractionApi";
// Fixed (1,1) path of the inherited talk gate and Jessica's installed
// interaction callback. Host callbacks report transport separately from AL.
struct JessicaInteractionBindings {
    void* context;
    int (__cdecl *speech_manager)(void*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,unsigned* present) noexcept;
    int (__cdecl *expeditor)(void*,void* npc,void**) noexcept;
    int (__cdecl *eligible)(void*,void* exp,void* npc,unsigned*) noexcept;
    int (__cdecl *timing)(void*,void* exp,unsigned*) noexcept;
    int (__cdecl *player)(void*,void**) noexcept;
    // Original 2208afb3..2208b00d: read speech range then NPC and player
    // positions, subtract with float stores and compare squared distances.
    // Host retains the original range/global and position services.
    int (__cdecl *within_range)(void*,void* npc,void* player,unsigned*) noexcept;
    int (__cdecl *suppressed)(void*,void* npc,int,unsigned*) noexcept;
    int (__cdecl *speak)(void*,void* npc,const char*,int,unsigned*) noexcept;
    int (__cdecl *has_concept)(void*,void* exp,const char*,unsigned*) noexcept;
    int (__cdecl *reset_manager)(void*,float,int) noexcept;
};
struct JessicaInteractionApi {
    std::uint32_t version,size;
    // Return status: 1 completed (including original false), 0 rejected,
    // -1 transport failure. original_result receives original AL truth value.
    int (__cdecl *invoke)(HologramControl*,const JessicaInteractionBindings*,unsigned* original_result) noexcept;
    int (__cdecl *acknowledge)(HologramControl*) noexcept;
};
using GetJessicaInteractionApi=const JessicaInteractionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char response_policy_export[]="SiN_GetResponsePolicyApi";
struct ResponseHistoryBindings {
    void* context;
    // Original tree lookup/insert and allocation stay with the engine.
    // index==-1 denotes absent; transport failure is independent.
    int (__cdecl *lookup)(void*,void* exp,const char*,int* index) noexcept;
    int (__cdecl *insert)(void*,void* exp,const char*,float initial_time,int* index) noexcept;
    // Return the actual 28-byte map row, not an invented dense index.
    int (__cdecl *entry)(void*,void* exp,int index,void** row,unsigned* bytes) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *notify)(void*,void* exp,const char*) noexcept;
};
enum class ResponseTiming:unsigned {Normal,Urgent};
struct ResponsePolicyApi {
    std::uint32_t version,size;
    // Caller holds the owning NPC's interaction/spawn lock. These leaves
    // never acquire it; expeditor and row storage remain host-owned.
    int (__cdecl *timing)(const void* exp,unsigned bytes,ResponseTiming,float now,unsigned*) noexcept;
    int (__cdecl *has_concept)(void* exp,unsigned bytes,const ResponseHistoryBindings*,const char*,unsigned*) noexcept;
    // Original seed path 220a7190(concept,0,1), including owner notification.
    int (__cdecl *seed)(void* exp,unsigned bytes,const ResponseHistoryBindings*,const char*) noexcept;
};
using GetResponsePolicyApi=const ResponsePolicyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Original 220a38d0, response selection before playback. Caller serializes
// the expeditor. Criteria and response allocator/virtual methods are hosted.
inline constexpr char response_select_export[]="SiN_GetResponseSelectApi";
struct ResponseSelectionBindings {
    void* context;
    int (__cdecl *system)(void*,void* exp,void** system) noexcept;
    int (__cdecl *begin_criteria)(void*,void** criteria) noexcept;
    int (__cdecl *add)(void*,void* criteria,const char* key,const char* value,float weight) noexcept;
    // Host owns original bounded modifier parser (255-byte source, 128-byte
    // key and value buffers); add all parsed pairs at supplied weight.
    int (__cdecl *modifiers)(void*,void* criteria,const char*,float weight) noexcept;
    int (__cdecl *npc_criteria)(void*,void* exp,void* criteria) noexcept;
    int (__cdecl *player_criteria)(void*,void* criteria,int player_index) noexcept;
    int (__cdecl *allocate)(void*,unsigned bytes,void** response,unsigned* extent) noexcept;
    // On transport success with found==0, original finder branch does not
    // destroy response. Host must account for that allocation at its boundary.
    int (__cdecl *find)(void*,void* system,void* criteria,void* response,void* exp,unsigned* found) noexcept;
    int (__cdecl *draw)(void*,int low,int high,int*) noexcept;
    int (__cdecl *destroy)(void*,void* response) noexcept;
    int (__cdecl *end_criteria)(void*,void* criteria) noexcept;
};
struct ResponseSelectApi {
    std::uint32_t version,size;
    // 1 completed, including no matching response (null output);
    // 0 rejected, -1 transport error. On successful selection ownership
    // of response transfers to caller; outputs untouched on transport error.
    int (__cdecl *select)(void* exp,unsigned bytes,const ResponseSelectionBindings*,const char* name,const char* modifiers,void** response,unsigned* extent) noexcept;
};
using GetResponseSelectApi=const ResponseSelectApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;




}
