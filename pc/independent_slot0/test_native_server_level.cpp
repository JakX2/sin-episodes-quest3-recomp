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
    // Real direct-target queue records from the LevelInit restore branch.
    // Reuse the native service; never broadcast Save/Kill to matching names.
    using namespace sin_port::native_weapon_tables;
    struct QueueFixture {
        alignas(4) unsigned char root[0x38]{},rows[2][0x38]{};
        unsigned alloc=0,delivered=0,released=0,locks=0;void* target=nullptr;
    } qf;
    Fixture queued;queued.load=true;qf.target=queued.actor;
    put(queued.actor,0x378,0x2000u);
    sin_independent::EntityHandleEntry entries[1]{};entries[0].serial=2;entries[0].entity=reinterpret_cast<CBaseEntity*>(queued.actor);
    ServerEntityNamedEventQueueBindings qb{};qb.context=&qf;
    qb.allocate=[](void* p,unsigned* token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<QueueFixture*>(p);assert(q.alloc<2);*token=11+q.alloc;*row=q.rows[q.alloc++];*bytes=0x38;return 1;};
    qb.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<QueueFixture*>(p);assert(token==11||token==12);*row=q.rows[token-11];*bytes=0x38;return 1;};
    qb.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{assert(token==31||token==32);*yes=true;return 1;};
    float now=10.f;DirectQueue dq{qf.root,sizeof(qf.root),10,&now,entries,1,&qb,[](void*,const char* n,unsigned* token) noexcept ->int{*token=!std::strcmp(n,"Save")?31:32;return 1;},nullptr};
    b=queued.bind();b.queue_direct=nullptr;b.native_queue=&dq;a={queued.map,"entities","SE1_intro03","landmark",true,false};
    assert(level(&queued.state,&a,&b,&o)&&o.completed&&o.queued==2&&o.save_event.inserted&&o.kill_event.inserted&&o.save_event.fire_time==11.f&&o.kill_event.fire_time==11.1f&&qf.alloc==2);
    auto word=[](const void* row,unsigned off){unsigned v=0;std::memcpy(&v,static_cast<const unsigned char*>(row)+off,4);return v;};
    assert(word(qf.root,0x30)==11&&word(qf.rows[0],4)==0&&word(qf.rows[0],0x18)==0x2000&&word(qf.rows[0],8)==31&&word(qf.rows[0],0x30)==12&&word(qf.rows[1],0x34)==11);
    auto service_get=reinterpret_cast<GetServerEntityEventServiceApi>(GetProcAddress(server,server_entity_event_service_export));assert(service_get);
    auto* service=service_get(1,sizeof(ServerEntityEventServiceApi));assert(service);
    ServerEntityEventServiceBindings sb{};sb.context=&qf;sb.entities.entries=entries;sb.entities.count=1;sb.resolve=qb.resolve;
    sb.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<QueueFixture*>(p);if(enter)++q.locks;else --q.locks;return 1;};
    sb.accept=[](void* p,void* target,unsigned input,void* activator,void* caller,const void* variant,unsigned id) noexcept ->int{
        auto& q=*static_cast<QueueFixture*>(p);assert(q.locks==1&&target==q.target&&input==31+q.delivered&&!activator&&!caller&&!id);
        unsigned type=1;std::memcpy(&type,static_cast<const unsigned char*>(variant)+0x10,4);assert(!type);++q.delivered;return 1;
    };
    sb.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<QueueFixture*>(p);assert(token==11+q.released&&row==q.rows[q.released]);++q.released;return 1;};
    ServerEntityEventServiceResult sr{};int steps=0;
    assert(service->service(qf.root,sizeof(qf.root),10,10.99f,false,&steps,&sb,&sr)&&sr.future_stopped&&!qf.delivered);
    assert(service->service(qf.root,sizeof(qf.root),10,11.f,false,&steps,&sb,&sr)&&sr.direct==1&&qf.delivered==1&&qf.released==1&&sr.future_stopped&&!qf.locks);
    assert(service->service(qf.root,sizeof(qf.root),10,11.1f,false,&steps,&sb,&sr)&&sr.completed&&sr.direct==1&&qf.delivered==2&&qf.released==2&&!word(qf.root,0x30)&&!qf.locks);
    // Repeat actual LevelInit saved-level path with native Save AND
    // inherited Kill, no foreign AcceptInput callback.
    Fixture native_saved;native_saved.load=true;qf={};qf.target=native_saved.actor;
    namespace navmap=sin_port::native_map;
    put(native_saved.actor,0,navmap::autosave_vtable);put(native_saved.actor,0x378,0x2000u);
    put(native_saved.actor,0x20c,0xffffffffu);native_saved.actor[0x394]=1;native_saved.actor[0x1f0]=1;
    entries[0].entity=reinterpret_cast<CBaseEntity*>(native_saved.actor);now=10.f;
    b=native_saved.bind();b.queue_direct=nullptr;b.native_queue=&dq;a={native_saved.map,"entities","SE1_intro03","landmark",true,false};
    assert(level(&native_saved.state,&a,&b,&o)&&o.queued==2&&qf.alloc==2);
    struct SaveFixture {QueueFixture* queue;void* actor;void* attachment=nullptr;unsigned char gate=0;std::string calls;} sf{&qf,native_saved.actor};
    navmap::AutosaveBindings behavior{};behavior.context=&sf;
    behavior.new_level_unit=[](void* p) noexcept ->int{auto& f=*static_cast<SaveFixture*>(p);assert(f.queue->locks==1);f.calls+='N';return 1;};
    behavior.difficulty_gate=[](void* p,unsigned char* gate) noexcept ->int{auto& f=*static_cast<SaveFixture*>(p);assert(f.queue->locks==1);f.calls+='G';*gate=f.gate;return 1;};
    behavior.server_command=[](void* p,const char* command) noexcept ->int{auto& f=*static_cast<SaveFixture*>(p);assert(f.queue->locks==1&&!std::strcmp(command,"autosave\n"));f.calls+='S';return 1;};
    behavior.attachment_notice=[](void* p,void* attachment,void* actor) noexcept ->int{auto& f=*static_cast<SaveFixture*>(p);assert(attachment==f.attachment&&actor==f.actor);f.calls+='A';return 1;};
    behavior.clear_attachment=[](void* p,void* actor) noexcept ->int{auto& f=*static_cast<SaveFixture*>(p);assert(actor==f.actor);f.calls+='C';put(actor,0x20c,0xffffffffu);return 1;};
    behavior.remove=[](void* p,void* actor) noexcept ->int{auto& f=*static_cast<SaveFixture*>(p);assert(actor==f.actor);f.calls+='K';put(actor,0xb8,1u);return 1;};
    ServerNpcActivePostAnimationBindings storage{};
    ServerEntityAcceptInputBindings input{};input.storage=&storage;input.logic_autosave=&behavior;
    input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
        *text=token==31||token==0x22420850?"Save":token==32||token==0x2240b624?"Kill":nullptr;
        if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
    };
    ServerEntitySearchBindings search{};search.context=&sf;
    search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<SaveFixture*>(p);*bytes=a==f.actor?navmap::autosave_extent:0;return *bytes!=0;};
    sb.native_input=&input;sb.native_search=&search;sb.accept=nullptr;sb.live_now=&now;
    now=10.99f;
    assert(service->service(qf.root,sizeof(qf.root),10,now,false,&steps,&sb,&sr)&&sr.future_stopped&&sf.calls.empty());
    now=11.f;
    assert(service->service(qf.root,sizeof(qf.root),10,now,false,&steps,&sb,&sr)&&sr.direct==1&&sr.native_inputs==1&&sr.future_stopped&&sf.calls=="NGS"&&qf.released==1&&native_saved.actor[0x200]&&!word(native_saved.actor,0xb8));
    now=11.1f;
    assert(service->service(qf.root,sizeof(qf.root),10,now,false,&steps,&sb,&sr)&&sr.direct==1&&sr.native_inputs==1&&sf.calls=="NGSK"&&qf.released==2&&word(native_saved.actor,0xb8)==1&&!word(qf.root,0x30)&&!qf.locks);
    // Actual inherited base row78 with a live attachment, then stale
    // serial. Owner receives the exact virtual effects before removal.
    alignas(4) unsigned char attachment[4]{};
    sf.attachment=attachment;sin_independent::EntityHandleEntry attached[2]{};
    attached[0]=entries[0];attached[1].serial=3;attached[1].entity=reinterpret_cast<CBaseEntity*>(attachment);
    input.entities.entries=attached;input.entities.count=2;
    auto accept_get=reinterpret_cast<GetServerEntityAcceptInputApi>(GetProcAddress(server,server_entity_accept_input_export));assert(accept_get);
    auto* accept=accept_get(1,sizeof(ServerEntityAcceptInputApi));assert(accept);
    unsigned payload[5]{0,0,0,0xffffffffu,0};ServerEntityAcceptInputResult accepted{};
    put(native_saved.actor,0x20c,0x3001u);put(native_saved.actor,0xb8,0);
    assert(accept->run(native_saved.actor,sizeof(native_saved.actor),navmap::autosave_vtable,32,nullptr,nullptr,payload,sizeof(payload),0,&input,&accepted)&&accepted.native_handler&&accepted.map==0x224f0350&&accepted.row==78&&sf.calls=="NGSKACK"&&word(native_saved.actor,0x20c)==0xffffffffu);
    put(native_saved.actor,0x20c,0x4001u);put(native_saved.actor,0xb8,0);
    assert(accept->run(native_saved.actor,sizeof(native_saved.actor),navmap::autosave_vtable,32,nullptr,nullptr,payload,sizeof(payload),0,&input,&accepted)&&accepted.native_handler&&sf.calls=="NGSKACKK"&&word(native_saved.actor,0x20c)==0x4001u);
    // Native map pipeline is reachable from actual LevelInit slot2,
    // with no hosted parse_entities callback.
    namespace nm=sin_port::native_map;
    Fixture pipeline_level;auto lb=pipeline_level.bind();lb.parse_entities=nullptr;
    nm::Bindings mp{};mp.context=&pipeline_level;
    mp.create=[](void* p,const char* cls,void** a) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(!std::strcmp(cls,"item_dynamic_resupply"));*a=f.actor;return 1;};
    mp.clear_factory_global=[](void*) noexcept ->int{return 1;};
    mp.key_value=[](void*,void*,const char*,const char*) noexcept ->int{return 1;};
    nm::SpawnBindings ms{};ms.context=&pipeline_level;ms.extent=lb.extent;
    ms.classname=[](void*,void*,const char** cls) noexcept ->int{*cls="item_dynamic_resupply";return 1;};
    ms.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
    ms.debug_enabled=[](void*,bool* yes) noexcept ->int{*yes=false;return 1;};
    ms.spawn=[](void* p,void* a,int* code) noexcept ->int{auto& f=*static_cast<Fixture*>(p);assert(a==f.actor);f.seq+='N';*code=0;return 1;};
    nm::PipelineBindings pipeline{};pipeline.context=&pipeline_level;pipeline.parse=&mp;pipeline.spawn=&ms;
    pipeline.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
    pipeline.classify=[](void*,void*,nm::Category* category) noexcept ->int{*category=nm::Category::Ordinary;return 1;};
    const char lump[]="{ classname item_dynamic_resupply }";
    lb.native_map_pipeline=&pipeline;lb.entity_text_bytes=sizeof(lump);
    a={pipeline_level.map,lump,nullptr,nullptr,false,false};
    assert(level(&pipeline_level.state,&a,&lb,&o)&&o.completed&&o.parsed&&o.map_pipeline.completed&&o.map_pipeline.parse.created==1&&o.map_pipeline.spawn.spawned==1&&!o.map_pipeline.spawn.activated&&pipeline_level.seq=="[PCENTMD]");
    // Real slot2/3: native factory and keys, DispatchSpawn and
    // activation through the live registry; no hosted sequence leaf.
    {
        const auto level_seq=reinterpret_cast<InvokeLevelWithSequences>(GetProcAddress(server,scripted_sequence_level_export));
        const auto activate_seq=reinterpret_cast<InvokeActivateWithSequences>(GetProcAddress(server,scripted_sequence_activate_export));
        assert(level_seq&&activate_seq&&!GetProcAddress(client,scripted_sequence_level_export)&&!GetProcAddress(client,scripted_sequence_activate_export));
        struct SequenceLevel : Fixture {
            alignas(4) unsigned char scripts[2][nm::scripted_sequence_extent]{},npc[0x974]{};
            sin_independent::EntityHandleEntry registry[3]{};
            float now=10.f,interval=.125f;
            unsigned allocations=0,constructs=0,initializes=0,clears=0,solid=0,notifications=0,dispatch_enters=0,dispatch_leaves=0,base=0,foreign=0,listener_count=1,listened=0;
            void* listeners[1]{&listener_count};
        } sl;
        auto seq_extent=[](void* p,void* entity,unsigned* n) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(p);
            *n=entity==q.scripts[0]||entity==q.scripts[1]?nm::scripted_sequence_extent:entity==q.npc?sizeof(q.npc):0;return *n!=0;
        };
        auto token_text=[](void*,unsigned token,const char** text,unsigned* n) noexcept ->int{
            *text=token==21?"scripted_sequence":token==22?"first":token==23?"second":token==24?"guard":token==25?"missing":token==26?"npc_sintek_grunt":nullptr;
            if(!*text)return 0;*n=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        nm::RelayFactoryBindings owner{};owner.context=&sl;
        owner.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(p);assert(n==nm::scripted_sequence_extent&&q.allocations<2);
            *a=q.scripts[q.allocations++];*bytes=n;return 1;
        };
        owner.base_construct=[](void* p,void* a,int arg) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(p);assert(!arg&&a==q.scripts[q.constructs]);
            put(a,0x378,((3u+q.constructs)<<12)|q.constructs);put(a,0xb8,0x400000u);put(a,0x74,-1);++q.constructs;return 1;
        };
        owner.initialize=[](void* p,void* a,const char* cls) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(p);assert(!std::strcmp(cls,"scripted_sequence")&&a==q.scripts[q.initializes]);
            auto& entry=q.registry[q.initializes];entry.entity=reinterpret_cast<CBaseEntity*>(a);entry.serial=3+q.initializes;++q.initializes;return 1;
        };
        for(unsigned i=0;i<3;++i){
            if(i)sl.registry[i].previous=&sl.registry[i-1];if(i<2)sl.registry[i].next=&sl.registry[i+1];
        }
        sl.registry[2].entity=reinterpret_cast<CBaseEntity*>(sl.npc);sl.registry[2].serial=5;
        put(sl.npc,0,0x224954a4u);put(sl.npc,0x378,0x5002u);put(sl.npc,0xc0,24u);put(sl.npc,0xc,26u);put(sl.npc,0x970,0xffffffffu);
        auto map_get=reinterpret_cast<nm::GetApi>(GetProcAddress(server,nm::export_name));assert(map_get);
        const auto* map_api=map_get(1,sizeof(nm::Api));assert(map_api);
        nm::FactoryDictionary dictionary{};assert(map_api->register_factories(&dictionary));
        nm::FactoryBindings factories{};factories.context=&sl;factories.dictionary=&dictionary;
        factories.foreign_create=[](void*,const char*,void**,unsigned*) noexcept ->int{assert(false);return 0;};
        nm::KeyBindings key{};key.context=&sl;key.extent=seq_extent;
        key.intern=[](void*,const char* value,unsigned* token) noexcept ->int{
            const char* names[]{"scripted_sequence","first","second","guard","missing","npc_sintek_grunt"};
            for(unsigned i=0;i<6;++i)if(!std::strcmp(value,names[i])){*token=21+i;return 1;}return 0;
        };
        key.foreign=[](void*,void*,const char*,const char*,bool*) noexcept ->int{assert(false);return 0;};
        nm::Bindings parse{};parse.context=&sl;parse.native_factory=&factories;parse.native_keys=&key;
        parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<SequenceLevel*>(p)->clears;return 1;};
        nm::DispatchBindings dispatch{};dispatch.context=&sl;dispatch.entries=sl.registry;dispatch.entry_count=3;dispatch.current_map=&sl.map;
        dispatch.scope=[](void* p,bool enter) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(p);if(enter)++q.dispatch_enters;else ++q.dispatch_leaves;return 1;
        };
        dispatch.listeners=sl.listeners;dispatch.listener_count=&sl.listener_count;dispatch.listener_capacity=1;
        dispatch.notify=[](void* p,void* listener,void* actor) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(p);assert(listener==q.listeners[0]&&(actor==q.scripts[0]||actor==q.scripts[1])&&q.dispatch_enters==q.dispatch_leaves+1);++q.listened;return 1;
        };
        nm::SpawnBindings spawn{};spawn.context=&sl;spawn.extent=seq_extent;spawn.native_dispatch=&dispatch;
        spawn.classname=[](void*,void*,const char** cls) noexcept ->int{*cls="scripted_sequence";return 1;};
        spawn.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
        spawn.debug_enabled=[](void*,bool* yes) noexcept ->int{*yes=false;return 1;};
        spawn.virtual_spawn=[](void*,void*) noexcept ->int{assert(false);return 0;};
        nm::PipelineBindings pipe{};pipe.context=&sl;pipe.parse=&parse;pipe.spawn=&spawn;
        pipe.scope=[](void*,const char*,bool) noexcept ->int{return 1;};
        pipe.classify=[](void*,void*,nm::Category* category) noexcept ->int{*category=nm::Category::Ordinary;return 1;};
        ServerNpcOuterCadenceBindings cadence{};cadence.context=&sl;
        cadence.notify_think_flags=[](void* p,void* a) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(p);assert(a==q.scripts[0]);++q.notifications;return 1;
        };
        nm::ScriptedSequenceControlBindings control{};control.context=&sl;control.extent=seq_extent;
        control.now=&sl.now;control.interval=&sl.interval;control.cadence=&cadence;control.entries=sl.registry;control.entry_count=3;
        nm::ScriptedSequenceSpawnBindings seq_spawn{&control,&sl};
        seq_spawn.solid_type=[](void* p,void* a,int n) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(p);assert(!n&&(a==q.scripts[0]+0xfc||a==q.scripts[1]+0xfc));++q.solid;return 1;
        };
        nm::ScriptedSequenceMapBindings map_route{&owner,&seq_spawn,nullptr};
        auto level_b=sl.bind();level_b.parse_entities=nullptr;level_b.native_map_pipeline=&pipe;
        const char sequence_lump[]="{ classname scripted_sequence targetname first m_iszEntity guard m_iszNextScript second spawnflags 16 } { classname scripted_sequence targetname second m_iszEntity guard m_iszNextScript missing }";
        level_b.entity_text_bytes=sizeof(sequence_lump);
        LevelArgs la{sl.map,sequence_lump,nullptr,nullptr,false,false};
        auto invalid=spawn;invalid.native_dispatch=nullptr;pipe.spawn=&invalid;
        assert(!level_seq(&sl.state,&la,&level_b,&o,nullptr,&map_route)&&!sl.allocations&&!sl.state.level_ready&&sl.seq.empty());
        pipe.spawn=&spawn;
        assert(level_seq(&sl.state,&la,&level_b,&o,nullptr,&map_route)&&o.completed&&o.parsed&&o.map_pipeline.parse.native_creates==2&&o.map_pipeline.spawn.native_dispatches==2&&o.map_pipeline.spawn.native_spawns==2&&!o.map_pipeline.spawn.activated);
        assert(dictionary.count==12&&sl.allocations==2&&sl.constructs==2&&sl.initializes==2&&sl.clears==2&&sl.solid==2&&sl.notifications==1&&sl.dispatch_enters==2&&sl.dispatch_leaves==2&&sl.listened==2);
        assert(word(sl.scripts[0],0x74)==88&&word(sl.scripts[0],8)==0x221afc90u&&sl.scripts[0][0x3c0]&&word(sl.scripts[0],0x3d4)==0xffffffffu&&word(sl.scripts[1],0x74)==0xffffffffu);
        ServerEntitySearchBindings seq_search{};seq_search.context=&sl;seq_search.head=sl.registry;seq_search.extent=seq_extent;seq_search.string_text=token_text;
        nm::ScriptedSequenceActivateBindings seq_activate{&control,&seq_search,&sl};
        seq_activate.base_activate=[](void* p,void* actor) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(p);assert(q.state.level_ready&&actor==q.scripts[q.base]&&q.solid==2);++q.base;return 1;
        };
        ScriptedSequenceActivationRoute route{&seq_activate,nullptr,nullptr};ScriptedSequenceActivationSummary summary{};
        level_b.next_entity=[](void* p,void* previous,void** a) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(static_cast<Fixture*>(p));
            *a=!previous?q.scripts[0]:previous==q.scripts[0]?q.scripts[1]:previous==q.scripts[1]?q.npc:nullptr;return 1;
        };
        level_b.extent=[](void* p,void* actor,unsigned* n) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(static_cast<Fixture*>(p));
            *n=actor==q.scripts[0]||actor==q.scripts[1]?nm::scripted_sequence_extent:actor==q.npc?sizeof(q.npc):actor==q.player?sizeof(q.player):0;return *n!=0;
        };
        level_b.activate=[](void* p,void* actor) noexcept ->int{
            auto& q=*static_cast<SequenceLevel*>(static_cast<Fixture*>(p));assert(actor==q.npc&&q.base==2);++q.foreign;return 1;
        };
        sl.transition=true;
        assert(activate_seq(&sl.state,nullptr,64,1,&level_b,&o,nullptr,&route,&summary)&&o.activation_skipped&&!summary.attempted&&!sl.base);
        sl.transition=false;
        assert(activate_seq(&sl.state,nullptr,64,1,&level_b,&o,nullptr,&route,&summary)&&o.completed&&o.visited==3&&o.activated==3&&summary.attempted==2&&summary.completed==2&&summary.npc_found==2&&summary.next_found==1&&summary.next_name_cleared==1&&sl.base==2&&sl.foreign==1);
        assert(word(sl.scripts[0],0x3d4)==0x4001u&&word(sl.scripts[1],0x3d4)==0xffffffffu&&!word(sl.scripts[1],0x3a4)&&word(sl.npc,0x970)==0xffffffffu);
        assert(!activate_seq(&sl.state,nullptr,64,1,&level_b,&o,nullptr,&route,&summary)&&!summary.attempted);
    }
    FreeLibrary(server);FreeLibrary(client);
}
