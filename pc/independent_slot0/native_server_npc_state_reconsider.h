#pragma once
#include "native_server_npc_schedule_state.h"
#include "native_server_weapon_activity.h"
namespace sin_port::native_weapon_tables {
struct ServerWeaponScriptView; // Opaque script-view dependency of Elite reward bindings.
inline constexpr char server_npc_state_reconsider_export[]="SiN_GetServerNpcStateReconsiderApi";
struct ServerNpcStateBindings {
    ServerNpcScheduleBindings schedule;
    ServerWeaponActivityBindings entities;
    void* context;
    int (__cdecl *clear_enemy)(void*,void*) noexcept; // 2203e740(0,1)
    int (__cdecl *state_changed)(void*,void*,int,int) noexcept; // +4d0, optional on authenticated native route (RET 8)
    int (__cdecl *lost_enemy)(void*,void*,void*) noexcept; // 22358810
    int (__cdecl *squad_enemy)(void*,std::uint32_t,void*) noexcept; // 220a9520
    int (__cdecl *state_query)(void*,void*,int,int*) noexcept; // optional native direct branches; unresolved damage/sound/script leaves require callback
};
struct ServerNpcStateResult {int current,ideal;unsigned reconsider,changed,enemy_cleared,patrol_set,lost_enemy_called,squad_called;};
struct ServerNpcStateApi {
    std::uint32_t version,size;
    int (__cdecl *should_reconsider)(void*,unsigned,const ServerNpcStateBindings*,ServerNpcStateResult*) noexcept;
    int (__cdecl *choose_ideal)(void*,unsigned,const ServerNpcStateBindings*,ServerNpcStateResult*) noexcept;
    int (__cdecl *apply)(void*,unsigned,int,float,const ServerNpcStateBindings*,ServerNpcStateResult*) noexcept;
};
using GetServerNpcStateApi=const ServerNpcStateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_enemy_state_export[]="SiN_GetServerNpcEnemyStateApi";
struct ServerNpcEnemyStateBindings {
    void* context;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    ServerNpcConditionMapBindings conditions;
    int (__cdecl *enemy_changing)(void*,void*) noexcept; // actor +6e0
    int (__cdecl *release_squad_slot)(void*,std::uint32_t,void*,int) noexcept; // 220aaff0
    int (__cdecl *entity_handle)(void*,void*,std::uint32_t*) noexcept; // entity +8
};
struct ServerNpcEnemyStateResult {
    std::uint32_t old_handle,new_handle;
    unsigned changed,notified,squad_released,new_condition_set,new_condition_cleared,reset_fields;
};
struct ServerNpcEnemyConditionResult {
    unsigned prior_preserved,cleared,restored;
};
struct ServerNpcEnemyStateApi {
    std::uint32_t version,size;
    // Original 2203e740: target may be null; notify controls condition 0x19.
    int (__cdecl *set)(void*,unsigned,void*,bool,float,const ServerNpcEnemyStateBindings*,ServerNpcEnemyStateResult*) noexcept;
    // Original Grunt 22358ea0: clear 9 conditions and preserve prior 0x15.
    int (__cdecl *clear_combat_conditions)(void*,unsigned,const ServerNpcConditionMapBindings*,ServerNpcEnemyConditionResult*) noexcept;
};
using GetServerNpcEnemyStateApi=const ServerNpcEnemyStateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_enemy_notify_export[]="SiN_GetServerNpcEnemyNotifyApi";
struct ServerNpcEnemyNotifyContext {
    std::uint32_t original_vtable;
    unsigned actor_bytes;
    ServerNpcConditionMapBindings conditions;
    unsigned calls;
    ServerNpcEnemyConditionResult last;
};
struct ServerNpcEnemyNotifyApi {
    std::uint32_t version,size;
    // Original Grunt +6e0 = 22358ea0. Callback compatible with enemy_changing.
    int (__cdecl *notify)(void*,void*) noexcept;
};
using GetServerNpcEnemyNotifyApi=const ServerNpcEnemyNotifyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_enemy_memory_export[]="SiN_GetServerNpcEnemyMemoryApi";
// Keys are original PE32 entity-address tokens, NOT serialized entity handles.
// The caller maps original 32-bit storage tokens to bounded native objects.
struct ServerNpcEnemyMemoryBindings {
    void* context;
    int (__cdecl *storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
};
struct ServerNpcEnemyMemoryLookup {
    std::uint32_t key,record;
    unsigned index,visited,fallback_attempted,fallback_used,matched;
};
enum ServerNpcEnemyMemoryTimestamp : unsigned {
    NpcMemoryStamp1c=0,NpcMemoryStamp20=1,NpcMemoryStamp24=2,NpcMemoryStamp2c=3,NpcMemoryStamp30=4
};
struct ServerNpcEnemyMemoryApi {
    std::uint32_t version,size;
    // 2206b1a0: indexed tree + default-key retry when first key absent.
    int (__cdecl *lookup)(const void*,unsigned,std::uint32_t,bool,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryLookup*) noexcept;
    // 2206b400/450/4a0/560/5b0. Null input gives 0; missing record gives -FLT_MAX.
    int (__cdecl *timestamp)(const void*,unsigned,std::uint32_t,unsigned,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryLookup*,float*) noexcept;
    // 2206b360/3b0: cache exact vector bits in owner+24 or +30; miss preserves cache.
    int (__cdecl *position)(void*,unsigned,std::uint32_t,bool,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryLookup*,float[3]) noexcept;
    // 2206b270 writes record+2c if found.
    int (__cdecl *touch)(const void*,unsigned,std::uint32_t,float,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryLookup*) noexcept;
    // 2206b4f0: null false; missing record true; otherwise stamp1c+owner1c>now.
    int (__cdecl *recent)(const void*,unsigned,std::uint32_t,float,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryLookup*,bool*) noexcept;
};
using GetServerNpcEnemyMemoryApi=const ServerNpcEnemyMemoryApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_enemy_memory_query_export[]="SiN_GetServerNpcEnemyMemoryQueryApi";
struct ServerNpcEnemyMemoryQueryBindings {
    ServerNpcEnemyMemoryBindings storage;
    void* context;
    // Translate live native object pointer to the PE32 address token used by the tree.
    int (__cdecl *entity_key)(void*,void*,std::uint32_t*) noexcept;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
};
struct ServerNpcEnemyMemoryQueryApi {
    std::uint32_t version,size;
    // Callback-compatible with observation and enemy-refresh memory_record.
    // Context is a pointer to ServerNpcEnemyMemoryQueryBindings.
    int (__cdecl *last_seen)(void*,std::uint32_t,void*,bool*,float*) noexcept;
    // 22031780: resolve +8a0 then query +984 position cache.
    int (__cdecl *aim_vector)(void*,unsigned,const ServerNpcEnemyMemoryQueryBindings*,ServerNpcEnemyMemoryLookup*,float[3]) noexcept;
};
using GetServerNpcEnemyMemoryQueryApi=const ServerNpcEnemyMemoryQueryApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_best_enemy_export[]="SiN_GetServerNpcBestEnemyApi";
struct ServerNpcBestEnemyBindings {
    ServerNpcEnemyMemoryQueryBindings memory;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *disposition)(void*,void*,void*,int*) noexcept; // +3c0
    int (__cdecl *valid)(void*,void*,void*,bool*) noexcept; // +51c
    int (__cdecl *unreachable)(void*,void*,void*,bool*) noexcept; // +620
    int (__cdecl *priority)(void*,void*,void*,int*) noexcept; // +3c4
    int (__cdecl *origin)(void*,void*,float[3]) noexcept;
    int (__cdecl *sensed)(void*,std::uint32_t,void*,bool*) noexcept; // 220a2410
    int (__cdecl *visible)(void*,void*,void*,unsigned,bool*) noexcept; // +1f0
    int (__cdecl *component_best)(void*,std::uint32_t,void**) noexcept; // +9c
};
struct ServerNpcBestEnemyResult {
    void* enemy;
    int priority,distance_squared;
    unsigned unreachable,visible,visited,stale,dead,disposition_rejected,invalid,old,not_yet_valid,eluded,feared,unreachable_rejected,replaced,component_called;
};
struct ServerNpcBestEnemyApi {
    std::uint32_t version,size;
    // 2203d3e0 / 2216e1f0. Storage/tree must remain stable during callbacks.
    int (__cdecl *select)(void*,unsigned,float,const ServerNpcBestEnemyBindings*,ServerNpcBestEnemyResult*) noexcept;
};
using GetServerNpcBestEnemyApi=const ServerNpcBestEnemyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_unreachable_enemy_export[]="SiN_GetServerNpcUnreachableEnemyApi";
struct ServerNpcUnreachableEnemyBindings {
    ServerNpcEnemyMemoryBindings storage;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    void* context;
    int (__cdecl *origin)(void*,void*,float[3]) noexcept;
};
struct ServerNpcUnreachableEnemyResult {
    unsigned unreachable,visited,stale_removed,expired_removed,moved_removed,origin_called;
    int initial_count,final_count;
    double distance_squared;
};
struct ServerNpcUnreachableEnemyApi {
    std::uint32_t version,size;
    // 22041c30: actor+914 points to 20-byte records, +920 is count.
    int (__cdecl *query)(void*,unsigned,void*,float,const ServerNpcUnreachableEnemyBindings*,ServerNpcUnreachableEnemyResult*) noexcept;
};
using GetServerNpcUnreachableEnemyApi=const ServerNpcUnreachableEnemyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_best_enemy_query_export[]="SiN_GetServerNpcBestEnemyQueryApi";
struct ServerNpcBestEnemyQueryContext {
    const ServerNpcBestEnemyBindings* bindings;
    unsigned actor_bytes;
    float now;
    ServerNpcBestEnemyResult last;
};
struct ServerNpcBestEnemyQueryApi {
    std::uint32_t version,size;
    // Callback-compatible with Task135 select_enemy; context is per invocation.
    int (__cdecl *select)(void*,void*,std::uint32_t,void**) noexcept;
};
using GetServerNpcBestEnemyQueryApi=const ServerNpcBestEnemyQueryApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_enemy_memory_record_export[]="SiN_GetServerNpcEnemyMemoryRecordApi";
struct ServerNpcEnemyMemoryRecordResult {
    unsigned initialized,seen_written,position_written,changed_stamp_written,first_seen_written,eluded_cleared;
    double distance_squared;
};
struct ServerNpcEnemyMemoryExpiryResult {
    unsigned remove,stale,dead,protected_record,expired;
};
struct ServerNpcEnemyMemoryRecordApi {
    std::uint32_t version,size;
    // 22068c80 / new-record path of 2206bba0. Allocation and insertion external.
    int (__cdecl *initialize)(void*,unsigned,std::uint32_t,const float[3],const float[3],float,float,float,bool,ServerNpcEnemyMemoryRecordResult*) noexcept;
    // Existing-record path of 2206bba0; no fallback record lookup.
    int (__cdecl *update)(void*,unsigned,const float[3],float,bool,ServerNpcEnemyMemoryRecordResult*) noexcept;
    // 22068ce0. Caller supplies live-handle resolution and NPC state query.
    int (__cdecl *expiry)(const void*,unsigned,bool,bool,int,float,float,ServerNpcEnemyMemoryExpiryResult*) noexcept;
};
using GetServerNpcEnemyMemoryRecordApi=const ServerNpcEnemyMemoryRecordApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_enemy_memory_maintenance_export[]="SiN_GetServerNpcEnemyMemoryMaintenanceApi";
struct ServerNpcEnemyMemoryMaintenanceBindings {
    ServerNpcEnemyMemoryBindings storage;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    void* context;
    int (__cdecl *npc_state)(void*,void*,bool*,int*) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *origin)(void*,void*,float[3]) noexcept;
    // Original pool free then indexed-tree unlink, preserving other live indices.
    int (__cdecl *erase)(void*,void*,unsigned,unsigned,std::uint32_t) noexcept;
};
struct ServerNpcEnemyMemoryMaintenanceResult {
    unsigned visited,removed,stale,dead,expired,position_refreshed,secondary_refreshed,origin_calls;
};
struct ServerNpcEnemyMemoryMaintenanceApi {
    std::uint32_t version,size;
    // 2206b9f0: periodic in-order sweep, successor captured before removal.
    int (__cdecl *maintain)(void*,unsigned,float,const ServerNpcEnemyMemoryMaintenanceBindings*,ServerNpcEnemyMemoryMaintenanceResult*) noexcept;
};
using GetServerNpcEnemyMemoryMaintenanceApi=const ServerNpcEnemyMemoryMaintenanceApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_enemy_memory_tree_export[]="SiN_GetServerNpcEnemyMemoryTreeApi";
struct ServerNpcEnemyMemoryTreeResult {
    unsigned index,root,count,free_head,high_water,black_height,rotations,reused,changed;
    std::uint32_t key,record;
};
struct ServerNpcEnemyMemoryTreeApi {
    std::uint32_t version,size;
    // Validate unique-key bounded tree and free-list. No allocator ownership.
    int (__cdecl *validate)(void*,unsigned,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryTreeResult*) noexcept;
    // Preallocated storage only; caller owns record token allocation/free.
    int (__cdecl *insert)(void*,unsigned,std::uint32_t,std::uint32_t,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryTreeResult*) noexcept;
    int (__cdecl *erase_index)(void*,unsigned,unsigned,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryTreeResult*) noexcept;
};
using GetServerNpcEnemyMemoryTreeApi=const ServerNpcEnemyMemoryTreeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_enemy_memory_mutation_export[]="SiN_GetServerNpcEnemyMemoryMutationApi";
struct ServerNpcEnemyMemoryMutationBindings {
    ServerNpcEnemyMemoryBindings storage;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    void* context;
    // Explicit pool ownership; allocate returns nonzero token and bounded record.
    int (__cdecl *allocate)(void*,unsigned,std::uint32_t*,void**,unsigned*) noexcept;
    int (__cdecl *release)(void*,std::uint32_t) noexcept;
    int (__cdecl *entity_handle)(void*,void*,std::uint32_t*) noexcept;
    int (__cdecl *entity_key)(void*,void*,std::uint32_t*) noexcept;
    float constructor_vector[3];
};
struct ServerNpcEnemyMemoryMutationResult {
    ServerNpcEnemyMemoryTreeResult tree;
    ServerNpcEnemyMemoryRecordResult record;
    unsigned created,updated,removed,allocated,released;
    std::uint32_t key,token,creation_counter;
};
struct ServerNpcEnemyMemoryMutationApi {
    std::uint32_t version,size;
    // 2206bba0: exact-key update or allocation/insert. No fallback on update.
    int (__cdecl *upsert)(void*,unsigned,void*,const float[3],float,float,bool,const ServerNpcEnemyMemoryMutationBindings*,ServerNpcEnemyMemoryMutationResult*) noexcept;
    // 2206bdd0: remove exact key (including key0); pool release then unlink.
    int (__cdecl *remove)(void*,unsigned,std::uint32_t,const ServerNpcEnemyMemoryMutationBindings*,ServerNpcEnemyMemoryMutationResult*) noexcept;
    // Callback-compatible with periodic maintenance; context is MutationBindings.
    int (__cdecl *erase_record)(void*,void*,unsigned,unsigned,std::uint32_t) noexcept;
};
using GetServerNpcEnemyMemoryMutationApi=const ServerNpcEnemyMemoryMutationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_enemy_memory_control_export[]="SiN_GetServerNpcEnemyMemoryControlApi";
enum ServerNpcEnemyMemoryControl : unsigned {
    NpcMemoryExists=0,NpcMemoryEluded=1,NpcMemoryMarkEluded=2,NpcMemorySetEligibility=3,NpcMemorySetProtected=4
};
struct ServerNpcEnemyMemoryControlResult {
    ServerNpcEnemyMemoryLookup lookup;
    unsigned value,written;
};
struct ServerNpcEnemyMemoryControlApi {
    std::uint32_t version,size;
    // 2206b040: empty memory owner; optional adapter preallocation uses caller-owned storage.
    int (__cdecl *initialize)(void*,unsigned,std::uint32_t,unsigned,const float[3],const ServerNpcEnemyMemoryBindings*) noexcept;
    // Exact lookup only: 2206b290/300/600/660/6c0.
    int (__cdecl *control)(void*,unsigned,std::uint32_t,unsigned,float,std::uint8_t,const ServerNpcEnemyMemoryBindings*,ServerNpcEnemyMemoryControlResult*) noexcept;
};
using GetServerNpcEnemyMemoryControlApi=const ServerNpcEnemyMemoryControlApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_relation_export[]="SiN_GetServerNpcGruntRelationApi";
struct ServerNpcGruntRelationBindings {
    ServerNpcEnemyMemoryBindings storage;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    void* context;
    std::uint32_t original_vtable;
    int (__cdecl *classify)(void*,void*,int*) noexcept; // +d0
    int (__cdecl *matrix_row)(void*,int,const void**,unsigned*) noexcept; // original 15 rows of 16-byte records
    int (__cdecl *is_player)(void*,void*,bool*) noexcept; // +f4
    int (__cdecl *special_target)(void*,void*,bool*) noexcept; // +120
    int (__cdecl *as_npc)(void*,void*,void**) noexcept; // 22139f60
    int (__cdecl *npc_identity)(void*,void*,std::uint32_t*,unsigned*) noexcept;
    int (__cdecl *other_npc_gate)(void*,void*,void*,bool*) noexcept; // unverified +520
    int (__cdecl *component_valid)(void*,std::uint32_t,void*,bool*) noexcept; // +98
};
struct ServerNpcGruntRelationResult {
    int disposition,priority,actor_class,target_class;
    unsigned source,index,visited,class_queries,overridden,player_called,special_called;
};
struct ServerNpcGruntValidityResult {unsigned valid,component_called,npc_found,native_npc_gate,external_gate;};
struct ServerNpcGruntRelationApi {
    std::uint32_t version,size;
    // 222a7e20, 223585b0, 222a7f70. Source: 0 null, 1 entity, 2 class, 3 matrix.
    int (__cdecl *relation)(void*,unsigned,void*,bool,const ServerNpcGruntRelationBindings*,ServerNpcGruntRelationResult*) noexcept;
    // 2216e1d0 / 22030de0 / known Grunt +520=22030e10.
    int (__cdecl *valid)(void*,unsigned,void*,const ServerNpcGruntRelationBindings*,ServerNpcGruntValidityResult*) noexcept;
};
using GetServerNpcGruntRelationApi=const ServerNpcGruntRelationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_relation_matrix_export[]="SiN_GetServerNpcRelationMatrixApi";
struct ServerNpcRelationCell {std::uint32_t handle;int classification,disposition,priority;};
static_assert(sizeof(ServerNpcRelationCell)==16);
struct ServerNpcRelationMatrixApi {
    std::uint32_t version,size;
    // Original 222d3ae0 defaults. Caller owns all 225 mutable cells.
    int (__cdecl *initialize)(ServerNpcRelationCell*,unsigned) noexcept;
    int (__cdecl *set)(ServerNpcRelationCell*,unsigned,int,int,int,int) noexcept;
    // Callback-compatible with relation.matrix_row; context points to 225 cells.
    int (__cdecl *row)(void*,int,const void**,unsigned*) noexcept;
};
using GetServerNpcRelationMatrixApi=const ServerNpcRelationMatrixApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_best_enemy_export[]="SiN_GetServerNpcGruntBestEnemyApi";
struct ServerNpcGruntBestEnemyResult {
    ServerNpcBestEnemyResult selection;
    unsigned disposition_queries,priority_queries,validity_queries,cache_queries;
};
struct ServerNpcGruntBestEnemyApi {
    std::uint32_t version,size;
    // Native relation/validity/cache replace the corresponding BestEnemy callbacks.
    int (__cdecl *select)(void*,unsigned,float,const ServerNpcBestEnemyBindings*,const ServerNpcGruntRelationBindings*,const ServerNpcUnreachableEnemyBindings*,ServerNpcGruntBestEnemyResult*) noexcept;
};
using GetServerNpcGruntBestEnemyApi=const ServerNpcGruntBestEnemyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_memory_report_export[]="SiN_GetServerNpcGruntMemoryReportApi";
struct ServerNpcGruntMemoryReportBindings {
    ServerNpcEnemyMemoryMutationBindings mutation;
    void* context;
    std::uint32_t original_vtable;
    float idle_delay,alert_delay;
    int (__cdecl *cue)(void*,void*,const char*,int,int) noexcept;
    // Original squad member +678; receives reporting actor as informer.
    int (__cdecl *forward)(void*,void*,void*,const float[3],void*) noexcept;
};
struct ServerNpcGruntMemoryReportResult {
    ServerNpcEnemyMemoryMutationResult mutation;
    unsigned retail_return,blocked,no_memory,firsthand,refind_cue,idle_reset,squad_members,forwarded;
    float delay;
};
struct ServerNpcGruntMemoryReportApi {
    std::uint32_t version,size;
    // Original Grunt +678 22358590 -> 22030520; retail return means new record.
    int (__cdecl *report)(void*,unsigned,void*,const float[3],void*,float,const ServerNpcGruntMemoryReportBindings*,ServerNpcGruntMemoryReportResult*) noexcept;
};
using GetServerNpcGruntMemoryReportApi=const ServerNpcGruntMemoryReportApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_enemy_choice_gate_export[]="SiN_GetServerNpcGruntEnemyChoiceGateApi";
enum ServerNpcEnemyChoiceReason : unsigned {
    NpcChoiceRetain=0,NpcChoiceNoEnemy,NpcChoiceMemoryChanged,NpcChoiceEluded,NpcChoiceDisposition,NpcChoiceInvalid,NpcChoiceCondition,NpcChoiceDeathTimer,NpcChoicePeriodicCondition,NpcChoiceNeverSeen,NpcChoiceOldSight,NpcChoiceMissingRecord,NpcChoiceEligibility
};
struct ServerNpcGruntEnemyChoiceGateBindings {
    ServerNpcGruntRelationBindings relation;
    ServerNpcEnemyMemoryQueryBindings memory;
    ServerNpcConditionMapBindings conditions;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *random)(void*,float,float,float*) noexcept;
};
struct ServerNpcGruntEnemyChoiceGateResult {
    unsigned choose,reason,condition_tests,counter_written,death_timer_started,periodic_timer_written;
    int condition;
    std::uint32_t counter;
};
struct ServerNpcGruntEnemyChoiceGateApi {
    std::uint32_t version,size;
    // Original +524=2203e870 on all eight Grunt-family classes.
    int (__cdecl *query)(void*,unsigned,float,const ServerNpcGruntEnemyChoiceGateBindings*,ServerNpcGruntEnemyChoiceGateResult*) noexcept;
};
using GetServerNpcGruntEnemyChoiceGateApi=const ServerNpcGruntEnemyChoiceGateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_enemy_switch_export[]="SiN_GetServerNpcGruntEnemySwitchApi";
struct ServerNpcGruntEnemySwitchBindings {
    ServerNpcGruntEnemyChoiceGateBindings gate;
    ServerNpcBestEnemyBindings best;
    ServerNpcUnreachableEnemyBindings cache;
    ServerNpcEnemyStateBindings enemy;
    ServerNpcScheduleBindings schedule;
    void* context;
    int (__cdecl *lost_cue)(void*,void*) noexcept; // +5a0, original speech cooldown
    int (__cdecl *fire_output)(void*,void*,unsigned) noexcept; // 22156900, actor+b08/ac0
};
struct ServerNpcGruntEnemySwitchResult {
    ServerNpcGruntEnemyChoiceGateResult gate;
    ServerNpcGruntBestEnemyResult selection;
    ServerNpcEnemyStateResult assignment;
    unsigned had_enemy,has_enemy,forced_missing,eluded,new_allowed,lost_allowed,deferred,selected,assigned,dead_condition,lost_condition,lost_cue,extra_squad_release,outputs;
    unsigned old_flags,new_flags;
};
struct ServerNpcGruntEnemySwitchApi {
    std::uint32_t version,size;
    // Original 2203ead0: schedule permission, choice gate, BestEnemy, setter.
    int (__cdecl *update)(void*,unsigned,float,const ServerNpcGruntEnemySwitchBindings*,ServerNpcGruntEnemySwitchResult*) noexcept;
};
using GetServerNpcGruntEnemySwitchApi=const ServerNpcGruntEnemySwitchApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_speech_export[]="SiN_GetServerNpcGruntSpeechApi";
enum ServerNpcGruntSpeechKind : unsigned {NpcGruntLost=0,NpcGruntRefind=1,NpcGruntAlert=2};
struct ServerNpcGruntSpeechBindings {
    ServerNpcEnemyMemoryQueryBindings memory;
    void* context;
    std::uint32_t original_vtable;
    // Original 220a35c0. Return adapter success separately from signed speech result.
    int (__cdecl *emit)(void*,void*,unsigned,const char*,int,int,int*) noexcept;
    int (__cdecl *random)(void*,float,float,float*) noexcept;
};
struct ServerNpcGruntSpeechResult {
    const char* cue;
    unsigned suppressed,emitted,cooldown_written,enemy_live,memory_found;
    int speech_result;
    float last_seen,deadline;
};
struct ServerNpcGruntSpeechApi {
    std::uint32_t version,size;
    // 223588b0,22358950,22358970; emit receives actor and speech member offset cc0.
    int (__cdecl *speak)(void*,unsigned,unsigned,float,const ServerNpcGruntSpeechBindings*,ServerNpcGruntSpeechResult*) noexcept;
};
using GetServerNpcGruntSpeechApi=const ServerNpcGruntSpeechApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_sensed_entities_export[]="SiN_GetServerNpcSensedEntitiesApi";
struct ServerNpcSensedEntitiesBindings {
    ServerNpcEnemyMemoryBindings storage;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
};
struct ServerNpcSensedCursor {std::uint32_t packed;};
struct ServerNpcSensedResult {
    void* entity;
    unsigned visited,stale,category,index,first_stale;
};
struct ServerNpcSensedEntitiesApi {
    std::uint32_t version,size;
    // 220a1f70: -1 spans all three categories; first stale entry terminates.
    int (__cdecl *first)(const void*,unsigned,int,const ServerNpcSensedEntitiesBindings*,ServerNpcSensedCursor*,ServerNpcSensedResult*) noexcept;
    // 220a1ff0: skip stale subsequent handles.
    int (__cdecl *next)(const void*,unsigned,const ServerNpcSensedEntitiesBindings*,ServerNpcSensedCursor*,ServerNpcSensedResult*) noexcept;
    // 220a2410: exact entity membership using first/next.
    int (__cdecl *contains)(const void*,unsigned,void*,const ServerNpcSensedEntitiesBindings*,bool*) noexcept;
};
using GetServerNpcSensedEntitiesApi=const ServerNpcSensedEntitiesApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_visual_conditions_export[]="SiN_GetServerNpcGruntVisualConditionsApi";
struct ServerNpcGruntVisualBindings {
    ServerNpcSensedEntitiesBindings senses;
    ServerNpcGruntRelationBindings relation;
    ServerNpcGruntMemoryReportBindings report;
    ServerNpcConditionMapBindings conditions;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    // Called only when entity+b8 has dirty absolute-origin bit 11.
    int (__cdecl *refresh_transform)(void*,void*) noexcept;
};
struct ServerNpcGruntVisualResult {
    unsigned cleared,visited,stale,first_stale,special_mode,special_seen,current_seen,neutral,hostile,feared,friendly,unknown,report_calls,records_created,transforms;
    unsigned condition_sets;
};
struct ServerNpcGruntVisualApi {
    std::uint32_t version,size;
    // 22358600: base 22039d30 or special Grunt mode, native memory report.
    int (__cdecl *gather)(void*,unsigned,float,const ServerNpcGruntVisualBindings*,ServerNpcGruntVisualResult*) noexcept;
};
using GetServerNpcGruntVisualApi=const ServerNpcGruntVisualApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_hearing_export[]="SiN_GetServerNpcGruntHearingApi";
struct ServerNpcGruntHearingBindings {
    ServerNpcEnemyMemoryBindings storage;
    ServerNpcScheduleBindings schedule;
    void* context;
    std::uint32_t original_vtable;
    int (__cdecl *sound)(void*,int,const void**,unsigned*) noexcept; // 221b7c50
    int (__cdecl *fire_output)(void*,void*,unsigned) noexcept;
    int (__cdecl *sound_visible)(void*,void*,const void*,unsigned,bool*) noexcept;
    int (__cdecl *component_ignored)(void*,std::uint32_t,const void*,bool*) noexcept;
};
struct ServerNpcGruntHearingResult {
    unsigned visited,cleared,sets,unknown,ignored,danger_suppressed,special_suppressed,outputs;
};
struct ServerNpcGruntHearingFilterResult {unsigned accepted,visible_called,component_called,forced;};
struct ServerNpcGruntHearingApi {
    std::uint32_t version,size;
    // 223587d0 and 223587e0 / 2202f810.
    unsigned (__cdecl *mask)() noexcept;
    int (__cdecl *filter)(void*,unsigned,const void*,unsigned,const ServerNpcGruntHearingBindings*,ServerNpcGruntHearingFilterResult*) noexcept;
    // 22358e60 -> 22039fa0: accepted sound list to conditions and outputs.
    int (__cdecl *gather)(void*,unsigned,float,const ServerNpcGruntHearingBindings*,ServerNpcGruntHearingResult*) noexcept;
};
using GetServerNpcGruntHearingApi=const ServerNpcGruntHearingApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_sound_scan_export[]="SiN_GetServerNpcGruntSoundScanApi";
struct ServerNpcGruntSoundScanBindings {
    ServerNpcGruntHearingBindings hearing;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    void* context;
    int (__cdecl *active_head)(void*,int*) noexcept;
    int (__cdecl *sound_mutable)(void*,int,void**,unsigned*) noexcept;
    int (__cdecl *eye_position)(void*,void*,float[3]) noexcept;
};
struct ServerNpcGruntSoundScanResult {
    ServerNpcGruntHearingResult hearing;
    unsigned visited,accepted,mask_rejected,self_rejected,state_rejected,disabled_rejected,range_rejected,filter_rejected,blocked,eye_calls;
    int accepted_head;
};
struct ServerNpcGruntSoundScanApi {
    std::uint32_t version,size;
    // 220a1c50 / 220a1a80: construct accepted list, then native +4f4 gather.
    int (__cdecl *scan)(void*,unsigned,float,const ServerNpcGruntSoundScanBindings*,ServerNpcGruntSoundScanResult*) noexcept;
};
using GetServerNpcGruntSoundScanApi=const ServerNpcGruntSoundScanApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_perception_tick_export[]="SiN_GetServerNpcGruntPerceptionTickApi";
struct ServerNpcGruntPerceptionTickBindings {
    ServerNpcGruntVisualBindings visual;
    ServerNpcGruntSoundScanBindings sound;
    ServerNpcEnemyMemoryMaintenanceBindings maintenance;
    ServerNpcGruntEnemySwitchBindings enemy_switch;
    void* context;
    // 220a2a40,220a2c00,220a2f80: engine scene candidate generation.
    int (__cdecl *rebuild_category)(void*,void*,unsigned,int) noexcept;
};
struct ServerNpcGruntPerceptionTickResult {
    ServerNpcGruntVisualResult visual;
    ServerNpcGruntSoundScanResult sound;
    ServerNpcEnemyMemoryMaintenanceResult maintenance;
    ServerNpcGruntEnemySwitchResult enemy_switch;
    unsigned rebuilt,categories,completed;
    int radius;
};
struct ServerNpcGruntPerceptionTickApi {
    std::uint32_t version,size;
    // Actual ordered slice 2204579e..220457c1; not the outer NPC think routine.
    int (__cdecl *tick)(void*,unsigned,float,const ServerNpcGruntPerceptionTickBindings*,ServerNpcGruntPerceptionTickResult*) noexcept;
};
using GetServerNpcGruntPerceptionTickApi=const ServerNpcGruntPerceptionTickApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_combat_visibility_export[]="SiN_GetServerNpcGruntCombatVisibilityApi";
struct ServerNpcGruntCombatVisibilityBindings {
    ServerNpcEnemyMemoryQueryBindings memory;
    ServerNpcConditionMapBindings conditions;
    void* context;
    std::uint32_t original_vtable;
    int (__cdecl *trace)(void*,void*,void*,unsigned,bool*,void**) noexcept; // base 2213f7a0
    int (__cdecl *in_view)(void*,void*,void*,bool*) noexcept; // +300; +4ec is constant true
    int (__cdecl *special_target)(void*,void*,bool*) noexcept;
    int (__cdecl *entity_handle)(void*,void*,std::uint32_t*) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    // Value output carries entity handle; plain output has no handle.
    int (__cdecl *fire_output)(void*,void*,unsigned,bool,std::uint32_t) noexcept;
};
struct ServerNpcGruntCombatVisibilityResult {
    unsigned suppressed,traced,visible,in_view,previous_visible,dead,cleared,sets,outputs;
    std::uint32_t blocker;
};
struct ServerNpcGruntCombatVisibilityApi {
    std::uint32_t version,size;
    // +1f0 wrapper 22358420, then base trace unless eluded suppression.
    int (__cdecl *query)(void*,unsigned,void*,unsigned,const ServerNpcGruntCombatVisibilityBindings*,ServerNpcGruntCombatVisibilityResult*) noexcept;
    // 2203c600..2203ca14 prefix, terminates on dead target before range computation.
    int (__cdecl *gather)(void*,unsigned,void*,const ServerNpcGruntCombatVisibilityBindings*,ServerNpcGruntCombatVisibilityResult*) noexcept;
};
using GetServerNpcGruntCombatVisibilityApi=const ServerNpcGruntCombatVisibilityApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_combat_range_export[]="SiN_GetServerNpcGruntCombatRangeApi";
struct ServerNpcGruntCombatRangeBindings {
    ServerNpcGruntCombatVisibilityBindings visibility;
    ServerNpcGruntMemoryReportBindings report;
    void* context;
    int (__cdecl *center)(void*,void*,float[3]) noexcept; // +214
    int (__cdecl *origin)(void*,void*,float[3]) noexcept; // 220092d0
    int (__cdecl *velocity)(void*,void*,float[3]) noexcept; // +1e4
    int (__cdecl *random)(void*,float,float,float*) noexcept;
    int (__cdecl *enemy_facing_actor)(void*,void*,void*,bool*) noexcept; // target+fc then +300
};
struct ServerNpcGruntCombatRangeResult {
    ServerNpcGruntCombatVisibilityResult visibility;
    ServerNpcGruntMemoryReportResult report;
    unsigned dead,reported,firsthand,velocity_queries,facing_actor,weapon_range_used,too_far,attack_gate;
    float distance,threshold,observation[3];
};
struct ServerNpcGruntCombatRangeApi {
    std::uint32_t version,size;
    // 22035140: center distance with vertical overlap correction.
    int (__cdecl *distance)(void*,unsigned,void*,const ServerNpcGruntCombatRangeBindings*,float*) noexcept;
    // 2203c600..2203cc92, then 2203c590 attack gate only. Later attack/path work separate.
    int (__cdecl *gather)(void*,unsigned,void*,float,const ServerNpcGruntCombatRangeBindings*,ServerNpcGruntCombatRangeResult*) noexcept;
};
using GetServerNpcGruntCombatRangeApi=const ServerNpcGruntCombatRangeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_attack_conditions_export[]="SiN_GetServerNpcGruntAttackConditionsApi";
struct ServerNpcGruntAttackBindings {
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    ServerNpcConditionMapBindings conditions;
    void* context;
    std::uint32_t original_vtable;
    int (__cdecl *origin)(void*,void*,float[3]) noexcept;
    int (__cdecl *forward)(void*,void*,float[3]) noexcept;
    int (__cdecl *aim_point)(void*,void*,const float[3],float[3]) noexcept;
    int (__cdecl *eye)(void*,void*,float[3]) noexcept;
    int (__cdecl *line)(void*,void*,const float[3],const float[3],bool*) noexcept; // +6e8, set conditions
    int (__cdecl *weapon_caps)(void*,void*,unsigned*) noexcept; // weapon +4a0
    int (__cdecl *weapon_condition)(void*,void*,unsigned,float,float,int*) noexcept; // +4ac/b0/b4/b8
    // Original +6c8 trace from centers, mask46004003; hit entity returned.
    int (__cdecl *melee_trace)(void*,void*,void*,unsigned,void**) noexcept;
};
struct ServerNpcGruntAttackResult {
    float dot,distance;
    unsigned capabilities,weapon_capabilities,weapon_queries,native_queries,line_calls,line_clear,cleanup_calls,sets,suppressed,range_cleared,melee_traced;
    int conditions[4];
};
struct ServerNpcGruntAttackApi {
    std::uint32_t version,size;
    // 2203b440: caller already passed 2203c590 attack gate.
    int (__cdecl *gather)(void*,unsigned,void*,float,const ServerNpcGruntAttackBindings*,ServerNpcGruntAttackResult*) noexcept;
};
using GetServerNpcGruntAttackApi=const ServerNpcGruntAttackApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_combat_conditions_export[]="SiN_GetServerNpcGruntCombatConditionsApi";
struct ServerNpcGruntCombatConditionsBindings {
    ServerNpcGruntCombatRangeBindings range;
    ServerNpcGruntAttackBindings attack;
    ServerNpcUnreachableEnemyBindings cache;
    void* context;
    // 22035280 / 220353f0: navigation refresh policies not reconstructed here.
    int (__cdecl *navigation_update)(void*,void*,unsigned,float) noexcept;
    // +1ec visibility of remembered position; actual engine trace is explicit.
    int (__cdecl *point_visible)(void*,void*,const float[3],unsigned,bool*) noexcept;
};
struct ServerNpcGruntCombatConditionsResult {
    ServerNpcGruntCombatRangeResult range;
    ServerNpcGruntAttackResult attack;
    ServerNpcUnreachableEnemyResult cache;
    unsigned completed,attack_called,cleanup_called,nav_calls,unreachable_set,old_sight,close_mark,point_mark,point_queries;
    float last_seen,horizontal_distance,remembered[3];
};
struct ServerNpcGruntCombatConditionsApi {
    std::uint32_t version,size;
    // 2203c600..2203cea7; navigation/trace/weapon leaves explicitly bound.
    int (__cdecl *gather)(void*,unsigned,void*,float,const ServerNpcGruntCombatConditionsBindings*,ServerNpcGruntCombatConditionsResult*) noexcept;
};
using GetServerNpcGruntCombatConditionsApi=const ServerNpcGruntCombatConditionsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_goal_refresh_export[]="SiN_GetServerNpcGruntGoalRefreshApi";
struct ServerNpcGruntGoalRefreshBindings {
    ServerNpcEnemyMemoryQueryBindings memory;
    void* context;
    std::uint32_t original_vtable;
    // Shared retail deadline 224dbe0c, one instance per simulation.
    float* global_deadline;
    int (__cdecl *origin)(void*,void*,float[3]) noexcept;
    int (__cdecl *velocity)(void*,void*,float[3]) noexcept;
    int (__cdecl *eye)(void*,void*,float[3]) noexcept;
    int (__cdecl *special)(void*,void*,bool*) noexcept;
    int (__cdecl *retarget)(void*,std::uint32_t,void*) noexcept; // nav +18
    int (__cdecl *refresh)(void*,std::uint32_t,bool*) noexcept; // 2207a5c0(0,0)
    int (__cdecl *failure)(void*,void*,int) noexcept; // +47c
};
struct ServerNpcGruntGoalRefreshResult {
    unsigned skipped_mode,skipped_timer,skipped_type,retargeted,threshold_called,refreshed,failed,deadline_written,goal_type,goal_flags;
    float goal_delta[3],point[3],threshold,deviation;
};
struct ServerNpcGruntGoalRefreshApi {
    std::uint32_t version,size;
    // phase 0=22035280 enemy; phase 1=220353f0 target.
    int (__cdecl *update)(void*,unsigned,unsigned,float,const ServerNpcGruntGoalRefreshBindings*,ServerNpcGruntGoalRefreshResult*) noexcept;
};
using GetServerNpcGruntGoalRefreshApi=const ServerNpcGruntGoalRefreshApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_combat_goal_export[]="SiN_GetServerNpcGruntCombatGoalApi";
struct ServerNpcGruntCombatGoalApi {
    std::uint32_t version,size;
    // Same original combat sequence with native enemy/target goal policies in place of navigation_update.
    int (__cdecl *gather)(void*,unsigned,void*,float,const ServerNpcGruntCombatConditionsBindings*,const ServerNpcGruntGoalRefreshBindings*,ServerNpcGruntCombatConditionsResult*,ServerNpcGruntGoalRefreshResult[2]) noexcept;
};
using GetServerNpcGruntCombatGoalApi=const ServerNpcGruntCombatGoalApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_weapon_search_export[]="SiN_GetServerNpcGruntWeaponSearchApi";
struct ServerNpcGruntWeaponSearchBindings {
    ServerNpcGruntRelationBindings relation;
    ServerNpcScheduleBindings schedule;
    ServerNpcEnemyMemoryBindings storage;
    void* context;
    int max_clients;
    std::uint32_t special_squad_name; // original interned string token 22546d58
    int (__cdecl *local_player)(void*,void**) noexcept;
    int (__cdecl *weapon_caps)(void*,void*,unsigned*) noexcept;
    int (__cdecl *squad_predicate)(void*,void*,bool*) noexcept; // 220a85d0
    int (__cdecl *find_weapon)(void*,void*,const float[3],void**) noexcept; // 222a5a90
};
struct ServerNpcGruntWeaponSearchResult {
    unsigned eligible,cooldown_blocked,capability_blocked,combat_blocked,schedule_blocked,player_checked,friendly,armed_blocked,disabled,search_called,found,condition_set;
    float deadline,half_extents[3];
    void* weapon;
};
struct ServerNpcGruntWeaponSearchApi {
    std::uint32_t version,size;
    int (__cdecl *eligible)(void*,unsigned,float,const ServerNpcGruntWeaponSearchBindings*,ServerNpcGruntWeaponSearchResult*) noexcept;
    int (__cdecl *search)(void*,unsigned,float,const ServerNpcGruntWeaponSearchBindings*,ServerNpcGruntWeaponSearchResult*) noexcept;
};
using GetServerNpcGruntWeaponSearchApi=const ServerNpcGruntWeaponSearchApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_aux_conditions_export[]="SiN_GetServerNpcGruntAuxConditionsApi";
struct ServerNpcGruntAuxConditionsBindings {
    ServerNpcGruntCombatVisibilityBindings visibility;
    ServerNpcGruntGoalRefreshBindings goals;
    void* context;
    int (__cdecl *weapon_predicate)(void*,void*,unsigned,bool*) noexcept; // weapon +330/334/43c
    int (__cdecl *as_npc)(void*,void*,void**) noexcept; // 22139f60
};
struct ServerNpcGruntAuxConditionsResult {
    unsigned target_live,target_visible,target_condition,target_refresh,weapon_live,primary_checked,secondary_checked,secondary_uses_ammo,primary_empty,secondary_empty,snapshot,script_live,script_clear;
    ServerNpcGruntGoalRefreshResult goal;
};
struct ServerNpcGruntAuxConditionsApi {
    std::uint32_t version,size;
    // 2203ceb0: visible/occluded target condition, then native target goal update.
    int (__cdecl *target)(void*,unsigned,float,const ServerNpcGruntAuxConditionsBindings*,ServerNpcGruntAuxConditionsResult*) noexcept;
    // 2202d560: set ammo conditions; does not clear existing conditions.
    int (__cdecl *ammo)(void*,unsigned,const ServerNpcGruntAuxConditionsBindings*,ServerNpcGruntAuxConditionsResult*) noexcept;
    // 2203b110 and 221aecf0: snapshot/mask, scripted actor condition cleanup.
    int (__cdecl *snapshot)(void*,unsigned,const ServerNpcGruntAuxConditionsBindings*,ServerNpcGruntAuxConditionsResult*) noexcept;
};
using GetServerNpcGruntAuxConditionsApi=const ServerNpcGruntAuxConditionsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_post_update_export[]="SiN_GetServerNpcGruntPostUpdateApi";
struct ServerNpcGruntFlinchBindings {
    ServerNpcScheduleBindings schedule;
    void* context;
    int (__cdecl *select_activity)(void*,void*,bool,int,int*) noexcept;
    int (__cdecl *can_animate)(void*,void*,bool*) noexcept;
    int (__cdecl *activity_exists)(void*,void*,int,bool*) noexcept;
    int (__cdecl *play_activity)(void*,void*,int,int,int) noexcept;
    int (__cdecl *random)(void*,float,float,float*) noexcept;
};
struct ServerNpcGruntFlinchResult {
    unsigned heavy,light,interrupted,called,played,heavy_cleared,timer_started,timer_cleared;
    int activity;
    float deadline;
};
struct ServerNpcGruntPostUpdateBindings {
    ServerNpcGruntWeaponSearchBindings search;
    ServerNpcGruntCombatConditionsBindings combat;
    ServerNpcGruntAuxConditionsBindings aux;
    ServerNpcEnemyStateBindings enemy;
    ServerNpcGruntFlinchBindings flinch;
    ServerNpcScheduleBindings schedule;
};
struct ServerNpcGruntPostUpdateResult {
    ServerNpcGruntWeaponSearchResult search;
    ServerNpcGruntCombatConditionsResult combat;
    ServerNpcGruntGoalRefreshResult combat_goals[2];
    ServerNpcGruntAuxConditionsResult target,ammo,snapshot;
    ServerNpcGruntFlinchResult flinch;
    ServerNpcEnemyStateResult cleared_enemy;
    unsigned completed,skipped_state,skipped_perception,cleared_sense_conditions,new_enemy_condition,combat_called,enemy_cleared;
};
struct ServerNpcGruntPostUpdateApi {
    std::uint32_t version,size;
    // 2203bc60 and 2204e5e0: damage condition / flinch timer policy.
    int (__cdecl *flinch)(void*,unsigned,float,const ServerNpcGruntFlinchBindings*,ServerNpcGruntFlinchResult*) noexcept;
    // 220457c1..22045924, including 22045835 skipped-perception and 220458dc state0/7 branches.
    // Perception branch decision is supplied explicitly by caller, not recomputed here.
    int (__cdecl *update)(void*,unsigned,float,bool,const ServerNpcGruntPostUpdateBindings*,ServerNpcGruntPostUpdateResult*) noexcept;
};
using GetServerNpcGruntPostUpdateApi=const ServerNpcGruntPostUpdateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_update_gate_export[]="SiN_GetServerNpcGruntUpdateGateApi";
struct ServerNpcGruntUpdateGateBindings {
    ServerNpcEnemyMemoryBindings storage;
    ServerNpcConditionMapBindings conditions;
    void* context;
    std::uint32_t original_vtable;
    int (__cdecl *pvs_index)(void*,unsigned,bool*) noexcept; // 222895a0
    int (__cdecl *component_gate)(void*,std::uint32_t,bool*) noexcept; // +c0, mutable handled byte+8
    int (__cdecl *pre_perception)(void*,void*) noexcept; // 2203b8f0, separate ground/hazard policy
    int (__cdecl *random_int)(void*,int,int,int*) noexcept;
    int (__cdecl *idle_speech)(void*,void*,unsigned,const char*,int,int,int*) noexcept;
};
struct ServerNpcGruntUpdateGateResult {
    unsigned inactive,forced,force_cleared,yaw_flag_cleared,pvs_checked,pvs,condition_one,disabled,component_called,component_handled,gate_value,perceive,pre_called,idle_roll,idle_emitted;
    float yaw,desired_yaw,yaw_delta;
    int speech_result;
};
struct ServerNpcGruntUpdateGateApi {
    std::uint32_t version,size;
    // 220456b0..2204579e, minus cycle-counter diagnostics.
    int (__cdecl *prepare)(void*,unsigned,const ServerNpcGruntUpdateGateBindings*,ServerNpcGruntUpdateGateResult*) noexcept;
};
using GetServerNpcGruntUpdateGateApi=const ServerNpcGruntUpdateGateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_condition_update_export[]="SiN_GetServerNpcGruntConditionUpdateApi";
struct ServerNpcGruntConditionUpdateBindings {
    ServerNpcGruntUpdateGateBindings gate;
    ServerNpcGruntPerceptionTickBindings perception;
    ServerNpcGruntPostUpdateBindings post;
};
struct ServerNpcGruntConditionUpdateResult {
    ServerNpcGruntUpdateGateResult gate;
    ServerNpcGruntPerceptionTickResult perception;
    ServerNpcGruntPostUpdateResult post;
    unsigned completed,perception_called,post_called;
};
struct ServerNpcGruntConditionUpdateApi {
    std::uint32_t version,size;
    // 220456b0..22045924, not the outer NPC think; engine leaves remain bound.
    int (__cdecl *update)(void*,unsigned,float,const ServerNpcGruntConditionUpdateBindings*,ServerNpcGruntConditionUpdateResult*) noexcept;
};
using GetServerNpcGruntConditionUpdateApi=const ServerNpcGruntConditionUpdateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_gather_conditions_export[]="SiN_GetServerNpcGruntGatherConditionsApi";
struct ServerNpcGruntGatherConditionsBindings {
    ServerNpcGruntConditionUpdateBindings base;
    void* context;
    int max_clients;
    int (__cdecl *component_update)(void*,std::uint32_t) noexcept; // component +24
    int (__cdecl *classname_matches)(void*,void*,const char*,bool*) noexcept;
    // 2235b790 search leaf, owns original actor+cb4 mutation.
    int (__cdecl *find_object)(void*,void*,unsigned,float) noexcept;
    int (__cdecl *point_predicate)(void*,void*,const float[3],bool*) noexcept; // +788=2235b050
    int (__cdecl *local_player)(void*,void**) noexcept;
    int (__cdecl *player_interaction)(void*,void*,void*,bool*) noexcept; // player +494
};
struct ServerNpcGruntGatherConditionsResult {
    ServerNpcGruntConditionUpdateResult base;
    unsigned completed,base_called,component_called,hopper_exit,search_called,search_no_enemy,object_live,object_condition,point_called,point_schedule_blocked,point_condition,player_called,player_condition;
    float point[3];
};
struct ServerNpcGruntGatherConditionsApi {
    std::uint32_t version,size;
    // All eight Grunt-family original +420=2235dba0.
    int (__cdecl *gather)(void*,unsigned,float,const ServerNpcGruntGatherConditionsBindings*,ServerNpcGruntGatherConditionsResult*) noexcept;
};
using GetServerNpcGruntGatherConditionsApi=const ServerNpcGruntGatherConditionsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_point_policy_export[]="SiN_GetServerNpcGruntPointPolicyApi";
struct ServerNpcGruntPointPolicyBindings {
    ServerNpcEnemyMemoryBindings storage;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    void* context;
    std::uint32_t original_vtable;
    float invalid_solution[3]; // original global vector 225885b8
    int (__cdecl *origin)(void*,void*,float[3]) noexcept;
    int (__cdecl *notify_squad)(void*,const float[3],unsigned,int,float) noexcept;
    int (__cdecl *in_view)(void*,void*,const float[3],bool*) noexcept;
    int (__cdecl *visible)(void*,void*,const float[3],unsigned,bool*) noexcept;
    int (__cdecl *headroom)(void*,void*,bool*) noexcept;
    // Original engine solution leaves 220b3660 / 220b3050, no geometry implementation.
    int (__cdecl *solve)(void*,void*,const float[3],bool,float[3]) noexcept;
};
struct ServerNpcGruntPointPolicyResult {
    unsigned accepted,no_inventory,cooldown_blocked,range_blocked,scalar_blocked,squad_visited,squad_stale,squad_blocked,notified,in_view,visible,headroom_called,headroom_clear,solve_called,direct,deadline_written;
    float distance,deadline,solution[3];
};
struct ServerNpcGruntPointPolicyApi {
    std::uint32_t version,size;
    int (__cdecl *query)(void*,unsigned,const float[3],float,const ServerNpcGruntPointPolicyBindings*,ServerNpcGruntPointPolicyResult*) noexcept;
};
using GetServerNpcGruntPointPolicyApi=const ServerNpcGruntPointPolicyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_gather_point_export[]="SiN_GetServerNpcGruntGatherPointApi";
struct ServerNpcGruntGatherPointApi {
    std::uint32_t version,size;
    // Original +420 wrapper with native +788 policy, retaining separate scene leaves.
    int (__cdecl *gather)(void*,unsigned,float,const ServerNpcGruntGatherConditionsBindings*,const ServerNpcGruntPointPolicyBindings*,ServerNpcGruntGatherConditionsResult*,ServerNpcGruntPointPolicyResult*) noexcept;
};
using GetServerNpcGruntGatherPointApi=const ServerNpcGruntGatherPointApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_object_search_export[]="SiN_GetServerNpcGruntObjectSearchApi";
struct ServerNpcGruntObjectSearchBindings {
    ServerNpcGruntCombatVisibilityBindings visibility;
    void* context;
    // Scene enumeration in original order; list remains caller-owned during search.
    int (__cdecl *enumerate)(void*,const float[3],const float[3],const std::uint32_t**,unsigned*) noexcept;
    int (__cdecl *origin)(void*,void*,float[3]) noexcept;
    int (__cdecl *center)(void*,void*,float[3]) noexcept;
    int (__cdecl *eye)(void*,void*,float[3]) noexcept;
    int (__cdecl *actor_reference_height)(void*,void*,float*) noexcept; // 22200350 (.5,.5,.25)
    int (__cdecl *object_properties)(void*,void*,bool*,float*,bool*,bool*) noexcept; // +218, +38,+10,+dc
    int (__cdecl *world_model_top)(void*,void*,bool*,float*) noexcept;
    int (__cdecl *attached_object)(void*,void*,bool*) noexcept; // +c4 then +2a4
    int (__cdecl *point_test)(void*,void*,const float[3],unsigned,bool*) noexcept; // +48
    int (__cdecl *classname_matches)(void*,void*,const char*,bool*) noexcept;
    int (__cdecl *as_physics_prop)(void*,void*,void**) noexcept;
};
struct ServerNpcGruntObjectSearchResult {
    unsigned completed,no_enemy,enemy_range_blocked,enumerated,stale,property_rejected,qualified,visited,distance_rejected,direction_rejected,enemy_distance_rejected,height_rejected,model_rejected,visibility_rejected,attachment_rejected,point_rejected,class_rejected,prop_rejected,replaced,found;
    float enemy_distance,search_radius,reference_height,best_distance,mins[3],maxs[3];
    std::uint32_t selected_handle;
};
struct ServerNpcGruntObjectSearchApi {
    std::uint32_t version,size;
    // Original 2235b790 and enumeration filter 22358d60.
    int (__cdecl *search)(void*,unsigned,int,const ServerNpcGruntObjectSearchBindings*,ServerNpcGruntObjectSearchResult*) noexcept;
};
using GetServerNpcGruntObjectSearchApi=const ServerNpcGruntObjectSearchApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_gather_native_export[]="SiN_GetServerNpcGruntGatherNativeApi";
struct ServerNpcGruntGatherNativeBindings {
    ServerNpcGruntGatherConditionsBindings gather;
    ServerNpcGruntPointPolicyBindings point;
    ServerNpcGruntObjectSearchBindings object;
};
struct ServerNpcGruntGatherNativeResult {
    ServerNpcGruntGatherConditionsResult gather;
    ServerNpcGruntPointPolicyResult point;
    ServerNpcGruntObjectSearchResult object;
};
struct ServerNpcGruntGatherNativeApi {
    std::uint32_t version,size;
    int (__cdecl *gather)(void*,unsigned,float,const ServerNpcGruntGatherNativeBindings*,ServerNpcGruntGatherNativeResult*) noexcept;
};
using GetServerNpcGruntGatherNativeApi=const ServerNpcGruntGatherNativeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_preschedule_export[]="SiN_GetServerNpcGruntPrescheduleApi";
struct ServerNpcGruntPrescheduleBindings {
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    std::uint32_t original_vtable;
    ServerNpcConditionMapBindings conditions;
    void* context;
    float pending_range; // original cvar 225980d8 +28
    int (__cdecl *origin)(void*,void*,float[3]) noexcept;
    int (__cdecl *center)(void*,void*,float[3]) noexcept; // +214
    int (__cdecl *trace_fraction)(void*,void*,const float[3],const float[3],unsigned,float*) noexcept;
    int (__cdecl *as_pending_prop)(void*,void*,const void**,unsigned*) noexcept;
    int (__cdecl *entity_handle)(void*,void*,std::uint32_t*) noexcept;
    int (__cdecl *component_tick)(void*,std::uint32_t) noexcept; // +4c
    int (__cdecl *speech_queue_tick)(void*,void*,float) noexcept; // 220a3370 on actor+cc0
    int (__cdecl *rule_query)(void*,bool*) noexcept; // game rules +170
    int (__cdecl *death)(void*,void*) noexcept; // +1b4
};
struct ServerNpcGruntPrescheduleResult {
    unsigned completed,height_written,component_called,speech_called,rule_called,death_called,condition46_written;
    unsigned pending_live,enemy_live,height_rejected,range_rejected,direction_rejected,trace_rejected,cast_rejected,state_rejected,deadline_rejected,accepted,condition4c_written;
    float height_delta,distance,dot,trace_fraction;
    std::uint32_t target_handle;
};
struct ServerNpcGruntPrescheduleApi {
    std::uint32_t version,size;
    // Base Grunt 2235df10 (subclass-specific prefixes are separate).
    int (__cdecl *run)(void*,unsigned,float,const ServerNpcGruntPrescheduleBindings*,ServerNpcGruntPrescheduleResult*) noexcept;
    // Pure gameplay predicate of 22359a70, no handoff or condition write.
    int (__cdecl *pending)(void*,unsigned,float,const ServerNpcGruntPrescheduleBindings*,ServerNpcGruntPrescheduleResult*) noexcept;
};
using GetServerNpcGruntPrescheduleApi=const ServerNpcGruntPrescheduleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_speech_queue_export[]="SiN_GetServerNpcGruntSpeechQueueApi";
struct ServerNpcGruntSpeechQueueBindings {
    std::uint32_t original_vtable;
    void* context;
    // Original 22359170/22359180 read mutable globals; initial file values 1 and 75.
    float volume;
    int level;
    int (__cdecl *squad_storage)(void*,std::uint32_t,void**,unsigned*) noexcept;
    // 221b37c0: engine-owned sentence delivery. The raw member +8 is the final argument.
    int (__cdecl *deliver)(void*,std::uint32_t,int,float,int,int,std::uint32_t) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
};
struct ServerNpcGruntSpeechQueueResult {
    unsigned completed,empty,expired,priority_blocked,state_blocked,squad_used,delivered,owner_written,squad_written,cleared;
    int index,priority;
    float owner_deadline,squad_deadline;
};
struct ServerNpcGruntSpeechQueueApi {
    std::uint32_t version,size;
    int (__cdecl *tick)(void*,unsigned,float,const ServerNpcGruntSpeechQueueBindings*,ServerNpcGruntSpeechQueueResult*) noexcept;
    // Same original preschedule sequence with native 220a3370 in place of the legacy callback.
    int (__cdecl *preschedule)(void*,unsigned,float,const ServerNpcGruntPrescheduleBindings*,const ServerNpcGruntSpeechQueueBindings*,ServerNpcGruntPrescheduleResult*,ServerNpcGruntSpeechQueueResult*) noexcept;
};
using GetServerNpcGruntSpeechQueueApi=const ServerNpcGruntSpeechQueueApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_sentence_export[]="SiN_GetServerNpcGruntSentenceApi";
struct ServerNpcGruntSentenceBindings {
    ServerNpcGruntSpeechQueueBindings speech;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    // Original entity +204, embedded +fc virtual +2c, entity virtual +694.
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *component_eligible)(void*,void*,bool*) noexcept;
    int (__cdecl *entity_rejected)(void*,void*,bool*) noexcept;
    // 220a34f0: bounded string transformation and engine sentence request.
    // The signed request result is distinct from adapter success.
    int (__cdecl *enqueue)(void*,void*,const char*,int*) noexcept;
};
struct ServerNpcGruntSentenceResult {
    ServerNpcGruntSpeechQueueResult cooldown;
    unsigned completed,mode_allowed,queue_reset,queued,group_scans,eligible_count,candidate_eligible,leader_match;
    int request_result;
    void* first_eligible;
};
struct ServerNpcGruntSentenceApi {
    std::uint32_t version,size;
    int (__cdecl *gate)(void*,unsigned,unsigned,const ServerNpcGruntSentenceBindings*,ServerNpcGruntSentenceResult*) noexcept;
    int (__cdecl *dispatch)(void*,unsigned,float,const char*,int,unsigned,const ServerNpcGruntSentenceBindings*,ServerNpcGruntSentenceResult*) noexcept;
};
using GetServerNpcGruntSentenceApi=const ServerNpcGruntSentenceApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_native_speech_export[]="SiN_GetServerNpcGruntNativeSpeechApi";
struct ServerNpcGruntNativeSpeechResult {
    ServerNpcGruntSpeechResult cue;
    ServerNpcGruntSentenceResult sentence;
};
struct ServerNpcGruntSurvivorResult {
    ServerNpcGruntSentenceResult sentence;
    ServerNpcGruntSpeechQueueResult final_mark;
    const char* cue;
    unsigned completed,flag_blocked,removed_positive,eligible_count,last_member;
};
struct ServerNpcGruntNativeSpeechApi {
    std::uint32_t version,size;
    int (__cdecl *speak)(void*,unsigned,unsigned,float,const ServerNpcGruntSpeechBindings*,const ServerNpcGruntSentenceBindings*,ServerNpcGruntNativeSpeechResult*) noexcept;
    // 2235af70, after the group owner has removed a member.
    int (__cdecl *survivor)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntSentenceBindings*,ServerNpcGruntSurvivorResult*) noexcept;
};
using GetServerNpcGruntNativeSpeechApi=const ServerNpcGruntNativeSpeechApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_sentence_request_export[]="SiN_GetServerNpcGruntSentenceRequestApi";
struct ServerNpcGruntSentenceRequestBindings {
    void* context;
    bool manager_ready; // original 2256e684 nonzero
    int (__cdecl *suffix)(void*,std::uint32_t,const char**,unsigned*) noexcept; // raw actor+8e8
    int (__cdecl *group_index)(void*,const char*,int*) noexcept; // engine +84
    int (__cdecl *pick_name)(void*,int,char*,unsigned,int*) noexcept; // +74, 64 bytes
    int (__cdecl *name_index)(void*,const char*,int*) noexcept; // +7c, name+1
    // Engine submission/recipient/audio boundary 22145e00, preserving raw arguments.
    int (__cdecl *submit)(void*,std::uint32_t,int,float,int,int,std::uint32_t) noexcept;
};
struct ServerNpcGruntSentenceRequestResult {
    char group[128],selected[64];
    int group_id,pick_result,sentence_id,request_result;
    unsigned completed,suffix_used,manager_missing,group_called,pick_called,lookup_called,submitted;
};
struct ServerNpcGruntSentenceRequestApi {
    std::uint32_t version,size;
    int (__cdecl *request)(void*,unsigned,const char*,const ServerNpcGruntSpeechQueueBindings*,const ServerNpcGruntSentenceRequestBindings*,ServerNpcGruntSentenceRequestResult*) noexcept;
    int (__cdecl *dispatch)(void*,unsigned,float,const char*,int,unsigned,const ServerNpcGruntSentenceBindings*,const ServerNpcGruntSentenceRequestBindings*,ServerNpcGruntSentenceResult*,ServerNpcGruntSentenceRequestResult*) noexcept;
};
using GetServerNpcGruntSentenceRequestApi=const ServerNpcGruntSentenceRequestApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_request_speech_export[]="SiN_GetServerNpcGruntRequestSpeechApi";
struct ServerNpcGruntRequestSpeechResult {
    ServerNpcGruntNativeSpeechResult speech;
    ServerNpcGruntSentenceRequestResult request;
};
struct ServerNpcGruntRequestSurvivorResult {
    ServerNpcGruntSurvivorResult survivor;
    ServerNpcGruntSentenceRequestResult request;
};
struct ServerNpcGruntRequestSpeechApi {
    std::uint32_t version,size;
    int (__cdecl *speak)(void*,unsigned,unsigned,float,const ServerNpcGruntSpeechBindings*,const ServerNpcGruntSentenceBindings*,const ServerNpcGruntSentenceRequestBindings*,ServerNpcGruntRequestSpeechResult*) noexcept;
    int (__cdecl *survivor)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntSentenceBindings*,const ServerNpcGruntSentenceRequestBindings*,ServerNpcGruntRequestSurvivorResult*) noexcept;
};
using GetServerNpcGruntRequestSpeechApi=const ServerNpcGruntRequestSpeechApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_perception_speech_export[]="SiN_GetServerNpcGruntPerceptionSpeechApi";
struct ServerNpcGruntPerceptionSpeechBindings {
    ServerNpcGruntSentenceBindings sentence;
    ServerNpcGruntSentenceRequestBindings request;
    void* context;
    int (__cdecl *lost_random)(void*,float,float,float*) noexcept;
};
struct ServerNpcGruntPerceptionSpeechResult {
    ServerNpcGruntRequestSpeechResult lost;
    ServerNpcGruntSentenceResult idle,refind;
    ServerNpcGruntSentenceRequestResult idle_request,refind_request;
    unsigned lost_calls,idle_calls,refind_calls;
};
struct ServerNpcGruntPerceptionSpeechApi {
    std::uint32_t version,size;
    int (__cdecl *report)(void*,unsigned,void*,const float*,void*,float,const ServerNpcGruntMemoryReportBindings*,const ServerNpcGruntPerceptionSpeechBindings*,ServerNpcGruntMemoryReportResult*,ServerNpcGruntPerceptionSpeechResult*) noexcept;
    int (__cdecl *enemy_switch)(void*,unsigned,float,const ServerNpcGruntEnemySwitchBindings*,const ServerNpcGruntPerceptionSpeechBindings*,ServerNpcGruntEnemySwitchResult*,ServerNpcGruntPerceptionSpeechResult*) noexcept;
    int (__cdecl *prepare)(void*,unsigned,float,const ServerNpcGruntUpdateGateBindings*,const ServerNpcGruntPerceptionSpeechBindings*,ServerNpcGruntUpdateGateResult*,ServerNpcGruntPerceptionSpeechResult*) noexcept;
    int (__cdecl *update)(void*,unsigned,float,const ServerNpcGruntConditionUpdateBindings*,const ServerNpcGruntPerceptionSpeechBindings*,ServerNpcGruntConditionUpdateResult*,ServerNpcGruntPerceptionSpeechResult*) noexcept;
    int (__cdecl *gather)(void*,unsigned,float,const ServerNpcGruntGatherNativeBindings*,const ServerNpcGruntPerceptionSpeechBindings*,ServerNpcGruntGatherNativeResult*,ServerNpcGruntPerceptionSpeechResult*) noexcept;
};
using GetServerNpcGruntPerceptionSpeechApi=const ServerNpcGruntPerceptionSpeechApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_squad_leave_export[]="SiN_GetServerNpcGruntSquadLeaveApi";
struct ServerNpcGruntSquadLeaveBindings {
    ServerNpcGruntSentenceBindings sentence;
    ServerNpcGruntSentenceRequestBindings request;
    void* context;
    // Original primary vtable identity and native actor extent, not the replacement vptr.
    int (__cdecl *member_class)(void*,void*,std::uint32_t*,unsigned*) noexcept;
    int (__cdecl *other_notify)(void*,void*,void*) noexcept; // non-Grunt +750
};
struct ServerNpcGruntSquadLeaveResult {
    ServerNpcGruntRequestSurvivorResult last;
    unsigned completed,missing,removed,cleared,notifications,native_notifications,external_notifications,stale;
    int index,remaining;
};
struct ServerNpcGruntSquadLeaveApi {
    std::uint32_t version,size;
    // 220aa0d0: remove first live pointer match, ordered notifications, then +690 and +9c8 clear.
    int (__cdecl *leave)(void*,unsigned,std::uint32_t,std::uint32_t,bool,float,const ServerNpcGruntSquadLeaveBindings*,ServerNpcGruntSquadLeaveResult*) noexcept;
};
using GetServerNpcGruntSquadLeaveApi=const ServerNpcGruntSquadLeaveApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_squad_join_export[]="SiN_GetServerNpcGruntSquadJoinApi";
struct ServerNpcGruntSquadJoinBindings {
    ServerNpcGruntSquadLeaveBindings group;
    ServerNpcEnemyMemoryBindings source_memory;
    ServerNpcGruntMemoryReportBindings report;
    // Non-Grunt first member +670, if required; known Grunts read +984.
    int (__cdecl *other_memory)(void*,void*,std::uint32_t*) noexcept;
};
struct ServerNpcGruntSquadJoinResult {
    ServerNpcGruntSquadLeaveResult previous;
    ServerNpcGruntMemoryReportResult last_report;
    ServerNpcGruntPerceptionSpeechResult dialogue;
    unsigned completed,blocked,already_member,detached,evicted,appended,owner_written,first_resolved,records,stale,reports,created,updated;
    std::uint32_t evicted_handle;
    int index,count;
};
struct ServerNpcGruntSquadJoinApi {
    std::uint32_t version,size;
    // group_array_token is the independent representation of group+8, written at +4c.
    int (__cdecl *join)(void*,unsigned,std::uint32_t,std::uint32_t,std::uint32_t,float,const ServerNpcGruntSquadJoinBindings*,ServerNpcGruntSquadJoinResult*) noexcept;
    // Original owner wrapper 22054d10 clears +9c4 even after missing membership.
    int (__cdecl *detach)(void*,unsigned,std::uint32_t,float,const ServerNpcGruntSquadLeaveBindings*,ServerNpcGruntSquadLeaveResult*) noexcept;
};
using GetServerNpcGruntSquadJoinApi=const ServerNpcGruntSquadJoinApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_squad_registry_export[]="SiN_GetServerNpcGruntSquadRegistryApi";
struct ServerNpcGruntSquadRegistryBindings {
    ServerNpcEnemyMemoryBindings storage;
    void* context;
    int (__cdecl *name)(void*,std::uint32_t,const char**,unsigned*) noexcept;
    int (__cdecl *intern)(void*,const char*,std::uint32_t*) noexcept;
    int (__cdecl *allocate)(void*,unsigned,std::uint32_t*,std::uint32_t*,void**,unsigned*) noexcept;
    int (__cdecl *release)(void*,std::uint32_t) noexcept;
};
struct ServerNpcGruntSquadRegistryResult {
    ServerNpcGruntSquadJoinResult join;
    unsigned completed,visited,found,allocated,constructed,linked,released;
    std::uint32_t token,array_token,name_token;
};
struct ServerNpcGruntSquadRegistryApi {
    std::uint32_t version,size;
    int (__cdecl *lookup)(std::uint32_t,const char*,const ServerNpcGruntSquadRegistryBindings*,ServerNpcGruntSquadRegistryResult*) noexcept;
    int (__cdecl *create)(std::uint32_t*,const char*,const ServerNpcGruntSquadRegistryBindings*,ServerNpcGruntSquadRegistryResult*) noexcept;
    int (__cdecl *attach)(void*,unsigned,std::uint32_t,float,std::uint32_t*,const char*,const ServerNpcGruntSquadRegistryBindings*,const ServerNpcGruntSquadJoinBindings*,ServerNpcGruntSquadRegistryResult*) noexcept;
};
using GetServerNpcGruntSquadRegistryApi=const ServerNpcGruntSquadRegistryApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_squad_destroy_export[]="SiN_GetServerNpcGruntSquadDestroyApi";
struct ServerNpcGruntSquadDestroyBindings {
    ServerNpcGruntSquadRegistryBindings registry;
    void* context;
    int (__cdecl *release_records)(void*,std::uint32_t) noexcept;
};
struct ServerNpcGruntSquadDestroyResult {
    unsigned completed,visited,unlinked,empty_required,record_release_calls,external_records,inline_reset,group_released;
    std::uint32_t token,predecessor,successor;
};
struct ServerNpcGruntSquadDestroyApi {
    std::uint32_t version,size;
    // 220aa850: caller owns registry; safe native adapter requires no members.
    int (__cdecl *destroy)(std::uint32_t*,std::uint32_t,const ServerNpcGruntSquadDestroyBindings*,ServerNpcGruntSquadDestroyResult*) noexcept;
};
using GetServerNpcGruntSquadDestroyApi=const ServerNpcGruntSquadDestroyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_squad_slots_export[]="SiN_GetServerNpcGruntSquadSlotsApi";
struct ServerNpcGruntSquadSlotsBindings {
    ServerNpcEnemyMemoryBindings storage;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    void* context;
    int (__cdecl *member_class)(void*,void*,std::uint32_t*,unsigned*) noexcept;
    int (__cdecl *other_enemy)(void*,void*,void**) noexcept; // non-Grunt +154
    int (__cdecl *entity_handle)(void*,void*,std::uint32_t*) noexcept;
    // Resize an owned 8-byte record array, preserving old rows. Token must allow
    // contiguous +8*index arithmetic for original cached-pointer representation.
    int (__cdecl *resize)(void*,std::uint32_t,unsigned,std::uint32_t*,void**,unsigned*) noexcept;
};
enum ServerNpcGruntSquadBitOperation : unsigned {NpcSquadBitTest=0,NpcSquadBitSet=1,NpcSquadBitClear=2};
struct ServerNpcGruntSquadSlotsResult {
    unsigned completed,allowed,ungrouped,retained,refreshed,refresh_members,pruned,cached,scanned,appended,grown,accesses,old_unmarked,old_cleared,new_set,bit_value;
    int selected,index;
    std::uint32_t bitmap,record_token;
    float deadline;
};
struct ServerNpcGruntSquadSlotsApi {
    std::uint32_t version,size;
    int (__cdecl *get)(std::uint32_t,void*,float,const ServerNpcGruntSquadSlotsBindings*,ServerNpcGruntSquadSlotsResult*) noexcept;
    int (__cdecl *bit)(std::uint32_t,void*,int,unsigned,float,const ServerNpcGruntSquadSlotsBindings*,ServerNpcGruntSquadSlotsResult*) noexcept;
    // Original 22054d40 and 220aaeb0; singleton range is the Grunt condition gate.
    int (__cdecl *reserve)(void*,unsigned,int,int,float,const ServerNpcGruntSquadSlotsBindings*,ServerNpcGruntSquadSlotsResult*) noexcept;
    int (__cdecl *occupied)(std::uint32_t,void*,int,int,float,const ServerNpcGruntSquadSlotsBindings*,ServerNpcGruntSquadSlotsResult*) noexcept;
    int (__cdecl *release)(std::uint32_t,void*,int,float,const ServerNpcGruntSquadSlotsBindings*,ServerNpcGruntSquadSlotsResult*) noexcept;
};
using GetServerNpcGruntSquadSlotsApi=const ServerNpcGruntSquadSlotsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_perception_slots_export[]="SiN_GetServerNpcGruntPerceptionSlotsApi";
struct ServerNpcGruntPerceptionSlotsBindings {
    ServerNpcGruntPerceptionSpeechBindings dialogue;
    ServerNpcGruntSquadSlotsBindings slots;
};
struct ServerNpcGruntPerceptionSlotsResult {
    ServerNpcGruntPerceptionSpeechResult dialogue;
    ServerNpcGruntSquadSlotsResult last_release;
    unsigned release_calls;
};
struct ServerNpcGruntPerceptionSlotsApi {
    std::uint32_t version,size;
    int (__cdecl *set_enemy)(void*,unsigned,void*,bool,float,const ServerNpcEnemyStateBindings*,const ServerNpcGruntSquadSlotsBindings*,ServerNpcEnemyStateResult*,ServerNpcGruntPerceptionSlotsResult*) noexcept;
    int (__cdecl *enemy_switch)(void*,unsigned,float,const ServerNpcGruntEnemySwitchBindings*,const ServerNpcGruntPerceptionSlotsBindings*,ServerNpcGruntEnemySwitchResult*,ServerNpcGruntPerceptionSlotsResult*) noexcept;
    int (__cdecl *update)(void*,unsigned,float,const ServerNpcGruntConditionUpdateBindings*,const ServerNpcGruntPerceptionSlotsBindings*,ServerNpcGruntConditionUpdateResult*,ServerNpcGruntPerceptionSlotsResult*) noexcept;
    int (__cdecl *gather)(void*,unsigned,float,const ServerNpcGruntGatherNativeBindings*,const ServerNpcGruntPerceptionSlotsBindings*,ServerNpcGruntGatherNativeResult*,ServerNpcGruntPerceptionSlotsResult*) noexcept;
};
using GetServerNpcGruntPerceptionSlotsApi=const ServerNpcGruntPerceptionSlotsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_squad_report_export[]="SiN_GetServerNpcGruntSquadReportApi";
struct ServerNpcGruntSquadReportBindings {
    ServerNpcGruntPerceptionSlotsBindings perception;
    void* context;
    // Bind recipient's memory pool, original class, and extent. Class identity
    // must be original PE32 table, not the replacement DLL's virtual pointer.
    int (__cdecl *member_report)(void*,void*,std::uint32_t*,unsigned*,ServerNpcGruntMemoryReportBindings*) noexcept;
    int (__cdecl *other_report)(void*,void*,void*,const float*,void*) noexcept;
};
struct ServerNpcGruntSquadReportResult {
    ServerNpcGruntPerceptionSlotsResult perception;
    ServerNpcGruntMemoryReportResult last_member;
    ServerNpcGruntPerceptionSpeechResult last_dialogue;
    unsigned forwarded,native_reports,external_reports,created,updated,blocked;
};
struct ServerNpcGruntSquadReportApi {
    std::uint32_t version,size;
    int (__cdecl *report)(void*,unsigned,void*,const float*,void*,float,const ServerNpcGruntMemoryReportBindings*,const ServerNpcGruntSquadReportBindings*,ServerNpcGruntMemoryReportResult*,ServerNpcGruntSquadReportResult*) noexcept;
    int (__cdecl *update)(void*,unsigned,float,const ServerNpcGruntConditionUpdateBindings*,const ServerNpcGruntSquadReportBindings*,ServerNpcGruntConditionUpdateResult*,ServerNpcGruntSquadReportResult*) noexcept;
    int (__cdecl *gather)(void*,unsigned,float,const ServerNpcGruntGatherNativeBindings*,const ServerNpcGruntSquadReportBindings*,ServerNpcGruntGatherNativeResult*,ServerNpcGruntSquadReportResult*) noexcept;
};
using GetServerNpcGruntSquadReportApi=const ServerNpcGruntSquadReportApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_squad_enemy_export[]="SiN_GetServerNpcGruntSquadEnemyApi";
struct ServerNpcGruntSquadEnemyBindings {
    ServerNpcGruntSquadSlotsBindings slots;
    void* context;
    // Per-member class, extent, condition namespace, and memory/pointer key service.
    int (__cdecl *member)(void*,void*,std::uint32_t*,unsigned*,ServerNpcEnemyStateBindings*,ServerNpcEnemyMemoryQueryBindings*) noexcept;
    // Full foreign member path: original virtuals +154/+150/+670/+6e0 not established.
    int (__cdecl *other_member)(void*,void*,void*,float) noexcept;
};
struct ServerNpcGruntSquadEnemyResult {
    ServerNpcEnemyStateResult last_assignment;
    ServerNpcGruntPerceptionSlotsResult last_slots;
    ServerNpcEnemyMemoryLookup last_lookup;
    unsigned completed,null_target,visited,stale,native_members,external_members,same,condition_blocked,recent,unassigned,assigned;
    float last_seen;
};
struct ServerNpcGruntSquadEnemyApi {
    std::uint32_t version,size;
    // 220a9520: broadcast target to members with no enemy or old unseen enemy.
    int (__cdecl *broadcast)(std::uint32_t,void*,float,const ServerNpcGruntSquadEnemyBindings*,ServerNpcGruntSquadEnemyResult*) noexcept;
};
using GetServerNpcGruntSquadEnemyApi=const ServerNpcGruntSquadEnemyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_native_state_export[]="SiN_GetServerNpcGruntNativeStateApi";
struct ServerNpcGruntNativeStateBindings {
    ServerNpcGruntSquadEnemyBindings squad;
    ServerNpcGruntSentenceBindings sentence;
    ServerNpcGruntSentenceRequestBindings request;
    std::uint32_t original_vtable;
    void* context;
    int (__cdecl *target_classification)(void*,void*,int*) noexcept; // target +d0
};
struct ServerNpcGruntNativeStateResult {
    ServerNpcStateResult state;
    ServerNpcGruntSquadEnemyResult broadcast;
    ServerNpcEnemyStateResult clear;
    ServerNpcGruntPerceptionSlotsResult clear_slots;
    ServerNpcGruntSentenceResult sentence;
    ServerNpcGruntSentenceRequestResult request;
    const char* cue;
    int classification;
    unsigned cue_calls,broadcast_calls,clear_calls;
};
struct ServerNpcGruntNativeStateApi {
    std::uint32_t version,size;
    int (__cdecl *choose)(void*,unsigned,float,const ServerNpcStateBindings*,const ServerNpcGruntNativeStateBindings*,ServerNpcGruntNativeStateResult*) noexcept;
    int (__cdecl *apply)(void*,unsigned,int,float,const ServerNpcStateBindings*,const ServerNpcGruntNativeStateBindings*,ServerNpcGruntNativeStateResult*) noexcept;
};
using GetServerNpcGruntNativeStateApi=const ServerNpcGruntNativeStateApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_squad_input_export[]="SiN_GetServerNpcGruntSquadInputApi";
struct ServerNpcGruntSquadInputBindings {
    ServerNpcGruntSquadRegistryBindings registry;
    ServerNpcGruntSquadJoinBindings join;
    void* context;
    int (__cdecl *weapon_capabilities)(void*,void*,std::uint32_t*) noexcept;
};
struct ServerNpcGruntSquadInputResult {
    ServerNpcGruntSquadRegistryResult attachment;
    ServerNpcGruntSquadLeaveResult departure;
    unsigned completed,capability_blocked,nonstring,clearing,owner_written,weapon_queried;
    std::uint32_t name_token;
};
struct ServerNpcGruntSquadInputApi {
    std::uint32_t version,size;
    // 22032000: variant type at input+18, string token at +8; typed adapter.
    int (__cdecl *change)(void*,unsigned,std::uint32_t,float,std::uint32_t*,unsigned,std::uint32_t,const ServerNpcGruntSquadInputBindings*,ServerNpcGruntSquadInputResult*) noexcept;
};
using GetServerNpcGruntSquadInputApi=const ServerNpcGruntSquadInputApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_removal_export[]="SiN_GetServerNpcGruntRemovalApi";
struct ServerNpcGruntRemovalBindings {
    ServerNpcGruntSquadLeaveBindings leave;
    ServerNpcGruntSquadReportBindings reports;
    ServerNpcGruntMemoryReportBindings memory;
    void* context;
    // Original non-squad engine leaves, in call order.
    int (__cdecl *state4_link)(void*,void*,void*) noexcept;
    int (__cdecl *clear_link)(void*,void*,void*) noexcept;
    int (__cdecl *clear_owner)(void*,void*) noexcept;
    int (__cdecl *event)(void*,void*,void*) noexcept;
    int (__cdecl *origin)(void*,void*,float*) noexcept;
    int (__cdecl *finalize)(void*,void*) noexcept;
};
struct ServerNpcGruntRemovalResult {
    ServerNpcGruntSquadSlotsResult release;
    ServerNpcGruntSquadLeaveResult departure;
    ServerNpcEnemyMemoryLookup zero_key;
    ServerNpcGruntMemoryReportResult report;
    ServerNpcGruntSquadReportResult forwarding;
    unsigned completed,repeated,latched,state4_link,cleared_link,event,slot_released,report_checked,report_called,owner_cleared,finalized;
};
struct ServerNpcGruntRemovalApi {
    std::uint32_t version,size;
    // 22039180: ordered cleanup; caller supplies original actor handle and attacker.
    int (__cdecl *run)(void*,unsigned,std::uint32_t,void*,bool,float,const ServerNpcGruntRemovalBindings*,ServerNpcGruntRemovalResult*) noexcept;
};
using GetServerNpcGruntRemovalApi=const ServerNpcGruntRemovalApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_reaction_speech_export[]="SiN_GetServerNpcGruntReactionSpeechApi";
struct ServerNpcGruntReactionSpeechBindings {
    ServerNpcGruntSentenceBindings sentence;
    ServerNpcGruntSentenceRequestBindings request;
};
struct ServerNpcGruntReactionSpeechResult {
    ServerNpcGruntSentenceResult sentence;
    ServerNpcGruntSentenceRequestResult request;
    const char* cue;
    double ratio;
    float deadline;
    unsigned completed,blocked_flags,blocked_deadline,blocked_state,blocked_ready,blocked_damage,taunt_marked,pain_marked,deadline_written;
};
struct ServerNpcGruntReactionSpeechApi {
    std::uint32_t version,size;
    int (__cdecl *pain)(void*,unsigned,float,const ServerNpcGruntReactionSpeechBindings*,ServerNpcGruntReactionSpeechResult*) noexcept;
    int (__cdecl *death)(void*,unsigned,float,const ServerNpcGruntReactionSpeechBindings*,ServerNpcGruntReactionSpeechResult*) noexcept;
};
using GetServerNpcGruntReactionSpeechApi=const ServerNpcGruntReactionSpeechApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_event_speech_export[]="SiN_GetServerNpcGruntEventSpeechApi";
struct ServerNpcGruntEventSpeechBindings {
    ServerNpcGruntReactionSpeechBindings speech;
    void* context;
    int (__cdecl *random_int)(void*,int,int,int*) noexcept;
    int (__cdecl *enemy_target)(void*,void*,void**) noexcept; // current enemy virtual +fc
    int (__cdecl *position)(void*,void*,float*) noexcept; // +214
    int (__cdecl *target_accept)(void*,void*,void*,bool*) noexcept; // target +300
    int (__cdecl *visible)(void*,void*,void*,unsigned,unsigned,bool*) noexcept; // actor +1f0
    int (__cdecl *flank_metric)(void*,std::uint32_t,float*) noexcept; // +8f8 -> +24 -> 2209d410
};
struct ServerNpcGruntEventSpeechResult {
    ServerNpcGruntSpeechQueueResult admission;
    ServerNpcGruntSentenceResult sentence;
    ServerNpcGruntSentenceRequestResult request;
    const char* cue;
    double distance_squared;
    float metric;
    int roll;
    unsigned completed,ignored,roll_blocked,target_missing,admission_blocked,distance_blocked,target_blocked,visibility_blocked,metric_blocked;
};
struct ServerNpcGruntEventSpeechApi {
    std::uint32_t version,size;
    int (__cdecl *event)(void*,unsigned,unsigned,float,const ServerNpcGruntEventSpeechBindings*,ServerNpcGruntEventSpeechResult*) noexcept;
};
using GetServerNpcGruntEventSpeechApi=const ServerNpcGruntEventSpeechApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_squad_init_export[]="SiN_GetServerNpcGruntSquadInitApi";
struct ServerNpcGruntSquadInitBindings {
    ServerNpcGruntSquadRegistryBindings registry;
    ServerNpcGruntSquadJoinBindings join;
    void* context;
    int (__cdecl *base_initialize)(void*,void*) noexcept; // 22036120
    int (__cdecl *optional_initialize)(void*,void*) noexcept; // 22369ad0
};
struct ServerNpcGruntSquadInitResult {
    ServerNpcGruntSquadRegistryResult attachment;
    unsigned completed,capability_set,base_called,optional_called,existing_group,owner_written;
    std::uint32_t name_token;
};
struct ServerNpcGruntSquadInitApi {
    std::uint32_t version,size;
    // Source-bounded tail 22369bdb, not full Spawn. Caller attests original entry route.
    int (__cdecl *initialize)(void*,unsigned,std::uint32_t,float,std::uint32_t*,const ServerNpcGruntSquadInitBindings*,ServerNpcGruntSquadInitResult*) noexcept;
};
using GetServerNpcGruntSquadInitApi=const ServerNpcGruntSquadInitApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_behavior_removal_export[]="SiN_GetServerNpcGruntBehaviorRemovalApi";
struct ServerNpcGruntBehaviorRemovalBindings {
    ServerNpcGruntRemovalBindings removal;
    ServerNpcEnemyMemoryBindings component_storage;
    void* context;
    int (__cdecl *component_removed)(void*,std::uint32_t,void*,bool) noexcept; // +48
    int (__cdecl *component_end)(void*,std::uint32_t) noexcept; // +20
};
struct ServerNpcGruntBehaviorRemovalResult {
    ServerNpcGruntRemovalResult removal;
    ServerNpcGruntSquadSlotsResult active_release;
    unsigned completed,components,active_cleared,active_ended,slot_released;
    std::uint32_t previous_active;
};
struct ServerNpcGruntBehaviorRemovalApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,void*,bool,float,const ServerNpcGruntBehaviorRemovalBindings*,ServerNpcGruntBehaviorRemovalResult*) noexcept;
};
using GetServerNpcGruntBehaviorRemovalApi=const ServerNpcGruntBehaviorRemovalApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_damage_pain_prefix_export[]="SiN_GetServerNpcGruntDamagePainPrefixApi";
struct ServerNpcGruntDamagePainPrefixBindings {
    ServerNpcGruntReactionSpeechBindings reaction;
    void* context;
    int (__cdecl *base_damage)(void*,void*,void*,int*) noexcept; // 222a7b20, mutable damage info and native result
    int (__cdecl *state_signal)(void*,void*,int) noexcept; // actor +4d8
    int (__cdecl *as_npc)(void*,void*,bool*,unsigned*) noexcept; // +f4 and readable extent
};
struct ServerNpcGruntDamagePainPrefixResult {
    ServerNpcGruntReactionSpeechResult pain;
    int base_result;
    unsigned completed,damage_zeroed,base_called,signal_called,attacker_checked,same_squad,pain_called,tail_required;
};
struct ServerNpcGruntDamagePainPrefixApi {
    std::uint32_t version,size;
    // Original 22040140..220401d4 prefix only. Caller attests entry via the proper base route.
    int (__cdecl *run)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntDamagePainPrefixBindings*,ServerNpcGruntDamagePainPrefixResult*) noexcept;
};
using GetServerNpcGruntDamagePainPrefixApi=const ServerNpcGruntDamagePainPrefixApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_death_prefix_export[]="SiN_GetServerNpcGruntDeathPrefixApi";
struct ServerNpcGruntDeathPrefixBindings {
    ServerNpcGruntBehaviorRemovalBindings removal;
    ServerNpcGruntReactionSpeechBindings reaction;
    ServerNpcConditionMapBindings schedules;
    void* context;
    int (__cdecl *state_signal)(void*,void*,int) noexcept; // +4d8(0)
    int (__cdecl *prepare)(void*,void*,void*) noexcept; // 22042dc0
    int (__cdecl *life_change)(void*,void*) noexcept; // +1a8
    int (__cdecl *elite_link)(void*,void*) noexcept; // 22315450
    int (__cdecl *jet_release)(void*,std::uint32_t) noexcept; // 221b9770 manager +28
};
struct ServerNpcGruntDeathPrefixResult {
    ServerNpcGruntBehaviorRemovalResult removal;
    ServerNpcGruntReactionSpeechResult speech;
    unsigned completed,schedule_blocked,signal_called,prepared,life_changed,removed,subclass_called,jet_cleared,speech_called,tail_required;
    std::uint32_t attacker,jet_token;
};
struct ServerNpcGruntDeathPrefixApi {
    std::uint32_t version,size;
    // Original 22042e70..22042f07 only; caller attests entry to base death route.
    int (__cdecl *run)(void*,unsigned,std::uint32_t,void*,unsigned,float,const ServerNpcGruntDeathPrefixBindings*,ServerNpcGruntDeathPrefixResult*) noexcept;
};
using GetServerNpcGruntDeathPrefixApi=const ServerNpcGruntDeathPrefixApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_death_event_export[]="SiN_GetServerNpcGruntDeathEventApi";
struct ServerNpcGruntDeathEventBindings {
    ServerNpcGruntDeathPrefixBindings prefix;
    void* context;
    int (__cdecl *base_death)(void*,void*,void*) noexcept; // 222a7850
    int (__cdecl *activity_lookup)(void*,void*,int,int*) noexcept; // 220eebd0
    int (__cdecl *final_remove)(void*,void*) noexcept; // 22143d90
};
struct ServerNpcGruntDeathEventResult {
    ServerNpcGruntDeathPrefixResult prefix;
    ServerNpcConditionResult condition;
    int activity,previous_state;
    unsigned completed,field_cleared,base_called,activity_called,ideal_written,state_written,time_written,remove_called;
};
struct ServerNpcGruntDeathEventApi {
    std::uint32_t version,size;
    // Complete base orchestration 22042e70, not the outer subclass caller or hosted leaves.
    int (__cdecl *run)(void*,unsigned,std::uint32_t,void*,unsigned,float,const ServerNpcGruntDeathEventBindings*,ServerNpcGruntDeathEventResult*) noexcept;
};
using GetServerNpcGruntDeathEventApi=const ServerNpcGruntDeathEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_behavior_switch_export[]="SiN_GetServerNpcGruntBehaviorSwitchApi";
struct ServerNpcGruntBehaviorSwitchBindings {
    ServerNpcGruntSquadSlotsBindings slots;
    std::uint32_t original_vtable;
    std::uint8_t* activation_guard; // original 2254c0a0, shared per runtime
    void* context;
    int (__cdecl *begin)(void*,std::uint32_t) noexcept; // component +1c
    int (__cdecl *activate)(void*,std::uint32_t) noexcept; // live component +24
    int (__cdecl *end)(void*,std::uint32_t) noexcept; // old component +20
};
struct ServerNpcGruntBehaviorSwitchResult {
    ServerNpcGruntSquadSlotsResult release;
    std::uint32_t previous,requested,activated;
    unsigned completed,changed,began,activated_called,ended,slot_released;
};
struct ServerNpcGruntBehaviorSwitchApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,float,const ServerNpcGruntBehaviorSwitchBindings*,ServerNpcGruntBehaviorSwitchResult*) noexcept;
};
using GetServerNpcGruntBehaviorSwitchApi=const ServerNpcGruntBehaviorSwitchApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_death_prepared_event_export[]="SiN_GetServerNpcGruntDeathPreparedEventApi";
struct ServerNpcGruntDeathPrepareBindings {
    ServerNpcEnemyMemoryBindings dirty_storage; // resolve actor +16c when nonzero
    void* context;
    int (__cdecl *model_available)(void*,void*,bool*) noexcept; // 220eb8a0 result nonnull
    int (__cdecl *select)(void*,void*,void*,std::uint32_t,int*,std::uint32_t*) noexcept; // 22119c70
    int (__cdecl *activity_lookup)(void*,void*,int,int*) noexcept; // 220eebd0
};
struct ServerNpcGruntDeathPrepareResult {
    int selection,activity;
    std::uint32_t auxiliary;
    unsigned completed,model_available,selected,looked_up,activity_written,aux_written,local_dirty,external_dirty;
};
struct ServerNpcGruntDeathPreparedEventBindings {
    ServerNpcGruntDeathEventBindings event;
    ServerNpcGruntDeathPrepareBindings prepare;
};
struct ServerNpcGruntDeathPreparedEventResult {
    ServerNpcGruntDeathEventResult event;
    ServerNpcGruntDeathPrepareResult prepare;
    unsigned life_local_dirty,life_external_dirty;
};
struct ServerNpcGruntDeathPreparedEventApi {
    std::uint32_t version,size;
    int (__cdecl *prepare)(void*,unsigned,void*,unsigned,const ServerNpcGruntDeathPrepareBindings*,ServerNpcGruntDeathPrepareResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,void*,unsigned,float,const ServerNpcGruntDeathPreparedEventBindings*,ServerNpcGruntDeathPreparedEventResult*) noexcept;
};
using GetServerNpcGruntDeathPreparedEventApi=const ServerNpcGruntDeathPreparedEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_state_signal_export[]="SiN_GetServerNpcGruntStateSignalApi";
struct ServerNpcGruntStateSignalBindings {
    ServerNpcGruntSentenceBindings members; // group storage, original class, serial registry, eligibility
    ServerNpcEnemyMemoryBindings dirty_storage;
    void* context;
    int (__cdecl *refresh)(void*,void*) noexcept; // actor +58
    int (__cdecl *output)(void*,void*,unsigned) noexcept; // 22156900 embedded +bc8, actor as both entities
    int (__cdecl *member_signal)(void*,void*,int) noexcept; // other entity +4d8
    int (__cdecl *jet_prepare)(void*,void*) noexcept; // 22366a20
    int (__cdecl *jet_object)(void*,std::uint32_t) noexcept; // actor+218 object +8
};
struct ServerNpcGruntStateSignalResult {
    unsigned completed,state_cleared,flag_cleared,local_dirty,external_dirty,refreshed,output_called;
    unsigned scans,eligible,dead,self,propagated,jet_prepared,jet_linked,jet_object_called;
    std::uint32_t jet_handle;
};
struct ServerNpcGruntStateSignalApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,int,const ServerNpcGruntStateSignalBindings*,ServerNpcGruntStateSignalResult*) noexcept;
};
using GetServerNpcGruntStateSignalApi=const ServerNpcGruntStateSignalApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_combat_reactions_export[]="SiN_GetServerNpcGruntCombatReactionsApi";
struct ServerNpcGruntCombatDamageBindings {
    ServerNpcGruntDamagePainPrefixBindings damage;
    ServerNpcGruntStateSignalBindings signal;
};
struct ServerNpcGruntCombatDamageResult {
    ServerNpcGruntDamagePainPrefixResult damage;
    ServerNpcGruntStateSignalResult signal;
};
struct ServerNpcGruntCombatDeathBindings {
    ServerNpcGruntDeathPreparedEventBindings death;
    ServerNpcGruntStateSignalBindings signal;
};
struct ServerNpcGruntCombatDeathResult {
    ServerNpcGruntDeathPreparedEventResult death;
    ServerNpcGruntStateSignalResult signal;
};
struct ServerNpcGruntCombatReactionsApi {
    std::uint32_t version,size;
    int (__cdecl *damage)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntCombatDamageBindings*,ServerNpcGruntCombatDamageResult*) noexcept;
    int (__cdecl *death)(void*,unsigned,std::uint32_t,void*,unsigned,float,const ServerNpcGruntCombatDeathBindings*,ServerNpcGruntCombatDeathResult*) noexcept;
};
using GetServerNpcGruntCombatReactionsApi=const ServerNpcGruntCombatReactionsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_damage_outputs_export[]="SiN_GetServerNpcGruntDamageOutputsApi";
struct ServerNpcGruntDamageOutputBindings {
    ServerNpcGruntSentenceBindings entities;
    ServerNpcEnemyMemoryBindings names; // group+4 name pointer token
    ServerNpcConditionMapBindings conditions;
    std::uint32_t player_group_name; // original global 22546d58
    int max_clients;
    void* context;
    int (__cdecl *output)(void*,void*,unsigned) noexcept; // 22156900, actor both entities, delay 0
    int (__cdecl *is_player)(void*,void*,bool*) noexcept; // attacker +120
    int (__cdecl *cast_npc)(void*,void*,void**,unsigned*) noexcept; // original RTTI cast, may return null
    int (__cdecl *local_player)(void*,void**) noexcept; // 22288a40
};
struct ServerNpcGruntDamageOutputResult {
    ServerNpcConditionResult condition;
    unsigned completed,stamp_equal,generic_output,player_output,qualified_output,half_output,cast_called,local_player_called,eligible,tail_required;
};
struct ServerNpcGruntDamageOutputChainBindings {
    ServerNpcGruntCombatDamageBindings prefix;
    ServerNpcGruntDamageOutputBindings outputs;
};
struct ServerNpcGruntDamageOutputChainResult {
    ServerNpcGruntCombatDamageResult prefix;
    ServerNpcGruntDamageOutputResult outputs;
};
struct ServerNpcGruntDamageOutputsApi {
    std::uint32_t version,size;
    // Caller attests arrival at 220401d4 after accepted base damage.
    int (__cdecl *tail)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntDamageOutputBindings*,ServerNpcGruntDamageOutputResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntDamageOutputChainBindings*,ServerNpcGruntDamageOutputChainResult*) noexcept;
};
using GetServerNpcGruntDamageOutputsApi=const ServerNpcGruntDamageOutputsApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_damage_event_export[]="SiN_GetServerNpcGruntDamageEventApi";
struct ServerNpcGruntDamageTailBindings {
    ServerNpcEnemyMemoryQueryBindings memory;
    ServerNpcConditionMapBindings conditions;
    ServerNpcGruntSentenceBindings entities;
    void* context;
    int (__cdecl *accept)(void*,void*,void*,bool*) noexcept; // +300
    int (__cdecl *visible)(void*,void*,void*,unsigned,bool*) noexcept; // +1f0, 4081, last arg zero
    int (__cdecl *refresh_origin)(void*,void*) noexcept; // 2213fa60
    int (__cdecl *report)(void*,void*,void*,const float*,void*) noexcept; // +678
    int (__cdecl *is_player)(void*,void*,bool*) noexcept;
    int (__cdecl *notify_attacker)(void*,void*,void*) noexcept; // +380
    int (__cdecl *final_notice)(void*,void*,const float*,int,int,float,int) noexcept; // 221b8fa0
    float fallback_vector[3]; // original global 22572bdc
};
struct ServerNpcGruntDamageTailResult {
    ServerNpcEnemyMemoryLookup lookup,touch;
    ServerNpcConditionResult light,heavy,accumulated;
    float position[3],amount,total;
    unsigned completed,enabled,attacker_valid,attacker_flags,reported,report_existing,visible,accumulated_prior,player_stamp,memory_touched,notified,final_notice;
    int native_result;
};
struct ServerNpcGruntDamageEventBindings {
    ServerNpcGruntDamageOutputChainBindings chain;
    ServerNpcGruntDamageTailBindings tail;
};
struct ServerNpcGruntDamageEventResult {
    ServerNpcGruntDamageOutputChainResult chain;
    ServerNpcGruntDamageTailResult tail;
    int native_result;
    unsigned completed;
};
struct ServerNpcGruntDamageEventApi {
    std::uint32_t version,size;
    // Caller attests arrival at 220402ef.
    int (__cdecl *tail)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntDamageTailBindings*,ServerNpcGruntDamageTailResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntDamageEventBindings*,ServerNpcGruntDamageEventResult*) noexcept;
};
using GetServerNpcGruntDamageEventApi=const ServerNpcGruntDamageEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_damage_override_export[]="SiN_GetServerNpcGruntDamageOverrideApi";
struct ServerNpcGruntDamageOverrideBindings {
    ServerNpcGruntDamageEventBindings base;
    void* context;
    int (__cdecl *cast_grunt)(void*,void*,void**,unsigned*) noexcept; // RTTI, target 2252d5b0
    int (__cdecl *impact)(void*,void*,void*) noexcept; // 223594a0
    int (__cdecl *bone_hit)(void*,void*,const char*,const float*,float,bool*) noexcept; // 22358490
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *component_damage)(void*,std::uint32_t,void*,int*) noexcept; // +b0
};
struct ServerNpcGruntDamageOverrideResult {
    ServerNpcGruntDamageEventResult base;
    ServerNpcGruntSentenceResult berserk,cough,knee;
    ServerNpcGruntSentenceRequestResult berserk_request,cough_request,knee_request;
    ServerNpcConditionResult bit11,bit47;
    float knee_total,random_sample,copy_amount;
    std::uint32_t component;
    unsigned completed,cast_called,berserk_called,impact_called,rejected_flags,cough_blocked,cough_called,copy_used,knee_checked,knee_added,knee_reset,knee_called,random_called,component_called,base_called;
    int native_result;
};
struct ServerNpcGruntDamageOverrideApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntDamageOverrideBindings*,ServerNpcGruntDamageOverrideResult*) noexcept;
};
using GetServerNpcGruntDamageOverrideApi=const ServerNpcGruntDamageOverrideApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_damage_notification_export[]="SiN_GetServerNpcGruntDamageNotificationApi";
struct ServerNpcGruntDamageNotificationBindings {
    std::uint32_t original_vtable;
    void* context;
    int (__cdecl *as_npc)(void*,void*,void**) noexcept; // 22139f60
    int (__cdecl *count)(void*,int*) noexcept; // live global 225475cc
    int (__cdecl *member)(void*,int,void**,unsigned*,std::uint32_t*) noexcept; // live global 225475c0
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *disposition)(void*,void*,void*,int*) noexcept; // +3c0
    int (__cdecl *state_signal)(void*,void*,int) noexcept; // receiver +4d8
    int (__cdecl *other_receive)(void*,void*,void*,void*) noexcept; // unknown receiver +37c
};
struct ServerNpcGruntDamageReceiveResult {
    float distance_squared;
    unsigned completed,state_blocked,source_refreshed,receiver_refreshed,signal_called,marked;
};
struct ServerNpcGruntDamageNotificationResult {
    ServerNpcGruntDamageReceiveResult last_receive;
    unsigned completed,cast_called,source_refreshed,visited,null_member,self,vertical_blocked,horizontal_blocked,disposition_checked,disposition_blocked,native_received,hosted_received;
};
struct ServerNpcGruntDamageNotifiedEventBindings {
    ServerNpcGruntDamageEventBindings event;
    ServerNpcGruntDamageNotificationBindings notify;
};
struct ServerNpcGruntDamageNotifiedEventResult {
    ServerNpcGruntDamageEventResult event;
    ServerNpcGruntDamageNotificationResult notify;
};
struct ServerNpcGruntDamageNotificationApi {
    std::uint32_t version,size;
    int (__cdecl *receive)(void*,unsigned,void*,unsigned,const ServerNpcGruntDamageNotificationBindings*,ServerNpcGruntDamageReceiveResult*) noexcept;
    int (__cdecl *broadcast)(void*,unsigned,void*,const ServerNpcGruntDamageNotificationBindings*,ServerNpcGruntDamageNotificationResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntDamageNotifiedEventBindings*,ServerNpcGruntDamageNotifiedEventResult*) noexcept;
};
using GetServerNpcGruntDamageNotificationApi=const ServerNpcGruntDamageNotificationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_damage_detail_export[]="SiN_GetServerNpcGruntDamageDetailApi";
struct ServerNpcGruntBoneHitBindings {
    void* context;
    int (__cdecl *index)(void*,void*,const char*,int*) noexcept;
    int (__cdecl *matrix)(void*,void*,int,float*) noexcept; // row-major 3x4
};
struct ServerNpcGruntBoneHitResult {int index;float origin[3],squared;double distance;unsigned completed,found,hit;};
struct ServerNpcGruntImpactBindings {
    ServerNpcGruntReactionSpeechBindings reaction;
    void* context;
    int (__cdecl *rule)(void*,bool*) noexcept;
    int (__cdecl *detach)(void*,void*) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *lethal)(void*,void*,void*) noexcept;
    int (__cdecl *event)(void*,int,void*,void*) noexcept;
};
struct ServerNpcGruntImpactResult {
    ServerNpcGruntSentenceResult speech;
    ServerNpcGruntSentenceRequestResult request;
    float threshold,sample,deadline;
    unsigned completed,head_present,rule_called,detached,random_called,lethal_called,event_id,speech_called;
};
struct ServerNpcGruntDamageDetailBindings {
    ServerNpcGruntDamageOverrideBindings damage;
    ServerNpcGruntBoneHitBindings bone;
    ServerNpcGruntImpactBindings impact;
};
struct ServerNpcGruntDamageDetailResult {
    ServerNpcGruntDamageOverrideResult damage;
    ServerNpcGruntImpactResult impact;
    ServerNpcGruntBoneHitResult right,left;
};
struct ServerNpcGruntDamageDetailApi {
    std::uint32_t version,size;
    int (__cdecl *bone)(void*,const char*,const float*,float,const ServerNpcGruntBoneHitBindings*,ServerNpcGruntBoneHitResult*) noexcept;
    int (__cdecl *impact)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntImpactBindings*,ServerNpcGruntImpactResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntDamageDetailBindings*,ServerNpcGruntDamageDetailResult*) noexcept;
};
using GetServerNpcGruntDamageDetailApi=const ServerNpcGruntDamageDetailApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_damage_native_chain_export[]="SiN_GetServerNpcGruntDamageNativeChainApi";
struct ServerNpcGruntDamageNativeTailBindings {
    ServerNpcGruntDamageTailBindings tail;
    ServerNpcGruntDamageNotificationBindings notify;
    ServerNpcGruntMemoryReportBindings report;
    ServerNpcGruntSquadReportBindings squad;
};
struct ServerNpcGruntDamageNativeTailResult {
    ServerNpcGruntDamageTailResult tail;
    ServerNpcGruntDamageNotificationResult notify;
    ServerNpcGruntMemoryReportResult report;
    ServerNpcGruntSquadReportResult squad;
    unsigned report_called;
};
struct ServerNpcGruntDamageNativeChainBindings {
    ServerNpcGruntDamageDetailBindings detail;
    ServerNpcGruntDamageNotificationBindings notify;
    ServerNpcGruntMemoryReportBindings report;
    ServerNpcGruntSquadReportBindings squad;
};
struct ServerNpcGruntDamageNativeChainResult {
    ServerNpcGruntDamageDetailResult detail;
    ServerNpcGruntDamageNotificationResult notify;
    ServerNpcGruntMemoryReportResult report;
    ServerNpcGruntSquadReportResult squad;
    unsigned report_called;
};
struct ServerNpcGruntDamageNativeChainApi {
    std::uint32_t version,size;
    int (__cdecl *tail)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntDamageNativeTailBindings*,ServerNpcGruntDamageNativeTailResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntDamageNativeChainBindings*,ServerNpcGruntDamageNativeChainResult*) noexcept;
};
using GetServerNpcGruntDamageNativeChainApi=const ServerNpcGruntDamageNativeChainApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_death_selector_export[]="SiN_GetServerNpcGruntDeathSelectorApi";
struct ServerNpcGruntDeathSelectorBindings {
    void* context;
    int (__cdecl *model_available)(void*,void*,bool*) noexcept; // 220eb8a0, second lookup
    int (__cdecl *refresh_origin)(void*,void*) noexcept; // 2213fa60
    int (__cdecl *normalize)(void*,float*) noexcept; // original imported vector normalize
};
struct ServerNpcGruntDeathSelectorResult {
    float direction[3],forward[3],right[3];
    double forward_dot,right_dot;
    int selection;
    std::uint32_t auxiliary,hitgroup;
    unsigned completed,model_available,refreshed,normalized,right_dominant;
};
struct ServerNpcGruntDeathSelectedBindings {
    ServerNpcGruntCombatDeathBindings combat;
    ServerNpcGruntDeathSelectorBindings selector;
};
struct ServerNpcGruntDeathSelectedResult {
    ServerNpcGruntCombatDeathResult combat;
    ServerNpcGruntDeathSelectorResult selector;
};
struct ServerNpcGruntDeathSelectorApi {
    std::uint32_t version,size;
    int (__cdecl *select)(void*,unsigned,const void*,unsigned,std::uint32_t,const ServerNpcGruntDeathSelectorBindings*,ServerNpcGruntDeathSelectorResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,void*,unsigned,float,const ServerNpcGruntDeathSelectedBindings*,ServerNpcGruntDeathSelectedResult*) noexcept;
};
using GetServerNpcGruntDeathSelectorApi=const ServerNpcGruntDeathSelectorApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_life_damage_export[]="SiN_GetServerNpcGruntLifeDamageApi";
enum ServerNpcGruntLifeDamageRoute : unsigned {NpcDamageAlive=0,NpcDamageDying=1,NpcDamageDead=2};
struct ServerNpcGruntLifeDamageBindings {
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    std::uint32_t original_vtable;
    float* shared_direction; // original 22572bdc, three floats
    float default_direction[3]; // original 225885b8
    void* context;
    int (__cdecl *center)(void*,void*,float*) noexcept; // entity +214
    int (__cdecl *normalize)(void*,float*) noexcept; // imported vector normalize
    int (__cdecl *remove)(void*,void*) noexcept; // 22286bf0
    int (__cdecl *reaction)(void*,void*,void*,void*) noexcept; // +2d4(12,1,attacker,inflictor,0,0)
};
struct ServerNpcGruntLifeDamageResult {
    float direction[3],fraction_before,fraction_after;
    int health_before,health_after,native_result;
    unsigned completed,inflictor_valid,centers,normalized,direction_written,health_writes,fraction_written,removed,reaction_called;
};
struct ServerNpcGruntLifeDamageApi {
    std::uint32_t version,size;
    // Route is caller-attested original virtual entry; not a life-state guess.
    int (__cdecl *run)(void*,unsigned,void*,unsigned,unsigned,const ServerNpcGruntLifeDamageBindings*,ServerNpcGruntLifeDamageResult*) noexcept;
};
using GetServerNpcGruntLifeDamageApi=const ServerNpcGruntLifeDamageApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_alive_damage_export[]="SiN_GetServerNpcGruntAliveDamageApi";
struct ServerNpcGruntAliveDamageBindings {
    ServerNpcGruntDamageNativeChainBindings chain;
    ServerNpcGruntLifeDamageBindings life;
};
struct ServerNpcGruntAliveDamageResult {
    ServerNpcGruntDamageNativeChainResult chain;
    ServerNpcGruntLifeDamageResult life;
};
struct ServerNpcGruntAliveDamageApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntAliveDamageBindings*,ServerNpcGruntAliveDamageResult*) noexcept;
};
using GetServerNpcGruntAliveDamageApi=const ServerNpcGruntAliveDamageApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_ignition_export[]="SiN_GetServerNpcGruntIgnitionApi";
struct ServerNpcGruntIgnitionBindings {
    std::uint32_t original_vtable;
    void* context;
    ServerNpcEnemyMemoryBindings name_storage;
    int (__cdecl *attacker_handle)(void*,void*,std::uint32_t*) noexcept; // attacker +8
    int (__cdecl *inflictor_name)(void*,void*,std::uint32_t*) noexcept; // inflictor +c
    int (__cdecl *create_flame)(void*,void*,void*,void*,void**) noexcept; // 221605e0, fixed bool 1
    int (__cdecl *lifetime)(void*,void*,float) noexcept; // 22160060
    int (__cdecl *attach)(void*,void*,void*) noexcept; // 2214d960
    int (__cdecl *is_player)(void*,void*,bool*) noexcept; // attacker +120
    int (__cdecl *event)(void*,int,void*,void*) noexcept;
    int (__cdecl *output)(void*,void*,unsigned) noexcept;
};
struct ServerNpcGruntIgnitionResult {
    std::uint32_t attacker_handle,inflictor_name,flags_before,flags_after;
    unsigned completed,class_blocked,flag_blocked,attribution_written,created,lifetime_called,flag_written,attached,player_checked,event_called,output_called;
};
struct ServerNpcGruntIgnitionAliveBindings {
    ServerNpcGruntAliveDamageBindings alive;
    ServerNpcGruntIgnitionBindings ignition;
};
struct ServerNpcGruntIgnitionAliveResult {
    ServerNpcGruntAliveDamageResult alive;
    ServerNpcGruntIgnitionResult ignition;
};
struct ServerNpcGruntIgnitionApi {
    std::uint32_t version,size;
    // Fixed argument form of +2d4 used by 222a7b20, not arbitrary ignite inputs.
    int (__cdecl *react)(void*,unsigned,void*,void*,const ServerNpcGruntIgnitionBindings*,ServerNpcGruntIgnitionResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntIgnitionAliveBindings*,ServerNpcGruntIgnitionAliveResult*) noexcept;
};
using GetServerNpcGruntIgnitionApi=const ServerNpcGruntIgnitionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_life_dispatch_export[]="SiN_GetServerNpcGruntLifeDispatchApi";
struct ServerNpcGruntLifeDispatchBindings {
    ServerNpcGruntIgnitionAliveBindings combat;
    void* context;
    int (__cdecl *owner_type)(void*,void*,bool*) noexcept; // 2213dff0 RTTI on resolved +e8
    int (__cdecl *owner_scale)(void*,void*,void*,void*,float*) noexcept; // owner +130, +c, +60c/+24
    int (__cdecl *pre_effect)(void*,void*,const float*) noexcept; // original bit8 effect block
    int (__cdecl *object_death)(void*,std::uint32_t,int) noexcept; // +218 object's +5c(0)
    int (__cdecl *death)(void*,void*,void*,std::uint32_t) noexcept; // class-specific +e8
};
struct ServerNpcGruntLifeDispatchResult {
    ServerNpcGruntIgnitionAliveResult alive;
    ServerNpcGruntLifeDamageResult other;
    float scaled_amount,scale;
    int initial_state,native_result;
    unsigned completed,route,mode_blocked,owner_checked,owner_scaled,scale_blocked,effect_called,object_called,death_called,post_death_noop;
};
struct ServerNpcGruntLifeDispatchApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntLifeDispatchBindings*,ServerNpcGruntLifeDispatchResult*) noexcept;
};
using GetServerNpcGruntLifeDispatchApi=const ServerNpcGruntLifeDispatchApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_jetpack_death_mode_export[]="SiN_GetServerNpcJetpackDeathModeApi";
struct ServerNpcJetpackDeathModeBindings {
    std::uint32_t original_vtable;
    void* context;
    int (__cdecl *random_int)(void*,int,int,int*) noexcept;
    int (__cdecl *effect_rule)(void*,void*,bool*) noexcept; // 22572914 +184
    int (__cdecl *effect)(void*,void*) noexcept; // original 223667bb..223667f1
    int (__cdecl *force_mode3)(void*,bool*) noexcept; // 22572198 +170
    int (__cdecl *tail)(void*,void*,void*,unsigned,std::uint32_t) noexcept; // branch ID as data, never an executable VA
};
struct ServerNpcJetpackDeathModeResult {
    int sample,initial_mode,final_mode;
    std::uint32_t branch;
    unsigned completed,copied,on_fire,random_called,effect_checked,effect_called,force_checked,forced,tail_required,tail_called;
};
struct ServerNpcJetpackDeathDispatchBindings {
    ServerNpcGruntLifeDispatchBindings life;
    ServerNpcJetpackDeathModeBindings jet;
};
struct ServerNpcJetpackDeathDispatchResult {
    ServerNpcGruntLifeDispatchResult life;
    ServerNpcJetpackDeathModeResult jet;
};
struct ServerNpcJetpackDeathModeApi {
    std::uint32_t version,size;
    int (__cdecl *select)(void*,unsigned,void*,unsigned,const ServerNpcJetpackDeathModeBindings*,ServerNpcJetpackDeathModeResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcJetpackDeathDispatchBindings*,ServerNpcJetpackDeathDispatchResult*) noexcept;
};
using GetServerNpcJetpackDeathModeApi=const ServerNpcJetpackDeathModeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_ground_death_export[]="SiN_GetServerNpcGruntGroundDeathApi";
struct ServerNpcGruntGroundDeathBindings {
    ServerNpcGruntDeathSelectedBindings base;
    void* context;
    int (__cdecl *classification)(void*,void*,int*) noexcept; // attacker +d0
    int (__cdecl *loot_enabled)(void*,bool*) noexcept; // 2259c5b0 +2c
    int (__cdecl *weapon)(void*,void*,void**) noexcept; // 222aa800(0)
    int (__cdecl *loot_rule)(void*,bool*) noexcept; // 22572914 +1a4
    int (__cdecl *matches)(void*,void*,const char*,bool*) noexcept;
    // Host original randomized pose/creation helper; kick owns optional +218 physics.
    int (__cdecl *drop)(void*,void*,const char*,void**) noexcept;
    int (__cdecl *kick)(void*,void*) noexcept;
    int (__cdecl *random_int)(void*,int,int,int*) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *component_death)(void*,std::uint32_t,void*) noexcept; // +44
};
struct ServerNpcGruntGroundDeathResult {
    ServerNpcGruntDeathSelectedResult base;
    unsigned char copied[0x48];
    const char* ammo;
    int attacker_class,grenade_sample;
    unsigned completed,attacker_valid,loot_enabled,weapon_checked,rule_checked,matches,ammo_dropped,kick_called,grenade_dropped,copy_used,components,base_called;
};
struct ServerNpcGruntGroundDeathApi {
    std::uint32_t version,size;
    int (__cdecl *prepare)(void*,unsigned,void*,unsigned,const ServerNpcGruntGroundDeathBindings*,ServerNpcGruntGroundDeathResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,void*,unsigned,float,const ServerNpcGruntGroundDeathBindings*,ServerNpcGruntGroundDeathResult*) noexcept;
};
using GetServerNpcGruntGroundDeathApi=const ServerNpcGruntGroundDeathApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_ground_life_export[]="SiN_GetServerNpcGruntGroundLifeApi";
struct ServerNpcGruntGroundLifeBindings {
    ServerNpcGruntLifeDispatchBindings life;
    ServerNpcGruntGroundDeathBindings ground;
    ServerNpcJetpackDeathModeBindings jet;
};
struct ServerNpcGruntGroundLifeResult {
    ServerNpcGruntLifeDispatchResult life;
    ServerNpcGruntGroundDeathResult ground;
    ServerNpcJetpackDeathModeResult jet;
    unsigned ground_called;
};
struct ServerNpcGruntGroundLifeApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,const void*,unsigned,float,const ServerNpcGruntGroundLifeBindings*,ServerNpcGruntGroundLifeResult*) noexcept;
};
using GetServerNpcGruntGroundLifeApi=const ServerNpcGruntGroundLifeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_special_impact_export[]="SiN_GetServerNpcGruntSpecialImpactApi";
struct ServerNpcGruntSpecialImpactBindings {
    ServerNpcGruntSentenceBindings entities;
    ServerNpcEnemyMemoryBindings dirty_storage;
    ServerNpcConditionMapBindings schedules;
    void* context;
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *normalize)(void*,float*) noexcept;
    int (__cdecl *scale)(void*,float,float,float*) noexcept; // original service 225721b0 +4
    int (__cdecl *effect_tail)(void*,void*,const float*) noexcept; // 2235f0a4..2235f212
};
struct ServerNpcGruntSpecialImpactResult {
    float initial[3],normalized[3],scaled[3],origin[3],scale_bound,scale;
    unsigned completed,elite_noop,latch_written,attacker_refreshed,actor_refreshed,normalized_called,vector_writes,local_dirty,external_dirty,schedule_adjusted,tail_required,tail_called;
};
struct ServerNpcGruntSpecialDamageBindings {
    ServerNpcGruntDamageDetailBindings detail;
    ServerNpcGruntSpecialImpactBindings special;
};
struct ServerNpcGruntSpecialDamageResult {
    ServerNpcGruntDamageDetailResult detail;
    ServerNpcGruntSpecialImpactResult special;
};
struct ServerNpcGruntSpecialImpactApi {
    std::uint32_t version,size;
    int (__cdecl *set_vector)(void*,unsigned,const float*,const ServerNpcEnemyMemoryBindings*,unsigned*,unsigned*,unsigned*) noexcept;
    int (__cdecl *prefix)(void*,unsigned,void*,unsigned,const ServerNpcGruntSpecialImpactBindings*,ServerNpcGruntSpecialImpactResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,void*,unsigned,float,const ServerNpcGruntSpecialDamageBindings*,ServerNpcGruntSpecialDamageResult*) noexcept;
};
using GetServerNpcGruntSpecialImpactApi=const ServerNpcGruntSpecialImpactApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_jetpack_death_branches_export[]="SiN_GetServerNpcJetpackDeathBranchesApi";
struct ServerNpcJetpackDeathBranchBindings {
    void* context;
    ServerNpcEnemyMemoryBindings dirty_storage;
    int (__cdecl *remove)(void*,void*) noexcept; // 22286bf0 accepts null
    int (__cdecl *powerdown)(void*,void*,const char*) noexcept;
    int (__cdecl *sentence)(void*,void*,const char*) noexcept;
    int (__cdecl *ignite)(void*,void*,float,int,void*,void*,float,int) noexcept; // +2d4
    int (__cdecl *ragdoll)(void*,void*,void*,const float*) noexcept; // Jetpack +3ac
    int (__cdecl *time)(void*,float*) noexcept;
    int (__cdecl *install_think)(void*,void*,std::uint32_t) noexcept; // identity token, not executable VA
    int (__cdecl *schedule)(void*,void*,float) noexcept;
    int (__cdecl *effect_begin)(void*,void*) noexcept; // original temporary recipient scope
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *immediate_effect)(void*,void*) noexcept; // 22366980..223669ea
    int (__cdecl *effect_end)(void*,void*) noexcept;
};
struct ServerNpcJetpackDeathBranchResult {
    float force[3],stamp,scheduled;
    unsigned completed,mode,cleanup,removed,cleared,aux_removed,powerdown,sentence,ignited,life_changed,local_dirty,external_dirty,ragdoll,installed,scheduled_called,effect_scope,refreshed,effect_called,base_called,effect_ended;
};
struct ServerNpcJetpackDeathBranchesBindings {
    ServerNpcGruntGroundLifeBindings combat;
    ServerNpcJetpackDeathBranchBindings branch;
};
struct ServerNpcJetpackDeathBranchesResult {
    ServerNpcGruntGroundLifeResult combat;
    ServerNpcJetpackDeathBranchResult branch;
};
struct ServerNpcJetpackDeathBranchesApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,const void*,unsigned,float,const ServerNpcJetpackDeathBranchesBindings*,ServerNpcJetpackDeathBranchesResult*) noexcept;
};
using GetServerNpcJetpackDeathBranchesApi=const ServerNpcJetpackDeathBranchesApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_jetpack_death_think_export[]="SiN_GetServerNpcJetpackDeathThinkApi";
struct ServerNpcJetpackDeathThinkBindings {
    ServerNpcGruntGroundDeathBindings ground;
    void* context;
    int (__cdecl *time)(void*,float*) noexcept;
    // Host the original ragdoll motion block, including its RNG and live-time queries.
    int (__cdecl *motion)(void*,void*,void*,int) noexcept;
    int (__cdecl *effect_begin)(void*,void*) noexcept;
    int (__cdecl *terminal_effect)(void*,void*,void*,bool) noexcept; // mode4 selects +1028, others +1000
    int (__cdecl *effect_end)(void*,void*) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    int (__cdecl *install_think)(void*,void*,std::uint32_t) noexcept;
    int (__cdecl *schedule)(void*,void*,float) noexcept;
};
struct ServerNpcJetpackDeathThinkResult {
    ServerNpcGruntGroundDeathResult ground;
    float checked_time,scheduled;
    double elapsed;
    int motion_mode,terminal_mode;
    unsigned completed,motion_called,expired,effect_opened,effect_called,effect_closed,aux_removed,cleared,primary_removed,installed,mode_written,death_called,scheduled_called;
};
struct ServerNpcJetpackDeathThinkApi {
    std::uint32_t version,size;
    // Caller attests invocation of original 22365760; stored damage is live actor+102c.
    int (__cdecl *run)(void*,unsigned,std::uint32_t,const ServerNpcJetpackDeathThinkBindings*,ServerNpcJetpackDeathThinkResult*) noexcept;
};
using GetServerNpcJetpackDeathThinkApi=const ServerNpcJetpackDeathThinkApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_elite_life_export[]="SiN_GetServerNpcGruntEliteLifeApi";
// Original Elite 22354fde..22355268. Native sequence/arguments only;
// the engine supplies effects, RNG and the trace query.
struct ServerNpcGruntEliteEffectTrace {
    float start[3],end[3],fraction;
};
struct ServerNpcGruntEliteEffectBindings {
    void* context;
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *burst)(void*,const float*,const float*,void*,void*,int,int,unsigned,float) noexcept;
    int (__cdecl *light)(void*,const float*,int,int,int,int,float,float,float) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *upward)(void*,const float*,const float*,int,int,int) noexcept; // 221234d0
    int (__cdecl *downward)(void*,const float*,const float*,int,int) noexcept; // 2228c810
    int (__cdecl *trace)(void*,void*,const float*,const float*,unsigned,int,ServerNpcGruntEliteEffectTrace*) noexcept;
    int (__cdecl *debug_enabled)(void*,bool*) noexcept;
    int (__cdecl *debug_trace)(void*,const float*,const float*,int,int,int,int,float) noexcept;
    int (__cdecl *impact)(void*,const ServerNpcGruntEliteEffectTrace*,int) noexcept; // 2228bee0
};
// 22288a40 single-player helper. The owner provides PE32-shaped edict
// bytes and the engine's IServerUnknown::GetBaseEntity boundary.
struct ServerNpcEliteLocalPlayerBindings {
    void* context;
    int (__cdecl *max_clients)(void*,int*) noexcept;
    int (__cdecl *edict)(void*,int,const void**,unsigned*) noexcept;
    int (__cdecl *base_entity)(void*,std::uint32_t,void**) noexcept;
};
// Player +45c (222457d0): authenticated weapon inventory and ammo
// reward selection. Optional live ammo metadata overrides the native
// registered definitions; the entity factory remains an engine service.
struct ServerNpcEliteAmmoRewardBindings {
    void* context;
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    const ServerWeaponScriptView* scripts;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *ammo_index)(void*,const char*,int*) noexcept;
    int (__cdecl *max_carry)(void*,int,int*) noexcept;
    int (__cdecl *create)(void*,const char*,int,void**) noexcept;
};
struct ServerNpcGruntEliteDeathBindings {
    void* context;
    int (__cdecl *effect_begin)(void*,void*) noexcept;
    int (__cdecl *effects)(void*,void*) noexcept; // original 22354fde..22355268
    int (__cdecl *effect_end)(void*,void*) noexcept;
    int (__cdecl *rule)(void*,bool*) noexcept; // 22572914 +1a4
    int (__cdecl *player)(void*,void**) noexcept; // 22288a40
    int (__cdecl *entity)(void*,void*,void**) noexcept; // player +45c
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *position)(void*,void*,const float*) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *orientation)(void*,void*,const float*) noexcept;
    int (__cdecl *velocity)(void*,void*,float,float,float*) noexcept; // original 2202e350, separate RNG
    int (__cdecl *set_velocity)(void*,void*,const float*) noexcept;
    int (__cdecl *finish)(void*,void*) noexcept; // relocated entity +64
    const ServerNpcGruntEliteEffectBindings* native_effects;
    // Optional native 2202e350 vector generation; raw 2226ffa0 stream.
    int (__cdecl *velocity_random_int)(void*,int,int,int*) noexcept;
    const ServerNpcEliteLocalPlayerBindings* native_player;
    const ServerNpcEliteAmmoRewardBindings* native_reward;
};
struct ServerNpcGruntEliteDeathResult {
    float origin[3],angles[3],velocity[3];
    int prior_grenades;
    unsigned completed,grenades_cleared,effect_opened,effects_called,base_called,rule_checked,rule_allowed,player_found,entity_found,refreshed,positioned,random_calls,oriented,velocity_generated,velocity_set,finished,effect_closed;
    float effect_origin[3],effect_upper[3],effect_angles[3];
    unsigned native_effects_completed,effect_refreshes,effect_burst,effect_light,effect_randoms,effect_upward,effect_downward,effect_traces,effect_debug,effect_impacts,effect_iterations;
    unsigned velocity_native,velocity_randoms;
    unsigned player_lookup_native,player_slot_checked,player_edict_found,player_edict_free,player_unknown_called;
    int player_max_clients;
    unsigned reward_native,reward_slots,reward_weapons,reward_candidates,reward_equal_minimum,reward_factory;
    unsigned reward_native_indices,reward_native_maxima,reward_missing_class;
    int reward_ammo_index;
    float reward_minimum;
    const char* reward_classname;
};
struct ServerNpcGruntEliteLifeBindings {
    ServerNpcJetpackDeathBranchesBindings combat;
    ServerNpcGruntEliteDeathBindings elite;
};
struct ServerNpcGruntEliteLifeResult {
    ServerNpcJetpackDeathBranchesResult combat;
    ServerNpcGruntEliteDeathResult elite;
};
struct ServerNpcGruntEliteLifeApi {
    std::uint32_t version,size;
    int (__cdecl *post)(void*,unsigned,const ServerNpcGruntEliteDeathBindings*,ServerNpcGruntEliteDeathResult*) noexcept;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,const void*,unsigned,float,const ServerNpcGruntEliteLifeBindings*,ServerNpcGruntEliteLifeResult*) noexcept;
};
using GetServerNpcGruntEliteLifeApi=const ServerNpcGruntEliteLifeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_activity_translation_export[]="SiN_GetServerNpcGruntActivityTranslationApi";
struct ServerNpcGruntActivityTranslationBindings {
    ServerNpcGruntSentenceBindings entities;
    void* context;
    int launcher_activity,other_activity; // original globals 2259c4a8,2259c4ac
    int (__cdecl *has_weapon)(void*,void*,const char*,bool*) noexcept; // 222aa840(name,0)
    int (__cdecl *special_identity)(void*,void*,bool*) noexcept; // actor+dbc virtual +4a8 equals actor+db8
    int (__cdecl *component_translate)(void*,std::uint32_t,int,int*) noexcept; // +7c
    int (__cdecl *weapon_caps)(void*,void*,unsigned*) noexcept; // +4a0
    int (__cdecl *linked_activity)(void*,void*,void*,int*) noexcept; // +718, 2203da40
    int (__cdecl *activity_lookup)(void*,void*,int,int*) noexcept;
};
struct ServerNpcGruntActivityTranslationResult {
    int requested,translated,native_result,activity_lookup;
    float speed_squared,speed;
    std::uint32_t component,capabilities,jet_handle;
    unsigned completed,jet,jet_direct,weapon_checked,launcher_found,stance_written,special_checked,special_identity,component_called,base_called,caps_called,linked_called,lookup_called;
};
struct ServerNpcGruntActivityTranslationApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,int,const ServerNpcGruntActivityTranslationBindings*,ServerNpcGruntActivityTranslationResult*) noexcept;
};
using GetServerNpcGruntActivityTranslationApi=const ServerNpcGruntActivityTranslationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_ragdoll_route_export[]="SiN_GetServerNpcGruntRagdollRouteApi";
// Gameplay decisions only; actual ragdoll creation, collision and motion remain host services.
struct ServerNpcGruntRagdollRouteBindings {
    ServerNpcGruntSentenceBindings entities;
    void* context;
    int (__cdecl *base)(void*,void*,const void*,const float*,bool*) noexcept; // 222a5550
    int (__cdecl *ground_setup)(void*,void*,const void*) noexcept; // 2235a153..2235a221
    int (__cdecl *jet_setup)(void*,void*,const void*,std::uint32_t*) noexcept; // 22365511..223655af
    int (__cdecl *random_int)(void*,int,int,int*) noexcept;
    int (__cdecl *sentence)(void*,void*) noexcept; // 223655ca..223655e4
    int (__cdecl *ignite)(void*,void*,float,int,void*,void*,float,int) noexcept;
    int (__cdecl *attach)(void*,void*,void*) noexcept; // primary attachment block, 2236561c..223656c0
    int (__cdecl *reparent)(void*,void*,void*) noexcept; // 22008140, last original argument zero
    int (__cdecl *finish)(void*,void*) noexcept; // 22143d90
};
struct ServerNpcGruntRagdollRouteResult {
    int sample;
    std::uint32_t mode,source_handle,ragdoll_handle;
    unsigned completed,native_result,jet,source_valid,jet_special,ground_special,base_called,setup_called,handle_written,random_called,sentence_called,ignite_called,attachment_called,aux_reparented,finished;
};
struct ServerNpcGruntRagdollRouteApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,const float*,const ServerNpcGruntRagdollRouteBindings*,ServerNpcGruntRagdollRouteResult*) noexcept;
};
using GetServerNpcGruntRagdollRouteApi=const ServerNpcGruntRagdollRouteApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_linked_activity_export[]="SiN_GetServerNpcGruntLinkedActivityApi";
struct ServerNpcGruntLinkedActivityBindings {
    void* context;
    int (__cdecl *query)(void*,void*,void**) noexcept; // actor +154, called twice on admitted route
    int (__cdecl *lookup)(void*,void*,int,int*) noexcept; // 220eebd0
    int (__cdecl *prepare)(void*,void*,int,float*) noexcept; // +630, 220092d0, 2200deb0
    int (__cdecl *trace)(void*,void*,void*,const float*,float*) noexcept; // second query +1c4, 220385c0
};
struct ServerNpcGruntLinkedActivityResult {
    int native_result,subtype,lookup;
    float point[3],fraction;
    unsigned completed,link_valid,queries,query_admitted,subtype_admitted,lookup_called,prepared,traced;
};
struct ServerNpcGruntActivityFullBindings {
    ServerNpcGruntActivityTranslationBindings translation;
    ServerNpcGruntLinkedActivityBindings linked;
};
struct ServerNpcGruntActivityFullResult {
    ServerNpcGruntActivityTranslationResult translation;
    ServerNpcGruntLinkedActivityResult linked;
};
struct ServerNpcGruntLinkedActivityApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,int,const ServerNpcGruntActivityFullBindings*,ServerNpcGruntActivityFullResult*) noexcept;
};
using GetServerNpcGruntLinkedActivityApi=const ServerNpcGruntLinkedActivityApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_ragdoll_life_export[]="SiN_GetServerNpcGruntRagdollLifeApi";
struct ServerNpcGruntRagdollLifeBindings {
    ServerNpcGruntEliteLifeBindings life;
    ServerNpcGruntRagdollRouteBindings ragdoll;
};
struct ServerNpcGruntRagdollLifeResult {
    ServerNpcGruntEliteLifeResult life;
    ServerNpcGruntRagdollRouteResult ragdoll;
    unsigned ragdoll_entered;
};
struct ServerNpcGruntRagdollLifeApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,const void*,unsigned,float,const ServerNpcGruntRagdollLifeBindings*,ServerNpcGruntRagdollLifeResult*) noexcept;
};
using GetServerNpcGruntRagdollLifeApi=const ServerNpcGruntRagdollLifeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_component_damage_export[]="SiN_GetServerNpcGruntComponentDamageApi";
struct ServerNpcGruntComponentDamageBindings {
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    std::uint32_t original_behavior_vtable;
    void* context;
    int (__cdecl *parent_damage)(void*,std::uint32_t,const void*,int*) noexcept; // component+0c, parent virtual +2c
    int (__cdecl *source_predicate)(void*,void*,bool*) noexcept; // source virtual +120
    int (__cdecl *tank_predicate)(void*,void*,void*,bool*) noexcept; // 222ea510(tank,source)
    int (__cdecl *condition_map)(void*,void*,int,int*) noexcept; // behavior +2c, 22073700
    int (__cdecl *condition_set)(void*,std::uint32_t,int) noexcept; // owner+04, 22039760
};
struct ServerNpcGruntComponentDamageResult {
    int native_result,mapped_condition;
    std::uint32_t parent,owner,source_handle,tank_handle;
    unsigned completed,tank,delegated,source_valid,tank_valid,source_checked,source_admitted,tank_checked,tank_predicate_value,mapped,condition_set;
};
struct ServerNpcGruntComponentDamageApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,const ServerNpcGruntComponentDamageBindings*,ServerNpcGruntComponentDamageResult*) noexcept;
};
using GetServerNpcGruntComponentDamageApi=const ServerNpcGruntComponentDamageApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_behavior_life_export[]="SiN_GetServerNpcGruntBehaviorLifeApi";
struct ServerNpcGruntBehaviorDamageBindings {
    // Original PE32 identity token for the actor. Embedded pointers and vptr
    // identities are represented as data, never invoked as addresses.
    std::uint32_t actor_token;
    ServerNpcGruntComponentDamageBindings component;
};
struct ServerNpcGruntBehaviorDamageResult {
    ServerNpcGruntComponentDamageResult component;
    unsigned entered,embedded_offset,identity_valid,parent_native;
    std::uint32_t active_token,behavior_vtable,bridge_vtable;
};
struct ServerNpcGruntBehaviorLifeBindings {
    ServerNpcGruntRagdollLifeBindings combat;
    ServerNpcGruntBehaviorDamageBindings behavior;
};
struct ServerNpcGruntBehaviorLifeResult {
    ServerNpcGruntRagdollLifeResult combat;
    ServerNpcGruntBehaviorDamageResult behavior;
};
struct ServerNpcGruntBehaviorLifeApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,const void*,unsigned,float,const ServerNpcGruntBehaviorLifeBindings*,ServerNpcGruntBehaviorLifeResult*) noexcept;
};
using GetServerNpcGruntBehaviorLifeApi=const ServerNpcGruntBehaviorLifeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_behavior_condition_life_export[]="SiN_GetServerNpcGruntBehaviorConditionLifeApi";
struct ServerNpcGruntBehaviorConditionLifeBindings {
    ServerNpcGruntBehaviorLifeBindings life;
    // Original FuncTank descriptor 2259de50 and live range rows, not the owner descriptor.
    ServerNpcConditionMapBindings tank_descriptor;
};
struct ServerNpcGruntBehaviorConditionLifeResult {
    ServerNpcGruntBehaviorLifeResult life;
    ServerNpcConditionResult tank_mapping,owner_condition;
    unsigned tank_mapping_called,owner_condition_called;
};
struct ServerNpcGruntBehaviorConditionLifeApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,std::uint32_t,const void*,unsigned,float,const ServerNpcGruntBehaviorConditionLifeBindings*,ServerNpcGruntBehaviorConditionLifeResult*) noexcept;
};
using GetServerNpcGruntBehaviorConditionLifeApi=const ServerNpcGruntBehaviorConditionLifeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_behavior_activity_export[]="SiN_GetServerNpcGruntBehaviorActivityApi";
struct ServerNpcGruntBehaviorActivityBindings {
    ServerNpcGruntActivityFullBindings full;
    std::uint32_t actor_token;
    void* context;
    // Original 22022ab0 symbol lookup; token zero denotes the empty fallback string.
    int (__cdecl *symbol_lookup)(void*,std::uint32_t,int*) noexcept;
    ServerNpcEnemyMemoryBindings table_storage;
    std::uint32_t* standoff_comparator; // live original global 2254c694 identity
};
struct ServerNpcGruntBehaviorActivityResult {
    ServerNpcGruntActivityFullResult full;
    std::uint32_t active_token,behavior_vtable,bridge_vtable,standoff_key,name_token;
    int incoming,forwarded,native_result,subtype,linked_selection,state_before,state_after,symbol;
    unsigned entered,identity_valid,embedded_offset,direct,parent_native,link_valid,state_written,table_checked,table_visited,table_found,table_index,symbol_called;
};
struct ServerNpcGruntBehaviorActivityApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,int,const ServerNpcGruntBehaviorActivityBindings*,ServerNpcGruntBehaviorActivityResult*) noexcept;
};
using GetServerNpcGruntBehaviorActivityApi=const ServerNpcGruntBehaviorActivityApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_behavior_event_export[]="SiN_GetServerNpcGruntBehaviorEventApi";
struct ServerNpcGruntBehaviorEventBindings {
    ServerNpcGruntEventSpeechBindings speech;
    std::uint32_t actor_token;
    void* context;
    // Case entry is provenance, not a callable retail address; same component storage must remain valid.
    int (__cdecl *special)(void*,void*,void*,unsigned,std::uint32_t,const void*,unsigned) noexcept;
    int (__cdecl *shared)(void*,void*,const void*,unsigned) noexcept;
    int (__cdecl *event43)(void*,void*,const void*,unsigned) noexcept;
};
struct ServerNpcGruntBehaviorEventResult {
    ServerNpcGruntEventSpeechResult speech;
    std::uint32_t active_token,behavior_vtable,special_entry;
    int event,argument;
    unsigned completed,identity_valid,embedded_offset,marker_set,marker_cleared,special_called,consumed,parent_called,speech_native,bypass;
};
struct ServerNpcGruntBehaviorEventApi {
    std::uint32_t version,size;
    // Original 2202d930 gate; raw event begins with i32 ID and f32 argument.
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntBehaviorEventBindings*,ServerNpcGruntBehaviorEventResult*) noexcept;
};
using GetServerNpcGruntBehaviorEventApi=const ServerNpcGruntBehaviorEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_event43_export[]="SiN_GetServerNpcGruntEvent43Api";
struct ServerNpcGruntEvent43Bindings {
    ServerNpcGruntBehaviorEventBindings gate;
    void* context;
    int (__cdecl *weapon_caps)(void*,void*,unsigned*) noexcept;
    int (__cdecl *request)(void*,void*,int) noexcept; // 2203d330, virtual +47c
    int (__cdecl *refresh)(void*,void*) noexcept; // 22041a60, virtual +680
    int (__cdecl *clear_activity)(void*,void*,int) noexcept; // 22049c20 after native +878 clear
};
struct ServerNpcGruntEvent43Result {
    ServerNpcGruntBehaviorEventResult gate;
    unsigned capabilities,weapon_queries,capability_route,timer_active,timer_pending,requested,refreshed,cleared,activity_called,completed;
    int activity;
};
struct ServerNpcGruntEvent43Api {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntEvent43Bindings*,ServerNpcGruntEvent43Result*) noexcept;
};
using GetServerNpcGruntEvent43Api=const ServerNpcGruntEvent43Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_rappel_event_export[]="SiN_GetServerNpcGruntRappelEventApi";
struct ServerNpcGruntRappelEventBindings {
    ServerNpcGruntEvent43Bindings events;
    void* context;
    ServerNpcEnemyMemoryBindings navigation;
    const float* zero_vector; // live 225885b8
    int (__cdecl *refresh_origin)(void*,void*) noexcept; // 2213fa60
    int (__cdecl *trace)(void*,void*,const float*,const float*,unsigned,float*) noexcept; // 22014cc0, end Z
    int (__cdecl *sound)(void*,void*,const char*) noexcept; // 221b77b0
    int (__cdecl *rope)(void*,void*,void*,std::uint32_t*) noexcept; // 220de910 creation subroute; invalid sentinel if no attachment
    int (__cdecl *velocity)(void*,void*,const float*) noexcept; // 22141a00
    int (__cdecl *schedule_reset)(void*,void*,bool) noexcept; // 2203fff0 -> 2203d2d0
};
struct ServerNpcGruntRappelEventResult {
    ServerNpcGruntEvent43Result events;
    float angle,distances[2],velocity[3];
    std::uint32_t rope_handle;
    unsigned entered,completed,angle_written,refreshes,traces,sound_called,rope_valid,rope_created,velocity_called,flag_written,schedule_called;
    const char* sound;
};
struct ServerNpcGruntRappelEventApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntRappelEventBindings*,ServerNpcGruntRappelEventResult*) noexcept;
};
using GetServerNpcGruntRappelEventApi=const ServerNpcGruntRappelEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_rappel_schedule_export[]="SiN_GetServerNpcGruntRappelScheduleApi";
struct ServerNpcGruntRappelScheduleBindings {
    ServerNpcGruntRappelEventBindings events;
    ServerNpcConditionMapBindings conditions;
    ServerNpcGruntSquadSlotsBindings slots;
    void* context;
    int (__cdecl *random_int)(void*,int,int,int*) noexcept;
    int (__cdecl *random_float)(void*,float,float,double*) noexcept;
};
struct ServerNpcGruntRappelScheduleResult {
    ServerNpcGruntRappelEventResult events;
    ServerNpcConditionResult condition;
    ServerNpcGruntSquadSlotsResult slot;
    float deadline;
    unsigned entered,completed,guard_active,timer_active,rearmed,count,guard_cleared,slot_released,condition_checked,state_written;
};
struct ServerNpcGruntRappelScheduleApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntRappelScheduleBindings*,ServerNpcGruntRappelScheduleResult*) noexcept;
};
using GetServerNpcGruntRappelScheduleApi=const ServerNpcGruntRappelScheduleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_event_timer_export[]="SiN_GetServerNpcGruntEventTimerApi";
struct ServerNpcGruntEventTimerBindings {
    ServerNpcGruntRappelScheduleBindings events;
    ServerNpcEnemyMemoryBindings dirty_storage;
    void* context;
    // Source slot identities are data. Float results are rounded at each original FSTP.
    int (__cdecl *weapon_float)(void*,void*,unsigned,float*) noexcept;
    int (__cdecl *weapon_int)(void*,void*,unsigned,int*) noexcept;
};
struct ServerNpcGruntEventTimerResult {
    ServerNpcGruntRappelScheduleResult events;
    float previous_deadline,deadline;
    unsigned entered,completed,count_before,count_after,rearmed,clamped,active,weapon_queries,behavior_override,dirty_written,deadline_written;
};
struct ServerNpcGruntEventTimerApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntEventTimerBindings*,ServerNpcGruntEventTimerResult*) noexcept;
};
using GetServerNpcGruntEventTimerApi=const ServerNpcGruntEventTimerApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_tank_event_export[]="SiN_GetServerNpcGruntTankEventApi";
struct ServerNpcGruntTankEventBindings {
    ServerNpcGruntEventTimerBindings events;
    void* context;
    // Original virtual +47c receives the raw value, either 1 or a reason token.
    int (__cdecl *request)(void*,void*,std::uint32_t) noexcept;
    // 100000 path creation and 100001..100004 geometry/effects are explicit leaves.
    int (__cdecl *goal)(void*,void*,void*,bool*) noexcept;
    int (__cdecl *route_target)(void*,void*,void*) noexcept;
    int (__cdecl *position_error)(void*,void*,void*,float*) noexcept;
    int (__cdecl *navigation)(void*,void*,void*) noexcept;
    int (__cdecl *arrival)(void*,void*) noexcept;
    int (__cdecl *reject_position)(void*,void*) noexcept;
    int (__cdecl *sweep_point)(void*,void*,float,float*) noexcept;
    int (__cdecl *aim)(void*,void*,const float*,float,float,float) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
};
struct ServerNpcGruntTankEventResult {
    ServerNpcGruntEventTimerResult events;
    std::uint32_t handle,request_value;
    int random;
    float deadline,error[3],angle,point[3];
    unsigned entered,completed,valid,lookups,requested,goal_called,goal_success,route_called,error_called,aligned,navigation_called,arrival_called,rejected,deadline_written,angle_called,aim_called,tank_written,reset_called,random_called,ignored_signal;
};
struct ServerNpcGruntTankEventApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntTankEventBindings*,ServerNpcGruntTankEventResult*) noexcept;
};
using GetServerNpcGruntTankEventApi=const ServerNpcGruntTankEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_takecover_event_export[]="SiN_GetServerNpcGruntTakecoverEventApi";
struct ServerNpcGruntTakecoverEventBindings {
    ServerNpcGruntTankEventBindings events;
    void* context;
    const float* default_point; // live original 2251b1ec vector
    int (__cdecl *find_cover)(void*,void*,void*,float*,bool*) noexcept; // 22398b80
    int (__cdecl *goal)(void*,void*,const float*,bool*) noexcept; // 2204c660 + navigation virtual +14 mode2
    int (__cdecl *origin)(void*,void*,float*) noexcept; // 220092d0
    int (__cdecl *enemy_point)(void*,void*,float*) noexcept; // +1c4
    int (__cdecl *trace)(void*,void*,const float*,const float*,unsigned,float*) noexcept;
    int (__cdecl *set_origin)(void*,void*,const float*) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
};
struct ServerNpcGruntTakecoverEventResult {
    ServerNpcGruntTankEventResult events;
    float point[3],enemy[3],fraction,deadline;
    std::uint32_t request_value;
    int subtype,activity;
    unsigned entered,completed,search_called,search_ok,unchanged,attempts_added,random_calls,request_calls,goal_called,goal_ok,reset_called,linked_valid,enemy_valid,trace_called,origin_called,flag_cleared,velocity_called,activity_called;
};
struct ServerNpcGruntTakecoverEventApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntTakecoverEventBindings*,ServerNpcGruntTakecoverEventResult*) noexcept;
};
using GetServerNpcGruntTakecoverEventApi=const ServerNpcGruntTakecoverEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_standoff_event_export[]="SiN_GetServerNpcGruntStandoffEventApi";
struct ServerNpcGruntStandoffEventBindings {
    ServerNpcGruntTakecoverEventBindings events;
    void* context;
    const float* zero_vector; // live 225885b8
    int (__cdecl *single_player)(void*,void**) noexcept; // 22288a40
    int (__cdecl *disposition)(void*,void*,void*,int*) noexcept; // owner +3c0
    int (__cdecl *anchor_entity)(void*,void*,void**) noexcept; // fallback 220e01a0 entity selection
    int (__cdecl *origin)(void*,void*,float*) noexcept; // live origin with dirty refresh
    int (__cdecl *eye_point)(void*,void*,float*) noexcept; // +1c4
    // Original search entries; these perform engine navigation, not policy.
    int (__cdecl *arc)(void*,std::uint32_t,const float*,const float*,const float*,float,float,float*) noexcept;
    int (__cdecl *near_search)(void*,std::uint32_t,const float*,const float*,float,float*,bool*) noexcept;
    int (__cdecl *lateral)(void*,std::uint32_t,const float*,float*,bool*) noexcept;
    int (__cdecl *goal)(void*,void*,const float*,bool*) noexcept; // original 2202eb00, +14 mode0
    int max_clients;
};
struct ServerNpcGruntStandoffEventResult {
    ServerNpcGruntTakecoverEventResult events;
    float deadline,goal_deadline,reference[3],eye[3],anchor[3],point[3];
    int random;
    unsigned entered,completed,enemy_valid,player_queries,player_accepted,player_saved,cached_anchor,anchor_fallback,random_called,arc_calls,near_calls,lateral_calls,guard_set,guard_cleared,point_found,goal_called,goal_result,reset_called,request_called;
};
struct ServerNpcGruntStandoffEventApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntStandoffEventBindings*,ServerNpcGruntStandoffEventResult*) noexcept;
};
using GetServerNpcGruntStandoffEventApi=const ServerNpcGruntStandoffEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_actbusy_event_export[]="SiN_GetServerNpcGruntActBusyEventApi";
struct ServerNpcGruntActBusyEventBindings {
    ServerNpcGruntStandoffEventBindings events;
    void* context;
    const void* rows; unsigned row_count; // live 2259dd40, 40 bytes per row
    int debug_mode;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *origin)(void*,void*,float*) noexcept;
    int (__cdecl *facing_error)(void*,void*,double*) noexcept; // 2206d1b0
    int (__cdecl *flags_changed)(void*,void*,unsigned,unsigned) noexcept; // 221639f0
    int (__cdecl *flags_prepare)(void*,void*) noexcept; // +268
    int (__cdecl *prepare)(void*,void*,int) noexcept; // 22221890
    int (__cdecl *begin)(void*,void*,int,int,bool*) noexcept; // 2238ea60
    int (__cdecl *claim)(void*,void*,void*) noexcept; // 22065740
    int (__cdecl *notify)(void*,void*,void*,unsigned) noexcept; // linked +2ac/+2b0
    int (__cdecl *sequence)(void*,void*,std::uint32_t) noexcept; // 22043770
    int (__cdecl *duration)(void*,void*,float,bool) noexcept; // 220325c0
    int (__cdecl *restore_pose)(void*,void*,void*) noexcept; // 220ee200 + 22285aa0 / 22035ac0
    int (__cdecl *finish_pose)(void*,void*) noexcept; // 22035de0(true)
    int (__cdecl *release)(void*,void*,void*) noexcept; // 2238ddc0
    int (__cdecl *orientation)(void*,void*,float*) noexcept; // 2202f0b0
    int (__cdecl *set_transform)(void*,void*,const float*,const float*) noexcept; // +188
    int (__cdecl *memory_remove)(void*,std::uint32_t,int) noexcept; // 22073e00
    int (__cdecl *memory_add)(void*,std::uint32_t,int,std::uint32_t) noexcept; // 22073ed0 / 220ecb90 + 22073f90
    int (__cdecl *owner_event)(void*,void*,int,float) noexcept; // 220cda50 -> +444, marker restored natively
    int (__cdecl *debug_line)(void*,void*,void*) noexcept; // 2225c210
};
struct ServerNpcGruntActBusyEventResult {
    ServerNpcGruntStandoffEventResult events;
    ServerNpcConditionResult condition;
    std::uint32_t link_handle,sequence_token,request_value,memory_token;
    double distance,facing;
    float deadline,duration;
    int activity,memory_id;
    unsigned entered,completed,link_valid,admitted,lookups,flags_called,flags_changed,prepare_called,aligned,request_called,begin_called,claim_called,notify_called,row_valid,sequence_called,activity_called,duration_called,restore_called,finish_called,release_called,transform_called,navigation_written,reset_called,synthetic_called,memory_called,condition_checked,debug_called;
};
struct ServerNpcGruntActBusyEventApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntActBusyEventBindings*,ServerNpcGruntActBusyEventResult*) noexcept;
};
using GetServerNpcGruntActBusyEventApi=const ServerNpcGruntActBusyEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_follow_event_export[]="SiN_GetServerNpcGruntFollowEventApi";
struct ServerNpcGruntFollowEventBindings {
    ServerNpcGruntActBusyEventBindings events;
    void* context;
    const float* default_point; // live 2251b1ec
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *origin)(void*,void*,float*) noexcept;
    int (__cdecl *release_target)(void*,std::uint32_t) noexcept; // 220d63f0
    int (__cdecl *navigation_mode)(void*,void*,int*) noexcept; // 22074060
    int (__cdecl *navigation_clear)(void*,void*,int) noexcept; // 22079250
    int (__cdecl *target_changed)(void*,void*,bool) noexcept; // 220cd790
    int (__cdecl *search_point)(void*,void*,float,float*,bool*) noexcept; // 220d1920
    int (__cdecl *goal)(void*,void*,const float*) noexcept; // mode0, return ignored
    int (__cdecl *navigation_ready)(void*,void*,bool*) noexcept; // 220740a0
    int (__cdecl *cache_refresh)(void*,std::uint32_t,float*,bool*) noexcept; // 220d3e90
    int (__cdecl *cache_allowed)(void*,void*,bool*) noexcept; // 220d2610
    int (__cdecl *queue_event)(void*,void*,int) noexcept; // 2204fe40, schedule dispatch
    int (__cdecl *clear_link)(void*,void*) noexcept; // 2204f680(0)
    int (__cdecl *debug)(void*,void*) noexcept; // 220381b0
    bool debug_enabled;
};
struct ServerNpcGruntFollowEventResult {
    ServerNpcGruntActBusyEventResult events;
    std::uint32_t target_handle,link_handle,request_value;
    float point[3],deadline,cache_point[3],timer;
    double distance;
    int emitted_event,node_mode;
    unsigned entered,completed,target_valid,link_valid,lookups,reset_called,request_called,release_called,mode_called,navigation_cleared,ideal_written,changed_called,search_called,search_found,goal_called,cache_refreshed,cache_written,cache_allowed_called,cache_same_tick,point_written,timer_written,synthetic_called,queue_called,link_cleared,node_cleared,debug_called,marker_fallback,wait_done;
};
struct ServerNpcGruntFollowEventApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntFollowEventBindings*,ServerNpcGruntFollowEventResult*) noexcept;
};
using GetServerNpcGruntFollowEventApi=const ServerNpcGruntFollowEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_follow_search_export[]="SiN_GetServerNpcGruntFollowSearchApi";
struct ServerNpcGruntFollowSearchBindings {
    ServerNpcGruntFollowEventBindings events;
    void* context;
    int (__cdecl *target_cast)(void*,void*,bool*) noexcept; // exact RTTI 223a5bab
    int (__cdecl *memory_time)(void*,void*,float*) noexcept; // 220317a0
    int (__cdecl *memory_point)(void*,void*,float*) noexcept; // 22031780
    int (__cdecl *fallback_point)(void*,void*,float*) noexcept; // 220d2d70
    int (__cdecl *target_predicate)(void*,void*,bool*) noexcept; // +120
    int (__cdecl *peer_count)(void*,void*,int*) noexcept; // 220d40b0
    int (__cdecl *orientation)(void*,void*,float*) noexcept; // 2202f0b0
    int (__cdecl *direction)(void*,const float*,float*) noexcept; // 22269b30
    int (__cdecl *prepare_point)(void*,void*,float*,bool*) noexcept; // 220d3000
    int (__cdecl *facing)(void*,void*,const float*,bool*) noexcept; // +304
    int (__cdecl *turn)(void*,void*,const float*,float) noexcept; // 2206c670
    int (__cdecl *finish_turn)(void*,void*) noexcept; // +71c
};
struct ServerNpcGruntFollowSearchResult {
    ServerNpcGruntFollowEventResult events;
    float point[3],angles[3],direction[3],timer,memory_time;
    int roll,peer_count,peer_index;
    unsigned entered,completed,initial_restart,restarts,cast_called,cast_result,memory_called,memory_selected,fallback_called,target_called,early_reset,peer_called,direction_called,point_prepared,point_ok,facing_called,facing_ok,turn_called,finish_called,reset_called,synthetic_called;
};
struct ServerNpcGruntFollowSearchApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntFollowSearchBindings*,ServerNpcGruntFollowSearchResult*) noexcept;
};
using GetServerNpcGruntFollowSearchApi=const ServerNpcGruntFollowSearchApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_assault_event_export[]="SiN_GetServerNpcGruntAssaultEventApi";
struct ServerNpcGruntAssaultEventBindings {
    ServerNpcGruntFollowSearchBindings events;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *origin)(void*,void*,float*) noexcept;
    int (__cdecl *orientation)(void*,void*,float*) noexcept;
    int (__cdecl *goal)(void*,void*,void*,const float*,int,float,int,bool*) noexcept; // primary -1/mode0, fallback 5/256/mode1
    int (__cdecl *goal_orientation)(void*,void*,const float*) noexcept; // 22073fd0
    int (__cdecl *output)(void*,void*,unsigned,void*) noexcept; // 22156900, target output offset +3a4 or +3bc, owner argument
    int (__cdecl *script)(void*,void*,std::uint32_t) noexcept; // 22031030(...,0)
    int (__cdecl *advance_node)(void*,void*) noexcept; // 220ce5b0
    int (__cdecl *sequence_lookup)(void*,void*,std::uint32_t,int*) noexcept; // 220ecb90
    int (__cdecl *sequence_apply)(void*,void*,int) noexcept; // 220f2300
};
struct ServerNpcGruntAssaultEventResult {
    ServerNpcGruntFollowSearchResult events;
    ServerNpcConditionResult condition;
    std::uint32_t handle,request_value,sequence_token;
    float point[3],angles[3],duration,deadline;
    int mode,submode,sequence,event;
    unsigned entered,completed,lookups,linked_valid,request_called,goal_calls,primary_goal,fallback_goal,condition_checked,condition_cleared,goal_orientation_called,nav_written,finish_turn,signal_ignored,flag_written,output_called,script_called,advance_called,reset_called,mode_written,duration_called,sequence_called,sequence_applied,activity_called,synthetic_called,deadline_written;
};
struct ServerNpcGruntAssaultEventApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntAssaultEventBindings*,ServerNpcGruntAssaultEventResult*) noexcept;
};
using GetServerNpcGruntAssaultEventApi=const ServerNpcGruntAssaultEventApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_assault_node_export[]="SiN_GetServerNpcGruntAssaultNodeApi";
struct ServerNpcGruntAssaultNodeBindings {
    ServerNpcGruntAssaultEventBindings events;
    void* context;
    // 221619e0 + RTTI cast: return only the handle of a typed assault node or invalid sentinel.
    int (__cdecl *find_next)(void*,std::uint32_t,std::uint32_t*) noexcept;
    int (__cdecl *missing_next)(void*,std::uint32_t) noexcept;
    int (__cdecl *class_matches)(void*,void*,std::uint32_t,bool*) noexcept; // 221387f0
    int (__cdecl *end_assault)(void*,void*) noexcept; // 2204f2d0: optional compatibility override; null uses native schedule clear
};
struct ServerNpcGruntAssaultNodeResult {
    ServerNpcGruntAssaultEventResult events;
    std::uint32_t previous_handle,next_handle,name_token,reservation;
    unsigned entered,completed,find_called,next_valid,handle_written,flag_cleared,warning_called,mode_cleared,class_called,monk_exception,end_called,secondary_checked,secondary_cleared,script_called,output_called;
};
struct ServerNpcGruntAssaultNodeApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntAssaultNodeBindings*,ServerNpcGruntAssaultNodeResult*) noexcept;
};
using GetServerNpcGruntAssaultNodeApi=const ServerNpcGruntAssaultNodeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_assault_admission_export[]="SiN_GetServerNpcGruntAssaultAdmissionApi";
struct ServerNpcGruntAssaultAdmissionBindings {
    const sin_independent::EntityHandleEntry* entries;
    unsigned count;
    std::uint32_t actor_vtable,actor_token;
    ServerNpcConditionMapBindings conditions;
    void* context;
    int (__cdecl *owner_ready)(void*,void*,bool*) noexcept; // actor +450
    int (__cdecl *owner_target)(void*,void*,bool*) noexcept; // optional compatibility override for native +154
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
};
struct ServerNpcGruntAssaultAdmissionResult {
    ServerNpcConditionResult condition;
    std::uint32_t primary_handle,secondary_handle;
    float deadline,stamp;
    int mode,submode;
    unsigned identity_valid,completed,admitted,ready_called,ready,condition_checked,deadline_blocked,
        primary_checked,primary_valid,secondary_checked,secondary_valid,mode_cleared,flags_cleared,
        target_called,target_present,stamp_written,hold_checked,hold_set,hold_cleared,condition_set,submode_written;
};
struct ServerNpcGruntAssaultAdmissionApi {
    std::uint32_t version,size;
    int (__cdecl *validate)(void*,unsigned,const ServerNpcGruntAssaultAdmissionBindings*,ServerNpcGruntAssaultAdmissionResult*) noexcept; // 220ce550
    int (__cdecl *set_submode)(void*,unsigned,int,const ServerNpcGruntAssaultAdmissionBindings*,ServerNpcGruntAssaultAdmissionResult*) noexcept; // 220ce520
    int (__cdecl *admit)(void*,unsigned,float,const ServerNpcGruntAssaultAdmissionBindings*,ServerNpcGruntAssaultAdmissionResult*) noexcept; // 220ceaf0
};
using GetServerNpcGruntAssaultAdmissionApi=const ServerNpcGruntAssaultAdmissionApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_assault_schedule_export[]="SiN_GetServerNpcGruntAssaultScheduleApi";
struct ServerNpcGruntAssaultScheduleBindings {
    ServerNpcGruntAssaultAdmissionBindings admission;
    void* context;
    int (__cdecl *origin)(void*,void*,float*) noexcept; // original origin with dirty refresh
    int (__cdecl *parent_schedule)(void*,void*,int*) noexcept; // 220cd2d0 -> parent +4
};
struct ServerNpcGruntAssaultScheduleResult {
    ServerNpcGruntAssaultAdmissionResult admission;
    ServerNpcConditionResult condition;
    std::uint32_t reservation;
    int schedule,last_condition,signal;
    unsigned completed,condition_tests,origin_calls,distance_called,primary_flag_cleared,secondary_flag_cleared,
        target_called,target_present,signal_ignored,reservation_checked,reservation_released,parent_called;
    double distance_squared;
};
struct ServerNpcGruntAssaultScheduleApi {
    std::uint32_t version,size;
    int (__cdecl *deactivate)(void*,unsigned,const ServerNpcGruntAssaultScheduleBindings*,ServerNpcGruntAssaultScheduleResult*) noexcept; // 220cebf0
    int (__cdecl *select)(void*,unsigned,const ServerNpcGruntAssaultScheduleBindings*,ServerNpcGruntAssaultScheduleResult*) noexcept; // 220cedb0
};
using GetServerNpcGruntAssaultScheduleApi=const ServerNpcGruntAssaultScheduleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_assault_policy_export[]="SiN_GetServerNpcGruntAssaultPolicyApi";
struct ServerNpcGruntAssaultPolicyBindings {
    ServerNpcGruntAssaultAdmissionBindings admission;
    ServerNpcConditionMapBindings behavior_schedules; // descriptor callback receives component, first range +4
    ServerNpcConditionMapBindings owner_schedules; // descriptor callback receives actor, first range +4
    ServerNpcGruntSquadSlotsBindings slots;
    void* context;
    int (__cdecl *parent_translate)(void*,void*,int,int*) noexcept; // parent +8
    int (__cdecl *owner_target)(void*,void*,void**) noexcept; // optional override for native +154 target
};
struct ServerNpcGruntAssaultPolicyResult {
    ServerNpcGruntAssaultAdmissionResult admission;
    ServerNpcGruntSquadSlotsResult slot;
    ServerNpcConditionResult condition;
    int incoming,schedule,local_schedule,global_schedule,matched_schedule,slot_before;
    unsigned completed,condition_tests,parent_called,timer_active,slot_checked,slot_released,
        target_called,primary_checked,primary_valid,node_flag,schedule_checks,owner_maps,interrupt_stores;
};
struct ServerNpcGruntAssaultPolicyApi {
    std::uint32_t version,size;
    int (__cdecl *translate)(void*,unsigned,int,float,const ServerNpcGruntAssaultPolicyBindings*,ServerNpcGruntAssaultPolicyResult*) noexcept; // 220cec60
    int (__cdecl *interrupts)(void*,unsigned,const ServerNpcGruntAssaultPolicyBindings*,ServerNpcGruntAssaultPolicyResult*) noexcept; // 220ced00
};
using GetServerNpcGruntAssaultPolicyApi=const ServerNpcGruntAssaultPolicyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_assault_reset_export[]="SiN_GetServerNpcGruntAssaultResetApi";
struct ServerNpcGruntAssaultResetBindings {
    ServerNpcGruntAssaultAdmissionBindings admission;
    void* context;
    // Typed name lookup + cast + entity handle. found=false is original cast failure.
    int (__cdecl *find_primary)(void*,std::uint32_t,bool*,std::uint32_t*) noexcept;
    int (__cdecl *missing_primary)(void*,std::uint32_t) noexcept;
    int (__cdecl *class_matches)(void*,void*,std::uint32_t,bool*) noexcept;
};
struct ServerNpcGruntAssaultResetResult {
    ServerNpcGruntAssaultAdmissionResult admission;
    ServerNpcConditionResult condition;
    std::uint32_t secondary_handle,primary_handle,name_token,reservation;
    int mode,submode;
    unsigned completed,fields_reset,secondary_valid,find_called,found,handle_written,warning_called,mode_cleared,
        class_called,monk_exception,schedule_cleared,reservation_checked,reservation_live,reservation_released,
        condition_checked,mode_written,ready;
};
struct ServerNpcGruntAssaultResetApi {
    std::uint32_t version,size;
    int (__cdecl *bind_primary)(void*,unsigned,const ServerNpcGruntAssaultResetBindings*,ServerNpcGruntAssaultResetResult*) noexcept; // 220ce960
    int (__cdecl *release_live)(void*,unsigned,const ServerNpcGruntAssaultResetBindings*,ServerNpcGruntAssaultResetResult*) noexcept; // 220ce8a0
    int (__cdecl *mode_ready)(void*,unsigned,const ServerNpcGruntAssaultResetBindings*,ServerNpcGruntAssaultResetResult*) noexcept; // 220cea90
};
using GetServerNpcGruntAssaultResetApi=const ServerNpcGruntAssaultResetApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_assault_task_export[]="SiN_GetServerNpcGruntAssaultTaskApi";
struct ServerNpcGruntAssaultTaskBindings {
    const ServerNpcGruntAssaultNodeBindings* node; // same existing node/event/schedule services
    ServerNpcConditionMapBindings behavior_schedules,owner_schedules;
    void* context;
    int (__cdecl *navigation_set)(void*,void*,std::uint32_t,int) noexcept; // nav +48
    int (__cdecl *facing_error)(void*,void*,std::uint32_t,double*) noexcept; // 2206d1b0
    int (__cdecl *owner_target)(void*,void*,bool*) noexcept; // optional override for native +154 presence
    int (__cdecl *debug_draw)(void*,void*,int) noexcept; // engine debug line + box for selected schedule
    bool debug_enabled;
};
struct ServerNpcGruntAssaultTaskResult {
    ServerNpcConditionResult condition;
    int task,mode,submode,debug_schedule,global_schedule;
    std::uint32_t navigation_token,request_value,handle;
    unsigned completed,identity_valid,marker_cleared,marker_restored,navigation_called,facing_called,facing_ready,
        condition_tests,reset_calls,primary_checked,primary_valid,node_flag,advance_called,secondary_valid,
        request_called,mode_written,mode_ready,target_called,target_present,synthetic_called,
        deadline_checked,deadline_elapsed,debug_checks,owner_maps,debug_called;
    double facing_error;
    float deadline;
};
struct ServerNpcGruntAssaultTaskApi {
    std::uint32_t version,size;
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntAssaultTaskBindings*,ServerNpcGruntAssaultTaskResult*,ServerNpcGruntAssaultNodeResult*,ServerNpcGruntRappelScheduleResult*) noexcept; // 220cf770
};
using GetServerNpcGruntAssaultTaskApi=const ServerNpcGruntAssaultTaskApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_assault_rally_export[]="SiN_GetServerNpcGruntAssaultRallyApi";
struct ServerNpcGruntAssaultRallyBindings {
    ServerNpcGruntAssaultResetBindings reset;
    void* context;
    int (__cdecl *cast_rally)(void*,void*,void**) noexcept; // typed RTTI cast for direct selection
    int (__cdecl *next_named)(void*,std::uint32_t,void*,void**) noexcept; // find + cast; null terminates
    int (__cdecl *entity_handle)(void*,void*,std::uint32_t*) noexcept; // virtual +8
    int (__cdecl *origin)(void*,void*,float*) noexcept; // refresh dirty origin then copy
    int (__cdecl *warning)(void*,std::uint32_t,std::uint32_t) noexcept; // format token, optional argument token
};
struct ServerNpcGruntAssaultRallyResult {
    ServerNpcGruntAssaultResetResult reset;
    std::uint32_t name_token,selected_handle,owner_handle,reservation,warning_token;
    int priority,mode;
    unsigned completed,identity_valid,cast_called,search_calls,candidates,locked,priority_updates,tie_checks,
        origin_calls,selected,handle_written,claim_called,claimed,claim_blocked,mode_written,warning_called,
        mode_cleared,class_called,monk_exception,schedule_cleared,primary_reset;
    double previous_distance;
    float candidate_distance;
};
struct ServerNpcGruntAssaultRallyApi {
    std::uint32_t version,size;
    int (__cdecl *direct)(void*,unsigned,void*,int,const ServerNpcGruntAssaultRallyBindings*,ServerNpcGruntAssaultRallyResult*) noexcept; // 220cfac0
    int (__cdecl *named)(void*,unsigned,std::uint32_t,int,const ServerNpcGruntAssaultRallyBindings*,ServerNpcGruntAssaultRallyResult*) noexcept; // 220cfc00
};
using GetServerNpcGruntAssaultRallyApi=const ServerNpcGruntAssaultRallyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_assault_point_export[]="SiN_GetServerNpcGruntAssaultPointApi";
struct ServerNpcGruntAssaultPointBindings {
    ServerNpcGruntAssaultAdmissionBindings admission;
    void* context;
    int (__cdecl *origin)(void*,void*,float*) noexcept;
    // Original parent +1c; preserve the two additional raw PE32 arguments.
    int (__cdecl *parent)(void*,void*,const float*,std::uint32_t,std::uint32_t,bool*) noexcept;
};
struct ServerNpcGruntAssaultPointResult {
    ServerNpcGruntAssaultAdmissionResult admission;
    std::uint32_t handle;
    unsigned completed,selected_offset,link_valid,origin_called,range_rejected,parent_called,accepted;
    double distance_squared;
};
struct ServerNpcGruntAssaultPointApi {
    std::uint32_t version,size;
    int (__cdecl *query)(void*,unsigned,const float*,std::uint32_t,std::uint32_t,const ServerNpcGruntAssaultPointBindings*,ServerNpcGruntAssaultPointResult*) noexcept; // 220ce7a0
    int (__cdecl *private_id)(int) noexcept; // 220ce130
};
using GetServerNpcGruntAssaultPointApi=const ServerNpcGruntAssaultPointApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_assault_lifecycle_export[]="SiN_GetServerNpcGruntAssaultLifecycleApi";
struct ServerNpcGruntAssaultLifecycleBindings {
    const ServerNpcGruntAssaultTaskBindings* tasks; // its node bindings are shared by StartTask
    void* context;
    int (__cdecl *shared_run)(void*,void*,const void*,unsigned) noexcept; // 220538d0
    int (__cdecl *other_run)(void*,void*,void*,unsigned,std::uint32_t,const void*,unsigned) noexcept; // other verified active behavior +64
    // 2202d980 capability-specific path; leaves remain explicit engine services.
    int (__cdecl *prepare)(void*,void*) noexcept; // 2204d350(0,0)
    int (__cdecl *point)(void*,void*,float*) noexcept; // 22031780
    int (__cdecl *point_accept)(void*,void*,const float*,bool*) noexcept; // +304
    int (__cdecl *aim)(void*,void*,std::uint32_t,const float*,bool) noexcept; // 2206bff0 or 2206c710, flag c0000000
    int (__cdecl *owner_ready)(void*,void*,bool*) noexcept; // +298
    const float* invalid_point; // original global 225885b8
};
struct ServerNpcGruntAssaultLifecycleResult {
    std::uint32_t active_token,behavior_vtable,capabilities,navigation_token;
    int task;
    unsigned completed,virtual_route,capability_queries,weapon_queries,capability_route,identity_valid,
        marker_set,marker_cleared,consumed,native_run,other_called,shared_called,
        prepared,point_called,point_valid,point_accept_called,point_accepted,aim_called,
        aim_linked,ready_called,ready,timer_active,timer_pending,reset_called,refresh_called,activity_cleared,activity_called;
    float point[3];
};
struct ServerNpcGruntAssaultLifecycleApi {
    std::uint32_t version,size;
    // 2202d930 StartTask wrapper, already recovered through the node API.
    int (__cdecl *start)(void*,unsigned,const void*,unsigned,float,const ServerNpcGruntAssaultLifecycleBindings*,ServerNpcGruntAssaultNodeResult*) noexcept;
    // 2202daf0 RunTask wrapper; direct_actor=true selects the actual +784 implementation 2202d980.
    int (__cdecl *run)(void*,unsigned,const void*,unsigned,float,bool,const ServerNpcGruntAssaultLifecycleBindings*,
        ServerNpcGruntAssaultLifecycleResult*,ServerNpcGruntAssaultTaskResult*,ServerNpcGruntAssaultNodeResult*,
        ServerNpcGruntRappelScheduleResult*,ServerNpcGruntEventTimerResult*) noexcept;
};
using GetServerNpcGruntAssaultLifecycleApi=const ServerNpcGruntAssaultLifecycleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_assault_activation_export[]="SiN_GetServerNpcGruntAssaultActivationApi";
struct ServerNpcGruntAssaultActivationBindings {
    const ServerNpcGruntAssaultScheduleBindings* schedule;
    const ServerNpcGruntSquadSlotsBindings* slots;
    void* context;
    std::uint8_t* activation_guard; // original global byte 2254c0a0
    // Only for other verified component identities, original +1c/+24/+20.
    int (__cdecl *other_lifecycle)(void*,void*,void*,unsigned,std::uint32_t,unsigned) noexcept;
    int (__cdecl *other_admit)(void*,void*,void*,unsigned,std::uint32_t,bool*) noexcept;
};
struct ServerNpcGruntAssaultActivationResult {
    ServerNpcGruntAssaultAdmissionResult admission;
    ServerNpcGruntAssaultScheduleResult departure;
    ServerNpcGruntSquadSlotsResult slot;
    std::uint32_t previous,next,after,departure_vtable,arrival_vtable;
    unsigned completed,changed,active_written,arrival_called,guard_set,guard_cleared,post_arrival_called,
        native_departure,other_departure,slot_released,notification_ignored,admission_checked,
        admitted,force_kept,reevaluated,deactivated;
};
struct ServerNpcGruntAssaultActivationApi {
    std::uint32_t version,size;
    int (__cdecl *change)(void*,unsigned,std::uint32_t,float,const ServerNpcGruntAssaultActivationBindings*,ServerNpcGruntAssaultActivationResult*) noexcept; // 2216e510
    int (__cdecl *reevaluate)(void*,unsigned,std::uint32_t,bool,float,const ServerNpcGruntAssaultActivationBindings*,ServerNpcGruntAssaultActivationResult*) noexcept; // 2216e8c0
};
using GetServerNpcGruntAssaultActivationApi=const ServerNpcGruntAssaultActivationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_assault_arbitration_export[]="SiN_GetServerNpcGruntAssaultArbitrationApi";
struct ServerNpcGruntAssaultArbitrationBindings {
    const ServerNpcGruntAssaultActivationBindings* activation;
    const ServerNpcGruntAssaultPolicyBindings* policy;
    void* context;
    int (__cdecl *list)(void*,std::uint32_t,const void**,unsigned*) noexcept; // owner +c1c PE32 token array
    int (__cdecl *other_select)(void*,void*,void*,unsigned,std::uint32_t,int*,bool*) noexcept; // +58 then +d4
    int (__cdecl *shared_select)(void*,void*,int*) noexcept; // 2204fd80
    int (__cdecl *other_translate)(void*,void*,void*,unsigned,std::uint32_t,int,int*) noexcept; // +6c
    int (__cdecl *shared_translate)(void*,void*,int,int*) noexcept; // 2205f1c0
};
struct ServerNpcGruntAssaultArbitrationResult {
    ServerNpcGruntAssaultActivationResult transition;
    ServerNpcGruntAssaultAdmissionResult admission;
    ServerNpcGruntAssaultScheduleResult selection;
    ServerNpcGruntAssaultPolicyResult translation;
    std::uint32_t active_token,behavior_vtable,last_candidate,selected_token;
    int selected_index,incoming,local_schedule,schedule;
    unsigned completed,arbitrated,visited,admission_calls,owner_accepts,selected,
        marker_written,native_select,other_select,shared_select,private_id,mapped,
        reverse_mapped,native_translate,other_translate,shared_translate;
};
struct ServerNpcGruntAssaultArbitrationApi {
    std::uint32_t version,size;
    int (__cdecl *arbitrate)(void*,unsigned,float,const ServerNpcGruntAssaultArbitrationBindings*,ServerNpcGruntAssaultArbitrationResult*) noexcept; // 2234e590
    int (__cdecl *select)(void*,unsigned,float,bool,const ServerNpcGruntAssaultArbitrationBindings*,ServerNpcGruntAssaultArbitrationResult*) noexcept; // 22089700 / 2234e730
    int (__cdecl *translate)(void*,unsigned,int,float,const ServerNpcGruntAssaultArbitrationBindings*,ServerNpcGruntAssaultArbitrationResult*) noexcept; // 220897a0
};
using GetServerNpcGruntAssaultArbitrationApi=const ServerNpcGruntAssaultArbitrationApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_schedule_acquire_export[]="SiN_GetServerNpcGruntScheduleAcquireApi";
struct ServerNpcGruntScheduleAcquireBindings {
    const ServerNpcGruntAssaultArbitrationBindings* arbitration;
    ServerNpcScheduleBindings schedule;
    ServerNpcConditionMapBindings owner_schedules;
    std::uint32_t registry_head; // 2254a3c0 +4
    void* context;
    // Optional compatibility override for the regular Grunt +43c jump-table arms.
    // Null enables recovered native leaves; unresolved service-dependent paths fail closed.
    int (__cdecl *special_translate)(void*,void*,int,float,int*) noexcept;
    // Original diagnostic at boundary; format VA and numeric ID, component VA or 0.
    int (__cdecl *warning)(void*,void*,std::uint32_t,int,std::uint32_t) noexcept;
};
struct ServerNpcGruntScheduleAcquireResult {
    int requested,translated,lookup_id,owner_global;
    std::uint32_t active_token,behavior_vtable,schedule_token;
    unsigned completed,lookup_calls,records,behavior_lookup,owner_lookup,descriptor_unregistered,
        unmapped,missing,warning_calls,fallback,translated_native,translated_special,
        owner_global_written,installed;
};
struct ServerNpcGruntScheduleAcquireApi {
    std::uint32_t version,size;
    int (__cdecl *get)(void*,unsigned,int,float,const ServerNpcGruntScheduleAcquireBindings*,ServerNpcGruntScheduleAcquireResult*) noexcept; // 2216dfe0
    int (__cdecl *acquire)(void*,unsigned,int,float,const ServerNpcGruntScheduleAcquireBindings*,ServerNpcGruntScheduleAcquireResult*,ServerNpcGruntAssaultArbitrationResult*) noexcept; // 2205edf0
    int (__cdecl *install)(void*,unsigned,int,float,const ServerNpcGruntScheduleAcquireBindings*,ServerNpcGruntScheduleAcquireResult*,ServerNpcGruntAssaultArbitrationResult*,ServerNpcScheduleStateResult*) noexcept; // 2204fe40
};
using GetServerNpcGruntScheduleAcquireApi=const ServerNpcGruntScheduleAcquireApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_schedule_request_export[]="SiN_GetServerNpcGruntScheduleRequestApi";
struct ServerNpcGruntScheduleRequestBindings {
    const ServerNpcGruntScheduleAcquireBindings* acquisition;
    void* context;
    int (__cdecl *state_changed)(void*,void*,int,int) noexcept; // owner +4d0
    // Original 2204e670 conditional scene/animation query, only entered on its verified gate.
    int (__cdecl *animation_schedule)(void*,void*,int*) noexcept;
    // Original 2235e5b9..2235e6ec condition48 route. 0 continues to arbitration.
    int (__cdecl *condition48_schedule)(void*,void*,int*) noexcept;
    // Original 2235e6fb..2235e8aa tail; 0 means delegate at 2235e8aa.
    int (__cdecl *normal_tail)(void*,void*,int*) noexcept;
    // Jet no-winner non-state4 route 22366172; 0 means return to shared selection is not permitted.
    int (__cdecl *jet_tail)(void*,void*,int*) noexcept;
    // Follow +5c only; other seven embedded components clear marker and return 0.
    int (__cdecl *follow_failure)(void*,void*,void*,int,int,int,int*) noexcept;
};
struct ServerNpcGruntScheduleRequestResult {
    ServerNpcGruntScheduleAcquireResult acquisition;
    ServerNpcGruntAssaultArbitrationResult selection,translation;
    ServerNpcGruntAssaultActivationResult departure;
    ServerNpcConditionResult condition;
    int requested,owner_global,current_state,current_schedule,current_task,failure_reason;
    std::uint32_t prior_active,final_active;
    unsigned completed,marker_cleared,marker_used,deactivated,condition_tests,forced_schedule,
        target_checked,state_changed,shared_selected,highlevel_selected,one_shot,
        animation_called,condition48_called,arbitrated,tail_called,owner_global_written,
        failure_marker_set,failure_marker_cleared,follow_called,failure_shared,failure_mapped;
};
struct ServerNpcGruntScheduleRequestApi {
    std::uint32_t version,size;
    int (__cdecl *choose)(void*,unsigned,float,const ServerNpcGruntScheduleRequestBindings*,ServerNpcGruntScheduleRequestResult*) noexcept; // 2216e900 -> 2204fe80
    int (__cdecl *failure)(void*,unsigned,float,const ServerNpcGruntScheduleRequestBindings*,ServerNpcGruntScheduleRequestResult*) noexcept; // 2216e940 -> 2204e8a0
};
using GetServerNpcGruntScheduleRequestApi=const ServerNpcGruntScheduleRequestApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_selector_tail_export[]="SiN_GetServerNpcGruntSelectorTailApi";
struct ServerNpcGruntSelectorTailBindings {
    const ServerNpcGruntScheduleRequestBindings* request;
    void* context;
    int (__cdecl *auxiliary)(void*,std::uint32_t,void**,unsigned*) noexcept; // resolve +ca8 PE32 pointer
    // Two original scene predicates: owner +304(player origin), then player +300(owner).
    int (__cdecl *speech_gate)(void*,void*,bool*) noexcept;
    int (__cdecl *probability)(void*,float,bool*) noexcept; // original random service +1a0
    int (__cdecl *combat_schedule)(void*,void*,int*) noexcept; // +844
    // Original 2206b220, +2fc, 2213c760(0x4081,0) chain: true means choose 28.
    int (__cdecl *navigation_blocked)(void*,void*,bool*) noexcept;
};
struct ServerNpcGruntSelectorTailResult {
    ServerNpcConditionResult condition;
    ServerNpcGruntSquadSlotsResult slot;
    int schedule;
    std::uint32_t auxiliary_token;
    unsigned completed,condition_tests,condition_clears,auxiliary_reset,speech_gate_called,speech_gate_passed,
        probability_called,combat_called,navigation_called,slot_called,slot_allowed;
    float probability_argument;
};
struct ServerNpcGruntSelectorTailApi {
    std::uint32_t version,size;
    // Original normal tail starting at 2235e719 after condition74/+ca4 tests.
    int (__cdecl *tail)(void*,unsigned,float,const ServerNpcGruntSelectorTailBindings*,ServerNpcGruntSelectorTailResult*) noexcept;
    // Integrated owner-level choose, with native normal tail instead of prior broad callback.
    int (__cdecl *choose)(void*,unsigned,float,const ServerNpcGruntSelectorTailBindings*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*) noexcept;
};
using GetServerNpcGruntSelectorTailApi=const ServerNpcGruntSelectorTailApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_combat_selector_export[]="SiN_GetServerNpcGruntCombatSelectorApi";
struct ServerNpcGruntCombatSelectorBindings {
    const ServerNpcGruntSelectorTailBindings* tail;
    void* context;
    int (__cdecl *leader_match)(void*,std::uint32_t,void*,bool*) noexcept; // 220aa2d0
    int (__cdecl *leader)(void*,std::uint32_t,void**) noexcept; // 220a9210
    int (__cdecl *leader_target)(void*,void*,void**) noexcept; // leader +154
    int (__cdecl *speech_allowed)(void*,void*,int,bool*) noexcept; // +584
    int (__cdecl *speech_mark)(void*,void*,int,float) noexcept; // +588
    int (__cdecl *target_policy)(void*,void*,bool*) noexcept; // +78c(true)
    int (__cdecl *contact_cue)(void*,void*,void*) noexcept; // 2235a470
    int (__cdecl *final_cue)(void*,void*) noexcept; // 2235a3a0
    int (__cdecl *random_int)(void*,int,int,int*) noexcept;
    int (__cdecl *special_combat)(void*,void*,bool,int*) noexcept; // 2235a560 or jet 223636e0
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    // virtual +214 center or ordinary origin 220092d0; true selects +214.
    int (__cdecl *position)(void*,void*,bool,float*) noexcept;
};
struct ServerNpcGruntCombatSelectorResult {
    ServerNpcConditionResult condition;
    ServerNpcGruntSquadSlotsResult slot;
    int schedule,roll,special_schedule;
    unsigned completed,jet,condition_tests,target_queries,leader_checks,leader_queries,leader_target_queries,
        speech_checks,speech_marked,target_policy_called,slot_calls,slot_allowed,contact_called,final_called,
        random_called,special_called,flags_written,extent_calls,position_calls,distance_checked,range_adjusted;
    double distance_squared;
    float range_before,range_after;
};
struct ServerNpcGruntCombatSelectorApi {
    std::uint32_t version,size;
    int (__cdecl *select)(void*,unsigned,float,const ServerNpcGruntCombatSelectorBindings*,ServerNpcGruntCombatSelectorResult*) noexcept; // 2235d2c0 / 223641e0
    int (__cdecl *choose)(void*,unsigned,float,const ServerNpcGruntCombatSelectorBindings*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*) noexcept;
};
using GetServerNpcGruntCombatSelectorApi=const ServerNpcGruntCombatSelectorApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_jet_selector_export[]="SiN_GetServerNpcGruntJetSelectorApi";
struct ServerNpcGruntJetSelectorBindings {
    const ServerNpcGruntCombatSelectorBindings* combat;
    void* context;
    const float* invalid_point; // 225885b8 runtime vector
    int (__cdecl *class_matches)(void*,std::uint32_t,std::uint32_t,bool*) noexcept;
    int (__cdecl *enemy_visible)(void*,void*,void*,bool*) noexcept; // +300
    int (__cdecl *enemy_update)(void*,void*,void*) noexcept; // 22363d50 / 2203e740(1)
    int (__cdecl *global_present)(void*,bool*) noexcept; // 220330c0
    int (__cdecl *navigation_blocked)(void*,void*,bool*) noexcept; // original jet-specific nav chain
};
struct ServerNpcGruntJetSelectorResult {
    ServerNpcConditionResult condition;
    int schedule;
    unsigned completed,condition_tests,target_queries,extent_calls,position_calls,radius_checked,radius_rejected,
        visibility_called,condition_set,class_checks,obstacle_valid,obstacle_class_matched,enemy_update_called,
        auxiliary_valid,global_called,navigation_called,combat_called;
    double distance_squared;
};
struct ServerNpcGruntJetSelectorApi {
    std::uint32_t version,size;
    int (__cdecl *special)(void*,unsigned,float,const ServerNpcGruntJetSelectorBindings*,ServerNpcGruntJetSelectorResult*) noexcept; // 223636e0
    int (__cdecl *tail)(void*,unsigned,float,const ServerNpcGruntJetSelectorBindings*,ServerNpcGruntJetSelectorResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntJetSelectorResult*) noexcept; // 22366172
    int (__cdecl *choose)(void*,unsigned,float,const ServerNpcGruntJetSelectorBindings*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntJetSelectorResult*,ServerNpcGruntJetSelectorResult*) noexcept;
};
using GetServerNpcGruntJetSelectorApi=const ServerNpcGruntJetSelectorApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_special_combat_export[]="SiN_GetServerNpcGruntSpecialCombatApi";
struct ServerNpcGruntSpecialCombatBindings {
    const ServerNpcGruntCombatSelectorBindings* combat;
    void* context;
    int (__cdecl *class_matches)(void*,std::uint32_t,std::uint32_t,bool*) noexcept;
    int (__cdecl *target_class)(void*,void*,int*) noexcept; // target +d0
    int (__cdecl *target_view)(void*,void*,void*,bool*) noexcept; // 22139f60 then +300
    int (__cdecl *memory_stamp)(void*,void*,void*,bool,double*) noexcept; // true: 2206b5b0, false: 2206b450
    int (__cdecl *memory_position)(void*,void*,void*,float*) noexcept; // 2206b3b0
    int (__cdecl *memory_policy)(void*,void*,bool*) noexcept; // +78c(false)
    // Original query 220cdc10, radius 500, mask 0x2000000, at most 10 entries in engine order.
    int (__cdecl *enumerate)(void*,const float*,float,std::uint32_t,void**,unsigned,unsigned*) noexcept;
    int (__cdecl *cast_candidate)(void*,void*,void**) noexcept; // typed RTTI cast
    int (__cdecl *trace)(void*,void*,void*,void*,std::uint32_t,float*) noexcept; // candidate -> target; ignore actor
    int (__cdecl *candidate_radius)(void*,void*,float*) noexcept; // 222ff670
    int (__cdecl *entity_handle)(void*,void*,std::uint32_t*) noexcept; // +8
};
struct ServerNpcGruntSpecialCombatResult {
    ServerNpcConditionResult condition;
    ServerNpcGruntSquadSlotsResult slot;
    int schedule,target_class;
    std::uint32_t candidate_handle;
    unsigned completed,condition_tests,target_queries,extent_calls,slot_calls,slot_allowed,class_checks,
        obstacle_valid,obstacle_class_matched,target_class_called,target_view_called,stamp_calls,
        policy_calls,position_calls,enumerated,candidates,casts,trace_calls,radius_calls,
        candidate_written,flag_written,memory_position_called;
    double stamp,distance_squared;
    float age,distance,trace_fraction,radius;
};
struct ServerNpcGruntSpecialCombatApi {
    std::uint32_t version,size;
    int (__cdecl *special)(void*,unsigned,float,const ServerNpcGruntSpecialCombatBindings*,ServerNpcGruntSpecialCombatResult*) noexcept; // 2235a560
    int (__cdecl *choose)(void*,unsigned,float,const ServerNpcGruntSpecialCombatBindings*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntSpecialCombatResult*) noexcept;
};
using GetServerNpcGruntSpecialCombatApi=const ServerNpcGruntSpecialCombatApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Extended translation services; existing exported binding sizes are unchanged.
inline constexpr char server_npc_grunt_schedule_services_export[]="SiN_GetServerNpcGruntScheduleServicesApi";
struct ServerNpcGruntScheduleServices {
    ServerNpcEnemyMemoryBindings storage; // writable PE32 records and dirty flags
    void* context;
    // 22038f70: grow a 20-byte record vector, preserving all existing bytes.
    int (__cdecl *resize_unreachable)(void*,std::uint32_t,unsigned,std::uint32_t*,void**,unsigned*) noexcept;
    int (__cdecl *origin)(void*,void*,float*) noexcept; // refresh dirty absolute origin
    int (__cdecl *entity_handle)(void*,void*,std::uint32_t*) noexcept; // original target +8
};
struct ServerNpcGruntScheduleServicesApi {
    std::uint32_t version,size;
    int (__cdecl *acquire)(void*,unsigned,int,float,const ServerNpcGruntScheduleAcquireBindings*,const ServerNpcGruntScheduleServices*,ServerNpcGruntScheduleAcquireResult*,ServerNpcGruntAssaultArbitrationResult*) noexcept;
    int (__cdecl *choose_normal)(void*,unsigned,float,const ServerNpcGruntSpecialCombatBindings*,const ServerNpcGruntScheduleServices*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntSpecialCombatResult*) noexcept;
    int (__cdecl *choose_jet)(void*,unsigned,float,const ServerNpcGruntJetSelectorBindings*,const ServerNpcGruntScheduleServices*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntJetSelectorResult*,ServerNpcGruntJetSelectorResult*) noexcept;
};
using GetServerNpcGruntScheduleServicesApi=const ServerNpcGruntScheduleServicesApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_schedule_policy_export[]="SiN_GetServerNpcGruntSchedulePolicyApi";
struct ServerNpcGruntSchedulePolicyServices {
    const ServerNpcGruntScheduleServices* memory;
    const ServerNpcEnemyMemoryQueryBindings* enemy_memory;
    const ServerNpcGruntPointPolicyBindings* grenade;
    const ServerNpcGruntSentenceBindings* sentence;
    const ServerNpcGruntSentenceRequestBindings* sentence_request;
    void* context;
    int (__cdecl *random_int)(void*,int,int,int*) noexcept; // 225721b0 +8, inclusive
    int (__cdecl *group_gate)(void*,int,bool*) noexcept; // 22572914 +70
    int (__cdecl *probability)(void*,float,bool*) noexcept; // 22572914 +1a0
    bool activity_debug_enabled; // 2254d090 +2c
    int (__cdecl *activity_debug)(void*,void*,int,int) noexcept;
    int (__cdecl *activity_model)(void*,void*,bool*) noexcept; // 220eb8a0
    int (__cdecl *activity_parameters)(void*,void*,int,int*,int*,int*) noexcept; // 22049a60, +884,+888,+88c
};
struct ServerNpcGruntSchedulePolicyApi {
    std::uint32_t version,size;
    int (__cdecl *acquire)(void*,unsigned,int,float,const ServerNpcGruntScheduleAcquireBindings*,const ServerNpcGruntSchedulePolicyServices*,ServerNpcGruntScheduleAcquireResult*,ServerNpcGruntAssaultArbitrationResult*) noexcept;
    int (__cdecl *choose_normal)(void*,unsigned,float,const ServerNpcGruntSpecialCombatBindings*,const ServerNpcGruntSchedulePolicyServices*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntSpecialCombatResult*) noexcept;
    int (__cdecl *choose_jet)(void*,unsigned,float,const ServerNpcGruntJetSelectorBindings*,const ServerNpcGruntSchedulePolicyServices*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntJetSelectorResult*,ServerNpcGruntJetSelectorResult*) noexcept;
};
using GetServerNpcGruntSchedulePolicyApi=const ServerNpcGruntSchedulePolicyApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_schedule_scene_export[]="SiN_GetServerNpcGruntScheduleSceneApi";
struct ServerNpcGruntScheduleSceneServices {
    const ServerNpcGruntSchedulePolicyServices* policy;
    void* context;
    const float* invalid_point; // three floats, original 225885b8
    int (__cdecl *enabled)(void*,bool*) noexcept; // adaptive +188
    int (__cdecl *npc_gate)(void*,void*,bool*) noexcept; // target +53c(0)
    int (__cdecl *eye)(void*,void*,float*) noexcept; // actor +1c4
    int (__cdecl *hull_trace)(void*,void*,const float*,const float*,const float*,const float*,unsigned,float*,float*) noexcept; // 220335e0, ignored actor, collision group 0
    int (__cdecl *point_visible)(void*,void*,const float*,unsigned,bool*) noexcept; // 2213c760, null blocker output
};
struct ServerNpcGruntScheduleSceneApi {
    std::uint32_t version,size;
    int (__cdecl *target_policy)(void*,unsigned,float,bool,const ServerNpcGruntAssaultAdmissionBindings*,const ServerNpcGruntScheduleSceneServices*,bool*) noexcept;
    int (__cdecl *acquire)(void*,unsigned,int,float,const ServerNpcGruntScheduleAcquireBindings*,const ServerNpcGruntScheduleSceneServices*,ServerNpcGruntScheduleAcquireResult*,ServerNpcGruntAssaultArbitrationResult*) noexcept;
    int (__cdecl *choose_normal)(void*,unsigned,float,const ServerNpcGruntSpecialCombatBindings*,const ServerNpcGruntScheduleSceneServices*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntSpecialCombatResult*) noexcept;
    int (__cdecl *choose_jet)(void*,unsigned,float,const ServerNpcGruntJetSelectorBindings*,const ServerNpcGruntScheduleSceneServices*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntJetSelectorResult*,ServerNpcGruntJetSelectorResult*) noexcept;
};
using GetServerNpcGruntScheduleSceneApi=const ServerNpcGruntScheduleSceneApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_schedule_line_export[]="SiN_GetServerNpcGruntScheduleLineApi";
struct ServerNpcGruntScheduleLineServices {
    const ServerNpcGruntScheduleSceneServices* scene;
    void* context;
    int (__cdecl *center)(void*,void*,float*) noexcept; // +214
    int (__cdecl *random_float)(void*,float,float,float*) noexcept; // engine RNG +4
    int (__cdecl *shoot_position)(void*,void*,float*) noexcept; // owner +358
    int (__cdecl *line_trace)(void*,void*,const float*,const float*,unsigned,int,float*,void**) noexcept; // 22014cc0
    // Live player proxy +130 then +28: entity identity for line and delegated body point.
    int (__cdecl *proxy_entity)(void*,void*,void**) noexcept;
    int (__cdecl *combat_entity)(void*,void*,void**) noexcept; // blocker +130/+8/+fc or +fc
    int (__cdecl *disposition)(void*,void*,void*,int*) noexcept; // owner +3c0
    bool debug_enabled; // 225596e0 +2c
    int (__cdecl *debug_line)(void*,const float*,const float*) noexcept;
};
struct ServerNpcGruntScheduleLineApi {
    std::uint32_t version,size;
    int (__cdecl *body_point)(void*,const float*,bool,const ServerNpcGruntAssaultAdmissionBindings*,const ServerNpcGruntScheduleLineServices*,float*) noexcept;
    int (__cdecl *acquire)(void*,unsigned,int,float,const ServerNpcGruntScheduleAcquireBindings*,const ServerNpcGruntScheduleLineServices*,ServerNpcGruntScheduleAcquireResult*,ServerNpcGruntAssaultArbitrationResult*) noexcept;
    int (__cdecl *choose_normal)(void*,unsigned,float,const ServerNpcGruntSpecialCombatBindings*,const ServerNpcGruntScheduleLineServices*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntSpecialCombatResult*) noexcept;
};
using GetServerNpcGruntScheduleLineApi=const ServerNpcGruntScheduleLineApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

// Original owner +3cc (22031120) and +6e8 spread-threshold preparation.
inline constexpr char server_npc_grunt_adaptive_spread_export[]="SiN_GetServerNpcGruntAdaptiveSpreadApi";
struct ServerNpcGruntAdaptiveSpreadBindings {
    ServerNpcEnemyMemoryQueryBindings memory;
    void* context;
    int (__cdecl *base_spread)(void*,void*,void*,void*,float*) noexcept; // 222a4b60, engine +f4
    float duration; // 22547488 +28
    float multiplier; // 22547440 +28
    const float* angle_table; // Optional 225818a8 live table override; null + count 0 selects reconstructed table.
    unsigned angle_count; // 256 for explicit override, 0 for native table
};
struct ServerNpcGruntAdaptiveSpreadResult {
    ServerNpcEnemyMemoryLookup lookup;
    float vector[3],age,threshold;
    double factor;
    unsigned completed,base_called,record_found,scaled,angle_index,table_called,clamped,no_weapon;
};
struct ServerNpcGruntAdaptiveSpreadApi {
    std::uint32_t version,size;
    int (__cdecl *spread)(void*,unsigned,void*,void*,float,const ServerNpcGruntAdaptiveSpreadBindings*,ServerNpcGruntAdaptiveSpreadResult*) noexcept;
    int (__cdecl *threshold)(void*,unsigned,void*,float,const ServerNpcGruntAdaptiveSpreadBindings*,ServerNpcGruntAdaptiveSpreadResult*) noexcept;
};
using GetServerNpcGruntAdaptiveSpreadApi=const ServerNpcGruntAdaptiveSpreadApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;

inline constexpr char server_npc_grunt_schedule_cone_export[]="SiN_GetServerNpcGruntScheduleConeApi";
struct ServerNpcGruntScheduleConeServices {
    const ServerNpcGruntScheduleLineServices* line;
    const ServerNpcGruntAdaptiveSpreadBindings* spread;
    ServerNpcEnemyMemoryBindings squad_storage;
    void* context;
    int (__cdecl *normalize)(void*,float*,float*) noexcept; // original imported VectorNormalize, returns length
    int (__cdecl *hull_width)(void*,int,float*) noexcept; // 22066270, initialized hull table
    int (__cdecl *player_count)(void*,int*) noexcept;
    int (__cdecl *player_at)(void*,int,void**) noexcept; // 22286c70, 1-based
    int (__cdecl *entity_link)(void*,void*,bool*) noexcept; // +fc subobject +2c
    int (__cdecl *excluded)(void*,void*,bool*) noexcept; // +694 for unreviewed entity class
};
struct ServerNpcGruntScheduleConePredicateResult {
    float horizontal,target_length,candidate_length,spatial,width;
    double dot;
    unsigned completed,range_rejected,length_rejected,dot_accepted,behind,width_called,blocked;
};
struct ServerNpcGruntScheduleConeResult {
    ServerNpcGruntAdaptiveSpreadResult spread;
    ServerNpcGruntScheduleConePredicateResult predicate;
    unsigned completed,allowed,player_visited,player_skipped,player_blocked,squad_visited,squad_skipped,squad_blocked;
};
struct ServerNpcGruntScheduleConeApi {
    std::uint32_t version,size;
    int (__cdecl *predicate)(void*,const float*,const float*,const float*,float,float,const ServerNpcGruntAssaultAdmissionBindings*,const ServerNpcGruntScheduleConeServices*,ServerNpcGruntScheduleConePredicateResult*) noexcept;
    int (__cdecl *filter)(void*,unsigned,void*,unsigned,const float*,const float*,float,bool,const ServerNpcGruntAssaultAdmissionBindings*,const ServerNpcGruntScheduleConeServices*,ServerNpcGruntScheduleConeResult*) noexcept;
    int (__cdecl *acquire)(void*,unsigned,int,float,const ServerNpcGruntScheduleAcquireBindings*,const ServerNpcGruntScheduleConeServices*,ServerNpcGruntScheduleAcquireResult*,ServerNpcGruntAssaultArbitrationResult*) noexcept;
};
using GetServerNpcGruntScheduleConeApi=const ServerNpcGruntScheduleConeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// Full original +6e8 query (2203c2b0), including optional condition side effects.
// The two point-visibility services are deliberately separate from the
// weapon trace/proxy combat service: 2203c490 calls direct +fc on its blocker.
inline constexpr char server_npc_grunt_combat_line_export[]="SiN_GetServerNpcGruntCombatLineApi";
struct ServerNpcGruntCombatLineBindings {
    const ServerNpcGruntAssaultAdmissionBindings* admission;
    const ServerNpcGruntScheduleLineServices* line;
    const ServerNpcGruntScheduleConeServices* cone;
    void* context;
    int (__cdecl *point_blocker)(void*,void*,const float*,unsigned,void**) noexcept; // +1ec, return blocker independently of visibility bool
    int (__cdecl *point_combat)(void*,void*,bool*) noexcept; // blocker +fc non-null
    int (__cdecl *entity_handle)(void*,void*,std::uint32_t*) noexcept; // +8 handle for 22035220
};
struct ServerNpcGruntCombatLineResult {
    ServerNpcGruntScheduleConeResult cone;
    unsigned completed,allowed,initial_allowed,weapon_called,point_called,conditions_written,blocker_written;
    unsigned capabilities;
    int initial_condition,cone_condition;
    std::uint32_t blocker_handle;
};
struct ServerNpcGruntCombatLineApi {
    std::uint32_t version,size;
    int (__cdecl *query)(void*,unsigned,const float*,const float*,float,bool,const ServerNpcGruntCombatLineBindings*,ServerNpcGruntCombatLineResult*) noexcept;
};
using GetServerNpcGruntCombatLineApi=const ServerNpcGruntCombatLineApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
inline constexpr char server_npc_grunt_native_line_attack_export[]="SiN_GetServerNpcGruntNativeLineAttackApi";
struct ServerNpcGruntNativeLineAttackResult {
    ServerNpcGruntAttackResult attack;
    ServerNpcGruntCombatLineResult lines[2];
};
struct ServerNpcGruntNativeLineAttackApi {
    std::uint32_t version,size;
    int (__cdecl *gather)(void*,unsigned,void*,float,float,const ServerNpcGruntAttackBindings*,const ServerNpcGruntCombatLineBindings*,ServerNpcGruntNativeLineAttackResult*) noexcept;
};
using GetServerNpcGruntNativeLineAttackApi=const ServerNpcGruntNativeLineAttackApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// Additive cycle API: existing result layouts and legacy interfaces remain unchanged.
inline constexpr char server_npc_grunt_native_line_cycle_export[]="SiN_GetServerNpcGruntNativeLineCycleApi";
struct ServerNpcGruntNativeLineCycleApi {
    std::uint32_t version,size;
    int (__cdecl *combat)(void*,unsigned,void*,float,const ServerNpcGruntCombatConditionsBindings*,const ServerNpcGruntGoalRefreshBindings*,const ServerNpcGruntCombatLineBindings*,ServerNpcGruntCombatConditionsResult*,ServerNpcGruntGoalRefreshResult*,ServerNpcGruntNativeLineAttackResult*) noexcept;
    int (__cdecl *post)(void*,unsigned,float,bool,const ServerNpcGruntPostUpdateBindings*,const ServerNpcGruntCombatLineBindings*,ServerNpcGruntPostUpdateResult*,ServerNpcGruntNativeLineAttackResult*) noexcept;
    int (__cdecl *update)(void*,unsigned,float,const ServerNpcGruntConditionUpdateBindings*,const ServerNpcGruntCombatLineBindings*,ServerNpcGruntConditionUpdateResult*,ServerNpcGruntNativeLineAttackResult*) noexcept;
    int (__cdecl *gather)(void*,unsigned,float,const ServerNpcGruntGatherNativeBindings*,const ServerNpcGruntCombatLineBindings*,ServerNpcGruntGatherNativeResult*,ServerNpcGruntNativeLineAttackResult*) noexcept;
};
using GetServerNpcGruntNativeLineCycleApi=const ServerNpcGruntNativeLineCycleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// Original shared weapon +330/+334; owner cast is explicit for adjusted subobjects.
struct ServerWeaponScriptView;
struct ServerNpcWeaponAmmoBindings {
    const ServerWeaponScriptView* scripts;
    void* context;
    int (__cdecl *combat_owner)(void*,void*,void**) noexcept; // optional for authenticated Grunt/player/weapon primary vtables // owner +fc
    int (__cdecl *max_carry)(void*,int,int*) noexcept; // optional live ammo definition override
};
struct ServerNpcWeaponAmmoResult {
    unsigned completed,available,uses_clip,clip_positive,owner_live,combat_live,reserve_read,unlimited;
    int clip,ammo_type,max_carry,reserve;
};
inline constexpr char server_npc_weapon_ammo_export[]="SiN_GetServerNpcWeaponAmmoApi";
struct ServerNpcWeaponAmmoApi {
    std::uint32_t version,size;
    int (__cdecl *query)(void*,unsigned,const ServerNpcGruntAssaultAdmissionBindings*,const ServerNpcWeaponAmmoBindings*,ServerNpcWeaponAmmoResult*) noexcept;
    int (__cdecl *attack)(void*,unsigned,void*,float,float,const ServerNpcGruntAttackBindings*,const ServerNpcGruntCombatLineBindings*,const ServerNpcWeaponAmmoBindings*,ServerNpcGruntNativeLineAttackResult*) noexcept;
};
using GetServerNpcWeaponAmmoApi=const ServerNpcWeaponAmmoApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
// Original 22317300 weapon melee predictor. Native arithmetic and branch
// ordering; engine virtual queries and dirty-transform refresh remain explicit.
struct ServerNpcWeaponMeleeBindings {
    void* context;
    int (__cdecl *velocity_pair)(void*,void*,float*,float*) noexcept; // target +1e8
    int (__cdecl *center)(void*,void*,float*) noexcept; // +214
    int (__cdecl *forward)(void*,void*,float*) noexcept; // owner +31c
    int (__cdecl *random)(void*,float,float,float*) noexcept;
    int (__cdecl *second_prediction)(void*,void*,bool*) noexcept; // target +120
    int (__cdecl *refresh_velocity)(void*,void*) noexcept; // 2213d750
    int (__cdecl *refresh_origin)(void*,void*) noexcept; // 2213fa60
};
struct ServerNpcWeaponMeleeResult {
    unsigned completed,has_enemy,random_called,vertical_rejected,facing_rejected,second_called,second_enabled,second_accepted;
    int condition;
    float prediction_time,predicted[3],horizontal,second_horizontal;
    double vertical,predicted_dot;
};
inline constexpr char server_npc_weapon_melee_export[]="SiN_GetServerNpcWeaponMeleeApi";
struct ServerNpcWeaponMeleeApi {
    std::uint32_t version,size;
    int (__cdecl *query)(void*,unsigned,void*,float,float,const ServerNpcGruntAssaultAdmissionBindings*,const ServerNpcWeaponMeleeBindings*,ServerNpcWeaponMeleeResult*) noexcept;
    int (__cdecl *attack)(void*,unsigned,void*,float,float,const ServerNpcGruntAttackBindings*,const ServerNpcGruntCombatLineBindings*,const ServerNpcWeaponAmmoBindings*,const ServerNpcWeaponMeleeBindings*,ServerNpcGruntNativeLineAttackResult*,ServerNpcWeaponMeleeResult*) noexcept;
};
using GetServerNpcWeaponMeleeApi=const ServerNpcWeaponMeleeApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct ServerNpcWeaponPolicyBindings {
    const ServerNpcWeaponAmmoBindings* ammo;
    const ServerNpcWeaponMeleeBindings* melee;
};
struct ServerNpcWeaponPolicyResult {
    ServerNpcWeaponAmmoResult primary,secondary;
    ServerNpcWeaponMeleeResult melee;
};
inline constexpr char server_npc_weapon_policy_cycle_export[]="SiN_GetServerNpcWeaponPolicyCycleApi";
struct ServerNpcWeaponPolicyCycleApi {
    std::uint32_t version,size;
    int (__cdecl *ammo)(void*,unsigned,const ServerNpcGruntAuxConditionsBindings*,const ServerNpcGruntAssaultAdmissionBindings*,const ServerNpcWeaponAmmoBindings*,ServerNpcGruntAuxConditionsResult*,ServerNpcWeaponPolicyResult*) noexcept;
    int (__cdecl *post)(void*,unsigned,float,bool,const ServerNpcGruntPostUpdateBindings*,const ServerNpcGruntCombatLineBindings*,const ServerNpcWeaponPolicyBindings*,ServerNpcGruntPostUpdateResult*,ServerNpcGruntNativeLineAttackResult*,ServerNpcWeaponPolicyResult*) noexcept;
};
using GetServerNpcWeaponPolicyCycleApi=const ServerNpcWeaponPolicyCycleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
int native_line_squad_report_gather(void*,unsigned,float,const ServerNpcGruntGatherNativeBindings*,const ServerNpcGruntSquadReportBindings*,ServerNpcGruntGatherNativeResult*,ServerNpcGruntSquadReportResult*,const ServerNpcGruntCombatLineBindings*,ServerNpcGruntNativeLineAttackResult*,const ServerNpcWeaponPolicyBindings* = nullptr,ServerNpcWeaponPolicyResult* = nullptr) noexcept;
int grunt_schedule_cone_normal_internal(void*,unsigned,float,const ServerNpcGruntSpecialCombatBindings*,const ServerNpcGruntScheduleLineServices*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntSpecialCombatResult*,const ServerNpcGruntScheduleConeServices*) noexcept;
int grunt_schedule_line_jet_internal(void*,unsigned,float,const ServerNpcGruntJetSelectorBindings*,const ServerNpcGruntScheduleLineServices*,ServerNpcGruntScheduleRequestResult*,ServerNpcGruntSelectorTailResult*,ServerNpcGruntCombatSelectorResult*,ServerNpcGruntJetSelectorResult*,ServerNpcGruntJetSelectorResult*,const ServerNpcGruntScheduleConeServices* = nullptr) noexcept;
int grunt_schedule_line_failure_internal(void*,unsigned,float,const ServerNpcGruntScheduleRequestBindings*,const ServerNpcGruntScheduleLineServices*,ServerNpcGruntScheduleRequestResult*,const ServerNpcGruntScheduleConeServices* = nullptr) noexcept;
}
