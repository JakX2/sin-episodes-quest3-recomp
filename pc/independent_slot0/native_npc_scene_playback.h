#pragma once
#include "native_jessica_script.h"
#include "../../include/sin_compat/vcd_tokenizer.hpp"
namespace sin_port::native_weapon_tables {
inline constexpr char npc_scene_playback_export[]="SiN_GetNpcScenePlaybackApi";
struct SceneDurationBindings {
    void* context;
    // Host resolves each event from the original parsed scene vector.
    int (__cdecl *event)(void*,void* parsed,unsigned index,void** event,unsigned* bytes) noexcept;
};
struct NpcScenePlaybackBindings {
    void* context;
    // Engine creates and owns instanced_scripted_scene. Full object storage.
    int (__cdecl *create)(void*,const char* classname,void** entity,unsigned* bytes) noexcept;
    // Original 221b52f0 model-dependent scene name resolution.
    int (__cdecl *resolve_name)(void*,void* npc,const char*,char* dest,unsigned capacity) noexcept;
    int (__cdecl *actor_handle)(void*,void* npc,std::uint32_t*) noexcept;
    int (__cdecl *set_delay)(void*,void* scene,float delay) noexcept;
    int (__cdecl *register_scene)(void*,void* scene) noexcept;
    int (__cdecl *spawn_sound)(void*,void* scene) noexcept;
    int (__cdecl *activate)(void*,void* scene) noexcept;
    int (__cdecl *set_parsed_flag)(void*,std::uint32_t parsed,int argument) noexcept;
    int (__cdecl *start)(void*,void* scene) noexcept;
    int (__cdecl *missing)(void*,const char*,unsigned generated) noexcept;
    int (__cdecl *resolve_parsed)(void*,std::uint32_t,void**,unsigned*) noexcept;
    SceneDurationBindings duration;
};
struct NpcScenePlaybackApi {
    std::uint32_t version,size;
    // Original 22113c10 duration of parsed choreography; event end at
    // +188 unless bit-exact -1 sentinel, then event start at +184.
    int (__cdecl *duration)(void* parsed,unsigned bytes,const SceneDurationBindings*,float*) noexcept;
    // Fixed 220315e0 -> 221a4330 NPC path (no output handle, flag zero).
    int (__cdecl *scene)(void* npc,const char* name,float delay,const NpcScenePlaybackBindings*,float*) noexcept;
    // Fixed 22031600 -> 221a5710 NPC sound scene, no output handle.
    // The sound string is borrowed by the engine entity, as in original.
    int (__cdecl *sound)(void* npc,const char* name,const NpcScenePlaybackBindings*,float*) noexcept;
};
using GetNpcScenePlaybackApi=const NpcScenePlaybackApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_manager_query_export[]="SiN_GetSceneManagerQueryApi";
enum class SceneQueryKind:unsigned {Cancel,Active,FutureSpeech};
struct SceneManagerQueryBindings {
    void* context;
    // Host resolves vector entries and serial-validates scene EHANDLEs.
    int (__cdecl *entry)(void*,void* manager,unsigned index,std::uint32_t*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *instanced)(void*,void* scene,unsigned*) noexcept;
    int (__cdecl *parsed)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *actor)(void*,void* scene,unsigned index,void**) noexcept;
    int (__cdecl *cancel)(void*,void* scene) noexcept;
    SceneDurationBindings events;
};
struct SceneManagerQueryApi {
    std::uint32_t version,size;
    int (__cdecl *contains_actor)(void* scene,unsigned bytes,void* npc,const SceneManagerQueryBindings*,unsigned*) noexcept;
    int (__cdecl *future_speech)(void* parsed,unsigned bytes,const SceneDurationBindings*,unsigned*) noexcept;
    // Result for Cancel is the number of completed cancellation callbacks;
    // Active/FutureSpeech return an original boolean. No output on error.
    int (__cdecl *query)(void* manager,unsigned bytes,void* npc,SceneQueryKind,unsigned filter,const SceneManagerQueryBindings*,unsigned*) noexcept;
};
using GetSceneManagerQueryApi=const SceneManagerQueryApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_manager_runtime_export[]="SiN_GetSceneManagerRuntimeApi";
struct SceneManagerSession {
    std::uint32_t handle=0xffffffffu;
    unsigned initialized=0,busy=0,blocked=0;
};
struct SceneManagerRuntimeBindings {
    void* context;
    int (__cdecl *resolve)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *create)(void*,const char*,int edict_index,void**) noexcept;
    int (__cdecl *packed_handle)(void*,void*,std::uint32_t*) noexcept;
    // Original manager virtual +64 (spawn).
    int (__cdecl *spawn)(void*,void*) noexcept;
    SceneManagerQueryBindings query;
};
struct SceneManagerRuntimeApi {
    std::uint32_t version,size;
    int (__cdecl *ensure)(SceneManagerSession*,const SceneManagerRuntimeBindings*,void**,unsigned*) noexcept;
    int (__cdecl *query)(SceneManagerSession*,const SceneManagerRuntimeBindings*,void* npc,SceneQueryKind,unsigned filter,unsigned*) noexcept;
};
using GetSceneManagerRuntimeApi=const SceneManagerRuntimeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char response_choreography_export[]="SiN_GetResponseChoreographyApi";
struct ResponseChoreographyBindings {
    void* context;
    // Resolve exp+4 to the current, live NPC. Do not substitute another actor.
    int (__cdecl *owner)(void*,void* exp,void**) noexcept;
    // Intern/copy the generated sound name into storage valid for the
    // engine scene lifetime, even if the originating response is destroyed.
    int (__cdecl *retain_sound)(void*,const char*,const char**) noexcept;
    NpcScenePlaybackBindings playback;
    SceneManagerSession* manager;
    SceneManagerRuntimeBindings scenes;
};
struct ResponseChoreographyApi {
    std::uint32_t version,size;
    // The four legacy callbacks sound/scene/stop_scene/scene_blocked are
    // replaced by native choreography. Other engine services are unchanged.
    int (__cdecl *dispatch)(void* exp,unsigned bytes,const ResponseDispatchBindings*,const ResponseChoreographyBindings*,const char*,void* response,unsigned extent,unsigned* played) noexcept;
    int (__cdecl *speak)(void* exp,unsigned bytes,const ResponseSpeakBindings*,const ResponseChoreographyBindings*,const char*,const char* modifiers,unsigned* played) noexcept;
};
using GetResponseChoreographyApi=const ResponseChoreographyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_state_export[]="SiN_GetSceneStateApi";
struct SceneStateBindings {
    void* context;
    // Original parsed choreography resume 22115aa0.
    int (__cdecl *resume_parsed)(void*,std::uint32_t parsed) noexcept;
    // Original output +44c, activator and caller are the scene.
    int (__cdecl *completion)(void*,void* output,void* activator,void* caller,float delay) noexcept;
    int (__cdecl *finish)(void*,void* scene,int completed,int argument) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    // Original scene virtual +2b0, after wait fields have been cleared.
    int (__cdecl *resume_wait)(void*,void* scene) noexcept;
};
struct SceneStateApi {
    std::uint32_t version,size;
    int (__cdecl *pause)(void*,unsigned) noexcept;
    int (__cdecl *resume)(void*,unsigned,const SceneStateBindings*) noexcept;
    int (__cdecl *complete)(void*,unsigned,const SceneStateBindings*) noexcept;
    int (__cdecl *wait_tick)(void*,unsigned,const SceneStateBindings*) noexcept;
    int (__cdecl *reset_wait)(void*,unsigned) noexcept;
};
using GetSceneStateApi=const SceneStateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_manager_tick_export[]="SiN_GetSceneManagerTickApi";
inline constexpr unsigned scene_manager_bytes=0x3bc;
enum class SceneManagerStage:unsigned {Fresh,Constructing,Live,Destroying,Retired,Failed};
struct SceneManagerControl {
    void* object;
    unsigned bytes;
    SceneManagerStage stage=SceneManagerStage::Fresh;
    unsigned ticking=0;
};
struct SceneManagerLifecycleBindings {
    void* context;
    int (__cdecl *base_construct)(void*,void*,int) noexcept;
    int (__cdecl *base_spawn)(void*,void*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *schedule)(void*,void*,float,int) noexcept;
    int (__cdecl *free_vector)(void*,std::uint32_t) noexcept;
    int (__cdecl *base_destroy)(void*,void*) noexcept;
};
struct SceneManagerTickBindings {
    void* context;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *schedule)(void*,void*,float,int) noexcept;
    int (__cdecl *last_think)(void*,void*,int,float*) noexcept;
    int (__cdecl *disabled)(void*,unsigned*) noexcept;
    int (__cdecl *entry)(void*,void*,unsigned,std::uint32_t*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *advance)(void*,void*,float) noexcept;
    // Shift the remaining vector entries; native code decrements count.
    // Host owns allocation and validates the requested span.
    int (__cdecl *shift)(void*,void* manager,unsigned index,unsigned trailing) noexcept;
};
struct SceneManagerTickApi {
    std::uint32_t version,size;
    int (__cdecl *construct)(SceneManagerControl*,std::uint32_t live_table,const SceneManagerLifecycleBindings*) noexcept;
    int (__cdecl *spawn)(SceneManagerControl*,const SceneManagerLifecycleBindings*) noexcept;
    int (__cdecl *tick)(SceneManagerControl*,const SceneManagerTickBindings*) noexcept;
    int (__cdecl *clear)(SceneManagerControl*) noexcept;
    int (__cdecl *destroy)(SceneManagerControl*,const SceneManagerLifecycleBindings*) noexcept;
};
using GetSceneManagerTickApi=const SceneManagerTickApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_registration_export[]="SiN_GetSceneRegistrationApi";
struct SceneRegistrationBindings {
    void* context;
    int (__cdecl *packed_handle)(void*,void* scene,std::uint32_t*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    // Host validates the allocation word and returns its live writable span.
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned* bytes) noexcept;
    // Original vector grow (221a4f00), requested additional capacity.
    // Host updates +394/+398; it must not mutate logical count +3a0.
    int (__cdecl *grow)(void*,void* manager,unsigned additional) noexcept;
};
struct SceneRegistrationApi {
    std::uint32_t version,size;
    // Original register 221ab300 and remove 221a9fa0. Null scene is
    // supported: its resolved identity is null, as in original 221a7940.
    // Caller serializes list access; may run within a manager advance
    // callback. Output changed is a native diagnostic, not original return.
    int (__cdecl *add)(void*,unsigned,const SceneRegistrationBindings*,void* scene,unsigned* changed) noexcept;
    int (__cdecl *remove)(void*,unsigned,const SceneRegistrationBindings*,void* scene,unsigned* changed) noexcept;
};
using GetSceneRegistrationApi=const SceneRegistrationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_entity_lifecycle_export[]="SiN_GetSceneEntityLifecycleApi";
struct SceneEntityLifecycleBindings {
    void* context;
    int (__cdecl *resolve_actor)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *sound_name)(void*,std::uint32_t,const char**) noexcept;
    // Original 221a3c30 creates parsed choreography. Null is a normal result.
    int (__cdecl *generate)(void*,void* scene,void* actor,const char*,std::uint32_t*) noexcept;
    int (__cdecl *base_activate)(void*,void*) noexcept;
    // Original 221a2c90(scene,parsed,0) before actor cleanup.
    int (__cdecl *stop_events)(void*,void*,std::uint32_t,int) noexcept;
    int (__cdecl *parsed)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *actor)(void*,void*,unsigned,void**) noexcept;
    int (__cdecl *detach)(void*,void* actor,std::uint32_t parsed) noexcept;
    // Consumes parsed allocation on invocation, even on transport error.
    int (__cdecl *destroy_parsed)(void*,std::uint32_t) noexcept;
    int (__cdecl *base_remove)(void*,void*) noexcept;
    SceneManagerSession* manager;
    SceneManagerRuntimeBindings runtime;
    SceneRegistrationBindings registration;
};
struct SceneEntityLifecycleApi {
    std::uint32_t version,size;
    // Full object is kept live by host across callbacks. The base removal
    // must not free scene storage before unregistering.
    int (__cdecl *activate)(void*,unsigned,const SceneEntityLifecycleBindings*) noexcept;
    int (__cdecl *cleanup)(void*,unsigned,const SceneEntityLifecycleBindings*) noexcept;
    int (__cdecl *remove)(void*,unsigned,const SceneEntityLifecycleBindings*) noexcept;
};
using GetSceneEntityLifecycleApi=const SceneEntityLifecycleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char parsed_scene_resume_export[]="SiN_GetParsedSceneResumeApi";
struct ParsedSceneResumeBindings {
    void* context;
    // Resolve a PE32 event object and validate its extent.
    int (__cdecl *event)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Resolve raw vector words, not event objects.
    int (__cdecl *words)(void*,std::uint32_t,const void**,unsigned*) noexcept;
    // Temporary snapshot owned by the call, not the choreography. Host
    // allocation must remain distinct from source vector and events.
    int (__cdecl *allocate)(void*,unsigned bytes,void**,unsigned*) noexcept;
    int (__cdecl *release)(void*,void*) noexcept;
    // Original event callback interface at parsed+9c, virtual slot zero.
    int (__cdecl *start)(void*,std::uint32_t callback,float time,void* parsed,void* event) noexcept;
};
struct ParsedSceneResumeApi {
    std::uint32_t version,size;
    // 22115aa0: restart children of pending resume event, then reset index
    // and wait word. Failure leaves prior event mutations committed.
    int (__cdecl *resume)(void* parsed,unsigned bytes,const ParsedSceneResumeBindings*) noexcept;
};
using GetParsedSceneResumeApi=const ParsedSceneResumeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char parsed_event_classify_export[]="SiN_GetParsedEventClassifyApi";
enum class ParsedEventAction:unsigned {None,Start,StartTracked,Update,Stop};
struct ParsedEventDecision {ParsedEventAction action;unsigned active_count;};
struct ParsedEventClassifyBindings {
    void* context;
    // Resolves indexed pending/pause event from parsed+64 vector.
    int (__cdecl *pending)(void*,void* parsed,unsigned index,void**,unsigned*) noexcept;
    // Original 22111080 parent child-vector insertion.
    int (__cdecl *attach)(void*,void* parent,void* child) noexcept;
};
struct ParsedEventClassifyApi {
    std::uint32_t version,size;
    int (__cdecl *classify)(void* parsed,unsigned bytes,void* event,unsigned extent,float previous,float target,unsigned forward,const ParsedEventClassifyBindings*,ParsedEventDecision*) noexcept;
};
using GetParsedEventClassifyApi=const ParsedEventClassifyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char parsed_scene_simulation_export[]="SiN_GetParsedSceneSimulationApi";
struct ParsedSimulationBindings {
    void* context;
    ParsedEventClassifyBindings classifier;
    SceneDurationBindings events;
    // Original 22107170 channel index for a shared actor.
    int (__cdecl *channel_index)(void*,std::uint32_t actor,std::uint32_t channel,int*) noexcept;
    // Original tracked vector +50: append event after start callback.
    int (__cdecl *track)(void*,void* parsed,void* event) noexcept;
    // Original callback interface slots 0, 8, 4 (start, update, stop).
    int (__cdecl *callback)(void*,std::uint32_t interface_word,unsigned slot,float,void* parsed,void* event) noexcept;
};
struct ParsedSceneSimulationApi {
    std::uint32_t version,size;
    int (__cdecl *simulate)(void* parsed,unsigned bytes,float target,const ParsedSimulationBindings*) noexcept;
    int (__cdecl *less)(const void* a,unsigned a_bytes,const void* b,unsigned b_bytes,const ParsedSimulationBindings*,unsigned*) noexcept;
};
using GetParsedSceneSimulationApi=const ParsedSceneSimulationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_advance_export[]="SiN_GetSceneAdvanceApi";
struct SceneAdvanceBindings {
    void* context;
    // Original scene virtual +29c, then optional +2a8 and +294.
    int (__cdecl *pre_tick)(void*,void*) noexcept;
    int (__cdecl *start)(void*,void*) noexcept;
    int (__cdecl *paused_tick)(void*,void*) noexcept;
    int (__cdecl *resolve_actor)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *parsed)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Original nested +418 -> +18 -> +28 interval, host bounds checked.
    int (__cdecl *latency)(void*,std::uint32_t,float*) noexcept;
    int (__cdecl *finish)(void*,void* scene,int completed,int cleanup) noexcept;
    int (__cdecl *release_actors)(void*,void* scene,std::uint32_t parsed) noexcept;
    int (__cdecl *remove)(void*,void* scene) noexcept;
    ParsedSimulationBindings simulation;
};
struct SceneAdvanceApi {
    std::uint32_t version,size;
    int (__cdecl *finished)(const void* parsed,unsigned bytes,unsigned*) noexcept;
    int (__cdecl *scripted)(void* scene,unsigned bytes,float dt,const SceneAdvanceBindings*) noexcept;
    int (__cdecl *instanced)(void* scene,unsigned bytes,float dt,const SceneAdvanceBindings*) noexcept;
};
using GetSceneAdvanceApi=const SceneAdvanceApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char integrated_scene_playback_export[]="SiN_GetIntegratedScenePlaybackApi";
struct IntegratedScenePlaybackApi {
    std::uint32_t version,size;
    // Substitute recovered activation/registration for the legacy activate
    // callback. Entity allocation, start, and engine removal remain hosted.
    int (__cdecl *scene)(void* npc,const char*,float,const NpcScenePlaybackBindings*,const SceneEntityLifecycleBindings*,float*) noexcept;
    int (__cdecl *sound)(void* npc,const char*,const NpcScenePlaybackBindings*,const SceneEntityLifecycleBindings*,float*) noexcept;
};
using GetIntegratedScenePlaybackApi=const IntegratedScenePlaybackApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char integrated_response_export[]="SiN_GetIntegratedResponseApi";
struct IntegratedResponseApi {
    std::uint32_t version,size;
    // Interruption and activation must share one scene-manager session.
    int (__cdecl *dispatch)(void*,unsigned,const ResponseDispatchBindings*,const ResponseChoreographyBindings*,const SceneEntityLifecycleBindings*,const char*,void*,unsigned,unsigned*) noexcept;
    int (__cdecl *speak)(void*,unsigned,const ResponseSpeakBindings*,const ResponseChoreographyBindings*,const SceneEntityLifecycleBindings*,const char*,const char*,unsigned*) noexcept;
};
using GetIntegratedResponseApi=const IntegratedResponseApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct JessicaDialogueBindings;
struct CountDialogueBindings;
inline constexpr char integrated_npc_dialogue_export[]="SiN_GetIntegratedNpcDialogueApi";
struct IntegratedNpcDialogueApi {
    std::uint32_t version,size;
    // Optional managed cooldown source; otherwise use the original detached
    // cooldown binding. Both characters require the same scene session.
    int (__cdecl *jessica)(HologramControl*,JessicaAttentionState*,const JessicaDialogueBindings*,const TalkManagerRuntimeBindings*,const ResponseChoreographyBindings*,const SceneEntityLifecycleBindings*,const void*,unsigned) noexcept;
    int (__cdecl *count)(HologramControl*,const CountDialogueBindings*,const ResponseChoreographyBindings*,const SceneEntityLifecycleBindings*,const void*,unsigned) noexcept;
};
using GetIntegratedNpcDialogueApi=const IntegratedNpcDialogueApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char generated_scene_export[]="SiN_GetGeneratedSceneApi";
enum class GeneratedSceneChild:unsigned {Actor,Channel,Event};
enum class GeneratedSceneWarning:unsigned {Duration,SceneAllocation,ChildAllocation};
struct GeneratedSceneBindings {
    void* context;
    // Actor virtual +1c returns a model name word; null result means "".
    int (__cdecl *model_name)(void*,void* actor,const char**) noexcept;
    int (__cdecl *duration)(void*,const char* sound,const char* model,float*) noexcept;
    int (__cdecl *warning)(void*,GeneratedSceneWarning,const char* sound) noexcept;
    // Fresh original-compatible constructors. Child callback allocates,
    // constructs AND appends to the corresponding parsed vector even if null.
    int (__cdecl *create_parsed)(void*,void* callback,void**,unsigned*) noexcept;
    int (__cdecl *create_child)(void*,void* parsed,GeneratedSceneChild,void**,unsigned*) noexcept;
    // Resolve scene+394 string word. A zero word bypasses the callback.
    int (__cdecl *scene_name)(void*,std::uint32_t,const char**) noexcept;
    // Append channel to actor or event to channel; parent owns the vector.
    int (__cdecl *append)(void*,GeneratedSceneChild,void* parent,void* child) noexcept;
    // Consumes parsed allocation and its child vectors on invocation.
    int (__cdecl *destroy)(void*,void* parsed) noexcept;
    // Live substitute for original 221a17b0 debug-print callback.
    std::uint32_t diagnostic_word;
};
struct GeneratedSceneApi {
    std::uint32_t version,size;
    // Original 221a3c30. Normal missing duration/allocation returns 1
    // with zero output. Transport failure returns -1, no output committed.
    int (__cdecl *generate)(void* scene,unsigned bytes,void* actor,const char* sound,const GeneratedSceneBindings*,std::uint32_t*) noexcept;
};
using GetGeneratedSceneApi=const GeneratedSceneApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_construct_export[]="SiN_GetChoreographyConstructApi";
struct ParsedConstructBindings {
    void* context;
    // 2228f360: construct symbol table at parsed+164.
    int (__cdecl *symbol_construct)(void*,void*,int,int,unsigned) noexcept;
    // Live replacement comparator 22112550 for the parsed tree.
    std::uint32_t compare_word;
};
struct ChoreographyConstructApi {
    std::uint32_t version,size;
    // Original 221161d0/22111280, 22107360, 22109c70, 22110930/2210c600.
    // Caller owns fresh uninitialized allocation; no blanket clearing.
    int (__cdecl *parsed)(void*,unsigned,void* callback,const ParsedConstructBindings*) noexcept;
    int (__cdecl *actor)(void*,unsigned) noexcept;
    int (__cdecl *channel)(void*,unsigned) noexcept;
    // Counter is module-wide, not per scene; caller serializes construction.
    int (__cdecl *event)(void*,unsigned,void* parsed,std::uint32_t* counter) noexcept;
};
using GetChoreographyConstructApi=const ChoreographyConstructApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_vector_export[]="SiN_GetChoreographyVectorApi";
struct ChoreographyVectorBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Original malloc/realloc leaves. On success return writable allocation
    // of at least requested bytes, preserving all old bytes. old=0 means
    // allocate. Failure retains the previous allocation and ownership.
    int (__cdecl *resize)(void*,std::uint32_t,unsigned,void**,unsigned*) noexcept;
};
struct ChoreographyVectorApi {
    std::uint32_t version,size;
    // Context points to ChoreographyVectorBindings; boolean status.
    int (__cdecl *grow_memory)(void*,void*,unsigned,unsigned) noexcept;
    int (__cdecl *grow_vector)(void*,void*,unsigned,unsigned) noexcept;
    int (__cdecl *insert_word)(void*,void*,unsigned,std::uint32_t) noexcept;
    // ABI-compatible adapters for generated scene/parser callbacks.
    int (__cdecl *append_parsed)(void*,void*,GeneratedSceneChild,void*) noexcept;
    int (__cdecl *append_channel)(void*,void*,void*) noexcept;
    int (__cdecl *append_event)(void*,void*,void*) noexcept;
};
using GetChoreographyVectorApi=const ChoreographyVectorApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char allocated_generated_scene_export[]="SiN_GetAllocatedGeneratedSceneApi";
struct GeneratedSceneAllocationBindings {
    void* context;
    // Return fresh allocation, or successful null for ordinary OOM.
    int (__cdecl *allocate)(void*,unsigned requested,void**,unsigned*) noexcept;
    // Consumes an unpublished raw allocation; symbol constructor failure
    // must clean its own partial resources before this callback.
    int (__cdecl *release_unpublished)(void*,void*) noexcept;
    // Append to parsed vector, including null, and assume ownership of
    // child on success only. Parsed destruction owns published children.
    int (__cdecl *append_parsed)(void*,void* parsed,GeneratedSceneChild,void* child) noexcept;
    ParsedConstructBindings parsed;
    std::uint32_t* event_counter;
};
struct AllocatedGeneratedSceneApi {
    std::uint32_t version,size;
    // Replaces create_parsed/create_child and lifecycle.generate callbacks
    // with native constructors; raw allocation and vector insertion hosted.
    int (__cdecl *generate)(void*,unsigned,void*,const char*,const GeneratedSceneBindings*,const GeneratedSceneAllocationBindings*,std::uint32_t*) noexcept;
    int (__cdecl *activate)(void*,unsigned,const SceneEntityLifecycleBindings*,const GeneratedSceneBindings*,const GeneratedSceneAllocationBindings*) noexcept;
    int (__cdecl *sound)(void*,const char*,const NpcScenePlaybackBindings*,const SceneEntityLifecycleBindings*,const GeneratedSceneBindings*,const GeneratedSceneAllocationBindings*,float*) noexcept;
};
using GetAllocatedGeneratedSceneApi=const AllocatedGeneratedSceneApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char parsed_scene_reset_export[]="SiN_GetParsedSceneResetApi";
struct ParsedSceneResetBindings {
    SceneDurationBindings events;
    void* context;
    // Resolve vector allocation word to writable live storage.
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Original 22107c90: grow vector at its base, update allocation
    // and capacity, but not count. Additional capacity requested.
    int (__cdecl *grow)(void*,void* vector,unsigned additional) noexcept;
};
struct ParsedSceneResetApi {
    std::uint32_t version,size;
    // 2210b8b0 resets active byte and loop count (type 12).
    int (__cdecl *event_reset)(void*,unsigned) noexcept;
    // 22113cd0/22113d40: zero-anchored bounds, speech latency included.
    int (__cdecl *bounds)(void*,unsigned,const SceneDurationBindings*,float*,float*) noexcept;
    // 22117c20: rebuild pause and tracked lists, then reset clock and
    // preserve the two caller-supplied raw timing words at +7c/+80.
    int (__cdecl *reset)(void*,unsigned,unsigned forward,std::uint32_t,std::uint32_t,const ParsedSceneResetBindings*) noexcept;
};
using GetParsedSceneResetApi=const ParsedSceneResetApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_start_export[]="SiN_GetSceneStartApi";
enum class SceneStartResult:unsigned {NoAction,Started,ActorBlocked,Deferred};
struct SceneStartBindings {
    void* context;
    int (__cdecl *parsed)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Original scene virtual +340: actor resolved by parsed index.
    int (__cdecl *actor)(void*,void*,unsigned,void**,unsigned*) noexcept;
    // Original 22139f60 calls actor virtual +f4 each time.
    int (__cdecl *is_npc)(void*,void*,unsigned*) noexcept;
    // Original +5bc expeditor virtual and 220a4680 speech predicate.
    int (__cdecl *speaking)(void*,void*,unsigned*) noexcept;
    // 220fb1d0 associates parsed scene with actor.
    int (__cdecl *attach)(void*,void*,std::uint32_t) noexcept;
    int (__cdecl *refresh)(void*,void*) noexcept;
    int (__cdecl *stop_events)(void*,void*,std::uint32_t,int) noexcept;
    int (__cdecl *output)(void*,void* output,void* activator,void* caller,float delay) noexcept;
    // Original startup event collection/dispatch 221adaa0 + 221a9ba0;
    // scratch symbol table/vector allocation is still hosted.
    int (__cdecl *startup_events)(void*,void*,std::uint32_t) noexcept;
    ParsedSceneResetBindings reset;
};
struct SceneStartApi {
    std::uint32_t version,size;
    int (__cdecl *start)(void*,unsigned,const SceneStartBindings*,SceneStartResult*) noexcept;
};
using GetSceneStartApi=const SceneStartApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_actor_events_export[]="SiN_GetSceneActorEventsApi";
struct SceneActorEventBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *grow)(void*,void* vector,unsigned additional) noexcept;
    // Original actor virtual +2f0. Record remains allocated and vector
    // identity/count must not change during the hosted callback.
    int (__cdecl *stop_record)(void*,void* actor,void* record,int zero,int argument) noexcept;
};
struct SceneStopBindings {
    void* context;
    int (__cdecl *parsed)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *actor)(void*,void* scene,unsigned,void**,unsigned*) noexcept;
    SceneDurationBindings events;
    SceneActorEventBindings records;
};
struct SceneActorEventsApi {
    std::uint32_t version,size;
    int (__cdecl *attach)(void*,unsigned,std::uint32_t,const SceneActorEventBindings*) noexcept;
    int (__cdecl *detach)(void*,unsigned,std::uint32_t,const SceneActorEventBindings*) noexcept;
    int (__cdecl *stop_actor)(void*,unsigned,std::uint32_t,int,const SceneActorEventBindings*) noexcept;
    // Original 221a2c90, including nested type 11 choreography.
    int (__cdecl *stop_scene)(void*,unsigned,std::uint32_t,int,const SceneStopBindings*) noexcept;
};
using GetSceneActorEventsApi=const SceneActorEventsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_generated_lifecycle_export[]="SiN_GetNativeGeneratedLifecycleApi";
struct NativeGeneratedLifecycleBindings {
    NpcScenePlaybackBindings playback;
    SceneEntityLifecycleBindings lifecycle;
    GeneratedSceneBindings generation;
    GeneratedSceneAllocationBindings allocation;
    SceneStartBindings startup;
    SceneStopBindings stop;
};
struct NativeGeneratedLifecycleApi {
    std::uint32_t version,size;
    // Native construction, activation, actor association, reset and start.
    int (__cdecl *sound)(void*,const char*,const NativeGeneratedLifecycleBindings*,float*) noexcept;
    // Retry an existing scene start, e.g. when an actor finishes speaking.
    int (__cdecl *start)(void*,unsigned,const SceneStartBindings*,const SceneStopBindings*,SceneStartResult*) noexcept;
    // Native recursive record stop and actor detach, then original
    // compatible hosted parsed destruction and manager unregister.
    int (__cdecl *remove)(void*,unsigned,const SceneEntityLifecycleBindings*,const SceneStopBindings*) noexcept;
};
using GetNativeGeneratedLifecycleApi=const NativeGeneratedLifecycleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_finish_export[]="SiN_GetSceneFinishApi";
struct SceneFinishBindings {
    void* context;
    // Waiting entity handle vector at scene+534, count +540.
    int (__cdecl *entry)(void*,void*,unsigned,std::uint32_t*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *notify)(void*,void* waiting,void* scene) noexcept;
    int (__cdecl *refresh)(void*,void*) noexcept;
    int (__cdecl *output)(void*,void* output,void* activator,void* caller,float) noexcept;
    ParsedSceneResetBindings reset;
    SceneStopBindings stop;
};
struct SceneFinishApi {
    std::uint32_t version,size;
    // Original 221a7230, virtual +28c on both scene vtables.
    int (__cdecl *finish)(void*,unsigned,int stop_argument,unsigned fire_output,const SceneFinishBindings*) noexcept;
    // Existing original tick routes with native finish in place of host
    // virtual +28c. Scripted non-NPC release remains a host service.
    int (__cdecl *advance)(void*,unsigned,float,const SceneAdvanceBindings*,const SceneFinishBindings*,unsigned instanced) noexcept;
};
using GetSceneFinishApi=const SceneFinishApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char actor_event_record_export[]="SiN_GetActorEventRecordApi";
enum class ActorEventDiagnostic:unsigned {NullSceneOrEvent,NullChoreoActor,StartRejected};
struct ActorEventRecordBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *grow)(void*,void* vector,unsigned) noexcept;
    int (__cdecl *handle)(void*,void* target,std::uint32_t*) noexcept;
    int (__cdecl *diagnostic)(void*,ActorEventDiagnostic,std::uint32_t parsed) noexcept;
    // Original +2e4/+2f0/+2f4. Transport success distinct from AL.
    int (__cdecl *begin)(void*,void* actor,void* record,std::uint32_t parsed,void* event,std::uint32_t choreo_actor,void* target,unsigned*) noexcept;
    int (__cdecl *stop)(void*,void* actor,void* record,int argument,int zero,unsigned*) noexcept;
    int (__cdecl *check)(void*,void* actor,void* record,float,std::uint32_t parsed,void* event,unsigned*) noexcept;
};
struct ActorEventRecordApi {
    std::uint32_t version,size;
    // Original 220fb220: construct record, start via virtual, append on AL.
    int (__cdecl *add)(void*,unsigned,std::uint32_t,void* event,unsigned,void* target,const ActorEventRecordBindings*,unsigned*) noexcept;
    // 220f8340: forward scan, remove first accepted matching record.
    int (__cdecl *stop)(void*,unsigned,std::uint32_t,void* event,int,const ActorEventRecordBindings*,unsigned*) noexcept;
    // 220f83f0: first matching record check; absent record means true.
    int (__cdecl *check)(void*,unsigned,float,std::uint32_t,void* event,const ActorEventRecordBindings*,unsigned*) noexcept;
};
using GetActorEventRecordApi=const ActorEventRecordApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_speech_policy_export[]="SiN_GetSceneSpeechPolicyApi";
struct SceneSpeechTiming {float offset,absolute_start,remaining;};
struct SceneSpeechQueueBindings {
    void* context;
    int (__cdecl *handle)(void*,void* actor,std::uint32_t*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *grow)(void*,void* vector,unsigned additional) noexcept;
};
struct SceneSpeechPolicyApi {
    std::uint32_t version,size;
    // 2210c880: output empty and available=0 for suppressed token.
    int (__cdecl *token)(const void*,unsigned,char*,unsigned,unsigned*) noexcept;
    // 221a3df0: client "english" Q_atoi value (absent means English);
    // force_alternate is original global ConVar +2c. Output still contains
    // primary sound if original branch returns available=0.
    int (__cdecl *sound)(const void*,unsigned,unsigned has_language,int language,unsigned force_alternate,char*,unsigned,unsigned*) noexcept;
    int (__cdecl *timing)(const void*,unsigned,const void*,unsigned,float now,SceneSpeechTiming*) noexcept;
    // 221ab350/221a8260: 0x8c pending speech records at manager+3a8.
    int (__cdecl *queue)(void*,unsigned,void* actor,const char*,int level,float offset,const SceneSpeechQueueBindings*) noexcept;
};
using GetSceneSpeechPolicyApi=const SceneSpeechPolicyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_speech_execution_export[]="SiN_GetSceneSpeechExecutionApi";
struct SceneSpeechExecutionBindings {
    void* context;
    // Original 220f6450 recipient filter. Host owns and releases opaque filter.
    int (__cdecl *open_filter)(void*,void* actor,float,void**) noexcept;
    int (__cdecl *close_filter)(void*,void*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    // Original base-actor RTTI/virtual or NPC semaphore preparation, using
    // remaining duration. Engine owns classification and virtual effects.
    int (__cdecl *prepare_actor)(void*,void* scene,void* actor,float remaining) noexcept;
    int (__cdecl *recipient_count)(void*,void* filter,int*) noexcept;
    // Null recipient is an ordinary stale/missing player.
    int (__cdecl *recipient)(void*,void* filter,unsigned index,void**) noexcept;
    int (__cdecl *language)(void*,void* player,unsigned* present,int* english,unsigned* forced) noexcept;
    // Original 221b7760. Host creates single-recipient engine filter.
    int (__cdecl *emit)(void*,void* actor,void* player,const char*,int channel,float volume,int level,float absolute_start) noexcept;
    // Original caption registry then client preference filtering 22139fa0.
    int (__cdecl *caption_register)(void*,std::uint32_t,const char*) noexcept;
    int (__cdecl *caption_preferences)(void*,void* filter) noexcept;
    // Original 221a7060; host computes range from emission origins.
    int (__cdecl *caption_far)(void*,void* player,unsigned*) noexcept;
    int (__cdecl *remove_recipient)(void*,void* filter,void* player) noexcept;
    // Original CloseCaption user message fields: CRC32, duration byte, 1.
    int (__cdecl *caption_send)(void*,void* filter,std::uint32_t,unsigned duration_byte,unsigned flag) noexcept;
    ActorEventRecordBindings records;
    SceneManagerSession* manager;
    SceneManagerRuntimeBindings runtime;
    SceneSpeechQueueBindings queue;
};
struct SceneSpeechExecutionApi {
    std::uint32_t version,size;
    // Original 221ac400, engine effects hosted. Actor may be null (no-op).
    int (__cdecl *start)(void* scene,unsigned,void* actor,unsigned,std::uint32_t parsed,void* event,unsigned,int level,const SceneSpeechExecutionBindings*) noexcept;
};
using GetSceneSpeechExecutionApi=const SceneSpeechExecutionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_speech_callback_export[]="SiN_GetSceneSpeechCallbackApi";
enum class SceneSpeechTrace:unsigned {Ignored,Start,Stop,MissingActor};
struct SceneSpeechCallbackBindings {
    void* context;
    // Original scene +33c resolves a choreography actor, not entity handle.
    int (__cdecl *actor)(void*,void* scene,std::uint32_t choreo_actor,void**,unsigned*) noexcept;
    int (__cdecl *trace)(void*,void* scene,void* event,float,SceneSpeechTrace) noexcept;
    SceneSpeechExecutionBindings speech;
};
struct SceneSpeechCallbackApi {
    std::uint32_t version,size;
    // 221a1850/70/90/b0 and type-5 branches of 221a2270/2830/2b60/2ba0.
    // Interface is scene+410, owner word +4; zero owner is a no-op.
    // Other event types are not implemented by this specialized route.
    int (__cdecl *invoke)(const void* callback,unsigned callback_bytes,void* scene,unsigned scene_bytes,
        unsigned slot,float time,void* parsed,unsigned parsed_bytes,void* event,unsigned event_bytes,
        const SceneSpeechCallbackBindings*,unsigned* result) noexcept;
};
using GetSceneSpeechCallbackApi=const SceneSpeechCallbackApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Shared scripted/instanced actor cache: 221a2c20, 221aaf50, 22114060.
inline constexpr char scene_actor_cache_export[]="SiN_GetSceneActorCacheApi";
struct SceneActorCacheBindings {
    void* context;
    int (__cdecl *parsed)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *grow)(void*,void* vector,unsigned additional) noexcept;
    // Serial-validated EHANDLE resolution; null is a stale handle.
    int (__cdecl *resolve)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Original virtual +338 (different for scripted and instanced scenes).
    // Choreography actor word is from parsed+14; host extracts its name.
    int (__cdecl *lookup)(void*,void* scene,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *handle)(void*,void*,std::uint32_t*) noexcept;
    int (__cdecl *invalid_index)(void*,void* scene,int,unsigned) noexcept;
};
struct SceneActorCacheApi {
    std::uint32_t version,size;
    int (__cdecl *index)(void*,unsigned,int,const SceneActorCacheBindings*,void**,unsigned*) noexcept;
    int (__cdecl *actor)(void*,unsigned,std::uint32_t,const SceneActorCacheBindings*,void**,unsigned*) noexcept;
};
using GetSceneActorCacheApi=const SceneActorCacheApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char scene_actor_names_export[]="SiN_GetSceneActorNamesApi";
struct SceneActorNameBindings {
    void* context;
    int (__cdecl *resolve)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *handle)(void*,void*,std::uint32_t*) noexcept;
    int (__cdecl *text)(void*,std::uint32_t,const char**) noexcept;
    int (__cdecl *max_clients)(void*,int*) noexcept;
    int (__cdecl *local_player)(void*,void**,unsigned*) noexcept;
    // 221619e0 entity-list FindEntityByName; previous continues wildcard search.
    int (__cdecl *find)(void*,const char*,void* previous,void* search_context,void**,unsigned*) noexcept;
    // Original virtual +f4 returns NPC eligibility in AL.
    int (__cdecl *is_npc)(void*,void*,unsigned*) noexcept;
    // Original RTTI cast to CAI_BaseActor; may return null.
    int (__cdecl *cast_actor)(void*,void*,void**,unsigned*) noexcept;
    int (__cdecl *npc_lookup)(void*,void* npc,const char*,void**,unsigned*) noexcept;
};
struct SceneActorNamesApi {
    std::uint32_t version,size;
    // 221a40d0; resolve player aliases, target aliases and contextual names.
    int (__cdecl *entity)(void*,unsigned,const char*,void* search_context,unsigned require_actor,const SceneActorNameBindings*,void**,unsigned*) noexcept;
    // 221a2c50 / 221a5870; instanced scenes prefer the NPC at +55c.
    int (__cdecl *actor)(void*,unsigned,const char*,unsigned instanced,const SceneActorNameBindings*,void**,unsigned*) noexcept;
};
using GetSceneActorNamesApi=const SceneActorNamesApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char resolved_scene_actor_export[]="SiN_GetResolvedSceneActorApi";
struct ResolvedSceneActorBindings {
    SceneActorCacheBindings cache;
    SceneActorNameBindings names;
    unsigned instanced;
    // Resolve parsed CChoreoActor pointer word to its actual object.
    int (__cdecl *choreo_object)(void*,std::uint32_t,const void**,unsigned*) noexcept;
    void* context;
};
struct ResolvedSceneActorApi {
    std::uint32_t version,size;
    int (__cdecl *index)(void*,unsigned,int,const ResolvedSceneActorBindings*,void**,unsigned*) noexcept;
    int (__cdecl *actor)(void*,unsigned,std::uint32_t,const ResolvedSceneActorBindings*,void**,unsigned*) noexcept;
    // Phase821 callback but actor resolution runs native cache + name path.
    // Legacy SceneSpeechCallbackBindings.actor is unused.
    int (__cdecl *speech)(const void*,unsigned,void*,unsigned,unsigned,float,void*,unsigned,void*,unsigned,
        const SceneSpeechCallbackBindings*,const ResolvedSceneActorBindings*,unsigned*) noexcept;
};
using GetResolvedSceneActorApi=const ResolvedSceneActorApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_speech_preparation_export[]="SiN_GetSceneSpeechPreparationApi";
struct SceneSpeechPreparationBindings {
    void* context;
    SpeechSemaphorePair* state; // same server-session pair as response expeditor
    unsigned instanced;
    int (__cdecl *cast_base_actor)(void*,void*,void**) noexcept;
    int (__cdecl *actor_notify)(void*,void* base_actor,float remaining,float margin) noexcept;
    int (__cdecl *is_npc)(void*,void*,unsigned*) noexcept;
    int (__cdecl *classify)(void*,void*,int,unsigned*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *handle)(void*,void*,std::uint32_t*) noexcept;
};
struct SceneSpeechPreparationApi {
    std::uint32_t version,size;
    // 221ac495..221ac534; 220a3d00 semaphore acquisition.
    int (__cdecl *prepare)(void*,unsigned,void* actor,float remaining,const SceneSpeechPreparationBindings*) noexcept;
    // Native speech path with original actor preparation, no hosted prepare_actor.
    int (__cdecl *start)(void*,unsigned,void*,unsigned,std::uint32_t,void*,unsigned,int,
        const SceneSpeechExecutionBindings*,const SceneSpeechPreparationBindings*) noexcept;
    // Native type5 callback, optional original cached resolver.
    int (__cdecl *callback)(const void*,unsigned,void*,unsigned,unsigned,float,void*,unsigned,void*,unsigned,
        const SceneSpeechCallbackBindings*,const ResolvedSceneActorBindings*,const SceneSpeechPreparationBindings*,unsigned*) noexcept;
};
using GetSceneSpeechPreparationApi=const SceneSpeechPreparationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char scene_speech_actor_note_export[]="SiN_GetSceneSpeechActorNoteApi";
struct SceneSpeechActorNoteBindings {
    SceneSpeechPreparationBindings preparation; // same state shared with native response note
    void* context;
    int (__cdecl *extent)(void*,void* base_actor,unsigned*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t word,void**,unsigned*) noexcept;
    ResponseNoteBindings note;
};
struct SceneSpeechActorNoteApi {
    std::uint32_t version,size;
    // 2216e430 -> 22028d30 -> 220a4590. Requires original base actor layout.
    int (__cdecl *notify)(const SceneSpeechActorNoteBindings*,void* actor,float remaining,float margin) noexcept;
    // Adapt to existing choreography preparation ABI without changing its size.
    // Keep cfg alive for the entire synchronous use of returned callbacks.
    int (__cdecl *adapt)(const SceneSpeechActorNoteBindings*,SceneSpeechPreparationBindings*) noexcept;
};
using GetSceneSpeechActorNoteApi=const SceneSpeechActorNoteApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char scene_speech_actor_sink_export[]="SiN_GetSceneSpeechActorSinkApi";
enum class SceneSpeechActorKind:unsigned {BaseActor,Jessica,JC,Count,NpcGenericActor,GenericActor};
struct SceneSpeechActorSinkBindings {
    void* context;
    // Resolve original expeditor+4 owner and identify its authenticated class.
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *kind)(void*,void* actor,SceneSpeechActorKind*) noexcept;
    ResponseHistoryBindings history;
    ResponseNoteBindings transport; // clock, diagnostic, classify, owner_handle only
};
struct SceneSpeechActorSinkApi {
    std::uint32_t version,size;
    // Sink +4: Jessica seeds TLK_HELLO once; other verified classes no-op.
    // Sink +8: JC/Count/NPC generic use inverted actor+0xb6 bit0.
    // The caller holds cfg through synchronous use of adapted callbacks.
    int (__cdecl *adapt)(const SceneSpeechActorSinkBindings*,ResponseNoteBindings*) noexcept;
};
using GetSceneSpeechActorSinkApi=const SceneSpeechActorSinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_choreography_clock_export[]="SiN_GetNativeChoreographyClockApi";
// Runtime clock over the original parsed PE32 graph. The graph owns vector
// storage; reset/simulation/resume use native graph operations. Engine speech
// effects and not-yet-recovered event-type callbacks remain explicit leaves.
struct NativeChoreographyClockBindings {
    ChoreographyVectorBindings vectors;
    int (__cdecl *release_memory)(void*,std::uint32_t) noexcept;
    void* context;
    void* scene;
    unsigned scene_bytes;
    const SceneSpeechCallbackBindings* speech;
    const ResolvedSceneActorBindings* actors; // optional native name/cache path
    const SceneSpeechPreparationBindings* preparation; // optional native prep
    int (__cdecl *other_event)(void*,std::uint32_t,unsigned,float,void*,void*) noexcept;
};
struct NativeChoreographyClockApi {
    std::uint32_t version,size;
    int (__cdecl *reset)(void*,unsigned,unsigned,std::uint32_t,std::uint32_t,const NativeChoreographyClockBindings*) noexcept;
    int (__cdecl *simulate)(void*,unsigned,float,const NativeChoreographyClockBindings*) noexcept;
    int (__cdecl *resume)(void*,unsigned,const NativeChoreographyClockBindings*) noexcept;
};
using GetNativeChoreographyClockApi=const NativeChoreographyClockApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_choreography_dispatch_export[]="SiN_GetNativeChoreographyDispatchApi";
enum class ChoreographyDispatchTrace:unsigned {Ignored,MissingActor,Start,Stop,MissingTarget};
// Original scene callback switch 221a2270/221a2830/221a2b60/221a2ba0.
// Engine virtual leaves receive their exact recovered byte offset, with
// normalized arguments. Native clock handles nested reset/simulation.
struct NativeChoreographyDispatchBindings {
    void* context;
    void* scene;
    unsigned scene_bytes;
    const NativeChoreographyClockBindings* clock;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *actor)(void*,void*,std::uint32_t,void**,unsigned*) noexcept;
    const ResolvedSceneActorBindings* actors; // optional native cache/name resolver
    int (__cdecl *target)(void*,void*,const char*,void* actor,unsigned,void**) noexcept;
    int (__cdecl *virtual_event)(void*,void* scene,unsigned virtual_offset,void* parsed,void* event,void* actor,void* target,const char* param) noexcept;
    int (__cdecl *trace)(void*,void*,void*,float,ChoreographyDispatchTrace) noexcept;
    int (__cdecl *output)(void*,void*,void* activator,void* caller,float) noexcept;
    int (__cdecl *debug_text)(void*,const char*) noexcept;
    const ActorEventRecordBindings* records;
};
struct NativeChoreographyDispatchApi {
    std::uint32_t version,size;
    int (__cdecl *invoke)(std::uint32_t,unsigned,float,void*,void*,const NativeChoreographyDispatchBindings*,unsigned*) noexcept;
    // Compatible with NativeChoreographyClockBindings.other_event; context
    // points to a live NativeChoreographyDispatchBindings. Type5 uses the
    // clock's separate speech route rather than this specialized switch.
    int (__cdecl *other_event)(void*,std::uint32_t,unsigned,float,void*,void*) noexcept;
};
using GetNativeChoreographyDispatchApi=const NativeChoreographyDispatchApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_choreography_scene_leaf_export[]="SiN_GetNativeChoreographySceneLeafApi";
// Original scripted/instanced scene vtables 2242dd3c/2242e89c.
// The normalized invoke entry is compatible with dispatch.virtual_event;
// its context points to this binding (or a host adapter).
struct NativeChoreographySceneLeafBindings {
    void* context;
    unsigned scene_bytes;
    unsigned instanced;
    const ActorEventRecordBindings* records;
    const NativeChoreographyClockBindings* clock;
    int (__cdecl *actor_extent)(void*,void*,unsigned*) noexcept;
    // Original instanced movement/face capability gate 221a7430.
    int (__cdecl *admit_instanced)(void*,void* actor,unsigned*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *load_subscene)(void*,void* scene,const char*,std::uint32_t*) noexcept;
    int (__cdecl *now)(void*,float*) noexcept;
    int (__cdecl *output)(void*,void*,void*,void*,float) noexcept;
    // Scripted section/pause policy remains an explicit engine leaf.
    int (__cdecl *section)(void*,void*,void*,void*,const char*) noexcept;
};
struct NativeChoreographySceneLeafApi {
    std::uint32_t version,size;
    int (__cdecl *invoke)(void*,void*,unsigned,void*,void*,void*,void*,const char*) noexcept;
};
using GetNativeChoreographySceneLeafApi=const NativeChoreographySceneLeafApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_choreography_section_export[]="SiN_GetNativeChoreographySectionApi";
struct NativeChoreographySectionBindings {
    void* context;
    unsigned scene_bytes;
    const NativeChoreographyClockBindings* clock;
    const NativeChoreographyDispatchBindings* dispatch;
    int (__cdecl *clock_now)(void*,float*) noexcept;
    int (__cdecl *last_think)(void*,void*,int,float*) noexcept;
    int (__cdecl *output)(void*,void*,void*,void*,float) noexcept;
    int (__cdecl *finish)(void*,void*,int,int) noexcept;
    // Original debug warning on timeout only. Action 1 cancel, 2 resume,
    // otherwise unknown. No warning on ordinary condition completion.
    int (__cdecl *timeout)(void*,void*,int) noexcept;
};
struct NativeChoreographySectionApi {
    std::uint32_t version,size;
    // Adapter for scene leaf section callback; context = bindings.
    int (__cdecl *enter)(void*,void*,void*,void*,const char*) noexcept;
    // Adapter for scene advance paused_tick; context = bindings.
    int (__cdecl *tick)(void*,void*) noexcept;
    // Original 22113dc0 tracked condition scan, boolean in out.
    int (__cdecl *ready)(void*,unsigned,const NativeChoreographySectionBindings*,unsigned*) noexcept;
};
using GetNativeChoreographySectionApi=const NativeChoreographySectionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_scene_movement_gate_export[]="SiN_GetNativeSceneMovementGateApi";
struct NativeSceneMovementGateBindings {
    void* context;
    void* scene;
    int (__cdecl *is_npc)(void*,void*,unsigned*) noexcept;
    int (__cdecl *actor_extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Original NPC virtual +760 returns class schedule-ID space; the
    // first local-to-global range record is inline at result+4.
    int (__cdecl *schedule_space)(void*,void*,void**,unsigned*) noexcept;
    int (__cdecl *suppressed)(void*,void* scene,void* actor) noexcept;
};
struct NativeSceneMovementGateApi {
    std::uint32_t version,size;
    // Original 22073700; returns -1 for absent mapping, transport
    // failure for malformed/unresolvable range chain.
    int (__cdecl *map_schedule)(void*,unsigned,int,const NativeSceneMovementGateBindings*,int*) noexcept;
    // Original 221a7430; compatible with scene-leaf admit_instanced.
    int (__cdecl *admit)(void*,void*,unsigned*) noexcept;
};
using GetNativeSceneMovementGateApi=const NativeSceneMovementGateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_scene_execution_export[]="SiN_GetNativeSceneExecutionApi";
// Composed server execution boundary. Each template retains its own host
// context; native operations replace graph dispatch, virtual scene leaves,
// section parsing/paused ticks and optionally AI admission. No adapter is
// stored in an entity or parsed scene. All callbacks are synchronous.
struct NativeSceneExecutionBindings {
    unsigned instanced;
    NativeChoreographyClockBindings clock;
    NativeChoreographyDispatchBindings dispatch;
    NativeChoreographySceneLeafBindings leaves;
    NativeChoreographySectionBindings section;
    const NativeSceneMovementGateBindings* gate;
    SceneAdvanceBindings advance;
    const SceneFinishBindings* finish; // optional native finish, with graph reset replaced
};
struct NativeSceneExecutionApi {
    std::uint32_t version,size;
    int (__cdecl *reset)(void*,unsigned,unsigned,std::uint32_t,std::uint32_t,const NativeSceneExecutionBindings*) noexcept;
    int (__cdecl *simulate)(void*,unsigned,float,const NativeSceneExecutionBindings*) noexcept;
    int (__cdecl *resume)(void*,unsigned,const NativeSceneExecutionBindings*) noexcept;
    int (__cdecl *advance)(void*,unsigned,float,const NativeSceneExecutionBindings*) noexcept;
};
using GetNativeSceneExecutionApi=const NativeSceneExecutionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char scene_speech_drain_export[]="SiN_GetSceneSpeechDrainApi";
struct SceneSpeechDrainBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *resolve)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *open_filter)(void*,void* actor,float scale,void**) noexcept;
    // Aggregate recipient filter, unlike the ordinary per-client route.
    int (__cdecl *emit)(void*,void* filter,void* actor,const char*,int channel,float volume,int level,float absolute_start) noexcept;
    int (__cdecl *close_filter)(void*,void*) noexcept;
};
struct SceneSpeechDrainApi {
    std::uint32_t version,size;
    // 221aa010, consumes the original pending vector count on success.
    // Failure can follow a partial emission; caller must not blindly retry.
    int (__cdecl *drain)(void*,unsigned,const SceneSpeechDrainBindings*,unsigned* emitted) noexcept;
    // 221aa970 wrapper, uses same session as scene creation/registration.
    int (__cdecl *flush)(SceneManagerSession*,const SceneManagerRuntimeBindings*,const SceneSpeechDrainBindings*,unsigned*) noexcept;
};
using GetSceneSpeechDrainApi=const SceneSpeechDrainApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_generated_response_export[]="SiN_GetNativeGeneratedResponseApi";
struct NativeGeneratedResponseBindings {
    ResponseChoreographyBindings response;
    NativeGeneratedLifecycleBindings generated;
};
struct NativeGeneratedResponseApi {
    std::uint32_t version,size;
    // Non-raw Speak uses native generated choreography and native startup.
    // Named Scene uses existing named scene playback bindings. Interruption
    // and activation must refer to the same scene-manager session.
    int (__cdecl *dispatch)(void*,unsigned,const ResponseDispatchBindings*,const NativeGeneratedResponseBindings*,const char*,void*,unsigned,unsigned*) noexcept;
    int (__cdecl *speak)(void*,unsigned,const ResponseSpeakBindings*,const NativeGeneratedResponseBindings*,const char*,const char*,unsigned*) noexcept;
};
using GetNativeGeneratedResponseApi=const NativeGeneratedResponseApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_destruction_export[]="SiN_GetChoreographyDestructionApi";
enum class ChoreographyRelease:unsigned {Vector,Actor,Event,Channel,Parsed};
struct ChoreographyDestructionBindings {
    void* context;
    // Resolve a live original PE32 word; reject previously freed allocations.
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Consume on invocation. Caller must not retry after partial failure.
    int (__cdecl *release)(void*,std::uint32_t,ChoreographyRelease) noexcept;
    int (__cdecl *symbol_destroy)(void*,void*) noexcept;
    // Original auxiliary virtual scalar deleting destructor (argument 1).
    int (__cdecl *delete_aux)(void*,std::uint32_t,unsigned) noexcept;
};
struct ChoreographyDestructionApi {
    std::uint32_t version,size;
    // 22116310 + 2226e800: destroy children, vectors and root.
    // Storage must be uniquely owned (except explicitly borrowed vectors).
    int (__cdecl *parsed)(void*,unsigned,const ChoreographyDestructionBindings*) noexcept;
    // 2210ff10 + 2226e800: delete a standalone event including nested scene.
    int (__cdecl *event)(void*,unsigned,const ChoreographyDestructionBindings*) noexcept;
};
using GetChoreographyDestructionApi=const ChoreographyDestructionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_scene_retirement_export[]="SiN_GetNativeSceneRetirementApi";
struct NativeSceneRetirementApi {
    std::uint32_t version,size;
    // Native stop/detach/destruction, without lifecycle.destroy_parsed.
    // Caller owns the scene entity until base removal and unregister finish.
    int (__cdecl *cleanup)(void*,unsigned,const SceneEntityLifecycleBindings*,const SceneStopBindings*,const ChoreographyDestructionBindings*) noexcept;
    int (__cdecl *remove)(void*,unsigned,const SceneEntityLifecycleBindings*,const SceneStopBindings*,const ChoreographyDestructionBindings*) noexcept;
};
using GetNativeSceneRetirementApi=const NativeSceneRetirementApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_symbol_export[]="SiN_GetChoreographySymbolApi";
// Original process-wide 2259012c..22590137 sentinel state.
struct ChoreographySymbolSentinel {std::uint16_t words[4];std::uint32_t flags;};
enum class ChoreographySymbolRelease:unsigned {String,StringVector,Tree};
struct ChoreographySymbolBindings {
    void* context;
    ChoreographySymbolSentinel* sentinel;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Original allocator receives even a null string word. Consumes on invocation.
    int (__cdecl *release)(void*,std::uint32_t,ChoreographySymbolRelease) noexcept;
};
struct ChoreographySymbolApi {
    std::uint32_t version,size;
    // 2228f360/2228eab0 with initial allocation zero, as in parsed scene construction.
    // Comparator is the host-compatible original case-sensitive/insensitive word.
    int (__cdecl *construct_empty)(void*,unsigned,int grow,std::uint32_t comparator) noexcept;
    // 2228f660/2228ec50: rebuild free list then release strings, vector and tree.
    int (__cdecl *destroy)(void*,unsigned,const ChoreographySymbolBindings*) noexcept;
};
using GetChoreographySymbolApi=const ChoreographySymbolApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_symbol_intern_export[]="SiN_GetChoreographySymbolInternApi";
struct ChoreographySymbolInternBindings {
    void* context;
    ChoreographySymbolSentinel* sentinel;
    unsigned insensitive; // original case-insensitive C-locale comparator
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *grow_memory)(void*,void*,unsigned,unsigned) noexcept;
    int (__cdecl *allocate)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *release_unpublished)(void*,std::uint32_t) noexcept;
};
struct ChoreographySymbolInternApi {
    std::uint32_t version,size;
    // Compatible with ChoreographyScaleBindings.intern. Context points at
    // a live ChoreographySymbolInternBindings; boolean success.
    int (__cdecl *intern)(void*,void*,const char*,std::uint16_t*) noexcept;
};
using GetChoreographySymbolInternApi=const ChoreographySymbolInternApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char owned_choreography_export[]="SiN_GetOwnedChoreographyApi";
struct OwnedChoreographyBindings {
    ChoreographyDestructionBindings destruction;
    ChoreographySymbolBindings symbols;
    std::uint32_t insensitive_comparator;
};
struct OwnedChoreographyApi {
    std::uint32_t version,size;
    // Native symbol construction and failure cleanup, legacy callbacks unused.
    int (__cdecl *generate)(void*,unsigned,void*,const char*,const GeneratedSceneBindings*,const GeneratedSceneAllocationBindings*,const OwnedChoreographyBindings*,std::uint32_t*) noexcept;
    int (__cdecl *sound)(void*,const char*,const NativeGeneratedLifecycleBindings*,const OwnedChoreographyBindings*,float*) noexcept;
    int (__cdecl *remove)(void*,unsigned,const SceneEntityLifecycleBindings*,const SceneStopBindings*,const OwnedChoreographyBindings*) noexcept;
};
using GetOwnedChoreographyApi=const OwnedChoreographyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char named_scene_load_export[]="SiN_GetNamedSceneLoadApi";
struct NamedSceneCacheBindings {
    void* context;
    // Original 221ad090 prefetch, then symbol/tree lookup. FFFFFFFF is absent.
    int (__cdecl *ensure)(void*,const char*) noexcept;
    int (__cdecl *find)(void*,const char*,std::uint32_t*) noexcept;
    // Null data with length zero means evicted; nonnull with zero is empty.
    // A nonnull data result holds a lock until unlock, even for empty data.
    int (__cdecl *lock)(void*,std::uint32_t,const void**,unsigned*) noexcept;
    int (__cdecl *reload)(void*,const char*,std::uint32_t,std::uint32_t*) noexcept;
    int (__cdecl *unlock)(void*,std::uint32_t) noexcept;
};
struct NamedSceneLoadBindings {
    void* context;
    NamedSceneCacheBindings cache;
    // Exact original engine Q_SetExtension import; called on a bounded buffer.
    int (__cdecl *set_extension)(void*,char*,const char*,unsigned) noexcept;
    int (__cdecl *allocate)(void*,unsigned,void**,unsigned*) noexcept;
    // Raw copy/unpublished root allocation release, consumes on invocation.
    int (__cdecl *release)(void*,void*) noexcept;
    int (__cdecl *missing)(void*,const char*) noexcept;
    // Original 22119900 parser body, host binds token cursor to NUL-terminated
    // private data. Native constructs and owns parsed object before call.
    int (__cdecl *parse)(void*,void*,const char* path,const char* data,unsigned bytes) noexcept;
    std::uint32_t compare_word,diagnostic_word;
    OwnedChoreographyBindings owned;
};
struct NamedSceneLoadApi {
    std::uint32_t version,size;
    // 221ad670 + 221ad1b0 + 22119b20. Missing/empty returns 1 and zero word.
    int (__cdecl *load)(void* scene,unsigned bytes,const char*,const NamedSceneLoadBindings*,std::uint32_t*) noexcept;
};
using GetNamedSceneLoadApi=const NamedSceneLoadApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_text_parser_export[]="SiN_GetChoreographyTextParserApi";
enum class ChoreographyTextBlock:unsigned {Event,Actor,SceneRamp,ScaleSettings};
// Historical enum labels CaptionGroups/GestureTimes were reversed in the
// Phase837 evidence note. Keep numeric ABI, expose corrected aliases.
enum class ChoreographyTextFinalize:unsigned {RelativeTags=0,CaptionGroups=1,GestureTimes=2,GestureReconcile=1,CaptionReconcile=2};
struct ChoreographyTextParserBindings {
    void* context;
    std::uint32_t tokenizer_word;
    // Bind the private scene text to the original-compatible tokenizer.
    int (__cdecl *begin)(void*,const char*,unsigned) noexcept;
    // Advance with original argument 1; report normal EOF separately.
    int (__cdecl *next)(void*,unsigned,unsigned*) noexcept;
    int (__cdecl *token)(void*,const char**) noexcept;
    // Each block consumes its own nested tokens, leaving the cursor at end.
    int (__cdecl *block)(void*,void*,ChoreographyTextBlock) noexcept;
    int (__cdecl *unexpected)(void*,const char*) noexcept;
    int (__cdecl *finalize)(void*,void*,ChoreographyTextFinalize) noexcept;
};
struct ChoreographyTextParserApi {
    std::uint32_t version,size;
    // Original 22119900: caller has already initialized tokenizer cursor.
    int (__cdecl *parse)(void*,unsigned,const char*,const ChoreographyTextParserBindings*) noexcept;
    // Native load + top-level parse, old NamedSceneLoadBindings.parse unused.
    int (__cdecl *load)(void*,unsigned,const char*,const NamedSceneLoadBindings*,const ChoreographyTextParserBindings*,std::uint32_t*) noexcept;
};
using GetChoreographyTextParserApi=const ChoreographyTextParserApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char named_scene_precache_export[]="SiN_GetNamedScenePrecacheApi";
struct NamedScenePrecacheBindings {
    void* context;
    NamedSceneLoadBindings load;
    // Optional native top-level parser; otherwise the existing parser body.
    const ChoreographyTextParserBindings* parser;
    int (__cdecl *text)(void*,std::uint32_t,const char**) noexcept;
    int (__cdecl *precache_sound)(void*,const char*) noexcept;
};
struct NamedScenePrecacheApi {
    std::uint32_t version,size;
    // 221ad950: process type 5 sound/caption precache and type 11 subscenes.
    int (__cdecl *events)(void*,unsigned,void*,unsigned,const NamedScenePrecacheBindings*) noexcept;
    // 221ada50: generated scene no-op; otherwise cleanup, load, precache.
    int (__cdecl *spawn)(void*,unsigned,const SceneEntityLifecycleBindings*,const SceneStopBindings*,const NamedScenePrecacheBindings*) noexcept;
};
using GetNamedScenePrecacheApi=const NamedScenePrecacheApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_actor_parser_export[]="SiN_GetChoreographyActorParserApi";
struct ChoreographyActorParserBindings {
    ChoreographyTextParserBindings text;
    GeneratedSceneAllocationBindings allocation;
    void* context;
    // Append channel to actor, after the entire channel body is parsed.
    int (__cdecl *append_channel)(void*,void* actor,void* channel) noexcept;
    // 22116dc0 event parser, with current token "event".
    int (__cdecl *event)(void*,void* parsed,void* actor,void* channel) noexcept;
};
struct ChoreographyActorParserApi {
    std::uint32_t version,size;
    // Original 22118660 / 221179c0. Published partial objects remain
    // owned by parsed scene on any failure, never retry a partial block.
    int (__cdecl *actor)(void*,unsigned,const ChoreographyActorParserBindings*) noexcept;
    int (__cdecl *parse)(void*,unsigned,const char*,const ChoreographyActorParserBindings*) noexcept;
    int (__cdecl *load)(void*,unsigned,const char*,const NamedSceneLoadBindings*,const ChoreographyActorParserBindings*,std::uint32_t*) noexcept;
};
using GetChoreographyActorParserApi=const ChoreographyActorParserApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_event_parser_export[]="SiN_GetChoreographyEventParserApi";
enum class ChoreographyEventBlock:unsigned {Tags,AbsoluteTags,FlexTimingTags,FlexAnimations,EventRamp};
struct ChoreographyEventParserBindings {
    ChoreographyActorParserBindings actors;
    void* context;
    // Original tokenizer virtual +8: more tokens on current line.
    int (__cdecl *same_line)(void*,unsigned*) noexcept;
    // Original CRT float conversion (223a5c8b) and timing-tag refresh.
    int (__cdecl *number)(void*,const char*,float*) noexcept;
    // Optional in native flex/finalized grammar: native 221105a0 refresh.
    int (__cdecl *end_changed)(void*,void*) noexcept;
    int (__cdecl *append_event)(void*,void* channel,void* event) noexcept;
    // Complex subgrammar leaves consume their nested tokens.
    int (__cdecl *block)(void*,void*,ChoreographyEventBlock) noexcept;
    // Original legacy ramp conversion (22110770/221107b0) after linking.
    // Optional in native curve/flex/finalized grammar.
    int (__cdecl *legacy_ramp)(void*,void*,float,float,float) noexcept;
};
struct ChoreographyEventParserApi {
    std::uint32_t version,size;
    int (__cdecl *event)(void*,unsigned,void* actor,void* channel,const ChoreographyEventParserBindings*) noexcept;
    int (__cdecl *parse)(void*,unsigned,const char*,const ChoreographyEventParserBindings*) noexcept;
    int (__cdecl *load)(void*,unsigned,const char*,const NamedSceneLoadBindings*,const ChoreographyEventParserBindings*,std::uint32_t*) noexcept;
};
using GetChoreographyEventParserApi=const ChoreographyEventParserApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_named_playback_export[]="SiN_GetNativeNamedPlaybackApi";
using NamedSpawnCallback=int (__cdecl *)(void*,void*) noexcept;
struct NativeNamedPlaybackBindings {
    NpcScenePlaybackBindings playback;
    SceneEntityLifecycleBindings lifecycle;
    SceneStartBindings startup;
    SceneStopBindings stop;
    NamedScenePrecacheBindings precache;
    const ChoreographyEventParserBindings* events;
    void* context;
    // Original 22287d80: invoke spawn synchronously exactly once, inside
    // engine registration's lock. Do not retain the callback/context.
    // This replaces register_scene, not an additional Spawn invocation.
    int (__cdecl *register_with_spawn)(void*,void*,void*,NamedSpawnCallback) noexcept;
};
struct NativeNamedPlaybackApi {
    std::uint32_t version,size;
    int (__cdecl *scene)(void*,const char*,float,const NativeNamedPlaybackBindings*,float*) noexcept;
};
using GetNativeNamedPlaybackApi=const NativeNamedPlaybackApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_timing_tags_export[]="SiN_GetChoreographyTimingTagsApi";
enum class ChoreographyTagKind:unsigned {Relative,FlexTiming,Playback,Shifted};
struct ChoreographyTimingTagBindings {
    void* context;
    // Same tokenizer and CRT numeric conversion as the enclosing event.
    ChoreographyTextParserBindings text;
    int (__cdecl *number)(void*,const char*,float*) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Grow backing capacity only, leaving logical count unchanged.
    int (__cdecl *grow)(void*,void* vector,unsigned additional,unsigned stride) noexcept;
};
struct ChoreographyTimingTagsApi {
    std::uint32_t version,size;
    // Original 221103e0/22110440/221105e0 and vector append/sort.
    int (__cdecl *append)(void*,unsigned,ChoreographyTagKind,const char*,float,unsigned,const ChoreographyTimingTagBindings*) noexcept;
    // Called at current tags/absolutetags/flextimingtags keyword.
    int (__cdecl *block)(void*,unsigned,ChoreographyEventBlock,const ChoreographyTimingTagBindings*) noexcept;
};
using GetChoreographyTimingTagsApi=const ChoreographyTimingTagsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char portable_vcd_tokenizer_export[]="SiN_GetPortableVcdTokenizerApi";
struct PortableVcdTokenizerApi {
    std::uint32_t version,size;
    // Callback signatures match parser leaves. Context is a live, initialized
    // sin_port::vcd::Tokenizer, borrowed input valid through final parse.
    int (__cdecl *begin)(void*,const char*,unsigned) noexcept;
    int (__cdecl *next)(void*,unsigned,unsigned*) noexcept;
    int (__cdecl *token)(void*,const char**) noexcept;
    int (__cdecl *same_line)(void*,unsigned*) noexcept;
    int (__cdecl *clear)(void*) noexcept;
};
using GetPortableVcdTokenizerApi=const PortableVcdTokenizerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char portable_vcd_grammar_export[]="SiN_GetPortableVcdGrammarApi";
struct PortableVcdGrammarBindings {
    // Old tokenizer, event block, root block and same_line callbacks are
    // unused. tokenizer_word still represents stable host parser identity.
    ChoreographyEventParserBindings events;
    ChoreographyTimingTagBindings tags;
    void* context;
    // Remaining complex blocks must consume the shared native token cursor.
    // Do not retain it or recursively enter the parser.
    int (__cdecl *root_block)(void*,void*,ChoreographyTextBlock,sin_port::vcd::Tokenizer*) noexcept;
    int (__cdecl *event_block)(void*,void*,ChoreographyEventBlock,sin_port::vcd::Tokenizer*) noexcept;
};
struct PortableVcdGrammarApi {
    std::uint32_t version,size;
    int (__cdecl *parse)(void*,unsigned,const char* filename,const char* data,unsigned bytes,const PortableVcdGrammarBindings*) noexcept;
    int (__cdecl *load)(void*,unsigned,const char*,const NamedSceneLoadBindings*,const PortableVcdGrammarBindings*,std::uint32_t*) noexcept;
    // Composes the native named playback bridge and portable grammar for
    // root and nested scene files, without hosted tokenization.
    int (__cdecl *scene)(void*,const char*,float,const NativeNamedPlaybackBindings*,const PortableVcdGrammarBindings*,float*) noexcept;
};
using GetPortableVcdGrammarApi=const PortableVcdGrammarApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_ramp_export[]="SiN_GetChoreographyRampApi";
enum class ChoreographyRampKind:unsigned {Event,Scene};
struct ChoreographyRampApi {
    std::uint32_t version,size;
    // Original 22116710/22118470: clear count, parse samples, sort and trim.
    // Uses the same tokenizer and 12-byte vector storage as timing tags.
    int (__cdecl *block)(void*,unsigned,ChoreographyRampKind,const ChoreographyTimingTagBindings*) noexcept;
    // Original 22110770: append value/time/selected byte to event or scene.
    int (__cdecl *append)(void*,unsigned,ChoreographyRampKind,float time,float value,unsigned selected,const ChoreographyTimingTagBindings*) noexcept;
    int (__cdecl *finish)(void*,unsigned,ChoreographyRampKind,const ChoreographyTimingTagBindings*) noexcept;
};
using GetChoreographyRampApi=const ChoreographyRampApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_scale_export[]="SiN_GetChoreographyScaleApi";
// Original process-global 225558b8..225558cb sentinel, distinct from symbols.
struct ChoreographyScaleSentinel {std::uint32_t left,right,parent,color,flags;};
struct ChoreographyScaleBindings {
    void* context;
    ChoreographyTextParserBindings text;
    ChoreographyScaleSentinel* sentinel;
    int (__cdecl *integer)(void*,const char*,int*) noexcept; // imported Q_atoi
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Original symbol intern (2228f7a0) for parsed+164.
    int (__cdecl *intern)(void*,void*,const char*,std::uint16_t*) noexcept;
    // Original tree insertion/rebalance for parsed+140. Return live index.
    int (__cdecl *insert)(void*,void*,std::uint16_t,std::uint32_t*) noexcept;
};
struct ChoreographyScaleApi {
    std::uint32_t version,size;
    int (__cdecl *find)(void*,unsigned,std::uint16_t,const ChoreographyScaleBindings*,std::uint32_t*) noexcept;
    int (__cdecl *block)(void*,unsigned,const ChoreographyScaleBindings*) noexcept;
};
using GetChoreographyScaleApi=const ChoreographyScaleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_scale_tree_export[]="SiN_GetChoreographyScaleTreeApi";
struct ChoreographyScaleTreeBindings {
    void* context;
    ChoreographyScaleSentinel* sentinel;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // CUtlMemory at tree+4: update allocation and capacity only, retain
    // existing slots. Additional capacity, not logical element count.
    int (__cdecl *grow_memory)(void*,void*,unsigned,unsigned) noexcept;
};
struct ChoreographyScaleTreeApi {
    std::uint32_t version,size;
    // Callback-compatible with ChoreographyScaleBindings.insert; context
    // points at a live ChoreographyScaleTreeBindings. Boolean return.
    int (__cdecl *insert)(void*,void*,std::uint16_t,std::uint32_t*) noexcept;
};
using GetChoreographyScaleTreeApi=const ChoreographyScaleTreeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char portable_vcd_curve_grammar_export[]="SiN_GetPortableVcdCurveGrammarApi";
struct PortableVcdCurveGrammarBindings {
    PortableVcdGrammarBindings grammar;
    ChoreographyScaleBindings scales;
};
struct PortableVcdCurveGrammarApi {
    std::uint32_t version,size;
    // Native ramps, scale block/lookup, tags and scalar grammar. Only
    // flexanimations remains a hosted block. Root block callback unused.
    int (__cdecl *parse)(void*,unsigned,const char*,const char*,unsigned,const PortableVcdCurveGrammarBindings*) noexcept;
    int (__cdecl *load)(void*,unsigned,const char*,const NamedSceneLoadBindings*,const PortableVcdCurveGrammarBindings*,std::uint32_t*) noexcept;
    int (__cdecl *scene)(void*,const char*,float,const NativeNamedPlaybackBindings*,const PortableVcdCurveGrammarBindings*,float*) noexcept;
};
using GetPortableVcdCurveGrammarApi=const PortableVcdCurveGrammarApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_flex_export[]="SiN_GetChoreographyFlexApi";
struct ChoreographyFlexBindings {
    ChoreographyTimingTagBindings samples;
    void* context;
    // 22110ff0: allocate, construct, own name and publish track in event+30c.
    // Returned extent >= 0x50; both sample vectors initially empty.
    int (__cdecl *create)(void*,void*,const char*,void**,unsigned*) noexcept;
    // 2210e040: consumes old track via original virtual deleting destructor.
    int (__cdecl *delete_aux)(void*,std::uint32_t,unsigned) noexcept;
};
struct ChoreographyFlexApi {
    std::uint32_t version,size;
    int (__cdecl *block)(void*,unsigned,const ChoreographyFlexBindings*) noexcept;
};
using GetChoreographyFlexApi=const ChoreographyFlexApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_flex_ownership_export[]="SiN_GetChoreographyFlexOwnershipApi";
enum class ChoreographyFlexAllocation:unsigned {Track,Name,SampleVector};
struct ChoreographyFlexOwnershipBindings {
    void* context;
    // Relocated original vtable or compatible native deleting-destructor table.
    std::uint32_t vtable;
    int (__cdecl *allocate)(void*,unsigned,ChoreographyFlexAllocation,void**,unsigned*) noexcept;
    // Consumes live allocation on invocation, including failure. Name/Track
    // use the original object allocator; SampleVector uses vector allocator.
    int (__cdecl *release)(void*,std::uint32_t,ChoreographyFlexAllocation) noexcept;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *grow)(void*,void*,unsigned,unsigned) noexcept;
};
struct ChoreographyFlexOwnershipApi {
    std::uint32_t version,size;
    int (__cdecl *construct)(void*,unsigned,void*,std::uint32_t) noexcept;
    // Directly compatible with ChoreographyFlexBindings.create and
    // delete_aux. Context is a live ChoreographyFlexOwnershipBindings.
    int (__cdecl *create)(void*,void*,const char*,void**,unsigned*) noexcept;
    int (__cdecl *destroy)(void*,std::uint32_t,unsigned) noexcept;
};
using GetChoreographyFlexOwnershipApi=const ChoreographyFlexOwnershipApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char portable_vcd_flex_grammar_export[]="SiN_GetPortableVcdFlexGrammarApi";
struct PortableVcdFlexGrammarBindings {
    PortableVcdCurveGrammarBindings curves;
    ChoreographyFlexBindings flex;
};
struct PortableVcdFlexGrammarApi {
    std::uint32_t version,size;
    // All original root/event block dispatch native. Allocation, symbol
    // insertion, track creation and finalization services still hosted.
    int (__cdecl *parse)(void*,unsigned,const char*,const char*,unsigned,const PortableVcdFlexGrammarBindings*) noexcept;
    int (__cdecl *load)(void*,unsigned,const char*,const NamedSceneLoadBindings*,const PortableVcdFlexGrammarBindings*,std::uint32_t*) noexcept;
    int (__cdecl *scene)(void*,const char*,float,const NativeNamedPlaybackBindings*,const PortableVcdFlexGrammarBindings*,float*) noexcept;
};
using GetPortableVcdFlexGrammarApi=const PortableVcdFlexGrammarApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_finalize_export[]="SiN_GetChoreographyFinalizeApi";
struct ChoreographyFinalizeBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Original 221111b0 diagnostic for unresolved relative tag.
    int (__cdecl *missing_tag)(void*,void* parsed,const char* event_name,const char* reference,const char* tag) noexcept;
    // Optional legacy overrides. Null selects native 2210a330/2210aa90.
    // Existing ABI layout unchanged.
    int (__cdecl *gesture_channel)(void*,void*) noexcept;
    int (__cdecl *caption_channel)(void*,void*) noexcept;
};
struct ChoreographyFinalizeApi {
    std::uint32_t version,size;
    int (__cdecl *stage)(void*,unsigned,ChoreographyTextFinalize,const ChoreographyFinalizeBindings*) noexcept;
    int (__cdecl *all)(void*,unsigned,const ChoreographyFinalizeBindings*) noexcept;
};
using GetChoreographyFinalizeApi=const ChoreographyFinalizeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char portable_vcd_finalized_grammar_export[]="SiN_GetPortableVcdFinalizedGrammarApi";
struct PortableVcdFinalizedGrammarBindings {
    PortableVcdFlexGrammarBindings grammar;
    ChoreographyFinalizeBindings finalize;
};
struct PortableVcdFinalizedGrammarApi {
    std::uint32_t version,size;
    int (__cdecl *parse)(void*,unsigned,const char*,const char*,unsigned,const PortableVcdFinalizedGrammarBindings*) noexcept;
    int (__cdecl *load)(void*,unsigned,const char*,const NamedSceneLoadBindings*,const PortableVcdFinalizedGrammarBindings*,std::uint32_t*) noexcept;
    int (__cdecl *scene)(void*,const char*,float,const NativeNamedPlaybackBindings*,const PortableVcdFinalizedGrammarBindings*,float*) noexcept;
};
using GetPortableVcdFinalizedGrammarApi=const PortableVcdFinalizedGrammarApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char choreography_startup_sound_export[]="SiN_GetChoreographyStartupSoundApi";
// Original 221adaa0/221a9ba0 collect and warm speech names, not actor dispatch.
struct ChoreographyStartupSoundBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // Return fresh owned valid parsed child; collector publishes it to parent.
    int (__cdecl *load_subscene)(void*,void*,const char*,std::uint32_t*) noexcept;
    // Optional gated local-player/language selection (221a3df0).
    int (__cdecl *select_sound)(void*,void*,void*,char*,unsigned,unsigned*) noexcept;
    int (__cdecl *warm_sound)(void*,const char*) noexcept;
};
struct ChoreographyStartupSoundApi {
    std::uint32_t version,size;
    // Context points to bindings. Boolean status. Linked children remain
    // parent-owned on subsequent failure; no actor records are published.
    int (__cdecl *warm)(void*,void*,std::uint32_t) noexcept;
};
using GetChoreographyStartupSoundApi=const ChoreographyStartupSoundApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char native_vcd_runtime_export[]="SiN_GetNativeVcdRuntimeApi";
// Raw engine/allocator services only. Object and memory allocator families
// are intentionally separate (2226e790/e800 vs 2226e390/e400/e3e0).
// All callbacks synchronous, no references to temporary adapters retained.
// Keep sentinels, counter, allocator and relocated words valid for scene lifetime.
struct NativeVcdRuntimeBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *allocate_object)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *resize_memory)(void*,std::uint32_t,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *release_object)(void*,std::uint32_t) noexcept;
    int (__cdecl *release_memory)(void*,std::uint32_t) noexcept;
    int (__cdecl *number)(void*,const char*,float*) noexcept;
    int (__cdecl *integer)(void*,const char*,int*) noexcept;
    int (__cdecl *unexpected)(void*,const char*) noexcept;
    int (__cdecl *missing_tag)(void*,void*,const char*,const char*,const char*) noexcept;
    std::uint32_t* event_counter;
    ChoreographyScaleSentinel* scale_sentinel;
    ChoreographySymbolSentinel* symbol_sentinel;
    std::uint32_t scale_comparator,symbol_comparator,flex_vtable,tokenizer_word,diagnostic_word;
};
struct NativeVcdRuntimeApi {
    std::uint32_t version,size;
    // Allocate, construct, parse/finalize; return owned root word, or zero
    // on failure. Caller serializes operations sharing module sentinels/counter.
    int (__cdecl *parse)(void* callback,const char* name,const char* data,unsigned bytes,const NativeVcdRuntimeBindings*,std::uint32_t*) noexcept;
    // Only cache/context/extension/missing are consumed from load template.
    int (__cdecl *load)(void*,unsigned,const char*,const NamedSceneLoadBindings*,const NativeVcdRuntimeBindings*,std::uint32_t*) noexcept;
    // Native named NPC path; overrides precache.load ownership/parser only.
    int (__cdecl *scene)(void*,const char*,float,const NativeNamedPlaybackBindings*,const NativeVcdRuntimeBindings*,float*) noexcept;
    // Consume owned root, including nested scenes. Never retry partial failure.
    int (__cdecl *destroy)(std::uint32_t,const NativeVcdRuntimeBindings*) noexcept;
    int (__cdecl *remove)(void*,unsigned,const SceneEntityLifecycleBindings*,const SceneStopBindings*,const NativeVcdRuntimeBindings*) noexcept;
};
using GetNativeVcdRuntimeApi=const NativeVcdRuntimeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;














}
