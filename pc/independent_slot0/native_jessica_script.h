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
inline constexpr char response_dispatch_export[]="SiN_GetResponseDispatchApi";
enum class ResponseKind:unsigned {None,Speak,Sentence,Scene,Response,Print};
struct ResponseDispatchBindings {
    void* context;
    int (__cdecl *sample)(void*,const void* interval,float*) noexcept;
    int (__cdecl *speaking)(void*,void* exp,unsigned*) noexcept;
    int (__cdecl *force_log)(void*,void* exp,const char* name) noexcept;
    int (__cdecl *stop_scene)(void*,void* exp,int argument) noexcept;
    int (__cdecl *interrupt)(void*,void* exp) noexcept;
    int (__cdecl *scene_blocked)(void*,void* exp,int argument,unsigned*) noexcept;
    int (__cdecl *refused_log)(void*,void* exp,const char* name) noexcept;
    // NPC vslots +66c and +668, original floating return duration.
    int (__cdecl *sound)(void*,void* exp,const char* text,float*) noexcept;
    int (__cdecl *scene)(void*,void* exp,const char* text,float delay,float*) noexcept;
    int (__cdecl *sound_duration)(void*,void* exp,const char* text,float*) noexcept;
    int (__cdecl *emit_raw)(void*,void* exp,const char* text,int,int) noexcept;
    int (__cdecl *note_duration)(void*,void* exp,float duration,float delay) noexcept;
    int (__cdecl *sentence)(void*,void* exp,const char* text,float delay,float volume,int level,int argument,int*) noexcept;
    int (__cdecl *print_enabled)(void*,unsigned*) noexcept;
    int (__cdecl *print)(void*,void* exp,const char* text) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    // Owns original diagnostic-only enable gates and overlay; called only
    // after success and +78 store. Type Print skips this overlay entirely.
    int (__cdecl *debug_success)(void*,void* exp,const char* concept_name,const char* text) noexcept;
    int (__cdecl *destroy)(void*,void* response) noexcept;
    // Ownership transfers on invocation, even if callback reports failure.
    // Original 220a7190(name,response,1).
    int (__cdecl *record)(void*,void* exp,const char* concept_name,void* response) noexcept;
};
struct ResponseDispatchApi {
    std::uint32_t version,size;
    // Valid invocation consumes selected response; preflight rejection (0)
    // leaves caller ownership. 1 completed, -1 transport error.
    int (__cdecl *dispatch)(void* exp,unsigned bytes,const ResponseDispatchBindings*,const char* concept_name,void* response,unsigned extent,unsigned* played) noexcept;
};
using GetResponseDispatchApi=const ResponseDispatchApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char response_timing_update_export[]="SiN_GetResponseTimingUpdateApi";
// One pair per server session, shared by all expeditors. The original
// globals are 224e2b34 and 224e2b3c, not per-NPC copies.
struct SpeechSemaphore {float release=0;std::uint32_t owner=0xffffffffu;};
struct SpeechSemaphorePair {SpeechSemaphore classified{},other{};};
struct ResponseNoteBindings {
    void* context;
    int (__cdecl *owner_start)(void*,void* exp) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    // Diagnostic enable and log are host-owned. Called after both deadlines.
    int (__cdecl *diagnostic)(void*,void* exp,float total,float delay,float normal_until) noexcept;
    int (__cdecl *uses_semaphore)(void*,void* exp,unsigned*) noexcept;
    int (__cdecl *classify)(void*,void* exp,int argument,unsigned*) noexcept;
    int (__cdecl *owner_handle)(void*,void* exp,std::uint32_t*) noexcept;
};
struct ResponseIntervalBindings {
    void* context;
    int (__cdecl *random)(void*,float low,float high,float*) noexcept;
};
struct ResponseTimingUpdateApi {
    std::uint32_t version,size;
    // Stable clock snapshot; original debug-only warning omitted here.
    int (__cdecl *speaking)(const void* exp,unsigned bytes,float now,unsigned*) noexcept;
    int (__cdecl *note)(void* exp,unsigned bytes,SpeechSemaphorePair*,const ResponseNoteBindings*,float duration,float delay) noexcept;
    int (__cdecl *sample)(const void* interval,unsigned bytes,const ResponseIntervalBindings*,float*) noexcept;
};
using GetResponseTimingUpdateApi=const ResponseTimingUpdateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char response_record_export[]="SiN_GetResponseRecordApi";
struct ResponseRecordBindings {
    ResponseHistoryBindings history;
    void* context;
    // Consumes the supplied response on invocation, including transport
    // failure. Validate original engine ownership before releasing it.
    int (__cdecl *destroy)(void*,void* response) noexcept;
};
struct ResponseRecordApi {
    std::uint32_t version,size;
    // Original 220a7190(name,response,notify). Valid call consumes response
    // except an already-owned identical pointer (rejected, not freed).
    // 0 preflight rejection retains caller ownership, -1 transport error.
    int (__cdecl *record)(void* exp,unsigned bytes,const ResponseRecordBindings*,const char* name,void* response,unsigned extent,unsigned notify) noexcept;
};
using GetResponseRecordApi=const ResponseRecordApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char response_speak_export[]="SiN_GetResponseSpeakApi";
struct ResponseSpeakBindings {
    ResponseSelectionBindings selection;
    ResponseDispatchBindings dispatch;
    void* context;
    // Original 220a7570 diagnostic gate; host decides whether to log.
    int (__cdecl *trace_selected)(void*,void* exp,const char* concept_name) noexcept;
};
struct ResponseSpeakApi {
    std::uint32_t version,size;
    // Original selection -> optional debug trace -> dispatch wrapper.
    // Caller owns expeditor serialization; nested callbacks may use the
    // recovered timing/history leaves without recursively taking NPC lock.
    int (__cdecl *speak)(void* exp,unsigned bytes,const ResponseSpeakBindings*,const char* concept_name,const char* modifiers,unsigned* played) noexcept;
};
using GetResponseSpeakApi=const ResponseSpeakApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char response_repeat_export[]="SiN_GetResponseRepeatApi";
struct ResponseRepeatBindings {
    void* context;
    int (__cdecl *lookup)(void*,void* exp,const char*,int*) noexcept;
    int (__cdecl *entry)(void*,void* exp,int index,void** row,unsigned* extent) noexcept;
    // Resolve the PE32 response pointer from row+18, with ownership checks.
    int (__cdecl *resolve)(void*,std::uint32_t,void** response,unsigned* extent) noexcept;
    ResponseIntervalBindings interval;
    int (__cdecl *clock)(void*,float*) noexcept;
};
struct SpeechGateBindings {
    void* context;
    int (__cdecl *uses)(void*,void* exp,unsigned*) noexcept;
    // Real virtual classification of the NPC (argument zero).
    int (__cdecl *classify)(void*,void* npc,int,unsigned*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    // Resolve packed EHANDLE with serial verification; stale returns null.
    int (__cdecl *resolve)(void*,std::uint32_t,void** npc) noexcept;
};
struct ResponseRepeatApi {
    std::uint32_t version,size;
    int (__cdecl *can_repeat)(void* exp,unsigned bytes,const ResponseRepeatBindings*,const char* concept_name,unsigned*) noexcept;
    int (__cdecl *last_time)(void* exp,unsigned bytes,const ResponseRepeatBindings*,const char* concept_name,float*) noexcept;
    int (__cdecl *semaphore_available)(void* exp,unsigned bytes,void* npc,const SpeechSemaphorePair*,const SpeechGateBindings*,unsigned*) noexcept;
};
using GetResponseRepeatApi=const ResponseRepeatApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char expeditor_lifecycle_export[]="SiN_GetExpeditorLifecycleApi";
enum class ExpeditorStage:unsigned {Fresh,Constructing,Live,Destroying,Retired,Failed};
struct ExpeditorControl {
    void* object;
    unsigned bytes;
    ExpeditorStage stage=ExpeditorStage::Fresh;
};
struct ExpeditorTables {
    // Live replacement table/comparator addresses, NEVER addresses copied
    // from the original PE. Host owns their ABI and lifetime.
    std::uint32_t primary,filter,base,compare;
};
struct ExpeditorLifecycleBindings {
    void* context;
    int (__cdecl *symbol_construct)(void*,void* symbol,int,int,unsigned) noexcept;
    int (__cdecl *symbol_destroy)(void*,void* symbol) noexcept;
    // Bounded allocated row storage, including free rows.
    int (__cdecl *row)(void*,void* exp,unsigned index,void** row,unsigned* extent) noexcept;
    int (__cdecl *destroy_response)(void*,std::uint32_t owned_response) noexcept;
    int (__cdecl *classify)(void*,void* npc,int,unsigned*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t handle,void** npc) noexcept;
    int (__cdecl *free_rows)(void*,std::uint32_t allocation) noexcept;
};
struct ExpeditorLifecycleApi {
    std::uint32_t version,size;
    // Fixed zero-allocation factory path 22028cf0 -> 220a6bc0.
    int (__cdecl *construct)(ExpeditorControl*,void* npc,const ExpeditorTables*,const ExpeditorLifecycleBindings*) noexcept;
    // Original 220a50f0 clear, keeping backing allocation and symbols.
    int (__cdecl *clear)(ExpeditorControl*,const ExpeditorLifecycleBindings*) noexcept;
    // Nondeleting destructor 220a6c30; host frees expeditor itself afterward.
    int (__cdecl *destroy)(ExpeditorControl*,void* npc,const ExpeditorTables*,SpeechSemaphorePair*,const ExpeditorLifecycleBindings*) noexcept;
};
using GetExpeditorLifecycleApi=const ExpeditorLifecycleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char talk_catalog_export[]="SiN_GetTalkCatalogApi";
struct TalkConceptDescriptor {
    const char* name;
    unsigned priority;
    float global_min,global_max,npc_min,npc_max,concept_min,concept_max;
    unsigned flags;
};
struct TalkCooldownState {float global[3]{};};
struct TalkCooldownBindings {
    void* context;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *random)(void*,float low,float high,float*) noexcept;
    // Original per-concept tree row lookup; missing returns null.
    // The returned float points to the row's +8 deadline.
    int (__cdecl *concept_deadline)(void*,const char*,float**) noexcept;
    // Original 2208f000 flag2 neighbor scan and seed propagation. Engine
    // handles live entity enumeration, geometry and actual expeditors.
    int (__cdecl *propagate)(void*,void* npc,const char*) noexcept;
    int (__cdecl *cooldown_disabled)(void*,unsigned*) noexcept;
};
struct TalkCatalogApi {
    std::uint32_t version,size,count;
    const TalkConceptDescriptor* (__cdecl *at)(unsigned) noexcept;
    const TalkConceptDescriptor* (__cdecl *lookup)(const char*) noexcept;
    int (__cdecl *group_ready)(const void* npc,unsigned bytes,const TalkCooldownState*,const TalkCooldownBindings*,unsigned priority,unsigned*) noexcept;
    int (__cdecl *concept_ready)(const TalkCooldownBindings*,const char*,unsigned*) noexcept;
    // Original 2208f000 post-response manager updates (host propagates flag2).
    int (__cdecl *note)(void* npc,unsigned bytes,TalkCooldownState*,const TalkCooldownBindings*,const char*) noexcept;
    // Fixed 22089480(-1,0) interaction reset.
    int (__cdecl *reset_interaction)(TalkCooldownState*,const TalkCooldownBindings*) noexcept;
};
using GetTalkCatalogApi=const TalkCatalogApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char companion_talk_export[]="SiN_GetCompanionTalkApi";
struct CompanionTalkBindings {
    void* context;
    int (__cdecl *manager)(void*) noexcept;
    // Host serial-validates handles and reports the live object's extent.
    int (__cdecl *resolve)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *expeditor)(void*,void* npc,void**,unsigned*) noexcept;
    int (__cdecl *player)(void*,void**) noexcept;
    int (__cdecl *within_range)(void*,void* npc,void* player,unsigned*) noexcept;
    // Original urgent: 221a7870(npc,0); normal: 221a7850(npc,1).
    int (__cdecl *suppressed)(void*,void* npc,int argument,unsigned*) noexcept;
    TalkCooldownBindings cooldown;
    SpeechGateBindings semaphore;
    ResponseRepeatBindings repeat;
    ResponseSpeakBindings speak;
};
struct CompanionTalkApi {
    std::uint32_t version,size;
    // Companion-sized objects only. Caller serializes the NPC. The
    // result separates original false from a rejected/failed operation.
    int (__cdecl *group_gate)(void* npc,unsigned bytes,const TalkCooldownState*,const SpeechSemaphorePair*,const CompanionTalkBindings*,unsigned priority,unsigned urgent,unsigned*) noexcept;
    int (__cdecl *concept_gate)(void* npc,unsigned bytes,const TalkCooldownState*,const SpeechSemaphorePair*,const CompanionTalkBindings*,const char*,unsigned urgent,unsigned*) noexcept;
    // 2208f500: gate, then NPC +78c wrapper, then 220a7570.
    int (__cdecl *request)(void* npc,unsigned bytes,const TalkCooldownState*,const SpeechSemaphorePair*,const CompanionTalkBindings*,const char*,const char* modifiers,unsigned urgent,unsigned*) noexcept;
};
using GetCompanionTalkApi=const CompanionTalkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char talk_propagation_export[]="SiN_GetTalkPropagationApi";
struct TalkPropagationBindings {
    void* context;
    // Original obtains the live list base once; count is queried again at
    // each iteration. Host keeps the enumeration token valid during callbacks.
    int (__cdecl *list)(void*,void** token) noexcept;
    int (__cdecl *count)(void*,int*) noexcept;
    // Includes the original RTTI adjustment to a companion NPC.
    // A successful null result means the candidate does not cast.
    int (__cdecl *candidate)(void*,void* token,unsigned index,void** npc,unsigned* bytes) noexcept;
    // Refresh cached absolute position using the existing engine service.
    int (__cdecl *refresh)(void*,void* npc) noexcept;
    int (__cdecl *visible)(void*,void* source,void* candidate,unsigned mask,void* blocker,unsigned*) noexcept;
    int (__cdecl *expeditor)(void*,void* npc,void** exp,unsigned* bytes,const ResponseHistoryBindings**) noexcept;
};
struct TalkPropagationApi {
    std::uint32_t version,size;
    // 2208f055..2208f18e: invoked only for a flag-2 concept. Does not
    // take a second NPC lock or recursively notify the receiving manager.
    int (__cdecl *propagate)(void* npc,unsigned bytes,const TalkPropagationBindings*,const char*) noexcept;
    // 220a7190(name,null,0), exposed for host paths other than propagation.
    int (__cdecl *seed_silent)(void* exp,unsigned bytes,const ResponseHistoryBindings*,const char*) noexcept;
};
using GetTalkPropagationApi=const TalkPropagationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char talk_manager_export[]="SiN_GetTalkManagerApi";
inline constexpr unsigned talk_manager_bytes=0x3bc;
enum class TalkManagerStage:unsigned {Fresh,Constructing,Live,Populating,Destroying,Retired,Failed};
struct TalkManagerControl {
    void* object;
    unsigned bytes;
    TalkManagerStage stage=TalkManagerStage::Fresh;
};
struct TalkManagerSession {
    TalkManagerControl* instance=nullptr; // original singleton at 225492a8
    unsigned creating=0,blocked=0;
};
struct TalkManagerTables {
    // Live replacement words supplied by host; original VAs are evidence only.
    std::uint32_t entity,tree_thunk,compare;
};
struct TalkManagerBindings {
    void* context;
    int (__cdecl *base_construct)(void*,void* manager,int argument) noexcept;
    // Factory creates a manager and publishes it through construct() on
    // this session, as the original constructor publishes its singleton.
    int (__cdecl *create)(void*,TalkManagerSession*,const char* classname,int edict_index) noexcept;
    // Original 22287d80 receives the newly published object.
    int (__cdecl *register_created)(void*,void* manager) noexcept;
    // Original 2208ec80 inserts all 48 descriptors into its tree.
    // Host owns symbol interning, tree allocation and duplicate handling.
    int (__cdecl *insert_deadline)(void*,void* tree,const char*,float initial) noexcept;
    int (__cdecl *free_tree)(void*,std::uint32_t allocation) noexcept;
    int (__cdecl *base_destroy)(void*,void* manager) noexcept;
};
struct TalkManagerApi {
    std::uint32_t version,size;
    int (__cdecl *construct)(TalkManagerSession*,TalkManagerControl*,const TalkManagerTables*,const TalkManagerBindings*) noexcept;
    int (__cdecl *ensure)(TalkManagerSession*,const TalkManagerBindings*,void** manager) noexcept;
    int (__cdecl *populate)(TalkManagerSession*,TalkManagerControl*,const TalkManagerBindings*) noexcept;
    // Copy the three original global deadline fields under the session lock.
    int (__cdecl *read_cooldowns)(const TalkManagerSession*,TalkCooldownState*) noexcept;
    int (__cdecl *write_cooldowns)(TalkManagerSession*,const TalkCooldownState*) noexcept;
    // Nondeleting original destructor; host frees entity allocation.
    int (__cdecl *destroy)(TalkManagerSession*,TalkManagerControl*,const TalkManagerTables*,const TalkManagerBindings*) noexcept;
};
using GetTalkManagerApi=const TalkManagerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char talk_manager_runtime_export[]="SiN_GetTalkManagerRuntimeApi";
struct TalkManagerRuntimeBindings {
    TalkManagerSession* session;
    TalkManagerBindings manager;
    TalkCooldownBindings cooldown;
};
struct TalkManagerRuntimeApi {
    std::uint32_t version,size;
    // Original 2208f540 adjusted owner notification. Validate the paired
    // expeditor owner words before accessing companion-only fields.
    int (__cdecl *notify)(const TalkManagerRuntimeBindings*,void* npc,unsigned bytes,void* exp,unsigned exp_bytes,const char*) noexcept;
    // Original 22089480(-1,0).
    int (__cdecl *reset_interaction)(const TalkManagerRuntimeBindings*) noexcept;
    // Group readiness uses live manager fields rather than a detached copy.
    int (__cdecl *group_ready)(const TalkManagerRuntimeBindings*,const void* npc,unsigned bytes,unsigned priority,unsigned*) noexcept;
};
using GetTalkManagerRuntimeApi=const TalkManagerRuntimeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;















}
