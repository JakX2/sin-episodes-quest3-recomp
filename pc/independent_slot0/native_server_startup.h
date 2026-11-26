#pragma once
#include "server_post_connect_core.h"
#include <cstdint>
namespace sin_port::native_server_startup {
inline constexpr char export_name[]="SiN_GetServerStartupApi";
inline constexpr char boundary_export[]="SiN_InvokeServerDllInitBoundary";
inline constexpr char gameinit_export[]="SiN_InvokeServerGameInitBoundary";
struct SystemSpec {unsigned getter,identity;};
// Original DLLInit service getters, order matters. Identities are
// original static addresses for auditing, not executable pointers.
inline constexpr SystemSpec external_services[]{
    {0x22273be0,0x2251ba84},{0x2222eca0,0x22575288},{0x2209df20,0x224e250c},
    {0x221c2f90,0x2250967c},{0x22091790,0x224e1e4c}
};
inline constexpr SystemSpec explicit_systems[]{
    {0x221f3db0,0x22572ae0},{0x2220d900,0x22513ae0},{0x222f0fa0,0x22595f58},
    {0x222b67d0,0x22521864},{0x2225f8e0,0x2257ae9c},{0x221b4e50,0x22507558}
};
inline constexpr unsigned registry_limit=256;
struct Pending {void* object;Pending* next;};
struct Registry {
    void* systems[registry_limit]{};
    unsigned count=0;
    Pending* pending=nullptr;
    bool initialized=false,draining=false;
};
struct RegistryBindings {
    void* context;
    int (__cdecl *duplicate)(void*,void*) noexcept;
    int (__cdecl *init)(void*,void*,bool*) noexcept;
};
struct RegistryResult {
    unsigned visited,duplicates,appended,init_calls,initialized,completed;
    void* failed;
};
enum class Stage : unsigned {
    None,PostConnect,InitialEvents,Console,Cheats,ExternalService,SystemGetter,
    ModEvents,InitializeSystems,SceneRepository,ModelSoundsRepository,
    DebugOverlay,ManagerAllocate,ManagerConstruct,EngineComplete,Done
};
struct State {
    sin_independent::ServerPostConnectState post;
    Registry registry;
    void* developer=nullptr;
    void* cheats=nullptr;
    void* debug_overlay=nullptr;
    void* manager=nullptr;
    void* partial_allocation=nullptr;
    bool attempted=false,complete=false,busy=false,game_init_attempted=false,game_init_complete=false;
    void* engine_server=nullptr;
};
struct Bindings {
    void* context;
    void* globals;
    sin_independent::StartupFactory engine,physics,filesystem;
    void* event_manager;
    void* cvar_interface;
    void* engine_server;
    // Existing native console registration owner (221e6450 + 2225bc00).
    // Returns developer and local-list sv_cheats, not FindVar substitute.
    int (__cdecl *console)(void*,void*,void**,void**) noexcept;
    int (__cdecl *get_system)(void*,const SystemSpec*,void**) noexcept;
    int (__cdecl *register_external)(void*,void*) noexcept;
    RegistryBindings registry;
    int (__cdecl *scene_repository)(void*,bool*) noexcept;
    int (__cdecl *model_sounds_repository)(void*,bool*) noexcept;
    int (__cdecl *allocate_manager)(void*,unsigned,void**) noexcept;
    int (__cdecl *construct_manager)(void*,void*,void**) noexcept;
    int (__cdecl *engine_complete)(void*) noexcept;
};
struct Result {
    Stage stage;
    int initial_events,mod_events;
    unsigned completed,post_connect,console_called,external_registered,systems_appended,
        scene_called,model_sounds_called,debug_queried,manager_allocated,manager_constructed,engine_completed;
    bool scene_ok,model_sounds_ok;
    RegistryResult registry;
};
// Original 221f4980 global vector and 2228f400 owned-name cache.
// Pointers refer to independently owned storage, not original PE data.
struct ResetState {
    void* vector;
    int vector_capacity,vector_grow,vector_count;
    void* vector_cache;
    unsigned char* index_records; // 12-byte records
    unsigned index_bytes;
    std::uint16_t index_capacity,index_first,index_used,index_free;
    unsigned* init_flags;
    std::uint16_t* default_record; // four words, initialized once
    void** allocations;
    unsigned allocation_capacity,allocation_count;
};
struct GameInitBindings {
    void* context;
    ResetState* reset;
    int (__cdecl *release)(void*,void*) noexcept;
};
struct GameInitResult {
    unsigned completed,vector_released,index_rebuilt,default_initialized,
        allocations_released,command_queued,command_executed;
};
struct Api {
    std::uint32_t version,size;
    int (__cdecl *register_auto)(Registry*,Pending*) noexcept;
    int (__cdecl *append)(Registry*,void*) noexcept;
    int (__cdecl *initialize)(Registry*,const RegistryBindings*,RegistryResult*) noexcept;
    int (__cdecl *post_connect)(State*,const Bindings*,Result*) noexcept;
    int (__cdecl *game_init)(State*,const GameInitBindings*,GameInitResult*) noexcept;
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
struct BoundaryResult {unsigned entered,acquired,sound_connected,original_return;Result tail;};
using InvokeBoundary=int (__cdecl *)(State*,const Bindings*,BoundaryResult*) noexcept;
using InvokeGameInit=int (__cdecl *)(State*,const GameInitBindings*,GameInitResult*) noexcept;
}
