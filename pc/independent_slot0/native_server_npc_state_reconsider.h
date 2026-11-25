#pragma once
#include "native_server_npc_schedule_state.h"
#include "native_server_weapon_activity.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_npc_state_reconsider_export[]="SiN_GetServerNpcStateReconsiderApi";
struct ServerNpcStateBindings {
    ServerNpcScheduleBindings schedule;
    ServerWeaponActivityBindings entities;
    void* context;
    int (__cdecl *clear_enemy)(void*,void*) noexcept; // 2203e740(0,1)
    int (__cdecl *state_changed)(void*,void*,int,int) noexcept; // +4d0
    int (__cdecl *lost_enemy)(void*,void*,void*) noexcept; // 22358810
    int (__cdecl *squad_enemy)(void*,std::uint32_t,void*) noexcept; // 220a9520
    int (__cdecl *state_query)(void*,void*,int,int*) noexcept; // 2203be80/2203c0f0/220415c0
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
}
