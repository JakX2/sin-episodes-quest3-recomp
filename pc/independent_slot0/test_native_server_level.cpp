#include "native_server_level.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::native_level;
template<class T>void put(void* a,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(a)+off,&v,sizeof(v));}
struct Fixture{
    Startup startup;bool transition=false,finalize=true,load=false,autosave=true,activation_value=false;
    int kind=-1;const char* map="SE1_docks01";unsigned words[4]{1,2,3,4},storage=0,system=0,manager=0;
    State state{};std::string seq;unsigned events=0;
    alignas(4) unsigned char actor[0x3b0]{},removed[0xbc]{},player[0xa0]{};
    Fixture(){startup.game_init_complete=true;startup.registry.count=1;startup.registry.systems[0]=&system;
        state.startup=&startup;state.transition=&transition;state.restore_finalize=&finalize;state.load_kind=&kind;state.current_map=&map;state.map_pair_words=words;
        state.transition_vector=&storage;state.transition_capacity=8;state.transition_count=3;state.transition_cache=&storage;
        put(actor,0,sin_port::native_resupply::resupply_vtable);put(removed,0xb8,2u);put(player,0x9c,43);}
    Bindings bind(){
        Bindings b{};b.context=this;
        b.scope=[](void* p,const char* n,bool enter) noexcept ->int{assert(!std::strcmp(n,"CServerGameDLL::LevelInit"));static_cast<Fixture*>(p)->seq+=enter?'[':']';return 1;};
        b.pre_level=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->seq+='P';return 1;};
        b.chapter=[](void* p,const char* map) noexcept ->int{assert(!std::strcmp(map,"SE1_docks01"));static_cast<Fixture*>(p)->seq+='C';return 1;};
        b.filename=[](void*,char* name,unsigned cap) noexcept ->int{assert(cap==260);std::memcpy(name,"save.sav",9);return 1;};
        b.map_event=[](void* p,int d,const char* map,const char* file) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(!std::strcmp(map,f.map));assert(d==7?!*file:!std::strcmp(file,"save.sav"));++f.events;f.seq+='E';return 1;};
        b.transition_warning=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->seq+='W';return 1;};
        b.cleanup=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->seq+='D';return 1;};
        b.release=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(a==&f.storage);f.seq+='F';return 1;};
        b.load_saved=[](void* p,const char* map,bool yes,bool* loaded) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(!std::strcmp(map,f.map)&&yes&&f.transition);*loaded=f.load;f.seq+='L';return 1;};
        b.parse_entities=[](void* p,const char* text,void* filter,bool activate) noexcept ->int{assert(!std::strcmp(text,"entities")&&!filter&&!activate);static_cast<Fixture*>(p)->seq+='N';return 1;};
        b.restore_transition=[](void* p,const char* old,const char* mark) noexcept ->int{assert(!std::strcmp(old,"SE1_intro03")&&!std::strcmp(mark,"landmark"));static_cast<Fixture*>(p)->seq+='R';return 1;};
        b.finalize_restore=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->seq+='X';return 1;};
        b.autosave_enabled=[](void* p,bool* yes) noexcept ->int{*yes=static_cast<Fixture*>(p)->autosave;return 1;};
        b.find_autosave=[](void* p,void** a) noexcept ->int{*a=static_cast<Fixture*>(p)->actor;return 1;};
        b.queue_direct=[](void* p,void* a,const char* input,float delay,void* activator,void* caller,int id) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(a==f.actor&&!activator&&!caller&&!id);if(!std::strcmp(input,"Save")){assert(delay==1);f.seq+='S';}else{assert(!std::strcmp(input,"Kill")&&delay==1.1f);f.seq+='K';}return 1;};
        b.post_level=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->seq+='T';return 1;};
        b.motd=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->seq+='M';return 1;};
        b.delete_queue_count=[](void*,unsigned* n) noexcept ->int{*n=1;return 1;};
        b.delete_warning=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->seq+='w';return 1;};
        b.next_entity=[](void* p,void* prev,void** next) noexcept ->int{auto& f=*static_cast<Fixture*>(p);*next=!prev?f.actor:prev==f.actor?f.removed:nullptr;return 1;};
        b.extent=[](void* p,void* a,unsigned* n) noexcept ->int{auto& f=*static_cast<Fixture*>(p);*n=a==f.actor?sizeof(f.actor):a==f.removed?sizeof(f.removed):sizeof(f.player);return 1;};
        b.entity_scope=[](void* p,bool enter) noexcept ->int{static_cast<Fixture*>(p)->seq+=enter?'(':')';return 1;};
        b.activate=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(a==f.actor);f.seq+='A';return 1;};
        b.system_activate=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(a==&f.system);f.seq+='Y';return 1;};
        b.set_activation_flag=[](void* p,int n) noexcept ->int{assert(!n);static_cast<Fixture*>(p)->seq+='Z';return 1;};
        b.local_player=[](void* p,void** a) noexcept ->int{*a=static_cast<Fixture*>(p)->player;return 1;};
        b.health_event=[](void* p,void* a,int d,int hp) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(a==f.player&&d==21&&hp==43);f.seq+='H';return 1;};
        b.activation_cvar=[](void* p,bool* v) noexcept ->int{*v=static_cast<Fixture*>(p)->activation_value;return 1;};
        b.activation_cvar_set=[](void* p,int n) noexcept ->int{assert(!n);static_cast<Fixture*>(p)->seq+='V';return 1;};
        b.manager_activate=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->seq+='G';return 1;};
        return b;
    }
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetApi>(GetProcAddress(server,export_name));assert(get&&!GetProcAddress(client,export_name));const auto* api=get(1,sizeof(Api));assert(api);
    Fixture f;auto b=f.bind();Result o{};LevelArgs a{f.map,"entities",nullptr,nullptr,false,false};
    assert(api->level_init(&f.state,&a,&b,&o)&&o.completed&&o.descriptor==1&&o.parsed&&!o.load_called&&f.seq=="[PCENTMD]"&&!f.finalize&&f.words[1]==0xffffffffu);
    f.seq.clear();
    assert(api->server_activate(&f.state,&b,&o)&&o.completed&&o.visited==2&&o.skipped==1&&o.activated==1&&o.systems==1&&o.health_event&&f.seq=="w(A)(Y)ZEHVG");
    assert(!api->server_activate(&f.state,&b,&o));
    Fixture saved;saved.transition=true;saved.load=true;b=saved.bind();a={saved.map,"entities","SE1_intro03","landmark",true,false};
    assert(api->level_init(&saved.state,&a,&b,&o)&&o.descriptor==4&&o.load_success&&!o.parsed&&o.restored&&o.finalized&&o.queued==2&&saved.seq=="[PCEWDFLRXSKTMD]");
    assert(api->server_activate(&saved.state,&b,&o)&&o.activation_skipped&&!saved.state.activation_attempted);
    saved.transition=false;saved.seq.clear();
    assert(api->server_activate(&saved.state,&b,&o)&&o.completed&&saved.seq=="w(A)(Y)ZEHVG");
    Fixture missing;b=missing.bind();a={missing.map,"entities",nullptr,nullptr,true,false};
    assert(api->level_init(&missing.state,&a,&b,&o)&&o.descriptor==2&&o.load_called&&!o.load_success&&o.parsed&&!o.restored&&!o.queued);
    Fixture background;b=background.bind();a={background.map,"entities",nullptr,nullptr,false,true};
    assert(api->level_init(&background.state,&a,&b,&o)&&o.descriptor==3);
    background.seq.clear();assert(api->server_activate(&background.state,&b,&o)&&!o.map_event&&background.seq=="w(A)(Y)ZHVG");
    // All twenty authenticated map prefix rows and original case-sensitive
    // localization substring gate. No artificial chapter unlock.
    unsigned game_calls=0;
    ChapterBindings cb{};cb.context=&game_calls;
    cb.game_argument=[](void* p,const char* name,const char* fallback,const char** game) noexcept ->int{
        assert(!std::strcmp(name,"-game")&&!std::strcmp(fallback,"se1"));++*static_cast<unsigned*>(p);*game=fallback;return 1;
    };
    cb.current=[](void*,const char**) noexcept ->int{assert(false);return 0;};
    cb.set=[](void*,const char*) noexcept ->int{assert(false);return 0;};
    ChapterResult cr{};
    for(unsigned i=0;i<20;++i)assert(api->unlock_chapter(chapters[i].map,&cb,&cr)&&cr.completed&&cr.matched&&cr.match_index==i&&!cr.prefix_found&&!cr.changed);
    assert(game_calls==20);
    assert(api->unlock_chapter("se1_U4LAB03_variant",&cb,&cr)&&cr.matched&&cr.match_index==10&&!cr.prefix_found);
    assert(api->unlock_chapter("unknown",&cb,&cr)&&!cr.matched&&game_calls==21);
    // Actual interface slot2 and slot3, controlled synchronous bindings.
    auto level=reinterpret_cast<InvokeLevel>(GetProcAddress(server,level_export));
    auto activate=reinterpret_cast<InvokeActivate>(GetProcAddress(server,activate_export));
    assert(level&&activate&&!GetProcAddress(client,level_export)&&!GetProcAddress(client,activate_export));
    Fixture actual;actual.startup.game_init_complete=false;b=actual.bind();a={actual.map,"entities",nullptr,nullptr,false,false};
    assert(!level(&actual.state,&a,&b,&o)&&actual.seq.empty());
    actual.startup.game_init_complete=true;b.chapter=nullptr;b.native_chapter=&cb;
    assert(level(&actual.state,&a,&b,&o)&&o.completed&&o.chapter_result.matched&&o.chapter_result.match_index==3&&actual.seq=="[PENTMD]");
    actual.seq.clear();assert(activate(&actual.state,nullptr,64,1,&b,&o)&&o.completed&&actual.seq=="w(A)(Y)ZEHVG");
    assert(!activate(&actual.state,nullptr,64,1,&b,&o));
    FreeLibrary(server);FreeLibrary(client);
}
