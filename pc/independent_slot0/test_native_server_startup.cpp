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
        Bindings v{};v.context=this;v.globals=&objects[13];v.engine=v.physics=v.filesystem=&factory;v.event_manager=&events;
        v.console=[](void* p,void** d,void** c) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(f.events.calls==1);*d=&f.objects[0];*c=f.cheats?&f.objects[1]:nullptr;return 1;};
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
    FreeLibrary(server);FreeLibrary(client);
}
