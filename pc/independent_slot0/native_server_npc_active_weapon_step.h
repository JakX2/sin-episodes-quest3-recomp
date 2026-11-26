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

inline constexpr char server_npc_wake_transition_export[]="SiN_GetServerNpcWakeTransitionApi";
struct ServerNpcWakeTransitionBindings {
    const ServerNpcActivePostAnimationBindings* storage;
    ServerWeaponActivityBindings entities;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *refresh_entity)(void*,void*) noexcept; // optional +58 override; native authenticated 2213c980 when null
    int (__cdecl *output)(void*,void*,unsigned) noexcept; // 22156900, +bc8
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
