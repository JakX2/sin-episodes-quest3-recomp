#pragma once
#include <cstdint>
#include "native_server_dynamic_resupply.h"
namespace sin_port::native_weapon_tables { struct ServerEntityVoidOutputBindings; }
namespace sin_port::native_map {
inline constexpr char export_name[]="SiN_GetServerMapEntityApi";
inline constexpr unsigned token_capacity=2048;
enum class Kind:unsigned {End,Word,Malformed};
struct Token {Kind kind;unsigned begin,next;char text[token_capacity];};
struct EntitySlice {unsigned begin,body,end;}; // end exclusive, includes closing brace
struct Pair {char key[token_capacity],value[token_capacity];};
enum class Family:unsigned {Foreign,Resupply,Pickup,Antigen};
struct FactorySpec {const char* name;Family family;unsigned original_object,original_vtable,extent;};
struct FactoryDictionary {const FactorySpec* slots[12]{};unsigned count=0;bool registered=false;};
struct FactoryBindings {
    void* context;
    FactoryDictionary* dictionary;
    const native_resupply::FactoryBindings* resupply;
    const native_resupply::PickupFactoryBindings* pickup;
    const native_resupply::AntigenFactoryBindings* antigen;
    int (__cdecl *foreign_create)(void*,const char*,void**,unsigned*) noexcept;
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
struct RelayBindings {
    void* context;
    const native_weapon_tables::ServerEntityVoidOutputBindings* output;
    int (__cdecl *remove)(void*,void*) noexcept;
    int (__cdecl *cancel)(void*,void*) noexcept;
    // Original direct void queue route: target, activator, caller = relay;
    // input EnableRefire, action ID zero. Owner resolves handles and clock.
    int (__cdecl *refire)(void*,void*,float) noexcept;
};
struct RelayResult {
    unsigned completed,handled,suppressed,fired,queued,removed,canceled,refire,actions;
    float maximum_delay,refire_delay;
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
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
