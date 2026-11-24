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
using GetApi=const Api* (SIN_FIXED_CALL *)(std::uint32_t,std::uint32_t) noexcept;
static_assert(sizeof(State)==24);
static_assert(sizeof(ChallengeOutput)==16);
} // namespace sin_port::fixed_service
