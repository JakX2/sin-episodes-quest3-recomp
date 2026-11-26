#include "native_server_startup.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
using namespace sin_port::native_server_startup;
struct Events {
    unsigned calls=0;
    virtual void unused() noexcept {assert(false);}
    virtual int load(const char* name) noexcept {
        assert(!std::strcmp(name,calls?"resource/ModEvents.res":"resource/gameevents.res"));
        return calls++?0:-7;
    }
};
static void* __cdecl factory(const char* name,int* status) {
    assert(!status&&!std::strcmp(name,"VDebugOverlay003"));return nullptr;
}
struct Fixture {
    State state;Events events;
    unsigned objects[14]{},manager[0x510/4]{},external=0,getters=0,init=0,dupes=0,scene=0,model=0,tail=0;
    bool cheats=true,fail_init=false,fail_scene=false,null_manager=false;
    const Api* api;
    Pending a{&objects[5],nullptr},b{&objects[11],nullptr},late{&objects[12],nullptr};
    Bindings bindings() {
        Bindings v{};v.context=this;v.globals=&objects[13];v.engine=v.physics=v.filesystem=&factory;v.event_manager=&events;v.cvar_interface=&objects[2];
        v.console=[](void* p,void* cv,void** d,void** c) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(cv==&f.objects[2]&&f.events.calls==1);*d=&f.objects[0];*c=f.cheats?&f.objects[1]:nullptr;return 1;};
        v.get_system=[](void* p,const SystemSpec* spec,void** o) noexcept ->int{auto& f=*static_cast<Fixture*>(p);const auto i=f.getters++;assert(i<11);const auto& expected=i<5?external_services[i]:explicit_systems[i-5];assert(spec->getter==expected.getter&&spec->identity==expected.identity);*o=&f.objects[i];return 1;};
        v.register_external=[](void* p,void* object) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(object==&f.objects[f.external]&&f.external<5);++f.external;return 1;};
        v.registry.context=this;
        v.registry.duplicate=[](void* p,void* object) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(object==&f.objects[5]);++f.dupes;return 1;};
        v.registry.init=[](void* p,void* object,bool* al) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(f.events.calls==2&&f.state.registry.initialized&&!f.state.registry.pending);assert(object==&f.objects[5+f.init]);++f.init;
            if(f.init==1)assert(f.api->register_auto(&f.state.registry,&f.late));
            *al=!f.fail_init||f.init!=2;return 1;};
        v.scene_repository=[](void* p,bool* yes) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(f.init==8);++f.scene;*yes=!f.fail_scene;return 1;};
        v.model_sounds_repository=[](void* p,bool* yes) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(f.scene==1);++f.model;*yes=true;return 1;};
        v.allocate_manager=[](void* p,unsigned bytes,void** a) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(bytes==0x510&&f.model==1);*a=f.null_manager?nullptr:f.manager;return 1;};
        v.construct_manager=[](void* p,void* a,void** m) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(a==f.manager);*m=a;return 1;};
        v.engine_complete=[](void* p) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(f.model==1);++f.tail;return 1;};
        return v;
    }
    explicit Fixture(const Api* a):api(a){assert(api->register_auto(&state.registry,&b));assert(api->register_auto(&state.registry,&this->a));}
};
static constexpr const char* names[]{
    "VEngineServer021","VoiceServer001","VEngineCvar002","VEngineServerStringTable001","StaticPropMgrServer002",
    "VEngineRandom001","IEngineSoundServer003","SpatialPartition001","VModelInfoServer001","EngineTraceServer003",
    "VFileSystem012","GAMEEVENTSMANAGER002","VEngineCache001","VSoundEmitter001","ServerUploadGameStats001","ISERVERPLUGINHELPERS001",
    "VDebugOverlay003"
};
static Fixture* active=nullptr;
static unsigned queries=0,sound_calls=0,miss=99;
static bool sound_ok=true;
static unsigned engine_commands=0,engine_executes=0;
static void __fastcall queue_command(void*,void*,const char* command){assert(!std::strcmp(command,"exec game.cfg\n"));++engine_commands;}
static void __fastcall execute_command(void*,void*){assert(engine_commands==engine_executes+1);++engine_executes;}
static void* engine_vtable[38]{};
static void** engine_object=engine_vtable;
static void* __cdecl engine_factory(const char*,int*);
struct Sound {
    virtual bool connect(sin_independent::StartupFactory factory) noexcept {
        assert(factory==&engine_factory&&queries==16);++sound_calls;return sound_ok;
    }
} sound;
static void* __cdecl engine_factory(const char* name,int* status){
    assert(active&&!status&&queries<17&&queries!=10&&!std::strcmp(name,names[queries]));
    const unsigned i=queries++;if(i==miss||i==16)return nullptr;
    if(i==0)return &engine_object;
    if(i==2)return &active->objects[2];
    if(i==11)return &active->events;
    if(i==13)return &sound;
    return &active->objects[0];
}
static void* __cdecl filesystem_factory(const char* name,int* status){
    assert(active&&!status&&queries==10&&!std::strcmp(name,names[10]));
    return queries++==miss?nullptr:&active->objects[0];
}
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetApi>(GetProcAddress(server,export_name));assert(get&&!GetProcAddress(client,export_name)&&!get(0,sizeof(Api)));
    auto* api=get(1,sizeof(Api));assert(api);
    Result o{};
    Fixture f(api);auto b=f.bindings();
    assert(api->post_connect(&f.state,&b,&o)&&o.completed&&o.stage==Stage::Done&&o.initial_events==-7&&o.mod_events==0&&o.external_registered==5&&o.systems_appended==6&&o.registry.duplicates==1&&o.registry.appended==1&&o.registry.init_calls==8&&f.state.registry.count==8&&o.manager_constructed&&f.state.manager==f.manager&&f.tail==1);
    assert(!api->post_connect(&f.state,&b,&o)&&f.tail==1);
    Fixture no_cheats(api);no_cheats.cheats=false;b=no_cheats.bindings();
    assert(!api->post_connect(&no_cheats.state,&b,&o)&&o.stage==Stage::Cheats&&o.console_called&&!no_cheats.getters);
    Fixture fail(api);fail.fail_init=true;b=fail.bindings();
    assert(!api->post_connect(&fail.state,&b,&o)&&o.stage==Stage::InitializeSystems&&o.registry.init_calls==2&&fail.state.registry.initialized&&!fail.scene&&!fail.model&&!fail.tail);
    Fixture scene(api);scene.fail_scene=true;b=scene.bindings();
    assert(!api->post_connect(&scene.state,&b,&o)&&o.stage==Stage::SceneRepository&&!scene.model);
    Fixture nullmgr(api);nullmgr.null_manager=true;b=nullmgr.bindings();
    assert(api->post_connect(&nullmgr.state,&b,&o)&&o.completed&&!o.manager_allocated&&!nullmgr.state.manager&&o.engine_completed);
    const auto invoke=reinterpret_cast<InvokeBoundary>(GetProcAddress(server,boundary_export));
    assert(invoke&&!GetProcAddress(client,boundary_export));
    BoundaryResult bo{};
    Fixture live(api);active=&live;b=live.bindings();
    b.engine=&engine_factory;b.filesystem=&filesystem_factory;b.event_manager=nullptr;b.cvar_interface=nullptr;
    queries=sound_calls=0;miss=99;sound_ok=true;
    assert(invoke(&live.state,&b,&bo)&&bo.entered&&bo.acquired==16&&bo.sound_connected&&bo.original_return&&bo.tail.completed&&queries==17&&sound_calls==1&&live.tail==1);
    assert(!invoke(&live.state,&b,&bo)&&queries==17);
    Fixture missing(api);active=&missing;b=missing.bindings();b.engine=&engine_factory;b.filesystem=&filesystem_factory;
    queries=sound_calls=0;miss=10;
    assert(!invoke(&missing.state,&b,&bo)&&bo.entered&&!bo.acquired&&!sound_calls&&queries==11&&!missing.state.attempted);
    Fixture reject(api);active=&reject;b=reject.bindings();b.engine=&engine_factory;b.filesystem=&filesystem_factory;
    queries=sound_calls=0;miss=99;sound_ok=false;
    assert(!invoke(&reject.state,&b,&bo)&&bo.entered&&bo.acquired==16&&!bo.sound_connected&&sound_calls==1&&!reject.state.attempted);
    Fixture badcheats(api);badcheats.cheats=false;active=&badcheats;b=badcheats.bindings();b.engine=&engine_factory;b.filesystem=&filesystem_factory;
    queries=sound_calls=0;sound_ok=true;
    assert(!invoke(&badcheats.state,&b,&bo)&&bo.sound_connected&&bo.tail.stage==Stage::Cheats&&!bo.original_return&&queries==16&&badcheats.state.attempted);
    // Unbound retail-facing slot still refuses after sixteen factories.
    Fixture unbound(api);active=&unbound;queries=sound_calls=0;
    using Create=void* (__cdecl *)(const char*,int*);
    auto create=reinterpret_cast<Create>(GetProcAddress(server,"CreateInterface"));
    assert(create);void* object=create("ServerGameDLL003",nullptr);assert(object);
    auto** vt=*reinterpret_cast<void***>(object);
    using Init=bool (__thiscall *)(void*,sin_independent::StartupFactory,sin_independent::StartupFactory,sin_independent::StartupFactory,void*);
    assert(!reinterpret_cast<Init>(vt[0])(object,&engine_factory,&factory,&filesystem_factory,&unbound.objects[13])&&queries==16&&!sound_calls&&!unbound.state.attempted);
    const auto gameinit=reinterpret_cast<InvokeGameInit>(GetProcAddress(server,gameinit_export));
    assert(gameinit&&!GetProcAddress(client,gameinit_export));
    engine_vtable[36]=reinterpret_cast<void*>(&queue_command);engine_vtable[37]=reinterpret_cast<void*>(&execute_command);
    unsigned char records[36];std::memset(records,0x55,sizeof(records));unsigned flags=0;std::uint16_t defaults[4]{};
    unsigned allocated[3]{};void* pointers[]{&allocated[1],&allocated[2]};
    ResetState reset{&allocated[0],8,0,3,&allocated[0],records,sizeof(records),3,1,2,2,&flags,defaults,pointers,2,2};
    struct Release {unsigned calls=0;unsigned* allocated;bool fail=false;} release{0,allocated};
    GameInitBindings gb{&release,&reset,[](void* p,void* a) noexcept ->int{auto& r=*static_cast<Release*>(p);assert(a==&r.allocated[r.calls]);++r.calls;return !r.fail;}};
    GameInitResult go{};
    assert(!gameinit(&badcheats.state,&gb,&go)&&!release.calls);
    assert(gameinit(&live.state,&gb,&go)&&go.completed&&go.vector_released&&go.index_rebuilt&&go.default_initialized&&go.allocations_released==2&&go.command_queued&&go.command_executed&&release.calls==3&&engine_commands==1&&engine_executes==1);
    assert(!reset.vector&&!reset.vector_cache&&!reset.vector_capacity&&!reset.vector_count&&!reset.allocation_count&&reset.index_first==0xffff&&!reset.index_used&&!reset.index_free&&flags==1&&defaults[0]==0xffff&&defaults[3]==1);
    for(unsigned i=0;i<3;++i){std::uint16_t index=0,next=0;std::memcpy(&index,records+12*i,2);std::memcpy(&next,records+12*i+2,2);assert(index==i&&next==(i==2?0xffff:i+1));assert(records[12*i+4]==0x55);}
    assert(!gameinit(&live.state,&gb,&go)&&engine_commands==1);
    using InitGame=bool (__thiscall *)(void*);
    assert(!reinterpret_cast<InitGame>(vt[1])(object)&&engine_commands==1);
    // External vector allocation survives reset; existing default flag
    // remains intact. Native slot refuses retry after release failure.
    Fixture another(api);active=&another;b=another.bindings();b.engine=&engine_factory;b.filesystem=&filesystem_factory;queries=sound_calls=0;
    assert(invoke(&another.state,&b,&bo));
    reset.vector=&allocated[0];reset.vector_cache=nullptr;reset.vector_capacity=8;reset.vector_grow=-1;reset.vector_count=7;reset.allocation_count=0;
    assert(gameinit(&another.state,&gb,&go)&&go.completed&&!go.vector_released&&!go.default_initialized&&reset.vector==&allocated[0]&&reset.vector_cache==reset.vector&&reset.vector_capacity==8&&!reset.vector_count&&engine_executes==2);
    FreeLibrary(server);FreeLibrary(client);
}
