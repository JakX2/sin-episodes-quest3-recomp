#pragma once
#include <cstdint>
#include "server_entity_handle_core.h"
namespace sin_port::native_resupply {
inline constexpr char export_name[]="SiN_GetServerDynamicResupplyApi";
inline constexpr unsigned count=6;
enum class Convar : unsigned { Modifier, HealthKit, Debug };
struct ConvarSpec {const char* name;const char* initial;unsigned flags;unsigned original_object;};
inline constexpr ConvarSpec convars[]{
    {"sk_dynamic_resupply_modifier","1.0",0,0x22596510u},
    {"sk_healthkit","25",0x4000u,0x225965e0u},
    {"g_debug_dynamicresupplies","0",0,0x22596558u}
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
struct AmmoPickupBindings {
    void* context;
    int (__cdecl *base_construct)(void*,void*) noexcept;
    int (__cdecl *index)(void*,const char*,int*) noexcept;
    int (__cdecl *unknown_ammo)(void*,const char*) noexcept;
    int (__cdecl *multiplier)(void*,int,float*) noexcept;
    int (__cdecl *give)(void*,void*,int,int,bool,int*) noexcept;
    int (__cdecl *respawn_rule)(void*,void*,int*) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
};
struct AmmoPickupResult {
    unsigned type,constructed,index_read,unknown,multiplier_read,give_called,rule_called,removed,accepted,completed;
    int index,units,requested,granted,rule;
    float multiplier;
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
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
