#pragma once
#include <cstdint>
#include "native_server_dynamic_resupply.h"
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
struct Bindings {
    void* context;
    int (__cdecl *should_create)(void*,const char*,bool*) noexcept; // optional map filter
    int (__cdecl *create)(void*,const char*,void**) noexcept;
    int (__cdecl *clear_factory_global)(void*) noexcept; // unfiltered original path
    int (__cdecl *key_value)(void*,void*,const char*,const char*) noexcept;
    int (__cdecl *missing)(void*,const char*) noexcept;
    int (__cdecl *accept_entity)(void*,void*,const char*,unsigned,const EntitySlice*) noexcept;
    const FactoryBindings* native_factory;
};
struct SpawnRow {void* actor;int depth;int priority;};
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
};
struct SpawnResult {
    unsigned completed,ranked,parent_reads,parents,debugged,spawned,failed,pruned,cleanups,activated,
        scopes,closes,engine_scopes,engine_closes,native_spawns,native_activations;
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
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
