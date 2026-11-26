#pragma once
#include "native_server_npc_schedule_image.h"
#include "native_server_npc_weapon_runtime.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_active_weapon_step_export[]="SiN_GetServerNpcActiveWeaponStepApi";
// Original active-update segment 220433d7..2204342a after 22040ce0 accepted.
// This is not the full 22043260 outer update or its post-animation tail.
struct ServerNpcActiveWeaponStepBindings {
    const ServerScheduleImage* image;
    const ServerNpcScheduleServicesBindings* schedule;
    const ServerGruntHelperTaskBindings* full;
    const ServerNpcSharedAttackBindings* attack;
    const ServerNpcSharedFacingBindings* facing;
    const ServerNpcSharedTargetAnimationBindings* target;
    const ServerNpcSharedMovementBindings* movement;
    const ServerNpcSharedCueBindings* cue;
    const ServerNpcSharedObservationBindings* observe;
    const ServerNpcSharedSoundCoverBindings* sound_cover;
    const ServerNpcSharedTacticalCoverBindings* tactical;
    const ServerNpcNativeCombatLineThinkBindings* think;
    const ServerNpcWeaponPolicyBindings* policy;
    const ServerNpcScriptWeaponEventFrameBindings* frame;
    void* context;
    int (__cdecl *reset_movement)(void*,void*) noexcept; // 2204d350(0,0)
    int (__cdecl *motor_activity)(void*,std::uint32_t,int) noexcept; // motor +48(-1)
};
struct ServerNpcActiveWeaponStepResult {
    ServerNpcNativeRequestThinkResult think;
    ServerNpcGruntNativeLineAttackResult outer_line,inner_line;
    ServerNpcWeaponPolicyResult outer_weapon,inner_weapon;
    ServerNpcScriptWeaponEventFrameResult animation;
    unsigned think_called,think_completed,reset_called,motor_called,animation_called,completed;
};
struct ServerNpcActiveWeaponStepApi {
    std::uint32_t version,size;
    // Grunt/Elite only. Future think deadline still advances animation.
    // All weapon resolution, script reads and condition mapping are shared.
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcActiveWeaponStepBindings*,ServerNpcActiveWeaponStepResult*) noexcept;
};
using GetServerNpcActiveWeaponStepApi=const ServerNpcActiveWeaponStepApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_active_jet_weapon_step_export[]="SiN_GetServerNpcActiveJetWeaponStepApi";
struct ServerNpcActiveJetWeaponStepBindings {
    ServerNpcActiveWeaponStepBindings base; // base.think must be null
    const ServerNpcNativeJetCombatLineThinkBindings* think;
};
struct ServerNpcActiveJetWeaponStepResult {
    ServerNpcNativeJetRequestThinkResult think;
    ServerNpcGruntNativeLineAttackResult outer_line,inner_line;
    ServerNpcWeaponPolicyResult outer_weapon,inner_weapon;
    ServerNpcScriptWeaponEventFrameResult animation;
    unsigned think_called,think_completed,reset_called,motor_called,animation_called,completed;
};
struct ServerNpcActiveJetWeaponStepApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const ServerNpcRunAnimationInput*,const ServerNpcActiveJetWeaponStepBindings*,ServerNpcActiveJetWeaponStepResult*) noexcept;
};
using GetServerNpcActiveJetWeaponStepApi=const ServerNpcActiveJetWeaponStepApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_active_post_animation_export[]="SiN_GetServerNpcActivePostAnimationApi";
struct ServerNpcActivePostAnimationBindings {
    void* context;
    // Resolve a bounded original pointer token; mutable for the network dirty word.
    int (__cdecl *resolve)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *navigation_update)(void*,void*,std::uint32_t,float) noexcept;
    int (__cdecl *prepare_pose)(void*,void*) noexcept;
    int (__cdecl *capability_pose)(void*,void*) noexcept; // +650
    int (__cdecl *timed_pose)(void*,void*,float) noexcept; // +644
    int (__cdecl *finish_pose)(void*,void*) noexcept; // +2e8
    int (__cdecl *motor_finish)(void*,void*,std::uint32_t) noexcept; // +64c -> motor+50
};
struct ServerNpcActivePostAnimationResult {
    unsigned navigation_called,path_active,path_changed,capabilities_first,capabilities_second;
    unsigned prepare_called,capability_called,timed_called,finish_called,motor_called,timestamp_changed,completed;
    float navigation_delta,pose_delta;
};
struct ServerNpcActivePostAnimationApi {
    std::uint32_t version,size;
    // 2204342c..22043467, fixed frame time. Engine leaves remain explicit.
    int (__cdecl *run)(void*,unsigned,float,float,const ServerNpcScheduleServicesBindings*,const ServerNpcActivePostAnimationBindings*,ServerNpcActivePostAnimationResult*) noexcept;
    int (__cdecl *ground)(void*,unsigned,const ServerNpcRunAnimationInput*,float,const ServerNpcActiveWeaponStepBindings*,const ServerNpcActivePostAnimationBindings*,ServerNpcActiveWeaponStepResult*,ServerNpcActivePostAnimationResult*) noexcept;
    int (__cdecl *jet)(void*,unsigned,const ServerNpcRunAnimationInput*,float,const ServerNpcActiveJetWeaponStepBindings*,const ServerNpcActivePostAnimationBindings*,ServerNpcActiveJetWeaponStepResult*,ServerNpcActivePostAnimationResult*) noexcept;
};
using GetServerNpcActivePostAnimationApi=const ServerNpcActivePostAnimationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_outer_admission_export[]="SiN_GetServerNpcOuterAdmissionApi";
struct ServerNpcOuterAdmissionBindings {
    const ServerNpcActivePostAnimationBindings* storage;
    void* context;
    unsigned global_flags; // 22546d5c, bits 0 and 1
    bool enabled; // 2254911c
    int step_threshold; // 224dbd98
    float* warning_deadline; // 22546d48, shared across actors
    int (__cdecl *warning)(void*,void*) noexcept; // 222858a0, diagnostic only
    int (__cdecl *pause_activity)(void*,void*,int) noexcept; // 22049b50(1)
    int (__cdecl *component_gate)(void*,std::uint32_t,bool*) noexcept; // +b8
    int (__cdecl *movement_probe)(void*,void*,int,bool*) noexcept; // +48(5,0)
};
struct ServerNpcOuterAdmissionResult {
    unsigned admitted,step_checked,path_active,playback_changed,warning_called,pause_called,marker_reset,completed;
};
struct ServerNpcOuterScriptGateResult {
    unsigned component_called,probe_called,value,changed,completed;
};
struct ServerNpcOuterAdmissionApi {
    std::uint32_t version,size;
    int (__cdecl *admit)(void*,unsigned,float,std::uint32_t,const ServerNpcOuterAdmissionBindings*,ServerNpcOuterAdmissionResult*) noexcept;
    int (__cdecl *script_gate)(void*,unsigned,float,std::uint32_t,const ServerNpcOuterAdmissionBindings*,ServerNpcOuterScriptGateResult*) noexcept;
};
using GetServerNpcOuterAdmissionApi=const ServerNpcOuterAdmissionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_outer_cadence_export[]="SiN_GetServerNpcOuterCadenceApi";
struct ServerNpcOuterCadenceBindings {
    const ServerNpcActivePostAnimationBindings* storage;
    void* context;
    bool optimize_a,optimize_b; // 22547050/+2c, 22546f30/+2c
    int (__cdecl *notify_think_flags)(void*,void*) noexcept; // 22163610
    int (__cdecl *dynamic_mode)(void*,void*,bool) noexcept; // remaining 2203aac1..2203aeec
};
struct ServerNpcOuterCadenceResult {
    unsigned unscheduled,mode_called,mode_direct,deadline_written,tick_written,flags_changed,notify_called,completed;
    int mode,tick;
    float deadline,next_time;
};
struct ServerNpcOuterCadenceApi {
    std::uint32_t version,size;
    // 2214d7b0 default (unnamed) think context only; -1 clears it.
    int (__cdecl *set_default)(void*,unsigned,float,float,std::uint32_t,const ServerNpcOuterCadenceBindings*,ServerNpcOuterCadenceResult*) noexcept;
    // 2204368c..2204376f. pvs is the entry snapshot, think_called is BL.
    int (__cdecl *finish)(void*,unsigned,float,float,std::uint32_t,bool,bool,const ServerNpcOuterCadenceBindings*,ServerNpcOuterCadenceResult*) noexcept;
};
using GetServerNpcOuterCadenceApi=const ServerNpcOuterCadenceApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_outer_update_export[]="SiN_GetServerNpcOuterUpdateApi";
struct ServerNpcOuterUpdateBindings {
    const ServerNpcActivePostAnimationBindings* post;
    ServerNpcOuterAdmissionBindings admission;
    ServerNpcOuterCadenceBindings cadence;
    void* context;
    bool active_scene; // 22549118 and +3a0; snapshot for this update
    int (__cdecl *pending_impact)(void*,void*) noexcept; // 22035b80, only +be1
    int (__cdecl *pvs)(void*,std::uint32_t,bool*) noexcept; // 222895a0
    int (__cdecl *wake)(void*,void*,bool) noexcept; // 2203af10 when +868 > 0
};
struct ServerNpcOuterUpdateResult {
    ServerNpcOuterCadenceResult clear,finish;
    ServerNpcOuterScriptGateResult script;
    ServerNpcOuterAdmissionResult admission;
    unsigned impact_called,pvs_called,pvs,condition_mapped,wake_called,sleep_bypassed,scene_bypassed,
        active_called,think_called,nav_sentinel_written,completed;
};
struct ServerNpcOuterUpdateApi {
    std::uint32_t version,size;
    // Fixed-time gameplay composition of 22043260. Profiling and diagnostic overlays excluded.
    int (__cdecl *ground)(void*,unsigned,const ServerNpcRunAnimationInput*,float,const ServerNpcActiveWeaponStepBindings*,const ServerNpcOuterUpdateBindings*,ServerNpcActiveWeaponStepResult*,ServerNpcActivePostAnimationResult*,ServerNpcOuterUpdateResult*) noexcept;
    int (__cdecl *jet)(void*,unsigned,const ServerNpcRunAnimationInput*,float,const ServerNpcActiveJetWeaponStepBindings*,const ServerNpcOuterUpdateBindings*,ServerNpcActiveJetWeaponStepResult*,ServerNpcActivePostAnimationResult*,ServerNpcOuterUpdateResult*) noexcept;
};
using GetServerNpcOuterUpdateApi=const ServerNpcOuterUpdateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_wake_policy_export[]="SiN_GetServerNpcWakePolicyApi";
struct ServerNpcWakePolicyBindings {
    ServerNpcConditionMapBindings conditions;
    void* context;
    int max_clients;
    int (__cdecl *local_player)(void*,void**) noexcept; // 22288a40
    int (__cdecl *client)(void*,int,void**) noexcept; // 22286c70
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *origin)(void*,void*,float*) noexcept; // 220092d0
    int (__cdecl *player_visible)(void*,void*,void*,unsigned,bool*) noexcept; // player +1f0
    int (__cdecl *missing_player)(void*) noexcept;
    int (__cdecl *active_sound_head)(void*,int*) noexcept;
    int (__cdecl *sound)(void*,int,const void**,unsigned*) noexcept;
    int (__cdecl *sound_audible)(void*,void*,std::uint32_t,const void*,bool*) noexcept; // 220a1a80
    int (__cdecl *sound_visible)(void*,void*,const void*,bool*) noexcept; // 22034b90
    int (__cdecl *wake)(void*,void*,bool) noexcept; // ground/elite 22040ea0, jet 22366e60
};
struct ServerNpcWakePolicyResult {
    unsigned completed,local_queried,missing_player,proximity_checked,damage_checked,players_visited,visibility_calls,
        sounds_visited,audible_calls,sound_visibility_calls,wake_calls,reason;
    float distance_squared;
};
enum ServerNpcWakeReason : unsigned {NpcWakeNone,NpcWakeMultiplayer,NpcWakeProximity,NpcWakePvs,NpcWakeDamage,NpcWakeVisiblePlayer,NpcWakeSound};
struct ServerNpcWakePolicyApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,bool,const ServerNpcWakePolicyBindings*,ServerNpcWakePolicyResult*) noexcept;
};
using GetServerNpcWakePolicyApi=const ServerNpcWakePolicyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Original void output wrapper 22156900 and action traversal 22156680.
// Queue construction, string-token interpretation and allocator remain host
// boundaries. The native action path owns countdown and linked-list mutation.
inline constexpr char server_entity_void_output_export[]="SiN_GetServerEntityVoidOutputApi";
struct ServerEntityVoidOutputAction {
    std::uint32_t target,input,parameter,variant_type,action_id;
    float delay;
};
struct ServerEntityVoidOutputBindings {
    void* context;
    int (__cdecl *resolve)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *enqueue)(void*,void* activator,void* caller,const ServerEntityVoidOutputAction*) noexcept;
    int (__cdecl *release)(void*,std::uint32_t,void*) noexcept;
    int (__cdecl *debug_action)(void*,void* caller,const ServerEntityVoidOutputAction*) noexcept;
};
struct ServerEntityVoidOutputResult {
    unsigned completed,visited,queued,parameter_overrides,countdowns,removed,debugged;
};
struct ServerEntityVoidOutputApi {
    std::uint32_t version,size;
    int (__cdecl *fire)(void* output,unsigned output_bytes,void* activator,void* caller,unsigned caller_bytes,float extra_delay,const ServerEntityVoidOutputBindings*,ServerEntityVoidOutputResult*) noexcept;
};
using GetServerEntityVoidOutputApi=const ServerEntityVoidOutputApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Original named-target queue constructor 22155300 and insertion 22154600.
// Token resolver and allocator must provide stable, distinct PE32 records.
inline constexpr char server_entity_named_event_queue_export[]="SiN_GetServerEntityNamedEventQueueApi";
struct ServerEntityNamedEventQueueBindings {
    void* context;
    int (__cdecl *resolve)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *allocate)(void*,std::uint32_t*,void**,unsigned*) noexcept;
    int (__cdecl *string_nonempty)(void*,std::uint32_t,bool*) noexcept;
};
struct ServerEntityNamedEventQueueResult {
    unsigned completed,visited,allocated,inserted,node,previous,next;
    float fire_time;
};
struct ServerEntityNamedEventQueueApi {
    std::uint32_t version,size;
    int (__cdecl *enqueue)(void* sentinel,unsigned sentinel_bytes,std::uint32_t sentinel_token,float now,std::uint32_t activator,std::uint32_t caller,const ServerEntityVoidOutputAction*,const ServerEntityNamedEventQueueBindings*,ServerEntityNamedEventQueueResult*) noexcept;
};
using GetServerEntityNamedEventQueueApi=const ServerEntityNamedEventQueueApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Original 22155530 queue service. Entity lookup and AcceptInput remain
// explicit engine boundaries, while timing, routing and unlinking are native.
inline constexpr char server_entity_event_service_export[]="SiN_GetServerEntityEventServiceApi";
struct ServerEntityEventServiceBindings {
    void* context;
    ServerWeaponActivityBindings entities;
    int (__cdecl *resolve)(void*,std::uint32_t,void**,unsigned*) noexcept;
    int (__cdecl *lock)(void*,bool enter) noexcept;
    int (__cdecl *find_named)(void*,void* previous,std::uint32_t name,void* activator,void* caller,void**) noexcept;
    int (__cdecl *find_class)(void*,void* previous,std::uint32_t name,void**) noexcept;
    int (__cdecl *accept)(void*,void* target,std::uint32_t input,void* activator,void* caller,const void* variant,std::uint32_t output_id) noexcept;
    int (__cdecl *release)(void*,std::uint32_t,void*) noexcept;
    int (__cdecl *missing)(void*,std::uint32_t target,std::uint32_t input,void* caller) noexcept;
};
struct ServerEntityEventServiceResult {
    unsigned completed,events,deliveries,named,direct,classname,missing,removed,locks,step_stopped,future_stopped;
};
struct ServerEntityEventServiceApi {
    std::uint32_t version,size;
    int (__cdecl *service)(void* sentinel,unsigned sentinel_bytes,std::uint32_t sentinel_token,float now,bool single_step,int* steps,const ServerEntityEventServiceBindings*,ServerEntityEventServiceResult*) noexcept;
};
using GetServerEntityEventServiceApi=const ServerEntityEventServiceApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Original CBaseEntity::AcceptInput 22141400. Datamap records and name
// tokens are original PE32-shaped storage, supplied by the independent owner.
inline constexpr char server_entity_accept_input_export[]="SiN_GetServerEntityAcceptInputApi";
struct ServerEntityInputData {
    void* activator;
    void* caller;
    unsigned char variant[0x14];
    std::uint32_t output_id;
};
struct ServerNpcWakeTransitionBindings;
struct ServerNpcWakeTransitionResult;
struct ServerEntitySearchBindings;
// Original squad manager 2254adec and membership helpers 220aa0d0/220aa1b0.
// Allocation/interning and the existing peer/memory effects remain explicit.
struct ServerNpcInputSquadBindings {
    void* context;
    std::uint32_t* head;
    int (__cdecl *weapon_capabilities)(void*,void*,unsigned*) noexcept;
    int (__cdecl *allocate)(void*,std::uint32_t*,void**,unsigned*) noexcept;
    int (__cdecl *intern)(void*,std::uint32_t,std::uint32_t*) noexcept;
    int (__cdecl *removed_notice)(void*,void* peer,void* removed) noexcept;
    int (__cdecl *copy_leader_memory)(void*,void* joining,void* leader) noexcept;
    int (__cdecl *warning)(void*,void*,std::uint32_t) noexcept;
};
// Original Jetpack input leaves; navigation reset, entity removal and
// fresh thrust-pair initialization remain explicit engine boundaries.
struct ServerNpcInputJetBindings {
    void* context;
    int (__cdecl *reset_navigation)(void*,void*,std::uint32_t) noexcept;
    int (__cdecl *remove_thrust)(void*,void*) noexcept;
    int (__cdecl *prepare_thrust)(void*,void*) noexcept;
};
// Original embedded rappel behavior 223f956c, class namespace 2254c648.
// BeginRappel's transform detachment remains an explicit engine operation.
struct ServerNpcInputRappelBindings {
    const ServerNpcConditionMapBindings* actor_conditions;
    void* context;
    int (__cdecl *detach)(void*,void*) noexcept;
};
// 22030a00 relationship parser and 222aa090/222a9f90 vector edits.
struct ServerNpcInputRelationshipBindings {
    void* context;
    // Grow the original vector at actor+638, preserving rows and count.
    int (__cdecl *grow)(void*,void*,unsigned) noexcept;
    // Class factory/classification fallback for unmatched names. A zero
    // found result means the original unknown-class diagnostic path.
    int (__cdecl *classify)(void*,const char*,unsigned*,bool*) noexcept;
    int (__cdecl *warning)(void*,void*,unsigned,const char*) noexcept;
};
struct ServerEntityAcceptInputBindings {
    const ServerNpcActivePostAnimationBindings* storage;
    void* context;
    ServerWeaponActivityBindings entities;
    int (__cdecl *name_equal)(void*,std::uint32_t,std::uint32_t,bool*) noexcept;
    int (__cdecl *convert)(void*,void* variant,unsigned target_type,bool* converted) noexcept;
    int (__cdecl *handler)(void*,void* entity,std::uint32_t original_handler,const ServerEntityInputData*) noexcept;
    int (__cdecl *diagnostic)(void*,void* entity,void* caller,std::uint32_t input,unsigned reason,const void* variant) noexcept;
    // Optional native 220320b0 InputWake route. Registry and dirty storage
    // are authoritative from AcceptInput; other handler tokens remain hosted.
    const ServerNpcWakeTransitionBindings* wake;
    ServerNpcWakeTransitionResult* wake_result;
    // Bounded C-locale string token resolver. Enables original ASCII
    // case-insensitive input matching and decimal string-to-int/bool leaves.
    int (__cdecl *string_text)(void*,std::uint32_t,const char**,unsigned*) noexcept;
    // Optional native variant entity conversion; AcceptInput registry wins.
    const ServerEntitySearchBindings* search;
    // Use the recovered input-only projection of the four NPC datamaps.
    // Other inherited maps still require the original-shaped storage service.
    bool native_npc_datamaps;
    // Native leaf inputs for authenticated NPC classes. now is the fixed
    // frame time used by InputIgnoreDangerSounds.
    bool native_npc_handlers;
    float now;
    // Optional diagnostic for original Jetpack path-control warning token.
    int (__cdecl *path_warning)(void*,void*,std::uint32_t) noexcept;
    const ServerNpcInputSquadBindings* squad;
    const ServerNpcInputJetBindings* jet;
    const ServerNpcInputRappelBindings* rappel;
    // Lowering health constructs an original 0x48 damage record; actual
    // damage application is delegated to the damage owner, never faked.
    int (__cdecl *health_damage)(void*,void*,const void*,unsigned) noexcept;
    const ServerNpcInputRelationshipBindings* relationship;
};
struct ServerEntityAcceptInputResult {
    unsigned completed,accepted,maps,rows,matched,map,row,handler,expected_type,actual_type,converted,written,dirty,reason,native_wake,native_maps,native_handler,handler_dirty,path_missing,squad_visited,squad_created,squad_removed,squad_added,squad_notices,squad_memory,squad_full,squad_denied,path_changed,path_reset,thrust_reused,thrust_prepared,thrust_removed,rappel_mapped,rappel_condition,rappel_detached,rappel_snapshot,health_healed,health_damage,health_blocked,relationship_specs,relationship_entities,relationship_classes,relationship_updated,relationship_added,relationship_warnings;
};
enum ServerEntityInputReason : unsigned {EntityInputNone,EntityInputUnknown,EntityInputBadConversion};
struct ServerEntityAcceptInputApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,std::uint32_t original_vtable,std::uint32_t input,void* activator,void* caller,const void* variant,unsigned variant_bytes,std::uint32_t output_id,const ServerEntityAcceptInputBindings*,ServerEntityAcceptInputResult*) noexcept;
};
using GetServerEntityAcceptInputApi=const ServerEntityAcceptInputApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Original 221619e0/22161100 active registry searches and 22161900
// contextual names. The independently owned registry supplies its live head.
inline constexpr char server_entity_search_export[]="SiN_GetServerEntitySearchApi";
struct ServerEntitySearchBindings {
    void* context;
    ServerWeaponActivityBindings entities;
    const sin_independent::EntityHandleEntry* head;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *string_text)(void*,std::uint32_t,const char**,unsigned*) noexcept;
    int (__cdecl *foreign_handle)(void*,void*,std::uint32_t*) noexcept;
    int (__cdecl *player_one)(void*,void**) noexcept;
    int (__cdecl *pvs_player)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *picker)(void*,void* player,void**) noexcept;
    int (__cdecl *invalid_name)(void*,std::uint32_t) noexcept;
};
struct ServerEntitySearchResult {
    void* entity;
    unsigned completed,visited,empty_slots,contextual,matched;
};
struct ServerEntitySearchApi {
    std::uint32_t version,size;
    int (__cdecl *named)(void* previous,std::uint32_t name,void* activator,void* caller,const ServerEntitySearchBindings*,ServerEntitySearchResult*) noexcept;
    int (__cdecl *classname)(void* previous,std::uint32_t name,const ServerEntitySearchBindings*,ServerEntitySearchResult*) noexcept;
};
using GetServerEntitySearchApi=const ServerEntitySearchApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_wake_transition_export[]="SiN_GetServerNpcWakeTransitionApi";
struct ServerNpcWakeTransitionBindings {
    const ServerNpcActivePostAnimationBindings* storage;
    ServerWeaponActivityBindings entities;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *refresh_entity)(void*,void*) noexcept; // optional +58 override; native authenticated 2213c980 when null
    int (__cdecl *output)(void*,void*,unsigned) noexcept; // 22156900, +bc8; optional only for unconnected output
    // Optional filtered squad iterator override 220a9300/220a93c0. Cursor starts at -1; native Grunt path when null.
    int (__cdecl *squad_next)(void*,std::uint32_t,int*,void**) noexcept;
    int (__cdecl *external_wake)(void*,void*,bool) noexcept;
    int (__cdecl *jet_prepare)(void*,void*) noexcept; // optional 22366a20 override; required for fresh attachments
    int (__cdecl *jet_attachment)(void*,std::uint32_t) noexcept; // +218 virtual +8
};
struct ServerNpcWakeTransitionResult {
    unsigned completed,transitions,flags_changed,refreshed,outputs,squad_visited,squad_woken,external_woken,jet_prepared,jet_linked,jet_attachment;
};
struct ServerNpcWakeTransitionApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,bool,const ServerNpcWakeTransitionBindings*,ServerNpcWakeTransitionResult*) noexcept;
};
using GetServerNpcWakeTransitionApi=const ServerNpcWakeTransitionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_outer_wake_update_export[]="SiN_GetServerNpcOuterWakeUpdateApi";
struct ServerNpcOuterWakeUpdateBindings {
    ServerNpcOuterUpdateBindings outer;
    ServerNpcWakePolicyBindings policy;
    ServerNpcWakeTransitionBindings transition;
};
struct ServerNpcOuterWakeUpdateResult {
    ServerNpcOuterUpdateResult outer;
    ServerNpcWakePolicyResult policy;
    ServerNpcWakeTransitionResult transition;
};
struct ServerNpcOuterWakeUpdateApi {
    std::uint32_t version,size;
    // Authoritative condition namespace, entity registry and dirty storage
    // are taken from the existing schedule/outer bindings. Legacy wake callbacks
    // are not invoked by this opt-in API.
    int (__cdecl *ground)(void*,unsigned,const ServerNpcRunAnimationInput*,float,const ServerNpcActiveWeaponStepBindings*,const ServerNpcOuterWakeUpdateBindings*,ServerNpcActiveWeaponStepResult*,ServerNpcActivePostAnimationResult*,ServerNpcOuterWakeUpdateResult*) noexcept;
    int (__cdecl *jet)(void*,unsigned,const ServerNpcRunAnimationInput*,float,const ServerNpcActiveJetWeaponStepBindings*,const ServerNpcOuterWakeUpdateBindings*,ServerNpcActiveJetWeaponStepResult*,ServerNpcActivePostAnimationResult*,ServerNpcOuterWakeUpdateResult*) noexcept;
};
using GetServerNpcOuterWakeUpdateApi=const ServerNpcOuterWakeUpdateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

}
