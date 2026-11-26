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
enum class Family:unsigned {Foreign,Resupply,Pickup,Antigen,Relay,LogicAuto,LogicTimer,LogicBranch,LogicCase};
struct FactorySpec {const char* name;Family family;unsigned original_object,original_vtable,extent;};
struct FactoryDictionary {const FactorySpec* slots[17]{};unsigned count=0;bool registered=false;};
struct RelayFactoryBindings {
    void* context;
    // Fresh allocation must supply zeroed derived extension, as expected
    // by the original constructor which writes only three output words.
    int (__cdecl *allocate)(void*,unsigned,void**,unsigned*) noexcept;
    int (__cdecl *base_construct)(void*,void*,int) noexcept;
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
};
struct AutoResult {
    unsigned completed,base_called,scheduled,global_checked,blocked,outputs,queued,removed;
    unsigned load_output,violence_output;
    int global_state,load_type;
    float next_think;
};
inline constexpr unsigned timer_vtable=0x22420e14,timer_extent=0x3f4;
inline constexpr unsigned branch_vtable=0x22422294,branch_extent=0x3c8;
inline constexpr unsigned case_vtable=0x22421d74,case_extent=0x56c;
inline constexpr unsigned compare_vtable=0x22422004,compare_extent=0x3fc;
struct CompareBindings {
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
};
struct CompareResult {
    unsigned completed,handled,stored,compared,outputs,queued,first_offset,last_offset;
    float value,threshold;
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
};
struct CaseResult {
    unsigned completed,handled,matched,selected,defaulted,eligible,visited,randomized,warned,fired,queued;
};
struct BranchBindings {
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
};
struct BranchResult {
    unsigned completed,handled,changed,fired,queued,output_offset,value;
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
};
struct TimerResult {
    unsigned completed,handled,suppressed,scheduled,randomized,fired,queued,output_offset,clamped,changed;
    float interval,next_think;
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
};
struct RelayResult {
    unsigned completed,handled,suppressed,fired,queued,removed,canceled,refire,actions;
    float maximum_delay,refire_delay;
    unsigned native_refire,native_cancel,cancel_visited,cancel_removed,protected_match;
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
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
