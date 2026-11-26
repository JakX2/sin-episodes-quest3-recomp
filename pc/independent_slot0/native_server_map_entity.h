#pragma once
#include <cstdint>
#include "native_server_dynamic_resupply.h"
namespace sin_port::native_weapon_tables { struct ServerEntityVoidOutputBindings; struct ServerEntityNamedEventQueueBindings; struct ServerNpcOuterCadenceBindings; }
namespace sin_port::native_map {
inline constexpr char export_name[]="SiN_GetServerMapEntityApi";
inline constexpr unsigned token_capacity=2048;
enum class Kind:unsigned {End,Word,Malformed};
struct Token {Kind kind;unsigned begin,next;char text[token_capacity];};
struct EntitySlice {unsigned begin,body,end;}; // end exclusive, includes closing brace
struct Pair {char key[token_capacity],value[token_capacity];};
enum class Family:unsigned {Foreign,Resupply,Pickup,Antigen,Relay,LogicAuto,LogicTimer,LogicBranch,LogicCase,LogicCompare,MathCounter,LogicMenuListener,LogicNavigation,LogicAutosave,LogicMultiCompare,MathRemap,MathColorBlend,TriggerDynamic,TriggerSecret};
struct FactorySpec {const char* name;Family family;unsigned original_object,original_vtable,extent;};
struct FactoryDictionary {const FactorySpec* slots[27]{};unsigned count=0;bool registered=false;};
struct RelayFactoryBindings {
    void* context;
    // Fresh allocation must supply zeroed derived extension, as expected
    // by the original constructor which writes only three output words.
    int (__cdecl *allocate)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *base_construct)(void*,void*,int) noexcept;
    int (__cdecl *initialize)(void*,void*,const char*) noexcept;
    int (__cdecl *release_networkable)(void*,void*) noexcept;
};
inline constexpr unsigned dynamic_trigger_vtable=0x2243b96c,dynamic_trigger_extent=0x47c;
inline constexpr unsigned secret_trigger_vtable=0x2243bc0c,secret_trigger_extent=0x470;
// Owner performs CBaseTrigger construction at 221be3c0, not CBaseEntity(1).
struct TriggerFactoryBindings {
    void* context;
    int (__cdecl *allocate)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *base_construct)(void*,void*) noexcept;
    int (__cdecl *initialize)(void*,void*,const char*) noexcept;
    int (__cdecl *release_networkable)(void*,void*) noexcept;
};
struct FactoryBindings {
    void* context;
    FactoryDictionary* dictionary;
    const native_resupply::FactoryBindings* resupply;
    const native_resupply::PickupFactoryBindings* pickup;
    const native_resupply::AntigenFactoryBindings* antigen;
    int (__cdecl *foreign_create)(void*,const char*,void**,unsigned*) noexcept;
    const RelayFactoryBindings* relay;
    const RelayFactoryBindings* logic_auto; // same allocation/base/init/networkable boundary
    const RelayFactoryBindings* logic_timer;
    const RelayFactoryBindings* logic_branch;
    const RelayFactoryBindings* logic_case;
    const RelayFactoryBindings* logic_compare;
    const RelayFactoryBindings* math_counter;
    const RelayFactoryBindings* logic_menu;
    const RelayFactoryBindings* logic_navigation;
    const RelayFactoryBindings* logic_autosave;
    const RelayFactoryBindings* logic_multicompare;
    const RelayFactoryBindings* math_remap;
    const RelayFactoryBindings* math_colorblend;
    const TriggerFactoryBindings* campaign_triggers;
};
struct FactoryResult {
    const FactorySpec* spec;
    void* actor;void* networkable;unsigned bytes,matched,completed;
};
struct Result {unsigned completed,entities,pairs,filtered,created,missing_factory,missing_classname,clears;unsigned offset,native_creates,foreign_creates;FactoryResult last_factory;};
// Pool storage must remain live until the native output firing path releases it.
// The counter is shared across all entity outputs, not per entity.
struct OutputActionBindings {
    void* context;
    unsigned* next_id;
    int (__cdecl *allocate)(void*,unsigned,void**) noexcept;
    int (__cdecl *release)(void*,void*) noexcept;
};
struct KeyBindings {
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *intern)(void*,const char*,unsigned*) noexcept;
    // Datamap FIELD_CUSTOM output parser; receives actual output address.
    int (__cdecl *output)(void*,void*,void*,const char*) noexcept;
    int (__cdecl *foreign)(void*,void*,const char*,const char*,bool*) noexcept;
    int (__cdecl *resupply_bounds_warning)(void*,void*) noexcept;
    const OutputActionBindings* native_output;
    int (__cdecl *navigation_warning)(void*,void*,const char*) noexcept;
};
struct KeyResult {
    unsigned completed,handled,known_class,base_field,scalar,interned,output,foreign,ignored_bounds;
    unsigned offset,field_type;
};
struct Bindings {
    void* context;
    int (__cdecl *should_create)(void*,const char*,bool*) noexcept; // optional map filter
    int (__cdecl *create)(void*,const char*,void**) noexcept;
    int (__cdecl *clear_factory_global)(void*) noexcept; // unfiltered original path
    int (__cdecl *key_value)(void*,void*,const char*,const char*) noexcept;
    int (__cdecl *missing)(void*,const char*) noexcept;
    int (__cdecl *accept_entity)(void*,void*,const char*,unsigned,const EntitySlice*) noexcept;
    const FactoryBindings* native_factory;
    const KeyBindings* native_keys;
};
struct SpawnRow {void* actor;int depth;int priority;};
struct DispatchBindings {
    void* context;
    const sin_independent::EntityHandleEntry* entries;unsigned entry_count;
    const char* const* current_map;
    int (__cdecl *resolve_string)(void*,unsigned,const char**) noexcept;
    int (__cdecl *scope)(void*,bool) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    int (__cdecl *dormant)(void*,void*) noexcept;
    // Live list, original 22162cd0 takes a starting count then reads
    // each listener slot in reverse. Owner retains valid slot storage.
    void* const* listeners;const unsigned* listener_count;unsigned listener_capacity;
    int (__cdecl *notify)(void*,void*,void*) noexcept;
};
struct DispatchResult {
    unsigned completed,entered,closed,captured_handle,spawn_called,native_spawn,stale,marked,
        global_lookup,global_added,global_dead,removed,dormant,notified;
    int native_result,global_index,global_state;
};
inline constexpr unsigned auto_vtable=0x2241f0d4,auto_extent=0x440;
struct RelayQueueBindings;
struct DeferredRemovalBindings;
struct AutoBindings {
    void* context;
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    int (__cdecl *base_activate)(void*,void*) noexcept;
    const float* now;
    const int* load_type;
    int (__cdecl *resolve_string)(void*,unsigned,const char**) noexcept;
    int (__cdecl *low_violence)(void*,bool*) noexcept;
    int (__cdecl *schedule)(void*,void*,float) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    void* schedule_context; // optional native tick scheduler
    const RelayQueueBindings* native_queue;
    const DeferredRemovalBindings* deferred;
};
struct AutoResult {
    unsigned completed,base_called,scheduled,global_checked,blocked,outputs,queued,removed;
    unsigned load_output,violence_output;
    int global_state,load_type;
    float next_think;
    unsigned native_output_queued;
};
inline constexpr unsigned timer_vtable=0x22420e14,timer_extent=0x3f4;
inline constexpr unsigned branch_vtable=0x22422294,branch_extent=0x3c8;
inline constexpr unsigned case_vtable=0x22421d74,case_extent=0x56c;
inline constexpr unsigned compare_vtable=0x22422004,compare_extent=0x3fc;
inline constexpr unsigned counter_vtable=0x22421ae4,counter_extent=0x3e8;
// Derived scalar-deleting destructor boundary, distinct from the
// factory's owner networkable-release boundary.
inline constexpr unsigned multicompare_vtable=0x22420b84,multicompare_extent=0x3d4;
// Shared CBaseEntity::InputKill owner effects. Removal is deferred.
struct EntityKillBindings {
    void* context;
    int (__cdecl *attachment_notice)(void*,void* attachment,void* actor) noexcept;
    int (__cdecl *clear_attachment)(void*,void* actor) noexcept;
    int (__cdecl *remove)(void*,void* actor) noexcept;
    const DeferredRemovalBindings* deferred;
};
inline bool kill_available(const EntityKillBindings* b) noexcept {return b&&(b->remove||b->deferred);}
struct EntityKillResult {
    unsigned completed,handled,attachment_notified,attachment_cleared,remove_requested;
};
struct MultiCompareBindings {
    void* context;
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    int (__cdecl *resolve)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *allocate)(void*,unsigned*,void**,unsigned*) noexcept;
    int (__cdecl *resolve_text)(void*,unsigned,const char**,unsigned*) noexcept;
    // Original zero-delay direct void event: target and caller self,
    // incoming activator and output ID, exact literal ForceRecompare.
    int (__cdecl *schedule)(void*,void*,void*,void*,const char*,float,unsigned) noexcept;
    const RelayQueueBindings* native_queue;
    const EntityKillBindings* kill;
    // Opt in to named output enqueue on the same live native queue.
    // Output bindings retain owner action resolution/release/debug only.
    bool native_outputs;
};
struct MultiCompareResult {
    unsigned completed,handled,converted,rejected,visited,inserted,replaced,scheduled,compared,equal,output_offset,queued,native_scheduled,native_output_queued;
};
inline constexpr unsigned autosave_vtable=0x2241f9e4,autosave_extent=0x398;
struct AutosaveBindings {
    void* context;
    int (__cdecl *new_level_unit)(void*) noexcept;
    // Read CDynamicDifficulty's original byte +6 AFTER new-level-unit.
    // Do not infer this gate from the independent fixed-skill toggle.
    int (__cdecl *difficulty_gate)(void*,unsigned char*) noexcept;
    int (__cdecl *server_command)(void*,const char*) noexcept;
    int (__cdecl *attachment_notice)(void*,void* attachment,void* actor) noexcept;
    int (__cdecl *clear_attachment)(void*,void* actor) noexcept;
    int (__cdecl *remove)(void*,void* actor) noexcept;
    const DeferredRemovalBindings* deferred;
};
struct AutosaveResult {
    unsigned completed,handled,new_level_unit,gate_read,suppressed,command_queued;
    unsigned char gate;
    unsigned attachment_notified,attachment_cleared,remove_requested;
};
inline constexpr unsigned navigation_vtable=0x2241ed04,navigation_listener_vtable=0x2241ecf0,navigation_extent=0x3a0;
struct NavigationBindings {
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    // Original named search, previous pointer and raw target token;
    // zero token denotes empty string. Return null to finish.
    int (__cdecl *find)(void*,void*,unsigned,void**) noexcept;
    int (__cdecl *listen)(void*,void*,bool) noexcept;
    int (__cdecl *match)(void*,void*,unsigned,bool*) noexcept;
    int (__cdecl *base_activate)(void*,void*) noexcept;
    int (__cdecl *base_remove)(void*,void*) noexcept;
    int (__cdecl *mark_edict)(void*,unsigned) noexcept;
};
struct NavigationResult {
    unsigned completed,handled,changed,on,registered,unregistered,base_called,visited,written,matched,dirty;
};
inline constexpr unsigned menu_vtable=0x22422534,menu_listener_vtable=0x22422524,menu_extent=0x518;
struct MenuBindings {
    void* context;
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    int (__cdecl *event_name)(void*,void*,const char**,unsigned*) noexcept;
    int (__cdecl *event_string)(void*,void*,const char*,const char*,const char**,unsigned*) noexcept;
    int (__cdecl *base_spawn)(void*,void*) noexcept;
    int (__cdecl *base_restore)(void*,void*) noexcept;
    // Return transport success, not the original manager's bool result.
    int (__cdecl *subscribe)(void*,void*,const char*,bool) noexcept;
    const RelayQueueBindings* native_queue;
};
struct MenuResult {
    unsigned completed,base_called,subscribed,name_reads,button_reads,selected,highlighted,outputs,queued,first_offset,last_offset,native_output_queued;
};
struct RegistryDestroyBindings;
struct LogicDestroyBindings {
    void* context;
    int (__cdecl *resolve)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *release_action)(void*,unsigned,void*) noexcept;
    int (__cdecl *base_destroy)(void*,void*) noexcept;
    int (__cdecl *free_actor)(void*,void*) noexcept;
    int (__cdecl *unregister_menu)(void*,void*) noexcept;
    const RegistryDestroyBindings* native_base;
};
struct LogicDestroyResult {
    unsigned completed,outputs,actions,validated,released,base_called,freed,unregistered,comparisons,comparisons_released;
    unsigned base_prefix,registry_unlinked,base_suffix;
    unsigned base_outputs,base_actions,base_released;
    unsigned trigger_vector_purged,trigger_vector_freed,trigger_vector_external;
};
// Separate entry: CBaseTrigger has three outputs and a vector before
// the common base destructor. Do not admit it to logic input maps.
struct TriggerDestroyBindings {
    const LogicDestroyBindings* shared;
    void* context;
    // Original vector allocator owner. Resolve returns the actual live
    // extent so overlapping storage can be refused before destruction.
    int (__cdecl *resolve_vector)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *free_vector)(void*,unsigned,void*) noexcept;
};
struct CounterBindings {
    void* context;
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    int (__cdecl *divide_warning)(void*,void*) noexcept;
    const RelayQueueBindings* native_queue;
};
struct CounterResult {
    unsigned completed,handled,stored,swapped,clamped,hit_max,hit_min,outputs,queued,warned;
    float value,minimum,maximum;
    unsigned native_output_queued;
};
inline constexpr unsigned remap_vtable=0x22421334,remap_extent=0x3bc;
inline constexpr unsigned colorblend_vtable=0x224215c4,colorblend_extent=0x3bc;
struct ColorBlendBindings {
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    const RelayQueueBindings* native_queue;
};
struct ColorBlendResult {
    unsigned completed,handled,reset_bounds,swapped,clamped,suppressed,stored,queued,native_output_queued,rgba;
    float input,clamped_input;
};
struct RemapBindings {
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    const RelayQueueBindings* native_queue;
};
struct RemapResult {
    unsigned completed,handled,reset_bounds,clamped,suppressed,stored,queued;
    float input,clamped_input,value;
    unsigned native_output_queued;
};
struct CompareBindings {
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    const RelayQueueBindings* native_queue;
};
struct CompareResult {
    unsigned completed,handled,stored,compared,outputs,queued,first_offset,last_offset;
    float value,threshold;
    unsigned native_output_queued;
};
struct CaseBindings {
    void* context;
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    int (__cdecl *resolve_text)(void*,unsigned,const char**,unsigned*) noexcept;
    // Optional original vstdlib formatter override, including unusual floats.
    int (__cdecl *format_variant)(void*,const void*,unsigned,char*,unsigned) noexcept;
    int (__cdecl *random_int)(void*,int,int,int*) noexcept;
    int (__cdecl *empty_warning)(void*,void*) noexcept;
    const sin_independent::EntityHandleEntry* entries;unsigned entry_count;
    void* entity_context;
    int (__cdecl *entity_extent)(void*,void*,unsigned*) noexcept;
    const RelayQueueBindings* native_queue;
};
struct CaseResult {
    unsigned completed,handled,matched,selected,defaulted,eligible,visited,randomized,warned,fired,queued,native_output_queued;
};
struct BranchBindings {
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    const RelayQueueBindings* native_queue;
};
struct BranchResult {
    unsigned completed,handled,changed,fired,queued,output_offset,value,native_output_queued;
};
struct TimerBindings {
    void* context;
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    const float* now;
    int (__cdecl *random)(void*,float,float,float*) noexcept;
    int (__cdecl *schedule)(void*,void*,float) noexcept;
    void* schedule_context;
    // Optional original default-context tick setter replaces hosted schedule.
    const native_weapon_tables::ServerNpcOuterCadenceBindings* cadence;
    const sin_independent::EntityHandleEntry* entries;unsigned entry_count;
    const float* tick_interval;
    const RelayQueueBindings* native_queue;
};
struct TimerResult {
    unsigned completed,handled,suppressed,scheduled,randomized,fired,queued,output_offset,clamped,changed;
    float interval,next_think;
    unsigned native_output_queued;
};
struct TimerPrimaryBindings {
    const TimerBindings* behavior;
    void* context;
    int (__cdecl *record)(void*,void*,int,float,int) noexcept;
    int (__cdecl *scope)(void*,void*,bool) noexcept;
};
struct TimerPrimaryResult {
    TimerResult behavior;
    unsigned completed,native_result,no_think,not_due,recorded,entered,closed,invoked;
    int due_tick,next_tick,last_tick;
    float recorded_time,last_time;
};
struct AutoPrimaryBindings {
    const AutoBindings* behavior;
    const native_weapon_tables::ServerNpcOuterCadenceBindings* cadence;
    const sin_independent::EntityHandleEntry* entries;unsigned entry_count;
    void* context;
    int (__cdecl *record)(void*,void*,int,float,int) noexcept;
    int (__cdecl *scope)(void*,void*,bool) noexcept;
};
struct AutoPrimaryResult {
    AutoResult behavior;
    unsigned completed,native_result,no_think,not_due,recorded,entered,closed,invoked,scheduled;
    int due_tick,last_tick;
    float recorded_time,last_time;
};
struct MenuBindings;
struct NavigationBindings;
// Original game-event manager +18 CreateEvent, event setters +28/+2c/+30,
// and manager +1c FireEvent. A successful create with null event is the
// retail unavailable-event branch; transport refusal is distinct.
struct TriggerControlBindings {
    void* context;
    // Original 22201900 receives embedded collision member +fc and
    // requested 16-bit solid flags. Owner must perform all side effects.
    int (__cdecl *solid_flags)(void*,void*,unsigned) noexcept;
    // Original 2213c490(actor,0), engine touch notification boundary.
    int (__cdecl *refresh)(void*,void*,int) noexcept;
};
struct TriggerControlResult {
    unsigned completed,handled,changed,disabled_written,solid_called,refreshed;
    unsigned prior_flags,requested_flags,disabled;
};
struct CampaignTriggerBindings {
    void* context;
    int (__cdecl *spawn_gate)(void*,bool*) noexcept;
    int (__cdecl *base_spawn)(void*,void*) noexcept;
    int (__cdecl *warning)(void*,const char*) noexcept;
    int (__cdecl *string_text)(void*,unsigned,const char**,unsigned*) noexcept;
    int (__cdecl *is_player)(void*,void*,bool*) noexcept;
    int (__cdecl *special)(void*,void*) noexcept;
    int (__cdecl *create_event)(void*,const char*,bool,void**) noexcept;
    int (__cdecl *set_int)(void*,void*,const char*,int) noexcept;
    int (__cdecl *set_float)(void*,void*,const char*,float) noexcept;
    int (__cdecl *set_string)(void*,void*,const char*,const char*) noexcept;
    int (__cdecl *fire_event)(void*,void*,bool) noexcept;
    // 22288a40 -> current owner +8 -> dereference full returned word.
    int (__cdecl *owner_word)(void*,int*) noexcept;
    int (__cdecl *end_game)(void*) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    const DeferredRemovalBindings* deferred;
    // Original 222bc4a0 lazy singleton owner. The native leaf writes
    // +11 only after event dispatch. Opt-in fixed policy is exclusive.
    int (__cdecl *difficulty_manager)(void*,void**,unsigned*) noexcept;
    bool fixed_campaign;
};
struct CampaignTriggerResult {
    unsigned completed,dynamic,spawn_suppressed,base_spawned,warned,nonplayer,
        special_called,event_attempted,event_unavailable,event_fired,manager_signaled,removed;
    int descriptor;
    unsigned native_removed,native_manager,fixed_observed,already_removed;
};
// Opt-in direct engine publication without changing the existing map API
// or binding/result record sizes. The caller borrows a live manager on
// the game thread. publication receives fixed_service::NativeEventResult
// or -1 when no event was attempted. Legacy event callbacks are bypassed.
inline constexpr char engine_trigger_spawn_export[]="SiN_SpawnCampaignTriggerWithEngineEvents";
inline constexpr char engine_trigger_touch_export[]="SiN_TouchCampaignTriggerWithEngineEvents";
inline constexpr int engine_event_not_attempted=-1;
using EngineTriggerSpawn=int (__cdecl *)(void*,unsigned,const CampaignTriggerBindings*,void*,CampaignTriggerResult*,int*) noexcept;
using EngineTriggerTouch=int (__cdecl *)(void*,unsigned,void*,const CampaignTriggerBindings*,void*,CampaignTriggerResult*,int*) noexcept;
struct SpawnBindings {
    void* context;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *classname)(void*,void*,const char**) noexcept;
    int (__cdecl *find_parent)(void*,unsigned,void**) noexcept;
    int (__cdecl *set_parent)(void*,void*,void*,int) noexcept;
    int (__cdecl *scope)(void*,const char*,bool) noexcept;
    int (__cdecl *engine_scope)(void*,bool) noexcept;
    int (__cdecl *debug_enabled)(void*,bool*) noexcept;
    int (__cdecl *debug_actor)(void*,void*) noexcept;
    int (__cdecl *spawn)(void*,void*,int*) noexcept;
    int (__cdecl *cleanup)(void*) noexcept;
    int (__cdecl *activate)(void*,void*) noexcept;
    // Existing native class-specific spawn methods. The dispatch wrapper
    // remains owner supplied unless this opt-in boundary is provided.
    const native_resupply::LifecycleBindings* resupply;
    const native_resupply::AntigenInteractionBindings* dispenser;
    const native_resupply::CanisterBindings* canister;
    int (__cdecl *native_spawn_begin)(void*,void*,bool*) noexcept;
    int (__cdecl *native_spawn_end)(void*,void*,int*) noexcept;
    const DispatchBindings* native_dispatch;
    int (__cdecl *virtual_spawn)(void*,void*) noexcept; // original +64, no wrapper
    const AutoBindings* logic_auto;
    const AutoPrimaryBindings* logic_auto_primary;
    const float* logic_auto_interval;
    const TimerBindings* logic_timer;
    bool native_math_counter;
    const MenuBindings* logic_menu;
    const NavigationBindings* logic_navigation;
    bool native_math_remap;
    bool native_math_colorblend;
    const CampaignTriggerBindings* campaign_triggers;
};
struct SpawnResult {
    unsigned completed,ranked,parent_reads,parents,debugged,spawned,failed,pruned,cleanups,activated,
        scopes,closes,engine_scopes,engine_closes,native_spawns,native_activations;
    unsigned native_dispatches;DispatchResult dispatch;
};
// Ordered classification mirrors original virtual +128 then RTTI
// CWorld, CNodeEnt, CLight, CPointTemplate, finally ordinary.
enum class Category:unsigned {Ordinary,TemplateEntity,World,Node,Light,PointTemplate};
struct PipelineBindings {
    void* context;
    const Bindings* parse;
    const SpawnBindings* spawn;
    int (__cdecl *scope)(void*,const char*,bool) noexcept;
    int (__cdecl *classify)(void*,void*,Category*) noexcept;
    int (__cdecl *capture_template)(void*,void*,const char*,unsigned,const EntitySlice*) noexcept;
    int (__cdecl *precache)(void*,void*) noexcept;
    int (__cdecl *remove)(void*,void*) noexcept;
    int (__cdecl *cleanup)(void*) noexcept;
    int (__cdecl *spawn_node)(void*,void*,const char*,unsigned,const EntitySlice*,int*) noexcept;
    int (__cdecl *template_prepare)(void*,void*,int*) noexcept;
    int (__cdecl *template_member)(void*,void*,int,void**) noexcept;
    int (__cdecl *template_capture_member)(void*,void*,void*,const char*,unsigned,const EntitySlice*,bool*) noexcept;
    int (__cdecl *template_finish)(void*,void*) noexcept;
};
struct PipelineResult {
    unsigned completed,scopes,closes,template_entities,worlds,nodes,lights,point_templates,
        template_members,captured,removed,cleanups,ordinary,immediate_spawns,immediate_failures;
    Result parse;
    SpawnResult spawn;
};
inline constexpr unsigned relay_vtable=0x224228cc,relay_extent=0x3b0,relay_output=0x394;
// All these retail primary tables share +50 = 221435b0 and inherit
// CBaseEntity::InputKill through their original derived datamap.
inline constexpr unsigned logic_kill_extent(unsigned vt) noexcept {
    switch(vt){
    case relay_vtable:return relay_extent;
    case auto_vtable:return auto_extent;
    case timer_vtable:return timer_extent;
    case branch_vtable:return branch_extent;
    case case_vtable:return case_extent;
    case compare_vtable:return compare_extent;
    case counter_vtable:return counter_extent;
    case menu_vtable:return menu_extent;
    case navigation_vtable:return navigation_extent;
    case autosave_vtable:return autosave_extent;
    case multicompare_vtable:return multicompare_extent;
    case remap_vtable:return remap_extent;
    case colorblend_vtable:return colorblend_extent;
    default:return 0;
    }
}
// The two SiN trigger primary tables share the base removal/update
// virtuals but have different input maps and derived destructors.
inline constexpr unsigned removal_entity_extent(unsigned vt) noexcept {
    return vt==dynamic_trigger_vtable?dynamic_trigger_extent:
           vt==secret_trigger_vtable?secret_trigger_extent:logic_kill_extent(vt);
}
// Owner supplies the existing live queue, stable registry and pool storage.
// A nonzero protected node is currently being dispatched and cannot be freed.
struct RelayQueueBindings {
    void* context;
    void* root;unsigned root_bytes,root_token;
    const native_weapon_tables::ServerEntityNamedEventQueueBindings* queue;
    const sin_independent::EntityHandleEntry* entries;unsigned entry_count;
    const float* now;
    const unsigned* protected_node;
    int (__cdecl *handle)(void*,void*,unsigned*) noexcept;
    int (__cdecl *intern)(void*,const char*,unsigned*) noexcept;
    int (__cdecl *release)(void*,unsigned,void*) noexcept;
};
struct RelayBindings {
    void* context;
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    int (__cdecl *remove)(void*,void*) noexcept;
    int (__cdecl *cancel)(void*,void*) noexcept;
    // Original direct void queue route: target, activator, caller = relay;
    // input EnableRefire, action ID zero. Owner resolves handles and clock.
    int (__cdecl *refire)(void*,void*,float) noexcept;
    const RelayQueueBindings* native_queue;
    bool native_fire;
    const DeferredRemovalBindings* deferred;
};
struct RelayResult {
    unsigned completed,handled,suppressed,fired,queued,removed,canceled,refire,actions;
    float maximum_delay,refire_delay;
    unsigned native_refire,native_cancel,cancel_visited,cancel_removed,protected_match;
    unsigned native_output_queued,native_removed;
};
// Independent owner storage for the original CUtlVector of networkable
// pointers (2255a8b8, count 2255a8c4). No retail allocator is assumed.
struct DeferredRemovalQueue {
    void** slots;
    unsigned capacity,count,reserved;
    bool cleanup_active,handle_access_disabled,chained_update;
};
struct DeletionListenerBindings {
    void* context;
    const sin_independent::EntityHandleEntry* entries;unsigned entry_count;
    void** slots;unsigned* count;unsigned capacity;
    // Owner retains live slot storage across callbacks.
    int (__cdecl *deleted)(void*,void* listener,void* entity) noexcept;
};
struct DeletionListenerResult {unsigned completed,stale,notified,unregistered;};
// Opt-in non-physics metadata portion of UpdateOnRemove, 2214471f..22144787.
struct RemovalMetadataBindings {
    void* context;
    int (__cdecl *string_text)(void*,unsigned,const char**,unsigned*) noexcept;
    int (__cdecl *edict)(void*,unsigned,void**,unsigned*) noexcept;
    // Original +224 cleanup only when +218 is nonzero. Owner handles
    // this existing engine/physics boundary, not the native metadata leaf.
    int (__cdecl *cleanup_224)(void*,void*) noexcept;
};
// Original descendant snapshot and auxiliary handle vector removal.
struct RemovalHierarchyBindings {
    void* context;
    // 22143610(0,0) and 221743f0: existing move/parent owner boundary.
    int (__cdecl *before_collect)(void*,void*) noexcept;
    int (__cdecl *extent)(void*,void*,unsigned*) noexcept;
    int (__cdecl *warning)(void*,void*,unsigned) noexcept;
    int (__cdecl *foreign_remove)(void*,void*) noexcept;
    unsigned* handles;unsigned* count;unsigned capacity;
};
struct NativeRemovalUpdateBindings {
    const DeletionListenerBindings* listeners;
    void* context;
    // Remainder at 2214471f, 22144787 with metadata, or 2214484f with hierarchy.
    int (__cdecl *tail)(void*,void*) noexcept;
    const RemovalMetadataBindings* metadata;
    const RemovalHierarchyBindings* hierarchy;
};
struct DeferredRemovalBindings {
    void* context;
    DeferredRemovalQueue* queue;
    // Authenticated primary +17c virtual, including the navigation override.
    int (__cdecl *update)(void*,void*,unsigned) noexcept;
    // Optional original pre-cleanup manager and profiling scope.
    int (__cdecl *pre_cleanup)(void*) noexcept;
    int (__cdecl *scope)(void*,bool) noexcept;
    // Networkable +18: owner releases the object and invalidates its registry.
    // False means this networkable is still live and may be retried.
    int (__cdecl *release)(void*,void*) noexcept;
    const NativeRemovalUpdateBindings* native_update;
};
struct DeferredRemovalResult {
    unsigned completed,already_marked,marked,updated,name_cleared,invalid_handle,queued,
        entered,pre_called,released,closed,remaining,discarded;
    unsigned native_update,notified,unregistered,stale_notification;
    unsigned metadata,edict_flag_added,global_checked,global_dead,global_noop,cleanup_224,flags_cleared,dirty_marked;
    unsigned hierarchy,children_collected,children_requested,children_native,children_foreign,handle_vector_removed,handle_vector_index;
};
// Original CBaseEntityList active and non-networked free chains.
struct RegistryChain {sin_independent::EntityHandleEntry* head;sin_independent::EntityHandleEntry* tail;};
struct RemovalRegistry {
    sin_independent::EntityHandleEntry* entries;unsigned count;
    RegistryChain active,free_nonnetworked;
    unsigned entity_count,edict_count;
    bool unlink_active;
};
struct RegistryRemovalBindings {
    void* context;
    RemovalRegistry* registry;
    // Resolve the original nonzero +16c edict token for SetRefEHandle.
    int (__cdecl *edict)(void*,unsigned,void**,unsigned*) noexcept;
};
struct RegistryRemovalResult {
    unsigned completed,empty,index,old_handle,old_serial,new_serial,handle_cleared,edict_cleared,counts_changed,unlinked,returned_to_free;
};
// Original base destructor split at the registry unlink call. Prefix
// and suffix retain the not-yet-reconstructed owner operations.
struct RegistryDestroyBindings {
    const RegistryRemovalBindings* registry;
    void* context;
    int (__cdecl *prefix)(void*,void*) noexcept; // 2214427b..221442af
    int (__cdecl *suffix)(void*,void*) noexcept; // 221442c3..22144357, or 221442fa.. when native_outputs
    bool native_outputs;
};
struct Api {
    std::uint32_t version,size;
    int (__cdecl *token)(const char*,unsigned,unsigned,Token*) noexcept;
    int (__cdecl *slice)(const char*,unsigned,unsigned,EntitySlice*) noexcept;
    int (__cdecl *lookup)(const char*,unsigned,const EntitySlice*,const char*,char*,unsigned,bool*) noexcept;
    int (__cdecl *parse)(const char*,unsigned,const Bindings*,Result*) noexcept;
    int (__cdecl *register_factories)(FactoryDictionary*) noexcept;
    const FactorySpec* (__cdecl *find_factory)(const FactoryDictionary*,const char*) noexcept;
    int (__cdecl *create_factory)(const char*,const FactoryBindings*,FactoryResult*) noexcept;
    int (__cdecl *spawn_ordinary)(SpawnRow*,unsigned,bool,const SpawnBindings*,SpawnResult*) noexcept;
    int (__cdecl *dispatch_spawn)(void*,const SpawnBindings*,DispatchResult*) noexcept;
    int (__cdecl *key_value)(void*,const char*,const char*,const KeyBindings*,KeyResult*) noexcept;
    int (__cdecl *parse_all)(const char*,unsigned,bool,const PipelineBindings*,PipelineResult*) noexcept;
    int (__cdecl *relay_input)(void*,unsigned,const char*,void*,const RelayBindings*,RelayResult*) noexcept;
    int (__cdecl *register_relay)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_relay)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *auto_activate)(void*,unsigned,const AutoBindings*,AutoResult*) noexcept;
    int (__cdecl *auto_think)(void*,unsigned,const AutoBindings*,AutoResult*) noexcept;
    int (__cdecl *register_auto)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_auto)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *auto_primary_activate)(void*,unsigned,float,const AutoPrimaryBindings*,AutoPrimaryResult*) noexcept;
    int (__cdecl *auto_primary_run)(void*,unsigned,int,float,const AutoPrimaryBindings*,AutoPrimaryResult*) noexcept;
    int (__cdecl *timer_spawn)(void*,unsigned,const TimerBindings*,TimerResult*) noexcept;
    int (__cdecl *timer_input)(void*,unsigned,const char*,unsigned,float,const TimerBindings*,TimerResult*) noexcept;
    int (__cdecl *register_timer)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_timer)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *timer_primary_run)(void*,unsigned,int,const TimerPrimaryBindings*,TimerPrimaryResult*) noexcept;
    int (__cdecl *branch_input)(void*,unsigned,const char*,void*,unsigned,unsigned char,const BranchBindings*,BranchResult*) noexcept;
    int (__cdecl *register_branch)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_branch)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *case_input)(void*,unsigned,const char*,void*,const void*,unsigned,const CaseBindings*,CaseResult*) noexcept;
    int (__cdecl *register_case)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_case)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *compare_input)(void*,unsigned,const char*,void*,unsigned,float,const CompareBindings*,CompareResult*) noexcept;
    int (__cdecl *register_compare)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_compare)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *counter_spawn)(void*,unsigned,CounterResult*) noexcept;
    int (__cdecl *counter_input)(void*,unsigned,const char*,void*,unsigned,float,const CounterBindings*,CounterResult*) noexcept;
    int (__cdecl *register_counter)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_counter)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *destroy_logic)(void*,unsigned,unsigned,const LogicDestroyBindings*,LogicDestroyResult*) noexcept;
    int (__cdecl *menu_listen)(void*,unsigned,bool,const MenuBindings*,MenuResult*) noexcept;
    int (__cdecl *menu_event)(void*,unsigned,void*,const MenuBindings*,MenuResult*) noexcept;
    int (__cdecl *register_menu)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_menu)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *navigation_input)(void*,unsigned,const char*,const NavigationBindings*,NavigationResult*) noexcept;
    int (__cdecl *navigation_activate)(void*,unsigned,const NavigationBindings*,NavigationResult*) noexcept;
    int (__cdecl *navigation_created)(void*,unsigned,void*,const NavigationBindings*,NavigationResult*) noexcept;
    int (__cdecl *navigation_remove)(void*,unsigned,const NavigationBindings*,NavigationResult*) noexcept;
    int (__cdecl *register_navigation)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_navigation)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *register_autosave)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_autosave)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *autosave_input)(void*,unsigned,const char*,const AutosaveBindings*,AutosaveResult*) noexcept;
    int (__cdecl *autosave_kill)(void*,unsigned,const sin_independent::EntityHandleEntry*,unsigned,const AutosaveBindings*,AutosaveResult*) noexcept;
    int (__cdecl *multicompare_input)(void*,unsigned,const char*,void*,const void*,unsigned,unsigned,const MultiCompareBindings*,MultiCompareResult*) noexcept;
    int (__cdecl *register_multicompare)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_multicompare)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *multicompare_kill)(void*,unsigned,const sin_independent::EntityHandleEntry*,unsigned,const EntityKillBindings*,EntityKillResult*) noexcept;
    int (__cdecl *remap_spawn)(void*,unsigned,RemapResult*) noexcept;
    int (__cdecl *remap_input)(void*,unsigned,const char*,void*,unsigned,float,const RemapBindings*,RemapResult*) noexcept;
    int (__cdecl *register_remap)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_remap)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *colorblend_spawn)(void*,unsigned,ColorBlendResult*) noexcept;
    int (__cdecl *colorblend_input)(void*,unsigned,const char*,void*,unsigned,float,const ColorBlendBindings*,ColorBlendResult*) noexcept;
    int (__cdecl *register_colorblend)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_colorblend)(void*,unsigned,void*,const RelayFactoryBindings*) noexcept;
    int (__cdecl *logic_kill)(void*,unsigned,unsigned,const sin_independent::EntityHandleEntry*,unsigned,const EntityKillBindings*,EntityKillResult*) noexcept;
    int (__cdecl *defer_logic_remove)(void*,unsigned,const DeferredRemovalBindings*,DeferredRemovalResult*) noexcept;
    int (__cdecl *cleanup_deferred)(const DeferredRemovalBindings*,DeferredRemovalResult*) noexcept;
    int (__cdecl *discard_deferred)(DeferredRemovalQueue*,DeferredRemovalResult*) noexcept;
    int (__cdecl *registry_unlink_logic)(void*,unsigned,unsigned,const RegistryRemovalBindings*,RegistryRemovalResult*) noexcept;
    int (__cdecl *notify_deleting)(unsigned,const DeletionListenerBindings*,DeletionListenerResult*) noexcept;
    int (__cdecl *register_campaign_triggers)(FactoryDictionary*) noexcept;
    int (__cdecl *destroy_campaign_trigger)(void*,unsigned,void*,const TriggerFactoryBindings*) noexcept;
    int (__cdecl *campaign_trigger_spawn)(void*,unsigned,const CampaignTriggerBindings*,CampaignTriggerResult*) noexcept;
    int (__cdecl *campaign_trigger_touch)(void*,unsigned,void*,const CampaignTriggerBindings*,CampaignTriggerResult*) noexcept;
    int (__cdecl *destroy_trigger_members)(void*,unsigned,unsigned,const TriggerDestroyBindings*,LogicDestroyResult*) noexcept;
    int (__cdecl *trigger_control)(void*,unsigned,const char*,const TriggerControlBindings*,TriggerControlResult*) noexcept;
    int (__cdecl *trigger_kill)(void*,unsigned,unsigned,const sin_independent::EntityHandleEntry*,unsigned,const EntityKillBindings*,EntityKillResult*) noexcept;
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
