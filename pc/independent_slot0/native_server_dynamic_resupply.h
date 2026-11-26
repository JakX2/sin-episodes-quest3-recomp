#pragma once
#include <cstdint>
#include "server_entity_handle_core.h"
namespace sin_port::native_weapon_tables { struct ServerEntityVoidOutputBindings; }
namespace sin_port::native_resupply {
inline constexpr char export_name[]="SiN_GetServerDynamicResupplyApi";
inline constexpr unsigned count=6;
enum class Convar : unsigned { Modifier, HealthKit, Debug, HealthVial, MegaHealth };
struct ConvarSpec {const char* name;const char* initial;unsigned flags;unsigned original_object;};
inline constexpr ConvarSpec convars[]{
    {"sk_dynamic_resupply_modifier","1.0",0,0x22596510u},
    {"sk_healthkit","25",0x4000u,0x225965e0u},
    {"g_debug_dynamicresupplies","0",0,0x22596558u},
    {"sk_healthvial","10",0x4000u,0x22596628u},
    {"sk_healthmega","100",0,0x225966b8u}
};
struct AmmoType {const char* classname;const char* name;int units;float full_weight;};
inline constexpr AmmoType ammo_types[count]{
    {"item_ammo_magnum","MAG",10,.5f},
    {"item_ammo_ar","AR",30,.5f},
    {"item_ammo_buckshot","BUCK",10,.5f},
    {"item_ammo_ar_grenade","ARNADE",1,.5f},
    {"item_ammo_flak","FLAK",1,.5f},
    {"item_ammo_duag","DUAG",5,.5f}
};
struct AmmoRow {float desired,current,delta;std::int32_t nearby;};
static_assert(sizeof(AmmoRow)==16);
struct InventoryView {
    const void* player;
    unsigned player_bytes;
    const sin_independent::EntityHandleEntry* entries;
    unsigned entry_count;
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
};
struct OwnershipResult {
    unsigned slots,weapons,primary_reads,secondary_reads,found;
    int slot;
    void* weapon;
};
struct Bindings {
    void* context;
    int (__cdecl *index)(void*,const char*,int*) noexcept;
    int (__cdecl *has_weapon)(void*,int,bool*) noexcept;
    int (__cdecl *max_carry)(void*,int,int*) noexcept;
    int (__cdecl *reserve)(void*,int,int*) noexcept;
    int (__cdecl *scale)(void*,float*) noexcept;
    const InventoryView* native_inventory;
    void* engine_cvar; // optional read-only VEngineCvar002
};
struct Result {unsigned rows,owned,index_native,max_native,reserve_reads,scale_reads,ownership_native,reserve_native;};
inline constexpr char health_classname[]="item_healthvial";
struct HealthBindings {
    void* context;
    int (__cdecl *vial_value)(void*,float*) noexcept;
    int (__cdecl *scale)(void*,float*) noexcept;
    void* engine_cvar;
};
struct HealthResult {unsigned vial_reads,scale_reads,completed;};
struct FallbackBindings {
    void* context;
    int (__cdecl *index)(void*,const char*,int*) noexcept;
    int (__cdecl *has_weapon)(void*,int,bool*) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    const InventoryView* native_inventory;
};
struct FallbackResult {
    unsigned enabled,index_reads,ownership_reads,eligible,forced_magnum,random_called,ownership_native;
    float cumulative[count],total,draw;
    int selected;
    const char* classname;
};
struct NearbyBindings {
    void* context;
    // Original PVS iterator 222895e0; nullptr previous starts a new pass.
    int (__cdecl *next_visible)(void*,void* actor,void* previous,void**) noexcept;
    int (__cdecl *center)(void*,void* entity,float*) noexcept;
    int (__cdecl *classname)(void*,void* entity,const char**) noexcept;
    // Optional debug mode 2 line: actor center -> candidate center.
    int (__cdecl *debug_line)(void*,const float*,const float*,int,int,int,bool,float) noexcept;
    // Optional per-row summary in any nonzero debug mode.
    int (__cdecl *debug_count)(void*,const char*,int) noexcept;
};
struct NearbyResult {unsigned visited,within,matched,center_reads,class_reads,lines,reports,completed;};
struct PickupFactoryBindings;
struct SpawnBindings {
    void* context;
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *refresh_motion)(void*,void*) noexcept;
    int (__cdecl *create)(void*,const char*,int,void**) noexcept;
    int (__cdecl *position)(void*,void*,const float*) noexcept;
    int (__cdecl *orientation)(void*,void*,const float*) noexcept;
    int (__cdecl *owner)(void*,void*,void*) noexcept;
    int (__cdecl *finalize)(void*,void*) noexcept;
    int (__cdecl *linear)(void*,void*,const float*) noexcept;
    int (__cdecl *angular)(void*,void*,const float*) noexcept;
    int (__cdecl *world_bounds)(void*,void*,float*,float*) noexcept;
    int (__cdecl *move_origin)(void*,void*,const float*) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    const PickupFactoryBindings* native_factory;
};
struct EvaluateBindings {
    const sin_independent::EntityHandleEntry* entries;
    unsigned entry_count;
    const std::uint32_t* master_handle;
    void* extent_context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    NearbyBindings nearby;
    HealthBindings health;
    Bindings ammo;
    FallbackBindings fallback;
    SpawnBindings spawn;
    bool native_player_inventory;
    void* engine_cvar; // authoritative live debug and optional nested reads
};
struct EvaluateResult {
    unsigned master_skipped,master_profile,origin_refreshes,motion_refreshes,
        factories,finalizes,normal_spawns,fallback_spawns,null_factory,
        corrections,randoms,think_cleared,removed,completed;
    unsigned native_factory_calls,native_factory_failures;
    void* partial_pickup;
    int selected_health,selected_ammo;
    unsigned debug_native;
    int debug_value;
    float next_position[3];
    AmmoRow health_row,ammo_rows[count];
    NearbyResult health_nearby,ammo_nearby;
    HealthResult health;
    Result ammo;
    FallbackResult fallback;
};
inline constexpr unsigned resupply_vtable=0x2246b4bcu,check_pvs_think=0x222f90d0u;
inline constexpr char resupply_classname[]="item_dynamic_resupply";
inline constexpr unsigned resupply_extent=0x3b0u,resupply_networkable_offset=0x160u;
inline constexpr unsigned resupply_factory_object=0x22528b70u,resupply_factory_vtable=0x2246b2c8u;
struct LifecycleBindings {
    void* context;
    // Base entity constructor and activation remain engine services.
    int (__cdecl *base_construct)(void*,void*,int) noexcept;
    int (__cdecl *mark_edict)(void*,std::uint32_t,unsigned) noexcept;
    int (__cdecl *precache)(void*,const char*,int) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *now)(void*,float*) noexcept;
    int (__cdecl *schedule)(void*,void*,float) noexcept;
    int (__cdecl *base_activate)(void*,void*) noexcept;
    int (__cdecl *entity_handle)(void*,void*,std::uint32_t*) noexcept;
    int (__cdecl *warning)(void*,const char*) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    const sin_independent::EntityHandleEntry* entries;
    unsigned entry_count;
    std::uint32_t* master_handle;
    void* schedule_context; // optional native scheduler adapter
};
struct LifecycleResult {
    unsigned base_called,flag_added,deferred_dirty,edict_marked,defaults_written,
        precached,manual,random_called,clock_read,scheduled,think_installed,
        master_assigned,master_retained,warning_called,think_cleared,removed,completed,native_handle;
    float delay,next_time;
};
struct DispatchBindings {
    void* context;
    // Original 222895a0: PVS-filtered edict for actor's +16c token.
    int (__cdecl *pvs_edict)(void*,std::uint32_t,const void**,unsigned*) noexcept;
    int (__cdecl *max_clients)(void*,int*) noexcept;
    int (__cdecl *edict)(void*,int,const void**,unsigned*) noexcept;
    int (__cdecl *base_entity)(void*,std::uint32_t,void**) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *now)(void*,float*) noexcept;
    int (__cdecl *schedule)(void*,void*,float) noexcept;
    const EvaluateBindings* evaluation;
    void* schedule_context; // optional native scheduler adapter
};
struct DispatchResult {
    unsigned pvs_called,local_called,slot_checked,edict_found,edict_free,unknown_called,
        player_found,extent_read,retry,evaluated,completed;
    int max_clients;
    float next_time;
    EvaluateResult evaluation;
};
// Engine allocation and base registration are hosted. Networkable is a
// view into owner storage, NOT a callable retail virtual object.
struct FactoryBindings {
    void* context;
    int (__cdecl *allocate)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *initialize)(void*,void*,const char*) noexcept;
    int (__cdecl *release_networkable)(void*,void*) noexcept;
    const LifecycleBindings* lifecycle;
};
struct FactoryResult {
    void* actor;
    void* networkable;
    unsigned bytes,allocation_called,allocated,constructed,initialized,released,completed;
    LifecycleResult lifecycle;
};
// Constructor-authenticated pickup fields. DUAG actual grant is 1,
 // independently of the resupply estimate of 5 nearby rounds.
struct PickupSpec {unsigned vtable,secondary,model_token,ammo_token;int units;};
inline constexpr PickupSpec ammo_pickups[count]{
    {0x22468b6c,0x22468b48,0x22468e50,0x2245f474,10},
    {0x22468ea4,0x22468e7c,0x22469188,0x2245f470,30},
    {0x224691dc,0x224691b4,0x224694c0,0x2245f468,10},
    {0x22469b54,0x22469b30,0x22469e38,0x2245f43c,1},
    {0x22469e8c,0x22469e64,0x2246a170,0x2245f424,1},
    {0x2246a1bc,0x2246a198,0x2246a4a0,0x2245f42c,1}
};
struct PlayerAmmoBindings {
    InventoryView inventory;
    void* context;
    int (__cdecl *max_carry)(void*,int,int*) noexcept;
    int (__cdecl *mark_edict)(void*,unsigned,unsigned) noexcept;
    int (__cdecl *sound)(void*,void*,const char*) noexcept;
    // SiN player override leaves: weapon +438/+43c and +330/+334.
    int (__cdecl *weapon_test)(void*,void*,unsigned,bool*) noexcept;
    int (__cdecl *denied_message)(void*,void*,int) noexcept;
    int (__cdecl *auto_switch_enabled)(void*,bool*) noexcept;
    int (__cdecl *next_best)(void*,void*,void*,void**) noexcept;
    int (__cdecl *switch_weapon)(void*,void*,void*,int) noexcept;
};
struct PlayerAmmoResult {
    unsigned sin_override,max_reads,has_ammo,slots,weapon_tests,permission,dirty,sound,denied,auto_checked,next_best_calls,switched,completed;
    int before,after,granted,maximum;
};
struct AmmoPickupBindings {
    void* context;
    int (__cdecl *base_construct)(void*,void*) noexcept;
    int (__cdecl *index)(void*,const char*,int*) noexcept;
    int (__cdecl *unknown_ammo)(void*,const char*) noexcept;
    int (__cdecl *multiplier)(void*,int,float*) noexcept;
    int (__cdecl *give)(void*,void*,int,int,bool,int*) noexcept;
    int (__cdecl *respawn_rule)(void*,void*,int*) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    const float* live_metric2; // CSE1Rules +134, GamePlayVarValue[2]
    bool native_singleplayer_rules;
    const PlayerAmmoBindings* native_grant;
};
struct AmmoPickupResult {
    unsigned type,constructed,index_read,unknown,multiplier_read,give_called,rule_called,removed,accepted,completed;
    int index,units,requested,granted,rule;
    float multiplier;
    PlayerAmmoResult native_grant;
};
inline constexpr unsigned vial_vtable=0x2246bc94u,vial_secondary=0x2246bc6cu;
struct PlayerHealthBindings {
    void* context;
    void* player;
    unsigned bytes;
    // 222b6260 event observes pre-clamp health and raw delta.
    int (__cdecl *health_event)(void*,void*,int,int,int,unsigned) noexcept;
    int (__cdecl *fade)(void*,void*,const unsigned char*,float,float,int) noexcept;
};
struct PlayerHealthResult {
    int before,maximum_before,maximum_after,converted,raw_health,raw_delta,event_type,after,delta,accepted;
    unsigned damage_mask_written,maximum_reset,overheal_flag_cleared,event_called,fade_called,completed;
};
inline constexpr unsigned megahealth_vtable=0x2246bfe4u,megahealth_secondary=0x2246bfbcu;
inline constexpr char megahealth_classname[]="item_megahealth";
struct VialPickupBindings {
    void* context;
    int (__cdecl *base_construct)(void*,void*) noexcept;
    int (__cdecl *value)(void*,float*) noexcept;
    void* engine_cvar;
    int (__cdecl *heal)(void*,void*,float,unsigned,int*) noexcept;
    // Native event identity, recipient and raw classname token. Zero token
    // represents the original empty string (223d47a8).
    int (__cdecl *item_pickup)(void*,void*,unsigned) noexcept;
    int (__cdecl *sound)(void*,void*,unsigned,const char*) noexcept;
    int (__cdecl *respawn_rule)(void*,void*,int*) noexcept;
    int (__cdecl *respawn)(void*,void*) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    bool native_singleplayer_rules;
    const PlayerHealthBindings* native_heal;
};
struct VialPickupResult {
    unsigned constructed,value_read,heal_called,message,sound,rule_called,respawned,removed,accepted,completed;
    float value;
    int healed,rule;
    unsigned mega_max_set;
    int mega_units,mega_max_before,mega_max_after;
    PlayerHealthResult native_heal;
};
inline constexpr unsigned item_touch_token=0x22175770u;
struct ItemTouchBindings {
    void* context;
    int (__cdecl *vehicle)(void*,void*,void**) noexcept;
    int (__cdecl *passenger)(void*,void*,int,void**) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *is_player)(void*,void*,bool*) noexcept;
    int (__cdecl *can_pickup)(void*,void*,void*,bool*) noexcept;
    int (__cdecl *picked_up)(void*,void*,void*) noexcept;
    int (__cdecl *respawn_rule)(void*,void*,int*) noexcept;
    int (__cdecl *respawn)(void*,void*) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    int (__cdecl *force_failed_pickup)(void*,bool*) noexcept;
    const AmmoPickupBindings* ammo;
    const VialPickupBindings* vial;
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    bool native_singleplayer_rules;
    const VialPickupBindings* mega;
};
struct ItemTouchResult {
    void* player;
    unsigned vehicle_reads,passenger_read,extent_reads,player_native,player_checked,blocked,
        permission_read,subclass_called,accepted,force_read,output_called,output_actions,
        touch_cleared,think_cleared,picked_up,rule_called,respawned,removed,completed;
    int rule;
    AmmoPickupResult ammo;
    VialPickupResult vial,mega;
};
struct PickupFactorySpec {unsigned object,vtable,create,extent;};
inline constexpr PickupFactorySpec pickup_factory_specs[count+2]{
    {0x22528114,0x22468a9c,0x222f5470,0x4d8},
    {0x22528118,0x22468aa8,0x222f55c0,0x4d8},
    {0x2252811c,0x22468ab4,0x222f5690,0x4d8},
    {0x22528128,0x22468ad8,0x222f5900,0x4d8},
    {0x2252812c,0x22468ae4,0x222f59d0,0x4d8},
    {0x22528130,0x22468af0,0x222f5aa0,0x4d8},
    {0x22528c08,0x2246b920,0x222fa280,0x4cc},
    {0x22528c0c,0x2246b92c,0x222fa470,0x4cc}
};
struct PickupFactoryBindings {
    void* context;
    int (__cdecl *allocate)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *initialize)(void*,void*,const char*) noexcept;
    int (__cdecl *release_networkable)(void*,void*) noexcept;
    const AmmoPickupBindings* ammo;
    const VialPickupBindings* vial;
    const VialPickupBindings* mega;
};
struct PickupFactoryResult {
    void* actor;
    void* networkable;
    unsigned type,bytes,allocation_called,allocated,constructed,initialized,released,completed;
    AmmoPickupResult ammo;
    VialPickupResult vial,mega;
};
// CAntigenDispenser: original PE identity tokens, never callable vtables.
inline constexpr unsigned antigen_vtable=0x2246aa6cu,antigen_extent=0x4e8u,
    antigen_think_token=0x222f7100u,antigen_touch_token=0x222f6660u;
struct AntigenBindings {
    void* context;
    const sin_independent::EntityHandleEntry* entries;
    unsigned entry_count;
    int (__cdecl *base_construct)(void*,void*) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *use_distance)(void*,void*,void*,float,bool*) noexcept;
    int (__cdecl *sound)(void*,void*,const char*,bool) noexcept;
    // Factory/attachment configuration of env_steam remains an engine leaf.
    // On successful creation return its live packed handle, or invalid on null.
    int (__cdecl *create_steam)(void*,void*,unsigned*) noexcept;
    int (__cdecl *configure_steam)(void*,void*,void*) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *schedule)(void*,void*,float) noexcept;
    int (__cdecl *pose)(void*,void*,const char*,float) noexcept;
    int (__cdecl *advance)(void*,void*) noexcept;
    const PlayerHealthBindings* health;
    void* schedule_context;
};
struct AntigenResult {
    unsigned constructed,loaded,player_checked,spatial_checked,empty,started,steam_created,
        steam_configured,steam_removed,loop_started,loop_stopped,stop_sound,
        origin_refreshes,clock_reads,scheduled,stopped,pose_called,advanced,healed,completed;
    float elapsed,dose,remaining,accumulator,integer_dose,next_time,pose;
    double distance_squared;
    PlayerHealthResult health;
};
inline constexpr unsigned antigen_canister_vtable=0x2246f094u,antigen_canister_extent=0x6a0u,
    antigen_canister_idle=0x222ff740u;
struct CanisterBindings {
    void* context;
    int (__cdecl *base_construct)(void*,void*) noexcept; // CPhysicsProp base, 221944b0
    int (__cdecl *spawn_setup)(void*,void*) noexcept; // generic CPropContainer spawn
    int (__cdecl *pose)(void*,void*,const char*,float) noexcept;
    int (__cdecl *advance)(void*,void*) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *schedule)(void*,void*,float) noexcept;
};
struct CanisterResult {
    unsigned constructed,base_spawn,amount_written,pose_called,think_installed,advanced,random_called,clock_read,scheduled,completed;
    float amount,fraction,random,next_time;
};
struct AntigenFactorySpec {const char* classname;unsigned object,vtable,create,extent;};
inline constexpr AntigenFactorySpec antigen_factories[]{
    {"item_antigen_dispenser",0x225285dcu,0x2246ad50u,0x222f6f80u,antigen_extent},
    {"prop_antigen_explosive_barrel",0x2252a0c0u,0x2246ec50u,0x223007d0u,antigen_canister_extent},
    {"prop_antigen_canister",0x2252a0c4u,0x2246ec50u,0x223007d0u,antigen_canister_extent}
};
struct AntigenFactoryBindings {
    void* context;
    int (__cdecl *allocate)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *initialize)(void*,void*,const char*) noexcept;
    int (__cdecl *release_networkable)(void*,void*) noexcept;
    const AntigenBindings* dispenser;
    const CanisterBindings* canister;
};
struct AntigenFactoryResult {
    void* actor;
    void* networkable;
    unsigned type,bytes,allocation_called,allocated,constructed,initialized,released,completed;
    AntigenResult dispenser;
    CanisterResult canister;
};
struct AntigenExchangeBindings {
    AntigenBindings control;
    void* context;
    // General CPropContainer RTTI cast for classes other than the verified
    // CAntigenCanister identity. No assumption that all containers are antigen.
    int (__cdecl *container_cast)(void*,void*,void**,unsigned*) noexcept;
    int (__cdecl *bodygroup)(void*,void*,int,int) noexcept;
    int (__cdecl *local_player)(void*,void**) noexcept;
    int (__cdecl *insert_notice)(void*,void*,void*) noexcept;
    int (__cdecl *eject_notice)(void*,void*,void*,const float*) noexcept;
    int (__cdecl *create)(void*,const char*,int,void**,unsigned*) noexcept;
    int (__cdecl *position)(void*,void*,const float*) noexcept;
    int (__cdecl *orientation)(void*,void*,const float*) noexcept;
    int (__cdecl *spawn)(void*,void*) noexcept;
    int (__cdecl *container_pose)(void*,void*,const char*,float) noexcept;
    int (__cdecl *entity_message)(void*,void*,int) noexcept;
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    const AntigenFactoryBindings* native_factory;
    const CanisterBindings* native_canister;
};
struct AntigenExchangeResult {
    void* container;
    unsigned cast_native,cast_called,eligible,rejected,transferred,created,spawned,
        bodygroup,container_pose,notice,removed,message,sound,think_cleared,
        output_called,output_actions,completed;
    float amount,reject_time,container_fraction;
    AntigenResult control;
    AntigenFactoryResult factory;
    CanisterResult canister_spawn,canister_amount;
};
inline constexpr unsigned antigen_animation_token=0x222f6aa0u,antigen_eject_token=0x222f7330u;
struct AntigenInteractionBindings {
    AntigenBindings control;
    void* context;
    // Original 222b61d0 "use" event, subobject zero. Optional event
    // absence is success; false indicates a failed host operation.
    int (__cdecl *use_event)(void*,int,void*,void*,int) noexcept;
    // Generic precache, model/solid and animation initialization remain
    // the existing engine's responsibility. No model loading here.
    int (__cdecl *spawn_setup)(void*,void*) noexcept;
    int (__cdecl *bodygroup)(void*,void*,int,int) noexcept;
    int (__cdecl *button_listener)(void*,void*,const char*) noexcept;
    int (__cdecl *create_screen)(void*,void*,unsigned*) noexcept;
    int (__cdecl *configure_screen)(void*,void*,void*) noexcept;
    int (__cdecl *screen_index)(void*,unsigned,int*) noexcept; // engine +48, edict token
};
struct AntigenInteractionResult {
    unsigned event_called,press,release,user_invalidated,setup,bodygroup,touch_installed,
        animation_installed,listener,screen_created,screen_configured,screen_reused,completed;
    int event_descriptor,screen_index;
    unsigned button_checked,button_matched,eject_installed;
    AntigenResult control;
};
// CPropContainer damage/termination, specialized to CAntigenCanister.
// The engine owns physical impulses, cloud/fragment creation and deferred deletion.
inline constexpr unsigned antigen_gas_vtable=0x2246e81cu,antigen_gas_extent=0x3bcu,
    antigen_gas_think_token=0x222fe510u;
// Gas caller supplies zero ignore class/entity and no distance falloff.
// Preserve original CTakeDamageInfo, not an invented TakeHealth pulse.
struct AntigenRadiusTrace {
    float start[3],end[3],fraction;
    void* hit;
    bool startsolid;
};
struct AntigenRadiusBindings {
    void* context;
    const sin_independent::EntityHandleEntry* entries;
    unsigned entry_count;
    int (__cdecl *contents)(void*,const float*,unsigned*) noexcept;
    int (__cdecl *enumerate)(void*,const float*,float,void**,unsigned,unsigned*) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *body_target)(void*,void*,const float*,float*) noexcept;
    int (__cdecl *trace)(void*,const float*,const float*,unsigned,void*,void*,AntigenRadiusTrace*) noexcept;
    int (__cdecl *strict_trace)(void*,bool*) noexcept;
    int (__cdecl *world)(void*,void**) noexcept;
    int (__cdecl *door_class)(void*,void*,bool*) noexcept;
    int (__cdecl *allow_corner)(void*,void*,bool*) noexcept;
    // Original 221f3354..221f3441 geometric retry, using existing trace service.
    int (__cdecl *corner_trace)(void*,void*,void*,const float*,const float*,AntigenRadiusTrace*) noexcept;
    int (__cdecl *mass)(void*,void*,float*) noexcept;
    int (__cdecl *normalize)(void*,float*) noexcept; // original VectorNormalize import
    int (__cdecl *prepare_force)(void*,void*,void*,const float*,const float*,float) noexcept;
    int (__cdecl *clear_multi)(void*) noexcept;
    int (__cdecl *trace_attack)(void*,void*,const void*,const float*,const AntigenRadiusTrace*) noexcept;
    int (__cdecl *apply_multi)(void*) noexcept;
    int (__cdecl *take_damage)(void*,void*,const void*) noexcept;
    int (__cdecl *after_effect)(void*,void*,const void*,const float*,const AntigenRadiusTrace*,const float*) noexcept;
    const PlayerHealthBindings* native_player_health;
};
struct AntigenPlayerAdmissionResult {
    unsigned completed,authenticated,base_player,dead,heal_called,damage_rejected;
    PlayerHealthResult healing;
};
struct AntigenRadiusResult {
    unsigned completed,contents_reads,enumerated,visited,ignored,water_rejected,body_targets,traces,
        strict_rejected,corner_calls,corner_rejected,obstruction_traces,obstruction_rejected,
        mass_reads,amount_rejected,force_calls,trace_attacks,direct_attacks,after_effects;
    bool wet;
    float source[3],last_amount,last_block,half_radius_squared;
    unsigned native_admissions,native_heals,native_rejected;
    AntigenPlayerAdmissionResult player;
};
struct AntigenGasBindings {
    void* context;
    int (__cdecl *allocate)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *base_construct)(void*,void*) noexcept;
    int (__cdecl *initialize)(void*,void*,const char*) noexcept;
    int (__cdecl *position)(void*,void*,const float*) noexcept;
    int (__cdecl *spawn_setup)(void*,void*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *random_float)(void*,float,float,float*) noexcept;
    int (__cdecl *schedule)(void*,void*,float) noexcept;
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    // Owner creates/configures the original particle entity; token is the
    // owner's PE32 pointer identity, not an entity handle.
    int (__cdecl *particle)(void*,void*,unsigned*) noexcept;
    int (__cdecl *remove_particle)(void*,unsigned) noexcept;
    int (__cdecl *sound)(void*,void*,const char*,bool) noexcept;
    // Original rules radius effect, including target admission and traces.
    int (__cdecl *radius_effect)(void*,void*,const float*,float,float,unsigned) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    const AntigenRadiusBindings* native_radius;
};
struct AntigenGasResult {
    void* actor;
    unsigned bytes,allocated,constructed,initialized,positioned,spawned,started,particle_created,
        particle_removed,spawn_sound,loop_sound,loop_stopped,clock_reads,random_reads,schedules,
        rate_reset,radius_called,elapsed_admitted,expired,removed,completed;
    float remaining,rate,radius,particle_life,elapsed,amount,next_time;
    AntigenRadiusResult radius_result;
};
struct AntigenDamageBindings {
    void* context;
    const sin_independent::EntityHandleEntry* entries;
    unsigned entry_count;
    int (__cdecl *damage_effect)(void*,void*,const void*) noexcept;
    int (__cdecl *handle)(void*,void*,unsigned*) noexcept;
    int (__cdecl *clock)(void*,float*) noexcept;
    int (__cdecl *start_leak)(void*,void*) noexcept;
    int (__cdecl *stop_sound)(void*,unsigned) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    int (__cdecl *break_event)(void*,int,void*,void*) noexcept;
    int (__cdecl *refresh_origin)(void*,void*) noexcept;
    int (__cdecl *sound)(void*,void*,unsigned) noexcept;
    int (__cdecl *cloud)(void*,void*,const float*,int,float,float,float,float) noexcept;
    // Foreign flag branches (non-antigen explosion and ignition) remain hosted.
    int (__cdecl *other_break_effect)(void*,void*,void*,unsigned) noexcept;
    int (__cdecl *schedule)(void*,void*,float) noexcept;
    int (__cdecl *fragment)(void*,void*,const char*,int,float,float,float) noexcept;
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    const AntigenGasBindings* native_gas;
};
struct AntigenDamageResult {
    unsigned completed,damage_admitted,damage_effect,attacker_resolved,attacker_handle,leak_started,
        leak_stopped,sound_stopped,steam_removed,leak_decremented,depleted,broken,break_event,
        output_called,output_actions,origin_refreshes,clouds,other_effects,break_sound,
        think_cleared,tick_cleared,removed,fragments;
    int native_result,leak_count;
    float elapsed,cloud_amount;
    AntigenGasResult gas;
};
struct Api {
    std::uint32_t version,size;
    // 222f8580: six desired ratios and pre-counted nearby pickup rows.
    int (__cdecl *ammo_ratios)(const float*,unsigned,AmmoRow*,unsigned,const Bindings*,Result*) noexcept;
    // 222f8aa0: health (+9c), max health (+98), desired (+394).
    int (__cdecl *health_ratio)(int,int,float,AmmoRow*,const HealthBindings*,HealthResult*) noexcept;
    // First strictly positive maximum, ties keep the earlier table row.
    int (__cdecl *greatest_deficit)(const AmmoRow*,unsigned,int*) noexcept;
    // 222f87a0: fallback selection, before original spawn helper.
    int (__cdecl *fallback)(unsigned,const float*,unsigned,const FallbackBindings*,FallbackResult*) noexcept;
    // 222f8930: independent health or six-ammo pass; other fields untouched.
    int (__cdecl *nearby)(void*,bool,int,AmmoRow*,unsigned,const NearbyBindings*,NearbyResult*) noexcept;
    // 222f8f40: evaluate health and ammo, spawn at most one of each,
    // optionally fall back, then clear think and request removal.
    int (__cdecl *evaluate)(void*,unsigned,const void*,unsigned,int,const EvaluateBindings*,EvaluateResult*) noexcept;
    // 222a44d0: first inventory weapon whose primary or secondary index matches.
    int (__cdecl *owns)(const InventoryView*,int,OwnershipResult*) noexcept;
    int (__cdecl *construct)(void*,unsigned,const LifecycleBindings*,LifecycleResult*) noexcept;
    int (__cdecl *spawn)(void*,unsigned,const LifecycleBindings*,LifecycleResult*) noexcept;
    int (__cdecl *activate)(void*,unsigned,const LifecycleBindings*,LifecycleResult*) noexcept;
    int (__cdecl *become_master)(void*,unsigned,const LifecycleBindings*,LifecycleResult*) noexcept;
    int (__cdecl *kill)(void*,unsigned,const LifecycleBindings*,LifecycleResult*) noexcept;
    // 222f90d0 and 222f9120; the latter ignores inputdata_t.
    int (__cdecl *check_pvs)(void*,unsigned,int,const DispatchBindings*,DispatchResult*) noexcept;
    int (__cdecl *calculate_type)(void*,unsigned,int,const DispatchBindings*,DispatchResult*) noexcept;
    // 222f9520/222f9540 and 222f8720, without executable PE addresses.
    int (__cdecl *factory_create)(const char*,const FactoryBindings*,FactoryResult*) noexcept;
    int (__cdecl *factory_destroy)(void*,unsigned,void*,const FactoryBindings*,FactoryResult*) noexcept;
    int (__cdecl *ammo_pickup_construct)(void*,unsigned,unsigned,const AmmoPickupBindings*,AmmoPickupResult*) noexcept;
    int (__cdecl *ammo_pickup_touch)(void*,unsigned,void*,const AmmoPickupBindings*,AmmoPickupResult*) noexcept;
    int (__cdecl *vial_construct)(void*,unsigned,const VialPickupBindings*,VialPickupResult*) noexcept;
    int (__cdecl *vial_touch)(void*,unsigned,void*,const VialPickupBindings*,VialPickupResult*) noexcept;
    // 22175770: native default CItem touch, on supported pickup classes.
    int (__cdecl *item_touch)(void*,unsigned,void*,unsigned,const ItemTouchBindings*,ItemTouchResult*) noexcept;
    int (__cdecl *player_give_ammo)(void*,int,int,bool,const PlayerAmmoBindings*,PlayerAmmoResult*) noexcept;
    int (__cdecl *player_take_health)(void*,float,unsigned,const PlayerHealthBindings*,PlayerHealthResult*) noexcept;
    int (__cdecl *pickup_factory_create)(const char*,const PickupFactoryBindings*,PickupFactoryResult*) noexcept;
    int (__cdecl *pickup_factory_destroy)(void*,unsigned,void*,const PickupFactoryBindings*,PickupFactoryResult*) noexcept;
    int (__cdecl *antigen_construct)(void*,unsigned,const AntigenBindings*,AntigenResult*) noexcept;
    int (__cdecl *antigen_use)(void*,unsigned,void*,unsigned,const AntigenBindings*,AntigenResult*) noexcept;
    int (__cdecl *antigen_think)(void*,unsigned,const AntigenBindings*,AntigenResult*) noexcept;
    int (__cdecl *antigen_insert)(void*,unsigned,void*,unsigned,const AntigenExchangeBindings*,AntigenExchangeResult*) noexcept;
    int (__cdecl *antigen_eject)(void*,unsigned,const AntigenExchangeBindings*,AntigenExchangeResult*) noexcept;
    int (__cdecl *antigen_use_event)(void*,unsigned,void*,unsigned,int,const AntigenInteractionBindings*,AntigenInteractionResult*) noexcept;
    int (__cdecl *antigen_spawn)(void*,unsigned,const AntigenInteractionBindings*,AntigenInteractionResult*) noexcept;
    int (__cdecl *antigen_animation_think)(void*,unsigned,const AntigenInteractionBindings*,AntigenInteractionResult*) noexcept;
    int (__cdecl *antigen_button_event)(void*,unsigned,const char*,int,const char*,const AntigenInteractionBindings*,AntigenInteractionResult*) noexcept;
    int (__cdecl *canister_construct)(void*,unsigned,const CanisterBindings*,CanisterResult*) noexcept;
    int (__cdecl *canister_set_amount)(void*,unsigned,float,const CanisterBindings*,CanisterResult*) noexcept;
    int (__cdecl *canister_spawn)(void*,unsigned,const CanisterBindings*,CanisterResult*) noexcept;
    int (__cdecl *canister_idle)(void*,unsigned,const CanisterBindings*,CanisterResult*) noexcept;
    int (__cdecl *antigen_factory_create)(const char*,const AntigenFactoryBindings*,AntigenFactoryResult*) noexcept;
    int (__cdecl *antigen_factory_destroy)(void*,unsigned,void*,const AntigenFactoryBindings*,AntigenFactoryResult*) noexcept;
    int (__cdecl *antigen_canister_damage)(void*,unsigned,const void*,unsigned,const AntigenDamageBindings*,AntigenDamageResult*) noexcept;
    int (__cdecl *antigen_canister_break)(void*,unsigned,const AntigenDamageBindings*,AntigenDamageResult*) noexcept;
    int (__cdecl *antigen_canister_stop_leak)(void*,unsigned,const AntigenDamageBindings*,AntigenDamageResult*) noexcept;
    int (__cdecl *antigen_gas_create)(const float*,float,float,float,float,const AntigenGasBindings*,AntigenGasResult*) noexcept;
    int (__cdecl *antigen_gas_think)(void*,unsigned,const AntigenGasBindings*,AntigenGasResult*) noexcept;
    int (__cdecl *antigen_gas_stop)(void*,unsigned,const AntigenGasBindings*,AntigenGasResult*) noexcept;
    int (__cdecl *mega_construct)(void*,unsigned,const VialPickupBindings*,VialPickupResult*) noexcept;
    int (__cdecl *mega_touch)(void*,unsigned,void*,const VialPickupBindings*,VialPickupResult*) noexcept;
    int (__cdecl *antigen_radius)(void*,unsigned,const float*,float,float,const AntigenRadiusBindings*,AntigenRadiusResult*) noexcept;
    int (__cdecl *antigen_player_admission)(void*,unsigned,const void*,unsigned,const AntigenRadiusBindings*,AntigenPlayerAdmissionResult*) noexcept;
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
