#pragma once
#include "native_jessica_script.h"
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











}
