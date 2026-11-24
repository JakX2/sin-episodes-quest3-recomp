#pragma once
// New port ABI, not an original Source engine interface.
// Calls are synchronous on the host's game thread. Output pointers must
// refer to caller-owned writable storage. Do not retain API pointers after
// unloading the module. No C++ objects or allocations cross the DLL boundary.
#include <cstdint>
#include <cstddef>

#if defined(_MSC_VER)
#define SIN_FIXED_CALL __cdecl
#else
#define SIN_FIXED_CALL
#endif

namespace sin_port::fixed_service {
inline constexpr std::uint32_t abi_version=1;
inline constexpr std::uint32_t metric_count=14;
inline constexpr char export_name[]="SiN_GetFixedDifficultyApi";
enum ChallengeInput : std::uint32_t { Initial=0, Current=1, Difference=2 };

struct State {
    std::uint32_t skill, initial_skill, revision, trigger_count;
    std::uint32_t completed, adaptive_enabled;
};

struct ChallengeOutput {
    std::uint32_t original_field_offset;
    std::uint32_t activator, caller;
    float value;
};

struct Api {
    std::uint32_t version, size, metrics;
    int (SIN_FIXED_CALL *begin_campaign)(int) noexcept;
    int (SIN_FIXED_CALL *end_campaign)() noexcept;
    int (SIN_FIXED_CALL *set_skill)(int) noexcept;
    int (SIN_FIXED_CALL *get_state)(State*) noexcept;
    int (SIN_FIXED_CALL *get_metric)(std::uint32_t,float*) noexcept;
    int (SIN_FIXED_CALL *scale_damage_taken)(float,std::uint32_t,float*) noexcept;
    int (SIN_FIXED_CALL *scale_damage_done)(float,float*) noexcept;
    int (SIN_FIXED_CALL *npc_shoot_delay)(float,float*) noexcept;
    int (SIN_FIXED_CALL *observe_trigger)(std::int32_t,float,std::int32_t) noexcept;
    int (SIN_FIXED_CALL *capture_challenge)(float*) noexcept;
    // Snapshot is normalized 0/0.5/1, from capture_challenge. It is
    // required only for Difference. The result is an output descriptor,
    // NOT a dispatch into an original engine entity or its raw memory.
    int (SIN_FIXED_CALL *query_challenge)(std::uint32_t,float,
                                         std::uint32_t,ChallengeOutput*) noexcept;
};
// Win32 original VEngineCvar002 bridge. Explicit host call at campaign
// start or a game-thread synchronization boundary; no engine pointer is
// retained, no console object is created or modified. Not an original export.
inline constexpr char engine_skill_export_name[]="SiN_ApplyEngineSkill";
enum EngineSkillOperation : std::uint32_t { BeginFromEngine=0, SyncFromEngine=1 };
using ApplyEngineSkill=int (SIN_FIXED_CALL *)(void*,std::uint32_t) noexcept;
// Explicit host boundary for the recovered GameInit command-buffer tail.
// Caller owns live VEngineServer021 and VEngineCvar002 interfaces, and
// must have completed preceding DLL startup requirements. The DLL-owned
// CGlobalState is cleared before the command-buffer tail.
// Not a replacement for original GameInit. No pointers are retained.
inline constexpr char configured_campaign_export_name[]="SiN_StartConfiguredCampaign";
enum ConfiguredStartResult : int {
    ConfigRejected=0, ConfigStarted=1, ConfigRanSkillUnavailable=2,
    ConfigQueueUncertain=3, ConfigExecuteUncertain=4
};
using StartConfiguredCampaign=int (SIN_FIXED_CALL *)(void*,void*) noexcept;
// Independent CGlobalState service, not original C++ layout or save format.
// String inputs must be live NUL-terminated buffers. Read copies both
// strings or refuses without modifying outputs. No owned memory escapes.
inline constexpr char global_state_export_name[]="SiN_GetGlobalStateApi";
inline constexpr std::uint32_t global_state_version=1;
struct GlobalStateApi {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *count)() noexcept;
    int (SIN_FIXED_CALL *find)(const char*) noexcept; // -1 absent
    int (SIN_FIXED_CALL *add)(const char*,const char*,std::int32_t) noexcept;
    int (SIN_FIXED_CALL *read)(int,char*,std::uint32_t,char*,std::uint32_t,std::int32_t*) noexcept;
    int (SIN_FIXED_CALL *set_state)(int,std::int32_t) noexcept;
    int (SIN_FIXED_CALL *set_level)(int,const char*) noexcept;
    int (SIN_FIXED_CALL *set_updates_disabled)(int) noexcept;
    int (SIN_FIXED_CALL *clear)() noexcept;
};
using GetGlobalStateApi=const GlobalStateApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
// Original-guided env_global behavior on the native CGlobalState owner.
// Caller resolves Source string_t and current level, and handles RemoveSelf.
// This is not a retail entity vtable, factory, or datamap.
inline constexpr char env_global_export_name[]="SiN_EnvGlobalDispatch";
enum EnvGlobalOperation : std::uint32_t {
    EnvSpawn=0, EnvTurnOn=1, EnvTurnOff=2, EnvRemove=3, EnvToggle=4
};
enum EnvGlobalResult : int {
    EnvRejected=0, EnvApplied=1, EnvUnchanged=2, EnvRemoveSelf=3, EnvBlocked=4
};
using EnvGlobalDispatch=int (SIN_FIXED_CALL *)(std::uint32_t,const char*,const char*,
    std::uint32_t,std::int32_t) noexcept;
// Native port behavior registry, not Source edicts or a retail datamap.
// A host assigns index/generation and routes only these class-specific
// keys. Map epochs distinguish reused engine identities across levels.
// All calls are synchronous; the output sink and its arguments are borrowed
// only during input(). The output's activator and caller are both self.
inline constexpr char map_entity_export_name[]="SiN_GetMapEntityApi";
inline constexpr std::uint32_t map_entity_version=1;
struct MapEntityRef {std::uint32_t epoch,index,generation;};
enum MapEntityResult : int {
    MapRejected=0, MapApplied=1, MapUnchanged=2, MapRemoveSelf=3,
    MapBlocked=4, MapUnknownClass=5, MapUnhandledKey=6,
    MapUnhandledInput=7, MapOutputFailed=8
};
using MapFloatOutput=int (SIN_FIXED_CALL *)(void*,const MapEntityRef*,
    const char*,std::uint32_t,float);
struct MapEntityApi {
    std::uint32_t version,size;
    std::uint32_t (SIN_FIXED_CALL *begin_level)(const char*) noexcept;
    int (SIN_FIXED_CALL *end_level)(std::uint32_t) noexcept;
    int (SIN_FIXED_CALL *create)(MapEntityRef,const char*) noexcept;
    int (SIN_FIXED_CALL *keyvalue)(MapEntityRef,const char*,const char*) noexcept;
    int (SIN_FIXED_CALL *spawn)(MapEntityRef) noexcept;
    int (SIN_FIXED_CALL *input)(MapEntityRef,const char*,MapFloatOutput,void*) noexcept;
    int (SIN_FIXED_CALL *remove)(MapEntityRef) noexcept;
    int (SIN_FIXED_CALL *count)(std::uint32_t) noexcept;
};
using GetMapEntityApi=const MapEntityApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
static_assert(sizeof(MapEntityRef)==12);
// Host callbacks for the already recovered SiN trigger subclasses.
// No callbacks or context pointers are retained after spawn/touch/retry.
// Host owns liveness, player predicate, native base trigger, game events,
// and deferred entity deletion. The module owns fixed campaign accounting.
inline constexpr char trigger_export_name[]="SiN_GetCampaignTriggerApi";
inline constexpr std::uint32_t trigger_version=1;
enum NativeTriggerCode : std::uint32_t {
    TriggerHandled=0, TriggerNonPlayer, TriggerAlreadyConsumed, TriggerUnknownEntity,
    TriggerInvalidActor, TriggerInvalidConfig, TriggerUnknownKey, TriggerHostNotLive,
    TriggerHostRefused, TriggerEventUnavailable, TriggerManagerRefused,
    TriggerRemovalPending, TriggerWrongClass, TriggerReentrant
};
enum NativeTriggerEventKind : std::uint32_t { TriggerDynamicEvent=0, TriggerGameEvent=1 };
struct NativeTriggerEvent {
    std::uint32_t kind;
    std::int32_t descriptor,owner,advisor,decision;
    float value,x,y,z;
    const char* name; // borrowed until publish callback returns
};
struct NativeTriggerHost {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *is_live)(void*,MapEntityRef);
    int (SIN_FIXED_CALL *is_player)(void*,MapEntityRef);
    int (SIN_FIXED_CALL *base_spawn)(void*,MapEntityRef);
    void (SIN_FIXED_CALL *warning)(void*,const char*);
    int (SIN_FIXED_CALL *owner_index)(void*);
    int (SIN_FIXED_CALL *special_action)(void*,MapEntityRef);
    int (SIN_FIXED_CALL *publish)(void*,MapEntityRef,const NativeTriggerEvent*);
    int (SIN_FIXED_CALL *queue_remove)(void*,MapEntityRef);
};
struct NativeTriggerSpawn {std::uint32_t engine_requests_removal,flags_120,spawnflags_b4;};
struct NativeTriggerTouch {float x,y,z;std::uint32_t flags_b8;};
struct NativeTriggerReport {
    std::uint32_t code,base_initialized,warning_emitted,event_published,manager_signaled,removal_queued;
};
struct CampaignTriggerApi {
    std::uint32_t version,size;
    // Nonzero return means a report was produced, not necessarily success.
    int (SIN_FIXED_CALL *spawn)(MapEntityRef,const NativeTriggerSpawn*,const NativeTriggerHost*,void*,NativeTriggerReport*) noexcept;
    int (SIN_FIXED_CALL *touch)(MapEntityRef,MapEntityRef,const NativeTriggerTouch*,const NativeTriggerHost*,void*,NativeTriggerReport*) noexcept;
    int (SIN_FIXED_CALL *retry_remove)(MapEntityRef,const NativeTriggerHost*,void*,NativeTriggerReport*) noexcept;
};
using GetCampaignTriggerApi=const CampaignTriggerApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
static_assert(sizeof(NativeTriggerReport)==24);
// Original engine GAMEEVENTSMANAGER002 writer, Win32 x86 only.
// The legacy port payload's advisor field serializes as "subject".
// FireEvent takes ownership even on a normal false return. Uncertain
// outcomes must never be blindly retried or freed again.
inline constexpr char event_publish_export_name[]="SiN_PublishCampaignTriggerEvent";
enum NativeEventResult : int {
    EventRejected=0,EventFired=1,EventUnavailable=2,EventFireReturnedFalse=3,
    EventDiscarded=4,EventCreateUncertain=5,EventObjectUnverified=6,
    EventCleanupUncertain=7,EventFireUncertain=8
};
using PublishCampaignEvent=int (SIN_FIXED_CALL *)(void*,const NativeTriggerEvent*) noexcept;
// Recovered original CHL2_Player damage entry gate and final damage-copy
// preparation, after the intervening original caller branches have passed.
// These are independent port ABI records, not a claim of complete TakeDamage.
inline constexpr char combat_export_name[]="SiN_GetCombatDamageApi";
inline constexpr std::uint32_t combat_version=1;
struct PlayerDamageRecord {
    std::uint32_t vector_bits[9],inflictor_handle,attacker_handle;
    float amount;
    std::uint32_t opaque_30;
    float base_damage;
    std::uint32_t damage_type,tail[3];
};
static_assert(sizeof(PlayerDamageRecord)==0x48);
static_assert(offsetof(PlayerDamageRecord,amount)==0x2c);
static_assert(offsetof(PlayerDamageRecord,base_damage)==0x34);
static_assert(offsetof(PlayerDamageRecord,damage_type)==0x38);
struct PlayerDamageObservation {
    std::uint32_t life_state,attacker_is_self,inflictor_is_self;
};
enum PlayerDamageCode : int {
    PlayerDamageRejected=0,PlayerDamageReady=1,PlayerDamageInvulnerable=2,
    PlayerDamageNotAlive=3
};
// Final, already-filtered common health stage, after player/NPC-specific
// rules and armor. take_damage==1 skips the integer/fraction update.
// Values are from the host's own live combat-character state.
struct CommonDamageInput {
    std::int32_t health;
    std::uint32_t take_damage;
    float fraction,amount;
};
struct CommonDamageOutput {
    std::int32_t health,integer_damage;
    float fraction;
    std::uint32_t fraction_write,health_changed;
};
enum CommonDamageCode : int {
    CommonDamageRejected=0,CommonDamageReduced=1,CommonDamageFractionOnly=2,
    CommonDamageEventsOnly=3
};
struct CombatDamageApi {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *player_gate)(std::uint32_t) noexcept;
    int (SIN_FIXED_CALL *prepare_player_copy)(const PlayerDamageRecord*,
        const PlayerDamageObservation*,PlayerDamageRecord*) noexcept;
    int (SIN_FIXED_CALL *common_health_stage)(const CommonDamageInput*,CommonDamageOutput*) noexcept;
};
using GetCombatDamageApi=const CombatDamageApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
// Original game-rule decisions driven by the selected fixed profile.
// The random stream is borrowed per call and sampled exactly once on
// nonzero thresholds. A zero threshold never advances the stream.
inline constexpr char npc_rules_export_name[]="SiN_GetNpcRulesApi";
inline constexpr char npc_engine_rng_export_name[]="SiN_DecideNpcWithEngineRandom";
inline constexpr std::uint32_t npc_rules_version=1;
enum NpcDecision : std::uint32_t {
    NpcSecondaryFire=0,NpcWearsHelmet,NpcHelmetShotOff,NpcTakesCover,
    NpcMobile,NpcSpawnCountDelta,NpcHealthKitDrop
};
struct NpcDecisionOutput {
    float threshold,sample;
    std::uint32_t drew_random;
    std::int32_t decision; // bool 0/1 except signed spawn-count delta
};
struct NpcSpread {float x,y,z;};
using NpcRandomFloat=int (SIN_FIXED_CALL *)(void*,float,float,float*);
struct NpcRulesApi {
    std::uint32_t version,size;
    // factor applies only to NpcMobile; host passes the original callsite
    // factor. Other decisions ignore factor. Failed calls leave out intact.
    int (SIN_FIXED_CALL *decide)(std::uint32_t,float,NpcRandomFloat,void*,NpcDecisionOutput*) noexcept;
    // Original attack-spread cache miss: scale the resolved base spread
    // and produce the 3-component value. Do NOT rescale an existing cache hit.
    int (SIN_FIXED_CALL *uncached_spread)(float,NpcSpread*) noexcept;
};
using GetNpcRulesApi=const NpcRulesApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
// Win32 only: live VEngineRandom001 pointer, original slot 1 RandomFloat.
using DecideNpcWithEngineRandom=int (SIN_FIXED_CALL *)(void*,std::uint32_t,float,NpcDecisionOutput*) noexcept;
// Port-owned parsed combat configuration, not a native KeyValues object.
// Exact mapping precedes wildcard at each level. A missing match uses the
// original default spread. Configuration persists until clear/reload.
inline constexpr char attack_spread_export_name[]="SiN_GetAttackSpreadApi";
inline constexpr std::uint32_t attack_spread_version=1;
struct AttackSpreadResult {
    float base,x,y,z;
    std::uint32_t used_fallback;
};
struct AttackSpreadApi {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *load_text)(const char*,std::uint32_t) noexcept;
    int (SIN_FIXED_CALL *clear)() noexcept;
    int (SIN_FIXED_CALL *query)(const char*,const char*,const char*,AttackSpreadResult*) noexcept;
};
using GetAttackSpreadApi=const AttackSpreadApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
static_assert(sizeof(AttackSpreadResult)==20);
// Win32 x86 VFileSystem012 bridge for the single original combat script.
// A failed attempt preserves the last successfully loaded configuration.
// On exception during Open or Close, ownership may be uncertain; do not retry
// automatically. No file handle or filesystem pointer is retained.
inline constexpr char spread_filesystem_export_name[]="SiN_LoadAttackSpreadFromEngine";
enum SpreadFileCode : std::uint32_t {
    SpreadFileRejected=0,SpreadFileLoaded,SpreadFileMissing,SpreadFileOpenUncertain,
    SpreadFileInvalidLength,SpreadFileReadFailed,SpreadFileCloseUncertain,SpreadFileParseFailed
};
struct SpreadFileReport {
    std::uint32_t code,expected_bytes,read_bytes,close_attempted,close_returned;
};
using LoadAttackSpreadFromEngine=int (SIN_FIXED_CALL *)(void*,SpreadFileReport*) noexcept;
static_assert(sizeof(SpreadFileReport)==20);
// Original CBaseCombatCharacter / CAI_BaseNPC slot243 behavior. The
// shell host resolves class names and the target's live enemy-memory
// record. No foreign entity pointer or memory object is retained.
inline constexpr char npc_attack_spread_export_name[]="SiN_GetNpcAttackSpreadApi";
inline constexpr char npc_attack_spread_engine_export_name[]="SiN_QueryNpcAttackSpreadFromEngine";
inline constexpr std::uint32_t npc_attack_spread_version=1;
struct NpcFocusMemory {
    std::uint32_t found;
    float curtime,time_valid_enemy;
};
struct NpcFocusTuning {
    float focus_time,defocused_multiplier;
};
struct NpcAttackSpreadResult {
    AttackSpreadResult rules;
    NpcSpread result;
    float elapsed,focus_multiplier;
    std::uint32_t defocused;
};
struct NpcAttackSpreadApi {
    std::uint32_t version,size;
    // Tuning is needed only when found is nonzero; fields from skipped
    // branches are not read. On failure, the caller output is unchanged.
    int (SIN_FIXED_CALL *query)(const char*,const char*,const char*,
        const NpcFocusMemory*,const NpcFocusTuning*,NpcAttackSpreadResult*) noexcept;
};
using GetNpcAttackSpreadApi=const NpcAttackSpreadApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
// Win32 variant obtains live float values from original VEngineCvar002.
// The multiplier is not looked up if no memory, focus <= 0, or expired.
using QueryNpcAttackSpreadFromEngine=int (SIN_FIXED_CALL *)(void*,const char*,const char*,const char*,
    const NpcFocusMemory*,NpcAttackSpreadResult*) noexcept;
static_assert(sizeof(NpcAttackSpreadResult)==44);
// Original slot244 spread bias and the combined slot244 -> slot243
// firing-parameter path. The host supplies a resolved enemy-memory snapshot.
inline constexpr char npc_aim_export_name[]="SiN_GetNpcAimApi";
inline constexpr char npc_aim_engine_export_name[]="SiN_QueryNpcAimFromEngine";
inline constexpr std::uint32_t npc_aim_version=1;
struct NpcAimMemory {NpcFocusMemory focus;float time_last_reacquired;};
struct NpcAimTuning {NpcFocusTuning spread;float shot_bias,pattern_focus_time;};
enum NpcBiasPath : std::uint32_t {NpcBiasUnfocused=0,NpcBiasNewEnemy,NpcBiasReacquired};
struct NpcBiasResult {float initial,bias,scale;std::uint32_t path;};
struct NpcAimResult {NpcBiasResult bias;NpcAttackSpreadResult spread;};
struct NpcAimApi {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *query)(const char*,const char*,const char*,
        const NpcAimMemory*,const NpcAimTuning*,NpcAimResult*) noexcept;
};
using GetNpcAimApi=const NpcAimApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
using QueryNpcAimFromEngine=int (SIN_FIXED_CALL *)(void*,const char*,const char*,const char*,
    const NpcAimMemory*,NpcAimResult*) noexcept;
static_assert(sizeof(NpcAimResult)==60);
// Virtual in-game shot-direction consumer. Basis vectors are supplied
// by the game host; no normalization, trace, or actual firing side effect.
inline constexpr char npc_shot_export_name[]="SiN_GetNpcShotApi";
inline constexpr char npc_shot_engine_export_name[]="SiN_QueryNpcShotFromEngine";
inline constexpr std::uint32_t npc_shot_version=1;
inline constexpr std::uint32_t npc_shot_attempt_limit=256;
struct NpcShotBasis {NpcSpread forward,right,up;};
struct NpcShotTuning {float bias_min,bias_max;};
enum NpcShotCode : std::uint32_t {
    NpcShotRejected=0,NpcShotAccepted,NpcShotRandomRefused,NpcShotInvalidSample,
    NpcShotAttemptLimit,NpcShotOverflow
};
struct NpcShotReport {
    std::uint32_t code,attempts,random_calls;
    float effective_bias,flatness,x,y;
    NpcSpread direction;
};
struct NpcShotApi {
    std::uint32_t version,size;
    // Reports attempted RNG calls even on failure. The direction is usable
    // only when code==NpcShotAccepted. No RNG rollback is possible.
    int (SIN_FIXED_CALL *sample)(const NpcShotBasis*,const NpcAimResult*,
        const NpcShotTuning*,NpcRandomFloat,void*,NpcShotReport*) noexcept;
};
using GetNpcShotApi=const NpcShotApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
struct NpcShotFromEngineResult {NpcAimResult aim;NpcShotReport shot;};
using QueryNpcShotFromEngine=int (SIN_FIXED_CALL *)(void*,void*,const char*,const char*,const char*,
    const NpcAimMemory*,const NpcShotBasis*,NpcShotFromEngineResult*) noexcept;
static_assert(sizeof(NpcShotReport)==40);
// Native map-owned SiN weapon ADS state. Actual player, viewmodel and
// animation/FOV effects remain engine-host responsibilities.
inline constexpr char weapon_export_name[]="SiN_GetWeaponBehaviorApi";
inline constexpr std::uint32_t weapon_version=1;
enum NativeWeaponKind : std::uint32_t {WeaponMagnum=0,WeaponAssaultRifle,WeaponScattergun};
struct NativeWeaponProfile {
    std::uint32_t kind;
    std::int32_t clip_size,clip2_size,default_clip,default_clip2,zoom_fov;
};
struct NativeWeaponState {std::uint32_t zoomed,restore_pending,needs_resync;};
struct NativeWeaponFrame {
    MapEntityRef owner;
    std::uint32_t has_player,player_ready,reload_active,external_block,pressed_buttons;
    // Original helper rechecks player eligibility. Host supplies that
    // observation separately from the earlier frame gate.
    std::uint32_t fov_allowed,vr_mode;
    std::int32_t current_activity,zoom_fov;
};
using NativeWeaponFov=int (SIN_FIXED_CALL *)(void*,MapEntityRef,MapEntityRef,std::int32_t,float);
using NativeWeaponActivity=int (SIN_FIXED_CALL *)(void*,MapEntityRef,std::int32_t);
struct NativeWeaponEffects {
    std::uint32_t version,size;
    NativeWeaponFov fov;
    NativeWeaponActivity activity;
};
enum NativeWeaponCode : std::uint32_t {WeaponRejected=0,WeaponUnchanged,WeaponEntered,WeaponExited,WeaponEffectsUncertain};
struct NativeWeaponReport {
    std::uint32_t code,interrupted,automatic_restore,fov_attempted,fov_delivered,activity_attempted,activity_delivered;
    NativeWeaponState state;
};
struct WeaponBehaviorApi {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *profile)(MapEntityRef,NativeWeaponProfile*) noexcept;
    int (SIN_FIXED_CALL *state)(MapEntityRef,NativeWeaponState*) noexcept;
    // Caller supplies actually observed native bits after ownership change,
    // save/load, or an uncertain callback. No foreign memory is written.
    int (SIN_FIXED_CALL *reconcile)(MapEntityRef,std::uint32_t,std::uint32_t) noexcept;
    int (SIN_FIXED_CALL *frame)(MapEntityRef,const NativeWeaponFrame*,const NativeWeaponEffects*,void*,NativeWeaponReport*) noexcept;
    int (SIN_FIXED_CALL *attack_activity)(MapEntityRef,std::uint32_t,std::int32_t,std::int32_t*) noexcept;
};
using GetWeaponBehaviorApi=const WeaponBehaviorApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
static_assert(sizeof(NativeWeaponProfile)==24);
static_assert(sizeof(NativeWeaponReport)==40);
// Original ScatterGun ammo-change/pump path after the base frame update.
// Live ammo and timers belong to the host, not duplicate DLL-owned stores.
inline constexpr char scattergun_export_name[]="SiN_GetScattergunPumpApi";
inline constexpr std::uint32_t scattergun_version=1;
struct ScattergunSnapshot {
    std::uint32_t prior_primary,prior_secondary,pending,needs_resync;
};
struct ScattergunPumpObservation {
    MapEntityRef owner;
    std::int32_t primary_clip;
    std::uint32_t secondary_state,reload_active,owner_present;
    float primary_deadline,curtime;
};
enum ScattergunTimer : std::uint32_t {ScattergunOwnerDeadline=0,ScattergunPrimaryDeadline,ScattergunSecondaryDeadline};
struct ScattergunPumpHost {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *sound)(void*,MapEntityRef,std::int32_t,float);
    int (SIN_FIXED_CALL *lookup_activity)(void*,MapEntityRef,const char*,std::int32_t*);
    int (SIN_FIXED_CALL *activity)(void*,MapEntityRef,std::int32_t);
    int (SIN_FIXED_CALL *sequence_duration)(void*,MapEntityRef,float*);
    int (SIN_FIXED_CALL *clock)(void*,float*);
    int (SIN_FIXED_CALL *write_deadline)(void*,MapEntityRef,std::uint32_t,float);
    int (SIN_FIXED_CALL *read_owner_deadline)(void*,MapEntityRef,float*);
};
enum ScattergunPumpCode : std::uint32_t {ScattergunRejected=0,ScattergunNoPump,ScattergunWaiting,ScattergunPumped,ScattergunUncertain};
struct ScattergunPumpReport {
    std::uint32_t code,primary_changed,secondary_changed,callbacks_attempted,callbacks_delivered;
    std::int32_t activity;
    float requested_deadline;
    ScattergunSnapshot state;
};
struct ScattergunPumpApi {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *state)(MapEntityRef,ScattergunSnapshot*) noexcept;
    int (SIN_FIXED_CALL *reconcile)(MapEntityRef,const ScattergunSnapshot*) noexcept;
    // Original slot209 records primary clip before its base tail-call.
    int (SIN_FIXED_CALL *before_deploy)(MapEntityRef,std::int32_t) noexcept;
    // Call after the original base frame behavior has executed. No host
    // callback is retained. Uncertain effects require observed resync.
    int (SIN_FIXED_CALL *after_base_update)(MapEntityRef,const ScattergunPumpObservation*,
        const ScattergunPumpHost*,void*,ScattergunPumpReport*) noexcept;
};
using GetScattergunPumpApi=const ScattergunPumpApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
static_assert(sizeof(ScattergunPumpReport)==44);
// Phase167 original secondary-fire control flow. This is a game host
// protocol; operations are not original engine vtable slots.
inline constexpr char scattergun_fire_export_name[]="SiN_GetScattergunSecondaryApi";
inline constexpr std::uint32_t scattergun_fire_version=1;
enum ScattergunFireQuery : std::uint32_t {
    FireOwnerCapable=0,FireOwnerMode,FireAcquireVectors,FireUsesClip,FireReadClip,
    FireSelectActivity,FireInterval,FireClock,FireAmmoType,FireOwnerAmmoCount
};
enum ScattergunFireEffect : std::uint32_t {
    FireEmpty=0,FireStateEvent,FireActivity,FirePrimaryTimer,FireSecondaryTimer,
    FireOwnerCallback,FireOwnerAnimation,FireConsumeClip,FireConsumeReserve,
    FireStoreSecondary,FirePost,FireFeedback,FirePreparePellets,FirePellet
};
// Query word is the observed integer/boolean, real the float.
// Effects use word0/word1 for IDs/counts, real0/real1 for float args.
// FireConsumeClip performs the original live decrement/dirty notification;
// FireConsumeReserve has word0=quantity, word1=ammo type. Timer effects
// use the real0 deadline, and FirePellet has word0=iteration, real0=1,
// real1=original game trace distance. The host resolves the original
// scattergun pattern; no PE address is exposed as a callable pointer.
struct ScattergunFireValue {std::uint32_t word0,word1;float real0,real1;};
struct ScattergunFireObservation {
    MapEntityRef owner;
    std::uint32_t owner_present,timing_arg,alternate_ammo_type;
};
struct ScattergunFireHost {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *query)(void*,MapEntityRef,MapEntityRef,std::uint32_t,
        std::uint32_t,ScattergunFireValue*);
    int (SIN_FIXED_CALL *effect)(void*,MapEntityRef,MapEntityRef,std::uint32_t,
        const ScattergunFireValue*);
};
enum ScattergunFireCode : std::uint32_t {
    FireRejected=0,FireMissingOwner,FireModeCooldown,FireNoVectors,
    FireEmptyClip,FireCompleted,FireUncertain
};
struct ScattergunFireReport {
    std::uint32_t code,queries_attempted,queries_delivered,effects_attempted,
        effects_delivered,pellets_delivered,needs_resync;
};
struct ScattergunSecondaryApi {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *fire)(MapEntityRef,const ScattergunFireObservation*,
        const ScattergunFireHost*,void*,ScattergunFireReport*) noexcept;
};
using GetScattergunSecondaryApi=const ScattergunSecondaryApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
static_assert(sizeof(ScattergunFireReport)==28);
// CGrenadeAR virtual game projectile. Host owns actual engine entity,
// traces, AI hearing, scheduling, damage and deferred deletion.
inline constexpr char grenade_export_name[]="SiN_GetARGrenadeApi";
inline constexpr std::uint32_t grenade_version=1;
struct GrenadeSpawn {NpcSpread origin,velocity;float time;};
struct GrenadeSnapshot {
    std::uint32_t armed,contact_flag,uncertain,removal_requested;
    std::int32_t danger_radius;
    float next_think,damage,blast_radius;
    NpcSpread origin,velocity;
};
enum GrenadeEffectKind : std::uint32_t {
    GrenadeSetContact=0,GrenadeClearContact,GrenadeFlightSound,GrenadeTraceEffect,
    GrenadeShake,GrenadeRadiusDamage,GrenadeExplosionSound,GrenadeQueueRemove
};
struct GrenadeEffect {
    std::uint32_t kind,type,channel,decal;
    NpcSpread origin;
    float damage,radius,duration,requested_amplitude,clamped_amplitude,frequency;
};
// GrenadeSetContact is the original setter request; GrenadeClearContact
// is the original detonation byte clear, NOT a claim of the same setter.
// Shake is a game effect descriptor, never a tracked-HMD pose operation.
struct GrenadeHost {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *emit)(void*,MapEntityRef,const GrenadeEffect*);
};
struct GrenadeTouch {
    std::uint32_t other_present,other_field_11c,other_flags_120,
        virtual_fc_nonnull,virtual_fc_matches,decal;
};
enum GrenadeCode : std::uint32_t {
    GrenadeRejected=0,GrenadeApplied,GrenadeNotDue,GrenadeDisarmed,GrenadeBlocked,GrenadeUncertain
};
struct GrenadeReport {
    std::uint32_t code,touch_gate,effects_attempted,effects_delivered;
    GrenadeSnapshot state;
};
struct ARGrenadeApi {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *spawn)(MapEntityRef,const GrenadeSpawn*) noexcept;
    int (SIN_FIXED_CALL *state)(MapEntityRef,GrenadeSnapshot*) noexcept;
    int (SIN_FIXED_CALL *kinematics)(MapEntityRef,const NpcSpread*,const NpcSpread*) noexcept;
    // Explicit host observation of original byte +0x795. This is distinct
    // from Runtime::armed. Never used to rearm after uncertain effects.
    int (SIN_FIXED_CALL *observe_contact)(MapEntityRef,std::uint32_t) noexcept;
    int (SIN_FIXED_CALL *think)(MapEntityRef,float,std::uint32_t,std::uint32_t,
        const GrenadeHost*,void*,GrenadeReport*) noexcept;
    int (SIN_FIXED_CALL *touch)(MapEntityRef,const GrenadeTouch*,const GrenadeHost*,void*,GrenadeReport*) noexcept;
    int (SIN_FIXED_CALL *detonate)(MapEntityRef,std::uint32_t,const GrenadeHost*,void*,GrenadeReport*) noexcept;
};
using GetARGrenadeApi=const ARGrenadeApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
static_assert(sizeof(GrenadeEffect)==52);
// Original in-game rifle secondary projectile path, with the existing
// project transaction policy. The engine owns its ammo and entity shell;
// native registry publication follows successful host commit.
inline constexpr char rifle_launch_export_name[]="SiN_GetRifleGrenadeApi";
inline constexpr std::uint32_t rifle_launch_version=1;
enum RifleOwnerKind : std::uint32_t {RifleOwnerUnknown=0,RifleOwnerPlayer,RifleOwnerGrunt};
enum RifleAmmoKind : std::uint32_t {RifleAmmoInvalid=0,RifleAmmoFinite,RifleAmmoUnknown};
struct RifleLaunchObservation {
    std::uint32_t owner_kind,live_owner_and_weapon,correct_owner,selected,
        reload_active,holstered,grunt_authorized,ammo_kind;
    std::int32_t ammo_remaining;
};
struct RifleLaunchInput {
    MapEntityRef owner;
    NpcSpread origin,direction;
    float time;
};
// prepare success returns a fresh generation-safe identity of an
// UNPUBLISHED engine shell. False means no staged shell exists.
// commit false promises no publication or ammo debit; abort then
// removes only that staged shell. Exceptions mean ownership uncertain.
struct RifleLaunchHost {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *authorize)(void*,MapEntityRef,MapEntityRef,RifleLaunchObservation*);
    int (SIN_FIXED_CALL *prepare)(void*,MapEntityRef,const char*,const char*,const GrenadeSpawn*,MapEntityRef*);
    int (SIN_FIXED_CALL *commit)(void*,MapEntityRef,MapEntityRef,std::int32_t);
    int (SIN_FIXED_CALL *abort)(void*,MapEntityRef);
};
enum RifleLaunchCode : std::uint32_t {
    RifleRejected=0,RifleUnauthorized,RifleAmmoUnverified,RifleNoAmmo,
    RifleInvalidInput,RiflePrepareRefused,RifleCapacityRefused,RifleCommitRefused,
    RifleLaunched,RifleLaunchUncertain
};
struct RifleLaunchReport {
    std::uint32_t code,prepared,attached,commit_attempted,committed,abort_attempted,aborted,uncertain;
    MapEntityRef projectile;
    float multiplier;
    std::int32_t planned_debit;
};
struct RifleGrenadeApi {
    std::uint32_t version,size;
    int (SIN_FIXED_CALL *launch)(MapEntityRef,const RifleLaunchInput*,const RifleLaunchHost*,void*,RifleLaunchReport*) noexcept;
    // 0=clear, 1=uncertain, -1=not a live spawned rifle. Uncertain
    // launches require external investigation, not automated replay.
    int (SIN_FIXED_CALL *uncertain)(MapEntityRef) noexcept;
};
using GetRifleGrenadeApi=const RifleGrenadeApi* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
static_assert(sizeof(RifleLaunchReport)==52);
using GetApi=const Api* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
static_assert(sizeof(State)==24);
static_assert(sizeof(ChallengeOutput)==16);
} // namespace sin_port::fixed_service
