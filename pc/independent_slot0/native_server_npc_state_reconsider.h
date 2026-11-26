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
}
