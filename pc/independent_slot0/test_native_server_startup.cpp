#include "native_server_startup.h"
#include "native_server_level.h"
#include "server_fixed_difficulty_service.h"
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
    struct Event {
        virtual ~Event()=default;
        virtual void s1(){} virtual void s2(){} virtual void s3(){} virtual void s4(){}
        virtual void s5(){} virtual void s6(){} virtual void s7(){} virtual void s8(){} virtual void s9(){}
        virtual void integer(const char* key,int value){
            const char* game_keys[]{"descriptor","owner","subject"};
            const char* health_keys[]{"descriptor","owner","subject","inflictor","health","amount"};
            const int health_values[]{21,0x5002,-1,-1,43,0};
            if(kind==0){assert(fields==0&&!std::strcmp(key,"descriptor")&&value==descriptor);}
            else if(kind==1){assert(fields<3&&!std::strcmp(key,game_keys[fields])&&value==(fields?-1:descriptor));}
            else if(kind==2){assert(fields<6&&!std::strcmp(key,health_keys[fields])&&value==health_values[fields]);}
            else{
                assert(kind==3);
                if(fields==0){
                    assert(!std::strcmp(key,"descriptor")&&value==0&&lookups&&*lookups==expected_lookups);
                    if(mutation){const unsigned changed=0x6002;std::memcpy(mutation,&changed,4);}
                }else if(fields==1)assert(!std::strcmp(key,"owner")&&value==expected_owner);
                else if(fields==2)assert(!std::strcmp(key,"subject")&&value==-1);
                else assert(fields==4&&!std::strcmp(key,"decision")&&value==7);
            }
            ++fields;
        }
        virtual void real(const char* key,float value){
            const char* keys[]{"value","x","y","z"};const float values[]{12.5f,1.f,2.f,3.f};
            assert(kind==3&&fields>=5&&fields<9&&!std::strcmp(key,keys[fields-5])&&value==values[fields-5]);++fields;
        }
        virtual void string(const char* key,const char* value){
            if(kind==0)assert((fields==1&&!std::strcmp(key,"mapname")&&!std::strcmp(value,"SE1_intro01"))||
                (fields==2&&!std::strcmp(key,"filename")&&!*value));
            else assert((kind==1||kind==3)&&fields==3&&!std::strcmp(key,"name")&&!std::strcmp(value,"secret"));
            ++fields;
        }
        unsigned fields=0,kind=0;int descriptor=11,expected_owner=0;
        unsigned* lookups=nullptr;unsigned expected_lookups=0;unsigned char* mutation=nullptr;
    } event;
    unsigned calls=0,creates=0,fires=0,frees=0;bool live=false,fire_ok=true,unavailable=false;
    virtual void unused() noexcept {assert(false);}
    virtual int load(const char* name) noexcept {
        assert(!std::strcmp(name,calls?"resource/ModEvents.res":"resource/gameevents.res"));
        return calls++?0:-7;
    }
    virtual void s2(){} virtual void s3(){} virtual void s4(){} virtual void s5(){}
    virtual void* create(const char* name,bool force){
        assert(calls==2&&!live&&!force);
        // LevelInit map(1), secret Spawn(11), secret Touch(12),
        // ServerActivate map(7), then health(21).
        constexpr const char* names[]{"map","game","game","map","health","dynamic_difficulty","dynamic_difficulty","dynamic_difficulty"};
        constexpr int descriptors[]{1,11,12,7,21,0,0,0};
        assert(creates<8&&!std::strcmp(name,names[creates]));
        event.descriptor=descriptors[creates++];event.kind=!std::strcmp(name,"map")?0:!std::strcmp(name,"game")?1:!std::strcmp(name,"health")?2:3;
        if(unavailable)return nullptr;
        live=true;event.fields=0;return &event;
    }
    virtual bool fire(void* object,bool no_broadcast){
        const unsigned expected=event.kind==0?3:event.kind==1?4:event.kind==2?6:9;
        assert(live&&object==&event&&!no_broadcast&&event.fields==expected);++fires;live=false;return fire_ok;
    }
    virtual void s8(){} virtual void s9(){}
    virtual void release(void* object){assert(live&&object==&event);++frees;live=false;}
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
static unsigned edict_queries=0;
static void* edict_result=nullptr;
static void* __fastcall get_edict(void* self,void*,int index){
    assert(self==&engine_object&&index==1);++edict_queries;return edict_result;
}
struct LocalNetworkable {
    virtual void s0(){} virtual void s1(){} virtual void s2(){} virtual void s3(){} virtual void s4(){}
    virtual void* base(){++calls;return actor;}
    void* actor=nullptr;unsigned calls=0;
};
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
    // Real interface slot0 factory acquisition -> slot1 GameInit -> slot2
    // native map pipeline -> secret Spawn -> borrowed engine event manager.
    namespace nl=sin_port::native_level;namespace nm=sin_port::native_map;namespace fs=sin_port::fixed_service;
    const auto init_events=reinterpret_cast<InvokeEventSession>(GetProcAddress(server,event_session_export));
    const auto level_events=reinterpret_cast<nl::InvokeLevelWithEvents>(GetProcAddress(server,nl::engine_events_level_export));
    const auto touch_events=reinterpret_cast<nl::TouchStartedTrigger>(GetProcAddress(server,nl::engine_events_touch_export));
    const auto activate_events=reinterpret_cast<nl::InvokeActivateWithEvents>(GetProcAddress(server,nl::engine_events_activate_export));
    assert(init_events&&level_events&&touch_events&&activate_events&&!GetProcAddress(client,event_session_export)&&!GetProcAddress(client,nl::engine_events_level_export)&&!GetProcAddress(client,nl::engine_events_activate_export));
    Fixture started(api);active=&started;b=started.bindings();b.engine=&engine_factory;b.filesystem=&filesystem_factory;
    b.event_manager=&reject.events;queries=sound_calls=0;
    alignas(4) unsigned char globals[0x38]{};
    auto set_clients=[&](int count){std::memcpy(globals+0x14,&count,4);};
    b.globals=globals;set_clients(1);
    EngineEventSession session{};
    assert(init_events(&started.state,&b,&bo,&session)&&session.startup==&started.state&&session.manager==&started.events&&session.thread==GetCurrentThreadId());
    assert(!reject.events.calls&&started.events.calls==2);
    struct Map {
        alignas(4) unsigned char actor[nm::dynamic_trigger_extent]{};
        alignas(4) unsigned char player[0x37c]{};
        alignas(4) unsigned char named_filter[0x3cc]{};
        unsigned removed=0,base_activations=0,filter_alloc=0,trigger_alloc=0,filter_spawn=0;
    } m;
    const unsigned player_vt=0x224aec3cu,handle=0x5002u;const int hp=43;
    std::memcpy(m.player,&player_vt,4);std::memcpy(m.player+0x378,&handle,4);std::memcpy(m.player+0x9c,&hp,4);
    auto put=[&](unsigned off,unsigned value){std::memcpy(m.actor+off,&value,4);};
    put(0,nm::secret_trigger_vtable);put(0xc0,1);m.actor[0x120]=8;
    nm::CampaignTriggerBindings tr{};tr.context=&m;
    tr.spawn_gate=[](void*,bool* suppress) noexcept ->int{*suppress=false;return 1;};
    tr.base_spawn=[](void*,void*) noexcept ->int{return 1;};
    tr.string_text=[](void*,unsigned token,const char** name,unsigned* bytes) noexcept ->int{assert(token==1);*name="secret";*bytes=7;return 1;};
    // No hosted predicate for authenticated original player/nonplayer classes.
    tr.remove=[](void* p,void*) noexcept ->int{++static_cast<Map*>(p)->removed;return 1;};
    nm::CampaignTriggerResult cr{};int publication=999;
    assert(!touch_events(&session,m.actor,sizeof(m.actor),&m.player,&tr,&cr,&publication)&&publication==nm::engine_event_not_attempted);
    assert(gameinit(&started.state,&gb,&go));
    const auto resolve_player=reinterpret_cast<ResolveStartedLocalPlayer>(GetProcAddress(server,local_player_export));
    assert(resolve_player&&!GetProcAddress(client,local_player_export));
    alignas(4) unsigned char edict[0x10]{};LocalNetworkable network;
    network.actor=m.player;
    auto set_edict_network=[&](void* p){std::memcpy(edict+0xc,&p,4);};
    auto set_edict_flags=[&](unsigned flags){std::memcpy(edict,&flags,4);};
    engine_vtable[19]=reinterpret_cast<void*>(&get_edict);
    void* player=reinterpret_cast<void*>(1);edict_queries=0;edict_result=edict;
    set_clients(2);assert(resolve_player(&session,&player)&&!player&&!edict_queries);
    set_clients(0);assert(resolve_player(&session,&player)&&!player&&!edict_queries);
    set_clients(1);edict_result=nullptr;
    assert(resolve_player(&session,&player)&&!player&&edict_queries==1);
    edict_result=reinterpret_cast<void*>(1);assert(!resolve_player(&session,&player)&&!player&&edict_queries==2);
    edict_result=edict;set_edict_flags(2);set_edict_network(&network);
    assert(resolve_player(&session,&player)&&!player&&edict_queries==3&&!network.calls);
    set_edict_flags(0);set_edict_network(nullptr);
    assert(resolve_player(&session,&player)&&!player&&edict_queries==4&&!network.calls);
    set_edict_network(reinterpret_cast<void*>(1));
    assert(!resolve_player(&session,&player)&&!player&&edict_queries==5);
    set_edict_network(&network);network.actor=nullptr;
    assert(resolve_player(&session,&player)&&!player&&network.calls==1&&edict_queries==6);
    network.actor=m.player;
    assert(resolve_player(&session,&player)&&player==m.player&&network.calls==2&&edict_queries==7);
    const auto map_get=reinterpret_cast<nm::GetApi>(GetProcAddress(server,nm::export_name));assert(map_get);
    const auto* map_api=map_get(1,sizeof(nm::Api));assert(map_api);
    nm::FactoryDictionary dictionary{};assert(map_api->register_factories(&dictionary)&&map_api->register_campaign_triggers(&dictionary));
    nm::TriggerFactoryBindings trigger_factory{};trigger_factory.context=&m;
    trigger_factory.allocate=[](void* p,unsigned n,void** actor,unsigned* bytes) noexcept ->int{
        auto& m=*static_cast<Map*>(p);assert(n==nm::secret_trigger_extent&&!m.trigger_alloc);++m.trigger_alloc;*actor=m.actor;*bytes=n;return 1;
    };
    trigger_factory.base_construct=[](void*,void*) noexcept ->int{return 1;};
    trigger_factory.initialize=[](void*,void*,const char* cls) noexcept ->int{return !std::strcmp(cls,"trigger_secret");};
    nm::FactoryBindings native_factory{};native_factory.dictionary=&dictionary;native_factory.campaign_triggers=&trigger_factory;
    nm::RelayFactoryBindings filter_owner{};filter_owner.context=&m;
    filter_owner.allocate=[](void* p,unsigned n,void** actor,unsigned* bytes) noexcept ->int{
        auto& m=*static_cast<Map*>(p);assert(n==sizeof(m.named_filter)&&!m.filter_alloc);++m.filter_alloc;*actor=m.named_filter;*bytes=n;return 1;
    };
    filter_owner.base_construct=[](void*,void*,int arg) noexcept ->int{return arg==1;};
    filter_owner.initialize=[](void* p,void* actor,const char* cls) noexcept ->int{
        auto& m=*static_cast<Map*>(p);assert(actor==m.named_filter&&!std::strcmp(cls,"filter_activator_name"));
        const unsigned handle=0x7000;std::memcpy(m.named_filter+0x378,&handle,4);return 1;
    };
    nm::CampaignFilterFactoryBindings filter_factory{};filter_factory.owner=&filter_owner;
    auto map_extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{
        auto& m=*static_cast<Map*>(p);*bytes=actor==m.actor?nm::secret_trigger_extent:actor==m.named_filter?sizeof(m.named_filter):0;return *bytes!=0;
    };
    nm::KeyBindings keys{};keys.context=&m;keys.extent=map_extent;
    keys.intern=[](void*,const char* value,unsigned* token) noexcept ->int{
        if(!std::strcmp(value,"secret"))*token=1;
        else if(!std::strcmp(value,"named_filter"))*token=31;
        else if(!std::strcmp(value,"hero"))*token=32;
        else if(!std::strcmp(value,"trigger_secret"))*token=33;
        else if(!std::strcmp(value,"filter_activator_name"))*token=34;
        else return 0;return 1;
    };
    nm::Bindings mp{};mp.context=&m;mp.native_factory=&native_factory;mp.native_keys=&keys;
    mp.clear_factory_global=[](void*) noexcept ->int{return 1;};
    nm::SpawnBindings ms{};ms.context=&m;ms.campaign_triggers=&tr;
    ms.extent=map_extent;
    ms.classname=[](void* p,void* actor,const char** cls) noexcept ->int{
        auto& m=*static_cast<Map*>(p);*cls=actor==m.actor?"trigger_secret":actor==m.named_filter?"filter_activator_name":nullptr;return *cls!=nullptr;
    };
    ms.spawn=[](void* p,void* actor,int* code) noexcept ->int{
        auto& m=*static_cast<Map*>(p);assert(actor==m.named_filter);++m.filter_spawn;*code=0;return 1;
    };
    ms.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
    ms.debug_enabled=[](void*,bool* yes) noexcept ->int{*yes=false;return 1;};
    ms.native_spawn_begin=[](void*,void*,bool* proceed) noexcept ->int{*proceed=true;return 1;};
    ms.native_spawn_end=[](void*,void*,int* code) noexcept ->int{*code=0;return 1;};
    nm::PipelineBindings pipeline{};pipeline.parse=&mp;pipeline.spawn=&ms;
    pipeline.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
    pipeline.classify=[](void*,void*,nm::Category* category) noexcept ->int{*category=nm::Category::Ordinary;return 1;};
    bool transition=false,finalize=false;int load_kind=-1;unsigned words[4]{};const char* current="SE1_intro01";
    nl::State ls{};ls.startup=&started.state;ls.transition=&transition;ls.restore_finalize=&finalize;ls.load_kind=&load_kind;ls.current_map=&current;ls.map_pair_words=words;
    nl::Bindings lb{};lb.native_map_pipeline=&pipeline;
    const char lump[]="{ classname trigger_secret targetname secret filtername named_filter } { classname filter_activator_name targetname named_filter filtername hero }";lb.entity_text_bytes=sizeof(lump);
    lb.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
    lb.pre_level=lb.post_level=lb.motd=lb.cleanup=[](void*) noexcept ->int{return 1;};
    lb.chapter=[](void*,const char*) noexcept ->int{return 1;};
    lb.filename=[](void*,char* name,unsigned) noexcept ->int{*name=0;return 1;};
    // No hosted map/health event callbacks on the session route.
    lb.context=&m;
    lb.delete_queue_count=[](void*,unsigned* count) noexcept ->int{*count=0;return 1;};
    lb.next_entity=[](void*,void*,void** next) noexcept ->int{*next=nullptr;return 1;};
    lb.extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{assert(actor==static_cast<Map*>(p)->player);*bytes=0x37c;return 1;};
    lb.entity_scope=[](void*,bool) noexcept ->int{return 1;};
    lb.system_activate=[](void*,void*) noexcept ->int{return 1;};
    lb.set_activation_flag=[](void*,int flag) noexcept ->int{assert(!flag);return 1;};
    // The local-player callback is intentionally absent: the session
    // uses acquired engine slot19 and the edict networkable slot5.
    lb.activation_cvar=[](void*,bool* value) noexcept ->int{*value=true;return 1;};
    lb.manager_activate=[](void*) noexcept ->int{return 1;};
    nl::LevelArgs la{current,lump,nullptr,nullptr,false,false};nl::Result lr{};
    auto wrong=session;wrong.startup=&live.state;
    assert(!level_events(&ls,&la,&lb,&lr,&wrong)&&!ls.level_ready&&!started.events.creates);
    wrong=session;wrong.thread=0;
    assert(!level_events(&ls,&la,&lb,&lr,&wrong)&&!ls.level_ready);
    const auto level_filters=reinterpret_cast<nl::InvokeLevelWithCampaignFilters>(GetProcAddress(server,nl::campaign_filters_level_export));
    assert(level_filters&&!GetProcAddress(client,nl::campaign_filters_level_export));
    assert(level_filters(&ls,&la,&lb,&lr,&session,&filter_factory)&&lr.completed&&lr.map_event&&lr.map_pipeline.spawn.native_spawns==1&&
        lr.map_pipeline.parse.native_creates==2&&lr.map_pipeline.spawn.spawned==2&&m.filter_alloc==1&&m.trigger_alloc==1&&m.filter_spawn==1&&
        started.events.fires==2&&started.events.creates==2);
    unsigned parsed_name=0,parsed_pattern=0,parsed_filter=0;
    std::memcpy(&parsed_name,m.named_filter+0xc0,4);std::memcpy(&parsed_pattern,m.named_filter+0x3c8,4);std::memcpy(&parsed_filter,m.actor+0x40c,4);
    assert(parsed_name==31&&parsed_pattern==32&&parsed_filter==31);
    const auto contact=reinterpret_cast<nl::DispatchCampaignContact>(GetProcAddress(server,nl::campaign_contact_export));
    assert(contact&&!GetProcAddress(client,nl::campaign_contact_export));
    nl::CampaignContactBindings cb{};cb.campaign=&tr;cb.context=&m;
    nl::CampaignContactResult contact_result{};
    using ContactKind=nl::CampaignContactKind;
    assert(contact(&session,m.actor,sizeof(m.actor),nullptr,0,ContactKind::Continuing,&cb,&contact_result)&&contact_result.missing_other&&!contact_result.touch_called);
    assert(!contact(&session,m.actor,4,&m.player,sizeof(m.player),ContactKind::Continuing,&cb,&contact_result));
    put(0xb8,1);
    assert(contact(&session,m.actor,sizeof(m.actor),reinterpret_cast<void*>(1),0,ContactKind::Continuing,&cb,&contact_result)&&contact_result.marked_actor&&!contact_result.touch_called);
    put(0xb8,0);m.player[0xb8]=1;
    assert(contact(&session,m.actor,sizeof(m.actor),&m.player,sizeof(m.player),ContactKind::Continuing,&cb,&contact_result)&&contact_result.marked_other&&!contact_result.touch_called);
    m.player[0xb8]=0;
    assert(!contact(&session,m.actor,sizeof(m.actor),&m.player,4,ContactKind::Continuing,&cb,&contact_result));
    assert(!contact(&session,m.actor,sizeof(m.actor),&m.player,sizeof(m.player),ContactKind::Starting,&cb,&contact_result)&&!contact_result.start_called);
    cb.start_touch=[](void*,void*,void*) noexcept ->int{return 0;};
    assert(!contact(&session,m.actor,sizeof(m.actor),&m.player,sizeof(m.player),ContactKind::Starting,&cb,&contact_result)&&contact_result.start_called&&!contact_result.touch_called);
    assert(started.events.creates==2&&!m.removed);
    const auto classify=reinterpret_cast<nm::ClassifyCampaignPlayer>(GetProcAddress(server,nm::player_predicate_export));
    assert(classify&&!GetProcAddress(client,nm::player_predicate_export));
    unsigned candidate=0;
    assert(classify(nullptr,4)==nm::PlayerPredicate::Invalid&&classify(&candidate,3)==nm::PlayerPredicate::Invalid);
    assert(classify(reinterpret_cast<void*>(1),4)==nm::PlayerPredicate::Invalid&&classify(&candidate,4)==nm::PlayerPredicate::Unknown);
    candidate=0x224aec3cu;assert(classify(&candidate,4)==nm::PlayerPredicate::Player);
    candidate=0x2245222cu;assert(classify(&candidate,4)==nm::PlayerPredicate::Player);
    candidate=0x2240ac2cu;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x2243b96cu;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x2243bc0cu;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x2241f0d4u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x22420e14u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x22422294u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x22421d74u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x22422004u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x22421ae4u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x22420b84u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x2241f9e4u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x2241ed04u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x22422534u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x22421334u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x224215c4u;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=0x224228ccu;assert(classify(&candidate,4)==nm::PlayerPredicate::NonPlayer);
    candidate=nm::relay_vtable;
    assert(touch_events(&session,m.actor,sizeof(m.actor),&candidate,&tr,&cr,&publication)&&cr.nonplayer&&publication==nm::engine_event_not_attempted&&!m.removed&&started.events.creates==2);
    candidate=0;
    assert(!touch_events(&session,m.actor,sizeof(m.actor),&candidate,&tr,&cr,&publication)&&!m.removed&&started.events.creates==2);
    assert(!touch_events(&session,m.actor,sizeof(m.actor),reinterpret_cast<void*>(1),&tr,&cr,&publication)&&!m.removed);
    tr.is_player=[](void*,void*,bool* yes) noexcept ->int{*yes=false;return 1;};
    assert(touch_events(&session,m.actor,sizeof(m.actor),&candidate,&tr,&cr,&publication)&&cr.nonplayer&&started.events.creates==2);
    tr.is_player=nullptr;
    // Native inherited StartTouch filter, vector append and scripted
    // output precede secret publication without a hosted StartTouch leaf.
    namespace nw=sin_port::native_weapon_tables;
    struct StartOutput {
        Map* map;
        unsigned vector[2]{},action[7]{},calls=0,filters=0,grows=0;
        bool allow=false,alias=false;
    } so{&m};
    so.action[0]=201;so.action[1]=202;so.action[4]=0xffffffffu;so.action[5]=77;
    nm::CampaignStartTouchBindings st{};st.context=&so;
    st.filter=[](void* p,void* actor,void* other,bool* yes) noexcept ->int{
        auto& s=*static_cast<StartOutput*>(p);assert(actor==s.map->actor&&other==s.map->player);++s.filters;*yes=s.allow;return 1;
    };
    st.grow_vector=[](void* p,void* actor,unsigned need) noexcept ->int{
        auto& s=*static_cast<StartOutput*>(p);assert(actor==s.map->actor&&need==1);
        ++s.grows;const unsigned token=8,capacity=2;std::memcpy(s.map->actor+0x45c,&token,4);std::memcpy(s.map->actor+0x460,&capacity,4);return 1;
    };
    st.resolve_vector=[](void* p,unsigned token,void** storage,unsigned* bytes) noexcept ->int{
        auto& s=*static_cast<StartOutput*>(p);assert(token==8);*storage=s.alias?static_cast<void*>(s.map->actor):static_cast<void*>(s.vector);*bytes=8;return 1;
    };
    nw::ServerEntityVoidOutputBindings ob{};ob.context=&so;
    ob.resolve=[](void* p,unsigned token,void** action,unsigned* bytes) noexcept ->int{
        auto& s=*static_cast<StartOutput*>(p);assert(token==9);*action=s.action;*bytes=0x1c;return 1;
    };
    ob.enqueue=[](void* p,void* activator,void* caller,const nw::ServerEntityVoidOutputAction* a) noexcept ->int{
        auto& s=*static_cast<StartOutput*>(p);assert(activator==s.map->player&&caller==s.map->actor&&a->target==201&&a->input==202&&a->action_id==77);
        unsigned count=0;std::memcpy(&count,s.map->actor+0x468,4);
        assert(count==s.calls+1&&s.vector[s.calls]==0x5002);
        ++s.calls;s.map->actor[0xb8]=1;return 1;
    };
    ob.release=[](void*,unsigned,void*) noexcept ->int{assert(false);return 0;};
    st.output=&ob;put(0x428,9);
    const auto start_output=reinterpret_cast<nm::StartCampaignTriggerOutput>(GetProcAddress(server,nm::start_touch_output_export));
    const auto contact_output=reinterpret_cast<nl::DispatchCampaignContactWithOutput>(GetProcAddress(server,nl::campaign_contact_output_export));
    assert(start_output&&contact_output&&!GetProcAddress(client,nm::start_touch_output_export)&&!GetProcAddress(client,nl::campaign_contact_output_export));
    nm::CampaignStartTouchResult sr{};
    assert(start_output(m.actor,sizeof(m.actor),m.player,sizeof(m.player),&st,&sr)&&sr.filter_called&&!sr.accepted&&!so.grows&&!so.calls);
    so.allow=true;put(0x464,0xffffffffu);
    assert(!start_output(m.actor,sizeof(m.actor),m.player,sizeof(m.player),&st,&sr)&&!so.grows&&!so.calls);
    put(0x464,0);so.alias=true;
    assert(!start_output(m.actor,sizeof(m.actor),m.player,sizeof(m.player),&st,&sr)&&sr.grew&&!sr.appended&&so.grows==1&&!so.calls);
    so.alias=false;cb.start_touch=nullptr;
    const auto filtered_contact=reinterpret_cast<nl::DispatchFilteredCampaignContact>(GetProcAddress(server,nl::filtered_campaign_contact_export));
    assert(filtered_contact&&!GetProcAddress(client,nl::filtered_campaign_contact_export));
    auto native_start=st;native_start.filter=nullptr;
    nm::CampaignFilterBindings filter{};nm::CampaignFilterResult fr{};
    put(0xb4,64);put(0x410,0xffffffffu);
    const auto previous_filters=so.filters;
    assert(filtered_contact(&session,m.actor,sizeof(m.actor),m.player,sizeof(m.player),ContactKind::Starting,&cb,&contact_result,&native_start,&sr,&filter,&fr)&&
        fr.accepted&&fr.eligibility==64&&fr.player&&so.filters==previous_filters&&
        sr.appended&&sr.output_fired&&sr.queued==1&&sr.count==1&&so.calls==1&&contact_result.start_called&&contact_result.touch_called&&
        contact_result.publication==fs::EventFired&&contact_result.trigger.removed&&m.removed==1&&started.events.fires==3&&!started.events.frees);
    put(0xb4,0);
    // Direct leaf preserves duplicate append and refuses external growth.
    assert(start_output(m.actor,sizeof(m.actor),m.player,sizeof(m.player),&st,&sr)&&sr.count==2&&so.calls==2&&so.vector[0]==so.vector[1]);
    put(0x464,0xffffffffu);
    assert(!start_output(m.actor,sizeof(m.actor),m.player,sizeof(m.player),&st,&sr)&&!sr.appended&&so.calls==2&&so.grows==1);
    put(0x464,0);
    assert(contact(&session,m.actor,sizeof(m.actor),&m.player,sizeof(m.player),ContactKind::Continuing,&cb,&contact_result)&&contact_result.marked_actor&&!contact_result.touch_called&&started.events.creates==3);
    put(0xb8,0);
    // Original map writer ignores ordinary FireEvent false. Health
    // remains published and activation continues; no double free.
    started.events.fire_ok=false;
    // Actual slot3 activation resolves a named filter before inherited
    // activation and performs ordinary map/health event publication.
    const auto activate_filters=reinterpret_cast<nl::InvokeActivateWithCampaignFilters>(GetProcAddress(server,nl::campaign_filters_activate_export));
    assert(activate_filters&&!GetProcAddress(client,nl::campaign_filters_activate_export));
    auto af=lb;
    af.next_entity=[](void* p,void* prev,void** next) noexcept ->int{
        auto& m=*static_cast<Map*>(p);
        *next=!prev?static_cast<void*>(m.actor):prev==m.actor?static_cast<void*>(m.named_filter):nullptr;return 1;
    };
    af.extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{
        auto& m=*static_cast<Map*>(p);
        *bytes=actor==m.actor?sizeof(m.actor):actor==m.named_filter?sizeof(m.named_filter):actor==m.player?sizeof(m.player):0;return *bytes!=0;
    };
    // Use the filter and name tokens created by slot2, not manual
    // reconstruction of the map entity at activation.
    put(0x410,0xffffffffu);
    sin_independent::EntityHandleEntry named_entry{};
    named_entry.entity=reinterpret_cast<CBaseEntity*>(m.named_filter);named_entry.serial=7;
    nw::ServerEntitySearchBindings search{};search.context=&m;search.head=&named_entry;search.entities.entries=&named_entry;search.entities.count=1;search.extent=af.extent;
    search.string_text=[](void*,unsigned token,const char** value,unsigned* bytes) noexcept ->int{
        if(token!=31)return 0;*value="named_filter";*bytes=13;return 1;
    };
    nm::CampaignFilterActivationBindings fab{};fab.context=&m;fab.search=&search;
    fab.base_activate=[](void* p,void* actor) noexcept ->int{
        auto& m=*static_cast<Map*>(p);
        if(actor==m.actor){
            unsigned handle=0;std::memcpy(&handle,m.actor+0x410,4);
            assert(handle==0x7000&&!m.base_activations);
        }else assert(actor==m.named_filter&&m.base_activations==1);
        ++m.base_activations;return 1;
    };
    nl::CampaignFilterActivationSummary fas{};
    assert(activate_filters(&ls,nullptr,0,1,&af,&lr,&session,&fab,&fas)&&lr.completed&&lr.map_event&&lr.health_event&&
        lr.activated==2&&fas.attempted==2&&fas.completed==2&&fas.lookups==1&&fas.resolved==1&&m.base_activations==2&&
        started.events.creates==5&&started.events.fires==5&&!started.events.frees);
    assert(network.calls==3&&edict_queries==8);
    assert(!activate_events(&ls,nullptr,0,1,&lb,&lr,&session)&&started.events.creates==5&&edict_queries==8);
    // Dynamic owner resolution occurs only after successful creation;
    // descriptor setter mutates the owner's full handle before it is read.
    put(0,nm::dynamic_trigger_vtable);put(0x470,0x41480000u);put(0x474,7);
    put(0x2bc,0x3f800000u);put(0x2c0,0x40000000u);put(0x2c4,0x40400000u);
    started.events.fire_ok=true;started.events.event.lookups=&edict_queries;
    started.events.event.expected_lookups=9;started.events.event.expected_owner=0x6002;
    started.events.event.mutation=m.player+0x378;
    assert(!tr.owner_word&&contact(&session,m.actor,sizeof(m.actor),&m.player,sizeof(m.player),ContactKind::Continuing,&cb,&contact_result)&&
        !contact_result.start_called&&contact_result.touch_called&&contact_result.publication==fs::EventFired&&m.removed==2&&edict_queries==9&&network.calls==4);
    set_clients(2);started.events.event.expected_owner=-1;started.events.event.mutation=nullptr;
    assert(touch_events(&session,m.actor,sizeof(m.actor),&m.player,&tr,&cr,&publication)&&publication==fs::EventFired&&m.removed==3&&edict_queries==9);
    set_clients(1);started.events.unavailable=true;
    assert(touch_events(&session,m.actor,sizeof(m.actor),&m.player,&tr,&cr,&publication)&&publication==fs::EventUnavailable&&cr.event_unavailable&&m.removed==4&&edict_queries==9&&started.events.creates==8&&started.events.fires==7&&!started.events.frees);
    session={};assert(!resolve_player(&session,&player)&&!player&&edict_queries==9);
    assert(!touch_events(&session,m.actor,sizeof(m.actor),&m.player,&tr,&cr,&publication)&&m.removed==4);
    FreeLibrary(server);FreeLibrary(client);
}
