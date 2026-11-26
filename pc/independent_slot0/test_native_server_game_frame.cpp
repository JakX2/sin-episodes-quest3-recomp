#include "native_server_game_frame.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <string>
using namespace sin_port::native_game_frame;
template<class T>void put(void* a,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(a)+off,&v,sizeof(v));}
struct Fixture {
    Globals g{0,0,0,10,.125f,1,80,.125f};
    bool transition=false,skip=false,profile=false;
    int double_step=1,profile_cvar=1;
    unsigned depth=0,cleanup=0,clients=0,edict=0,other=0;
    bool fail=false,stationary_mode=false;
    InvokeBoundary boundary=nullptr;const State* state=nullptr;const Bindings* bindings=nullptr;unsigned reentry_refused=0;
    unsigned edict_enters=0,edict_leaves=0,records=0;
    alignas(4) unsigned char edict_actor[0x3b0]{},foreign[0x3b0]{},native_actor[0x3b0]{};
    void* slots[3]{edict_actor,foreign,native_actor};
    std::uint16_t order[4]{2,0,1,0};
    std::string sequence;
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    const auto get=reinterpret_cast<GetApi>(GetProcAddress(server,export_name));assert(get&&!GetProcAddress(client,export_name)&&!get(0,sizeof(Api)));
    const auto* api=get(1,sizeof(Api));assert(api);
    Fixture f;State state{&f.g,&f.transition,&f.skip,&f.double_step,&f.profile_cvar,&f.profile,&f.depth};
    Bindings b{};b.context=&f;
    b.scope=[](void* p,const char* n,bool enter) noexcept ->int{auto& q=*static_cast<Fixture*>(p);if(!std::strcmp(n,"CServerGameDLL::GameFrame"))q.sequence+=enter?'[':']';else if(!std::strcmp(n,"Physics_RunThinkFunctions"))q.sequence+=enter?'(':')';else if(!std::strcmp(n,"ServiceEventQueue()"))q.sequence+=enter?'{':'}';else{assert(q.stationary_mode&&(!std::strcmp(n,"pEntity->PhysicsSimulate")||!std::strcmp(n,"CBaseEntity::PhysicsSimulate")||!std::strcmp(n,"CBaseEntity::PhysicsNone")));}return 1;};
    b.cleanup=[](void* p) noexcept ->int{auto& q=*static_cast<Fixture*>(p);++q.cleanup;q.sequence+='C';return 1;};
    b.pre_systems=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='S';return 1;};
    b.pre_actors=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='P';return 1;};
    b.manager=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='M';return 1;};
    b.engine_begin=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='B';return 1;};
    b.active=[](void* p,ActiveList* l) noexcept ->int{auto& q=*static_cast<Fixture*>(p);assert(q.depth==1);*l={q.order,3,4,q.slots,3};return 1;};
    b.extent=[](void*,void*,unsigned* n) noexcept ->int{*n=0x3b0;return 1;};
    b.edict_simulate=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<Fixture*>(p);assert((a==q.edict_actor||(q.stationary_mode&&a==q.native_actor))&&q.g.curtime==10&&q.depth==1);++q.edict;q.sequence+='E';q.g.curtime=999;q.slots[1]=nullptr;return 1;};
    b.other_think=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<Fixture*>(p);assert((a==q.foreign||(q.stationary_mode&&a==q.native_actor))&&q.g.curtime==10);++q.other;q.sequence+='T';q.g.curtime=888;return !q.fail;};
    b.client=[](void* p,int i,void** a) noexcept ->int{auto& q=*static_cast<Fixture*>(p);++q.clients;assert(i==1||i==2);*a=i==1?q.foreign:nullptr;return 1;};
    b.post_systems=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='s';return 1;};
    b.event_service=[](void* p) noexcept ->int{auto& q=*static_cast<Fixture*>(p);assert(q.g.curtime==10&&q.depth==0);q.sequence+='Q';return 1;};
    b.post_cleanup=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='c';return 1;};
    b.rules_frame=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='R';return 1;};
    b.trace_report=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='D';return 1;};
    b.frame_tail=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='F';return 1;};
    put(f.edict_actor,0x16c,1u);put(f.native_actor,0,sin_port::native_resupply::resupply_vtable);
    put(f.native_actor,0x378,0x2000u);put(f.native_actor,0xb8,0x400000u);
    sin_independent::EntityHandleEntry entries[1]{};entries[0].serial=2;entries[0].entity=reinterpret_cast<CBaseEntity*>(f.native_actor);
    ServerResupplyPrimaryThinkBindings rb{};rb.entities.entries=entries;rb.entities.count=1;b.resupply=&rb;
    Result out{};
    f.transition=true;
    assert(api->run(true,&state,&b,&out)&&out.completed&&out.transition&&f.sequence=="[]");
    f.transition=false;f.sequence.clear();
    assert(api->run(true,&state,&b,&out)&&out.completed&&out.double_step&&f.skip&&out.simulation_frametime==.25f&&out.cleanup_calls==3&&out.snapshot_count==3&&out.native_resupply==1&&out.resupply.no_think&&out.edict_calls==1&&out.foreign_thinks==1&&f.g.curtime==10&&f.g.frametime==.125f&&f.profile&&f.depth==0);
    assert(f.sequence=="[CSPMB(CET)s{Q}CcRDF]"&&out.scopes_entered==3&&out.scopes_closed==3);
    f.sequence.clear();assert(api->run(true,&state,&b,&out)&&out.skip_consumed&&!f.skip&&f.sequence=="[]");
    f.g.max_clients=3;f.sequence.clear();
    assert(api->run(false,&state,&b,&out)&&out.completed&&!out.double_step&&out.client_queries==2&&out.nulls==1&&out.foreign_thinks==1&&f.sequence=="[CSPMB(CT)s{Q}CcRDF]");
    f.g.max_clients=1;f.slots[1]=f.foreign;f.fail=true;f.sequence.clear();
    assert(!api->run(true,&state,&b,&out)&&!out.completed&&out.iteration_closed&&out.curtime_restored&&out.frametime_restored&&f.depth==0&&f.g.curtime==10&&f.g.frametime==.125f&&out.scopes_entered==out.scopes_closed&&!out.event_called);
    // Real edict-backed stationary resupply. Its +178 virtual is the
    // same 22223110 as dispenser/gas/canister, not a guessed shortcut.
    f.stationary_mode=true;f.fail=false;f.skip=false;f.double_step=0;
    f.g.tick=81;f.slots[0]=f.native_actor;f.slots[1]=nullptr;f.slots[2]=f.native_actor;
    f.order[0]=2;f.order[1]=0;f.order[2]=1;
    put(f.native_actor,0x16c,123u);put(f.native_actor,0xb0,80);put(f.native_actor,0x204,0);
    put(f.native_actor,0xf0,0xffffffffu);put(f.native_actor,0xb8,0u);put(f.native_actor,0x74,81);
    put(f.native_actor,8,0u); // original due null think
    rb.context=&f;rb.cadence.context=&f;
    rb.cadence.notify_think_flags=[](void*,void*) noexcept ->int{return 1;};
    rb.clock=[](void* p,float* v) noexcept ->int{*v=static_cast<Fixture*>(p)->g.curtime;return 1;};
    rb.record=[](void* p,void* a,int tick,float,int due) noexcept ->int{auto& q=*static_cast<Fixture*>(p);assert(a==q.native_actor&&tick==81&&due==81);++q.records;return 1;};
    rb.scope=[](void*,void*,bool) noexcept ->int{return 1;};
    b.edict_scope=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<Fixture*>(p);if(enter)++q.edict_enters;else ++q.edict_leaves;return 1;};
    assert(api->run(true,&state,&b,&out)&&out.completed&&out.stationary_calls==2&&out.stationary_deduplicated==1&&out.native_resupply==1&&out.resupply.null_token&&f.records==1&&out.edict_scopes_entered==2&&out.edict_scopes_closed==2&&out.nulls==1);
    // Named context and predictive player branches must not get lost.
    put(f.native_actor,0x50,1);f.slots[1]=nullptr;
    assert(api->run(true,&state,&b,&out)&&out.completed&&out.named_context_fallbacks==2&&out.edict_calls==2&&!out.native_resupply);
    put(f.native_actor,0x50,0);put(f.native_actor,0x388,static_cast<unsigned char>(1));
    assert(api->run(true,&state,&b,&out)&&out.completed&&out.edict_calls==2&&!out.stationary_calls);
    put(f.native_actor,0x16c,0u);put(f.native_actor,0x50,1);
    assert(api->run(true,&state,&b,&out)&&out.completed&&out.named_context_fallbacks==2&&out.foreign_thinks==2&&!out.native_resupply);
    // This is the actual ServerGameDLL003 slot4, not a parallel object.
    // The boundary is synchronous and one-shot; a reentrant bind refuses.
    const auto boundary=reinterpret_cast<InvokeBoundary>(GetProcAddress(server,boundary_export));
    assert(boundary&&!GetProcAddress(client,boundary_export));
    f.boundary=boundary;f.state=&state;f.bindings=&b;
    assert(!boundary(true,nullptr,&b,&out)&&!out.completed);
    f.transition=true;f.sequence.clear();
    assert(boundary(true,&state,&b,&out)&&out.completed&&out.transition&&f.sequence=="[]");
    f.transition=false;f.g.max_clients=1;f.skip=false;f.double_step=0;
    f.slots[0]=f.native_actor;f.slots[1]=nullptr;f.slots[2]=f.native_actor;
    put(f.native_actor,0x50,0);put(f.native_actor,0x388,static_cast<unsigned char>(0));put(f.native_actor,0x16c,123u);
    put(f.native_actor,0xb0,80);put(f.native_actor,0xb8,0x400000u);
    b.pre_systems=[](void* p) noexcept ->int{
        auto& q=*static_cast<Fixture*>(p);Result nested{};
        assert(!q.boundary(true,q.state,q.bindings,&nested)&&!nested.completed);++q.reentry_refused;return 1;
    };
    assert(boundary(true,&state,&b,&out)&&out.completed&&out.stationary_calls==2&&out.stationary_deduplicated==1&&out.native_resupply==1&&f.reentry_refused==1);
    // Check no invocation leaks across a failed synchronous call.
    b.pre_systems=[](void*) noexcept ->int{return 0;};
    assert(!boundary(true,&state,&b,&out)&&!out.completed&&out.frametime_restored);
    f.transition=true;
    assert(boundary(true,&state,&b,&out)&&out.completed&&out.transition);
    // Original CLogicAuto virtual Think override, via actual GameFrame slot4.
    namespace nm=sin_port::native_map;
    const auto get_map=reinterpret_cast<nm::GetApi>(GetProcAddress(server,nm::export_name));assert(get_map);
    const auto* map=get_map(1,sizeof(nm::Api));assert(map);
    struct AutoFixture {
        alignas(4) unsigned char actor[nm::auto_extent]{},actions[2][0x1c]{};
        Fixture* frame;
        int load=0;unsigned base=0,notifications=0,records=0,enters=0,leaves=0,violence=0,removes=0;
        float recorded=0;int tick=0,due=0;
        std::string outputs;
    } af{};af.frame=&f;
    put(af.actor,0,nm::auto_vtable);put(af.actor,0x378,0x3000u);put(af.actor,0x74,-1);
    put(af.actor,0xb8,0x400000u);put(af.actor,8,0xdeadbeefu); // not used by +bc
    put(af.actor,0x3f0,1u);put(af.actor,0x3a8,2u);
    for(unsigned i=0;i<2;++i){
        put(af.actions[i],0,201u);put(af.actions[i],4,202u);
        put(af.actions[i],0x10,0xffffffffu);put(af.actions[i],0x14,i+1);
    }
    sin_independent::EntityHandleEntry auto_entries[1]{};
    auto_entries[0].serial=3;auto_entries[0].entity=reinterpret_cast<CBaseEntity*>(af.actor);
    ServerEntityVoidOutputBindings auto_output{};auto_output.context=&af;
    auto_output.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
        auto& q=*static_cast<AutoFixture*>(p);if(token<1||token>2)return 0;*row=q.actions[token-1];*bytes=0x1c;return 1;
    };
    auto_output.enqueue=[](void* p,void* a,void* c,const ServerEntityVoidOutputAction* action) noexcept ->int{
        auto& q=*static_cast<AutoFixture*>(p);assert(!a&&c==q.actor&&q.frame->depth==1&&action->target==201&&action->input==202&&!action->variant_type);
        q.outputs+=char('0'+action->action_id);return 1;
    };
    nm::AutoBindings behavior{};behavior.context=&af;behavior.now=&f.g.curtime;behavior.load_type=&af.load;behavior.output=&auto_output;
    behavior.base_activate=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<AutoFixture*>(p);assert(a==q.actor);++q.base;return 1;};
    behavior.low_violence=[](void* p,bool* low) noexcept ->int{auto& q=*static_cast<AutoFixture*>(p);++q.violence;*low=false;return 1;};
    behavior.remove=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<AutoFixture*>(p);assert(a==q.actor);++q.removes;put(a,0xb8,0x400001u);return 1;};
    ServerNpcOuterCadenceBindings cadence{};cadence.context=&af;
    cadence.notify_think_flags=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<AutoFixture*>(p);assert(a==q.actor);++q.notifications;return 1;};
    nm::AutoPrimaryBindings ap{};ap.behavior=&behavior;ap.cadence=&cadence;ap.entries=auto_entries;ap.entry_count=1;ap.context=&af;
    ap.record=[](void* p,void* a,int tick,float time,int due) noexcept ->int{auto& q=*static_cast<AutoFixture*>(p);assert(a==q.actor);++q.records;q.recorded=time;q.tick=tick;q.due=due;return 1;};
    ap.scope=[](void* p,void* a,bool enter) noexcept ->int{auto& q=*static_cast<AutoFixture*>(p);assert(a==q.actor);if(enter)++q.enters;else ++q.leaves;return 1;};
    nm::AutoPrimaryResult apr{};
    f.transition=false;f.g.curtime=10.f;f.g.tick=80;f.skip=false;f.double_step=0;
    assert(map->auto_primary_activate(af.actor,sizeof(af.actor),f.g.interval,&ap,&apr)&&apr.scheduled&&apr.due_tick==82&&af.base==1&&af.notifications==1);
    b.logic_auto=&ap;b.pre_systems=[](void*) noexcept ->int{return 1;};
    b.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=nm::auto_extent;return 1;};
    b.event_service=[](void*) noexcept ->int{return 1;};
    f.slots[0]=af.actor;f.slots[1]=nullptr;f.slots[2]=nullptr;
    assert(boundary(true,&state,&b,&out)&&out.native_auto==1&&out.logic_auto.not_due&&!af.records&&af.outputs.empty());
    f.g.tick=82;f.g.curtime=10.2f;put(af.actor,0xb4,1u);
    assert(boundary(true,&state,&b,&out)&&out.native_auto==1&&out.logic_auto.invoked&&out.logic_auto.behavior.outputs==3&&out.logic_auto.behavior.queued==2&&af.outputs=="12"&&out.logic_auto.behavior.removed&&out.logic_auto.last_tick==82&&!out.logic_auto.native_result&&af.records==1&&af.recorded==10.2f&&af.due==82&&af.tick==82&&af.enters==1&&af.leaves==1&&af.notifications==2&&af.violence==1&&af.removes==1);
    assert(boundary(true,&state,&b,&out)&&out.native_auto==1&&out.logic_auto.no_think&&af.records==1&&af.outputs=="12");
    // Stale serial is rejected even before an idle check.
    auto_entries[0].serial=4;assert(!boundary(true,&state,&b,&out)&&out.iteration_closed&&out.curtime_restored);
    // Full native output chain: map-created actions -> scheduled auto ->
    // GameFrame event queue -> relay AcceptInput -> downstream and refire.
    struct CampaignFixture {
        alignas(4) unsigned char automatic[nm::auto_extent]{},relay[nm::relay_extent]{},target[4]{};
        alignas(4) unsigned char actions[2][0x1c]{},root[0x38]{},events[8][0x38]{};
        Fixture* frame=nullptr;
        const ServerEntityNamedEventQueueApi* api=nullptr;
        ServerEntityNamedEventQueueBindings queue{};
        unsigned allocated=0,action_id=0,action_releases=0,event_alloc=0,event_releases=0,delivered=0;
        unsigned notifications=0,records=0,base=0;
        bool locked=false;
        int load=0;
    } camp;
    camp.frame=&f;
    const auto queue_get=reinterpret_cast<GetServerEntityNamedEventQueueApi>(GetProcAddress(server,server_entity_named_event_queue_export));
    assert(queue_get);camp.api=queue_get(1,sizeof(ServerEntityNamedEventQueueApi));assert(camp.api);
    put(camp.automatic,0,nm::auto_vtable);put(camp.automatic,0x378,0x3000u);
    put(camp.automatic,0x74,-1);put(camp.automatic,0xb8,0x400000u);
    put(camp.relay,0,nm::relay_vtable);put(camp.relay,0x378,0x4001u);
    auto word=[](const void* a,unsigned off){unsigned v=0;std::memcpy(&v,static_cast<const unsigned char*>(a)+off,4);return v;};
    nm::OutputActionBindings actions{};actions.context=&camp;actions.next_id=&camp.action_id;
    actions.allocate=[](void* p,unsigned n,void** row) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);if(n!=0x1c||q.allocated==2)return 0;*row=q.actions[q.allocated++];return 1;
    };
    actions.release=[](void* p,void*) noexcept ->int{++static_cast<CampaignFixture*>(p)->action_releases;return 1;};
    nm::KeyBindings keys{};keys.context=&camp;keys.native_output=&actions;
    keys.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);*bytes=a==q.automatic?nm::auto_extent:a==q.relay?nm::relay_extent:0;return *bytes!=0;
    };
    keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
        const char* names[]{"startup_relay","Trigger","final","Use","EnableRefire"};
        for(unsigned i=0;i<5;++i)if(!std::strcmp(text,names[i])){*token=201+i;return 1;}return 0;
    };
    nm::KeyResult kr{};
    assert(map->key_value(camp.automatic,"OnNewGame","startup_relay,Trigger,,0,1",&keys,&kr)&&kr.output);
    assert(map->key_value(camp.relay,"OnTrigger","final,Use,,0.25,-1",&keys,&kr)&&kr.output);
    const unsigned auto_id=word(camp.actions[0],0x14),relay_id=word(camp.actions[1],0x14);
    camp.queue.context=&camp;
    camp.queue.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);if(!token||token>q.event_alloc)return 0;*row=q.events[token-1];*bytes=0x38;return 1;
    };
    camp.queue.allocate=[](void* p,unsigned* token,void** row,unsigned* bytes) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);if(q.event_alloc==8)return 0;*row=q.events[q.event_alloc++];*token=q.event_alloc;*bytes=0x38;return 1;
    };
    camp.queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token>=201&&token<=205;return 1;};
    ServerEntityVoidOutputBindings fire{};fire.context=&camp;
    fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);
        for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(q.actions[i]))){*row=q.actions[i];*bytes=0x1c;return 1;}return 0;
    };
    fire.enqueue=[](void* p,void* activator,void* caller,const ServerEntityVoidOutputAction* action) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);
        assert(!activator&&(caller==q.automatic||caller==q.relay));
        assert(caller==q.automatic?(!q.locked&&q.frame->depth==1):(q.locked&&!q.frame->depth));
        ServerEntityNamedEventQueueResult out{};
        return q.api->enqueue(q.root,sizeof(q.root),99,q.frame->g.curtime,0xffffffffu,caller==q.automatic?0x3000u:0x4001u,action,&q.queue,&out);
    };
    fire.release=[](void* p,unsigned token,void* row) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);assert(row==q.actions[0]&&token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(row)));++q.action_releases;return 1;
    };
    sin_independent::EntityHandleEntry ce[2]{};
    ce[0].serial=3;ce[0].entity=reinterpret_cast<CBaseEntity*>(camp.automatic);
    ce[1].serial=4;ce[1].entity=reinterpret_cast<CBaseEntity*>(camp.relay);
    nm::AutoBindings cb{};cb.context=&camp;cb.now=&f.g.curtime;cb.load_type=&camp.load;cb.output=&fire;
    cb.base_activate=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<CampaignFixture*>(p);assert(a==q.automatic);++q.base;return 1;};
    cb.low_violence=[](void*,bool* low) noexcept ->int{*low=false;return 1;};
    ServerNpcOuterCadenceBindings cc{};cc.context=&camp;
    cc.notify_think_flags=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<CampaignFixture*>(p);assert(a==q.automatic);++q.notifications;return 1;};
    nm::AutoPrimaryBindings cp{};cp.behavior=&cb;cp.cadence=&cc;cp.entries=ce;cp.entry_count=2;cp.context=&camp;
    cp.record=[](void* p,void* a,int tick,float time,int due) noexcept ->int{auto& q=*static_cast<CampaignFixture*>(p);assert(a==q.automatic&&tick==82&&due==82&&time==10.25f);++q.records;return 1;};
    cp.scope=[](void*,void*,bool) noexcept ->int{return 1;};
    nm::RelayQueueBindings rq{};rq.context=&camp;rq.root=camp.root;rq.root_bytes=sizeof(camp.root);rq.root_token=99;rq.queue=&camp.queue;
    rq.entries=ce;rq.entry_count=2;rq.now=&f.g.curtime;rq.intern=keys.intern;
    rq.handle=[](void* p,void* a,unsigned* handle) noexcept ->int{auto& q=*static_cast<CampaignFixture*>(p);assert(a==q.relay);*handle=0x4001;return 1;};
    nm::RelayBindings relay_bind{};relay_bind.context=&camp;relay_bind.output=&fire;relay_bind.native_queue=&rq;
    ServerNpcActivePostAnimationBindings storage{};
    ServerEntityAcceptInputBindings input{};input.storage=&storage;input.relay=&relay_bind;
    input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
        *text=token==202?"Trigger":token==205?"EnableRefire":nullptr;if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
    };
    ServerEntitySearchBindings search{};search.context=&camp;
    search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);*bytes=a==q.relay?nm::relay_extent:a==q.target?4u:0u;return *bytes!=0;
    };
    ServerEntityEventServiceBindings events{};events.context=&camp;events.entities.entries=ce;events.entities.count=2;events.resolve=camp.queue.resolve;
    events.native_search=&search;events.native_input=&input;
    events.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<CampaignFixture*>(p);assert(!q.frame->depth&&q.locked!=enter);q.locked=enter;return 1;};
    events.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** target) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);assert(q.locked&&!a&&(name==201||name==203));
        assert(c==(name==201?q.automatic:q.relay));*target=prev?nullptr:(name==201?q.relay:q.target);return 1;
    };
    events.accept=[](void* p,void* target,unsigned input,void* a,void* c,const void* variant,unsigned id) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);unsigned type=99,wanted=0;std::memcpy(&type,static_cast<const unsigned char*>(variant)+0x10,4);std::memcpy(&wanted,q.actions[1]+0x14,4);
        assert(q.locked&&target==q.target&&input==204&&!a&&c==q.relay&&!type&&id==wanted);++q.delivered;return 1;
    };
    events.release=[](void* p,unsigned token,void* row) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);assert(q.locked&&token&&token<=q.event_alloc&&row==q.events[token-1]);++q.event_releases;return 1;
    };
    rq.release=events.release;
    int steps=0;std::uint8_t single_step=0;EventQueue eq{camp.root,sizeof(camp.root),99,&steps,&single_step,&events};
    b.logic_auto=&cp;b.queue=&eq;f.slots[0]=camp.automatic;f.g.curtime=10.f;f.g.tick=80;
    assert(map->auto_primary_activate(camp.automatic,sizeof(camp.automatic),f.g.interval,&cp,&apr)&&apr.due_tick==82);
    assert(boundary(true,&state,&b,&out)&&out.logic_auto.not_due&&out.native_events&&!out.events.events&&!camp.event_alloc);
    f.g.tick=82;f.g.curtime=10.25f;
    assert(boundary(true,&state,&b,&out)&&out.logic_auto.invoked&&out.logic_auto.behavior.queued==1&&out.events.events==1&&out.events.native_inputs==1&&out.events.future_stopped&&camp.relay[0x3ad]&&camp.event_alloc==3&&camp.event_releases==1&&camp.action_releases==1&&!word(camp.automatic,0x3f0)&&word(camp.events[0],0x14)==auto_id&&word(camp.events[1],0x14)==relay_id&&!camp.delivered);
    f.g.tick=84;f.g.curtime=10.5f;
    assert(boundary(true,&state,&b,&out)&&out.logic_auto.no_think&&out.events.events==1&&out.events.foreign_inputs==1&&out.events.future_stopped&&camp.delivered==1&&camp.relay[0x3ad]);
    f.g.tick=85;f.g.curtime=10.625f;
    assert(boundary(true,&state,&b,&out)&&out.events.events==1&&out.events.native_inputs==1&&out.events.direct==1&&!camp.relay[0x3ad]&&!word(camp.root,0x30)&&camp.event_releases==3&&camp.records==1&&camp.notifications==2&&!camp.locked);
    // Native recurring timer through actual GameFrame and its own queue.
    // Reuse campaign pool with no surviving actions/events.
    std::memset(camp.automatic,0,sizeof(camp.automatic));
    std::memset(camp.actions,0,sizeof(camp.actions));std::memset(camp.events,0,sizeof(camp.events));
    camp.allocated=camp.action_id=camp.action_releases=camp.event_alloc=camp.event_releases=camp.delivered=0;
    camp.notifications=camp.records=0;
    put(camp.automatic,0,nm::timer_vtable);put(camp.automatic,0x378,0x3000u);
    put(camp.automatic,0x74,-1);put(camp.automatic,0xb8,0x400000u);
    put(camp.automatic,0x3e0,0x3e800000u); // .25 seconds
    assert(map->key_value(camp.automatic,"OnTimer","final,Use,,0,-1",&keys,&kr)&&kr.output);
    camp.queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{
        *yes=(token>=201&&token<=205)||token==0x22420090||token==0x223edce8||token==0x223edd00;return 1;
    };
    fire.enqueue=[](void* p,void* a,void* c,const ServerEntityVoidOutputAction* action) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);assert(a==q.automatic&&c==q.automatic&&q.frame->depth==1&&!q.locked);
        ServerEntityNamedEventQueueResult out{};
        return q.api->enqueue(q.root,sizeof(q.root),99,q.frame->g.curtime,0x3000u,0x3000u,action,&q.queue,&out);
    };
    nm::TimerBindings timer_behavior{};timer_behavior.context=&camp;timer_behavior.output=&fire;
    timer_behavior.now=&f.g.curtime;timer_behavior.cadence=&cc;timer_behavior.entries=ce;timer_behavior.entry_count=2;
    timer_behavior.tick_interval=&f.g.interval;
    nm::TimerPrimaryBindings timer_primary{};timer_primary.behavior=&timer_behavior;timer_primary.context=&camp;
    timer_primary.record=[](void* p,void* a,int tick,float time,int due) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);assert(a==q.automatic&&tick==due&&time==q.frame->g.curtime);++q.records;return 1;
    };
    timer_primary.scope=[](void*,void*,bool) noexcept ->int{return 1;};
    input.relay=nullptr;input.timer=&timer_behavior;
    input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
        *text=token==0x22420090?"RefireTime":token==0x223edd00?"Enable":token==0x223edce8?"Disable":nullptr;
        if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
    };
    search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);*bytes=a==q.automatic?nm::timer_extent:a==q.target?4u:0u;return *bytes!=0;
    };
    events.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** target) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);assert(q.locked&&name==203&&a==q.automatic&&c==q.automatic);*target=prev?nullptr:q.target;return 1;
    };
    events.accept=[](void* p,void* target,unsigned input,void* a,void* c,const void* variant,unsigned id) noexcept ->int{
        auto& q=*static_cast<CampaignFixture*>(p);unsigned type=99,wanted=0;std::memcpy(&type,static_cast<const unsigned char*>(variant)+0x10,4);std::memcpy(&wanted,q.actions[0]+0x14,4);
        assert(q.locked&&target==q.target&&input==204&&a==q.automatic&&c==q.automatic&&!type&&id==wanted);++q.delivered;return 1;
    };
    b.logic_auto=nullptr;b.logic_timer=&timer_primary;f.slots[0]=camp.automatic;
    f.g.curtime=20.f;f.g.tick=160;
    nm::TimerResult timer_spawn_result{};
    assert(map->timer_spawn(camp.automatic,sizeof(camp.automatic),&timer_behavior,&timer_spawn_result)&&word(camp.automatic,0x74)==162);
    // Deliberately stale owner interval; both frame actor and event paths
    // must override it with the authoritative .125-second live interval.
    const float stale_interval=.25f;timer_behavior.tick_interval=&stale_interval;
    auto enqueue_timer=[&](unsigned input_token,unsigned type,unsigned value,float delay){
        unsigned variant[5]{value,0,0,0xffffffffu,type};ServerEntityNamedEventQueueResult out{};
        return camp.api->enqueue_direct(camp.root,sizeof(camp.root),99,f.g.curtime,0x3000u,input_token,0x3000u,0x3000u,variant,sizeof(variant),delay,0,&camp.queue,&out);
    };
    assert(enqueue_timer(0x22420090,1,0x3f000000u,.125f)); // RefireTime .5
    assert(boundary(true,&state,&b,&out)&&out.logic_timer.not_due&&out.events.future_stopped);
    f.g.curtime=20.125f;f.g.tick=161;
    assert(boundary(true,&state,&b,&out)&&out.logic_timer.not_due&&out.events.native_inputs==1&&word(camp.automatic,0x74)==165);
    f.g.curtime=20.25f;f.g.tick=162;
    assert(boundary(true,&state,&b,&out)&&out.logic_timer.not_due&&!camp.delivered);
    f.g.curtime=20.625f;f.g.tick=165;
    assert(boundary(true,&state,&b,&out)&&out.native_timer==1&&out.logic_timer.invoked&&out.logic_timer.next_tick==169&&out.events.foreign_inputs==1&&camp.delivered==1);
    assert(enqueue_timer(0x223edce8,0,0,.125f));
    f.g.curtime=20.75f;f.g.tick=166;
    assert(boundary(true,&state,&b,&out)&&out.logic_timer.not_due&&out.events.native_inputs==1&&word(camp.automatic,0x74)==0xffffffffu);
    f.g.curtime=21.125f;f.g.tick=169;
    assert(boundary(true,&state,&b,&out)&&out.logic_timer.no_think&&camp.delivered==1);
    assert(enqueue_timer(0x223edd00,0,0,0));
    assert(boundary(true,&state,&b,&out)&&out.logic_timer.no_think&&out.events.native_inputs==1&&word(camp.automatic,0x74)==173);
    f.g.curtime=21.625f;f.g.tick=173;
    assert(boundary(true,&state,&b,&out)&&out.logic_timer.invoked&&out.logic_timer.next_tick==177&&out.events.foreign_inputs==1&&camp.delivered==2&&camp.records==2&&camp.event_alloc==5&&camp.event_releases==5&&!word(camp.root,0x30));
// Real GameFrame slot4: filter event at live frame time, then nested
    // output delivery. Reuse the campaign pool and queue without actor
    // simulation; filters are ordinary non-thinking entity storage.
    {
        const auto frame_filters=reinterpret_cast<InvokeFilterBoundary>(GetProcAddress(server,filter_boundary_export));
        assert(frame_filters&&!GetProcAddress(client,filter_boundary_export));
        struct FilterFrame {
            alignas(4) unsigned char filter[0x3cc]{},player[0x37c]{};
            CampaignFixture* campaign;
        } ff{};ff.campaign=&camp;
        put(ff.filter,0,nm::name_filter_vtable);put(ff.filter,0x378,0x5000u);put(ff.filter,0xc0,201u);
        put(ff.filter,0x3c8,206u);put(ff.player,0,0x224aec3cu);put(ff.player,0x378,0x6001u);put(ff.player,0xc0,206u);
        put(camp.relay,0,nm::relay_vtable);put(camp.relay,0x378,0x7002u);put(camp.relay,0xc0,203u);camp.relay[0x3ac]=1;
        std::memset(camp.root,0,sizeof(camp.root));std::memset(camp.events,0,sizeof(camp.events));std::memset(camp.actions,0,sizeof(camp.actions));
        camp.event_alloc=camp.event_releases=0;
        for(unsigned i=0;i<2;++i){
            put(camp.actions[i],0,203u);put(camp.actions[i],4,204u+i);put(camp.actions[i],0x10,0xffffffffu);
            put(ff.filter,i?0x3c4:0x3ac,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(camp.actions[i])));
        }
        sin_independent::EntityHandleEntry fe[3]{};
        void* fa[]{ff.filter,ff.player,camp.relay};
        for(unsigned i=0;i<3;++i){
            fe[i].serial=5+i;fe[i].entity=reinterpret_cast<CBaseEntity*>(fa[i]);
            if(i)fe[i].previous=&fe[i-1];if(i<2)fe[i].next=&fe[i+1];
        }
        auto extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
            auto& x=*static_cast<FilterFrame*>(p);
            *bytes=a==x.filter?sizeof(x.filter):a==x.player?sizeof(x.player):a==x.campaign->relay?nm::relay_extent:0;return *bytes!=0;
        };
        auto strings=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            constexpr const char* names[]{"filter","TestActivator","relay","Enable","Disable","hero"};
            if(token<201||token>206)return 0;*text=names[token-201];*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        auto filter_search=search;filter_search.context=&ff;filter_search.head=fe;filter_search.extent=extent;filter_search.string_text=strings;
        auto filter_input=input;filter_input.timer=nullptr;filter_input.relay=&relay_bind;filter_input.string_text=strings;
        auto filter_events=events;filter_events.entities.entries=fe;filter_events.entities.count=3;filter_events.native_search=&filter_search;
        filter_events.native_input=&filter_input;filter_events.find_named=nullptr;filter_events.accept=nullptr;
        auto filter_eq=eq;filter_eq.bindings=&filter_events;
        camp.queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token>=201&&token<=206;return 1;};
        auto filter_route=rq;filter_route.context=&ff;filter_route.entries=nullptr;filter_route.entry_count=0;filter_route.now=nullptr;
        filter_route.handle=[](void* p,void* a,unsigned* handle) noexcept ->int{
            auto& x=*static_cast<FilterFrame*>(p);
            if(a==x.filter)*handle=0x5000;else if(a==x.player)*handle=0x6001;else if(a==x.campaign->relay)*handle=0x7002;else return 0;return 1;
        };
        auto filter_fire=fire;filter_fire.enqueue=nullptr;
        nm::CampaignCustomFilterBindings predicate{};predicate.context=&ff;predicate.extent=extent;predicate.string_text=strings;
        nm::CampaignFilterInputBindings fb{&predicate,&filter_fire,&filter_route};
        auto frame=b;frame.logic_timer=nullptr;frame.logic_auto=nullptr;frame.queue=&filter_eq;
        frame.active=[](void*,ActiveList* list) noexcept ->int{*list={};return 1;};
        f.g.curtime=30.f;f.g.tick=240;single_step=0;
        ServerEntityVoidOutputAction a{201,202,0,0,99,.25f};
        ServerEntityNamedEventQueueResult qr{};CampaignFilterServiceSummary summary{};
        assert(camp.api->enqueue(camp.root,sizeof(camp.root),99,f.g.curtime,0x6001,0x6001,&a,&camp.queue,&qr));
        assert(frame_filters(true,&state,&frame,&out,&fb,&summary)&&out.events.future_stopped&&!summary.attempted&&camp.relay[0x3ac]);
        // Skip frame does not consume events, and neither does a
        // single-step stop before the nested output can be dispatched.
        f.g.curtime=30.25f;f.g.tick=242;f.skip=true;
        assert(frame_filters(true,&state,&frame,&out,&fb,&summary)&&out.skip_consumed&&!summary.attempted&&!camp.event_releases);
        steps=1;single_step=1;
        assert(frame_filters(true,&state,&frame,&out,&fb,&summary)&&out.events.step_stopped&&summary.passed==1&&summary.queued==1&&camp.event_releases==1&&camp.relay[0x3ac]);
        assert(word(camp.events[1],0xc)==0x6001&&word(camp.events[1],0x10)==0x5000);
        single_step=0;
        assert(frame_filters(true,&state,&frame,&out,&fb,&summary)&&out.events.native_inputs==1&&!camp.relay[0x3ac]&&camp.event_releases==2&&!word(camp.root,0x30));
        ff.filter[0x394]=1;a.delay=0.f;
        assert(camp.api->enqueue(camp.root,sizeof(camp.root),99,f.g.curtime,0x6001,0x6001,&a,&camp.queue,&qr));
        assert(frame_filters(true,&state,&frame,&out,&fb,&summary)&&out.events.native_inputs==2&&summary.failed==1&&camp.relay[0x3ac]&&camp.event_releases==4&&out.cleanup_calls==3&&!camp.locked);
    }
    // Real GameFrame: event stage schedules default think; the actor
    // stage on a later frame retries and acquires through native registry.
    {
        const auto frame_sequences=reinterpret_cast<InvokeSequenceBoundary>(GetProcAddress(server,sequence_boundary_export));
        assert(frame_sequences&&!GetProcAddress(client,sequence_boundary_export));
        struct SequenceFrameFixture {
            alignas(4) unsigned char seq[nm::scripted_sequence_extent]{},npc[0x974]{};
            sin_independent::EntityHandleEntry entries[2]{};
            Fixture* frame=nullptr;CampaignFixture* campaign=nullptr;
            unsigned records=0,enters=0,leaves=0,notifications=0,flags=0,reports=0,cancels=0;
            unsigned char rules=0;
            void* removal_slots[1]{};nm::DeferredRemovalQueue removal_queue{removal_slots,1};
            unsigned updates=0,releases=0;
            float stale_now=-100.f,stale_interval=.5f;
        } sf;
        sf.frame=&f;sf.campaign=&camp;
        put(sf.seq,0,nm::scripted_sequence_vtable);put(sf.seq,0x378,0x8000u);put(sf.seq,0xc,0x2242ee1cu);
        put(sf.seq,0xc0,302u);put(sf.seq,0x3a8,301u);put(sf.seq,0x3d0,0xffffffffu);put(sf.seq,0x3d4,0xffffffffu);put(sf.seq,0x3dc,0xffffffffu);
        put(sf.seq,0xb8,0x400000u);put(sf.seq,0x74,-1);
        put(sf.npc,0,0x224954a4u);put(sf.npc,0x378,0x9001u);put(sf.npc,0xc0,301u);put(sf.npc,0x970,0xffffffffu);
        sf.entries[0].entity=reinterpret_cast<CBaseEntity*>(sf.seq);sf.entries[0].serial=8;sf.entries[0].next=&sf.entries[1];
        sf.entries[1].entity=reinterpret_cast<CBaseEntity*>(sf.npc);sf.entries[1].serial=9;sf.entries[1].previous=&sf.entries[0];
        auto extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
            auto& sf=*static_cast<SequenceFrameFixture*>(p);*bytes=a==sf.seq?sizeof(sf.seq):a==sf.npc?sizeof(sf.npc):0;return *bytes!=0;
        };
        auto strings=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==301?"target":token==302?"sequence":token==303?"BeginSequence":token==304?"CancelSequence":token==0x2242ee1c?"scripted_sequence":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        ServerEntitySearchBindings seq_search{};seq_search.context=&sf;seq_search.extent=extent;seq_search.head=sf.entries;seq_search.string_text=strings;
        auto seq_input=input;seq_input.timer=nullptr;seq_input.string_text=strings;
        std::memset(camp.root,0,sizeof(camp.root));std::memset(camp.events,0,sizeof(camp.events));camp.event_alloc=camp.event_releases=0;
        const auto saved_nonempty=camp.queue.string_nonempty;
        camp.queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token>=301&&token<=304;return 1;};
        auto seq_events=events;seq_events.entities.entries=sf.entries;seq_events.entities.count=2;seq_events.native_input=&seq_input;
        seq_events.native_search=&seq_search;seq_events.find_named=nullptr;seq_events.accept=nullptr;
        auto seq_queue=eq;seq_queue.bindings=&seq_events;
        ServerNpcOuterCadenceBindings cadence{};cadence.context=&sf;
        cadence.notify_think_flags=[](void* p,void* a) noexcept ->int{
            auto& sf=*static_cast<SequenceFrameFixture*>(p);assert(a==sf.seq);++sf.notifications;return 1;
        };
        nm::ScriptedSequenceControlBindings c{};c.context=&sf;c.extent=extent;c.now=&sf.stale_now;c.interval=&sf.stale_interval;c.cadence=&cadence;
        c.cancel_warning=[](void*,unsigned) noexcept ->int{return 1;};
        nm::ScriptedSequencePossessionBindings pos{};pos.context=&sf;pos.extent=extent;
        pos.add_flags=[](void* p,void* a,unsigned) noexcept ->int{
            auto& sf=*static_cast<SequenceFrameFixture*>(p);assert(a==sf.npc&&sf.frame->depth==1&&!sf.campaign->locked);++sf.flags;return 1;
        };
        nm::ScriptedSequenceCancelBindings cancel{};cancel.context=&sf;cancel.control=&c;
        cancel.restore_flags=[](void* p,void* a,unsigned) noexcept ->int{
            auto& sf=*static_cast<SequenceFrameFixture*>(p);assert(a==sf.npc&&!sf.frame->depth&&sf.campaign->locked);++sf.cancels;return 1;
        };
        nm::ScriptedSequenceAcquireBindings acquire{&c,&seq_search,&sf};
        nm::ScriptedSequenceThinkBindings think{&acquire,&pos,&cancel,&sf.rules,&sf};
        think.report=[](void* p,void* seq,unsigned stage,void* npc,unsigned) noexcept ->int{
            auto& sf=*static_cast<SequenceFrameFixture*>(p);assert(seq==sf.seq&&stage==1&&npc==sf.npc&&sf.frame->depth==1);++sf.reports;return 1;
        };
        nm::ScriptedSequencePrimaryBindings primary{&think,&sf};
        primary.record=[](void* p,void* a,int tick,float time,int due) noexcept ->int{
            auto& sf=*static_cast<SequenceFrameFixture*>(p);assert(a==sf.seq&&sf.frame->depth==1&&tick==due&&time==sf.frame->g.curtime);++sf.records;return 1;
        };
        primary.scope=[](void* p,void* a,bool enter) noexcept ->int{
            auto& sf=*static_cast<SequenceFrameFixture*>(p);assert(a==sf.seq&&sf.frame->depth==1);if(enter)++sf.enters;else ++sf.leaves;return 1;
        };
        nm::ScriptedSequenceNativeControlBindings route{&c,&pos,&cancel};
        ScriptedSequenceFrameBindings seq_frame{&primary,&route,nullptr};
        ScriptedSequenceFrameResult sr{};
        auto frame=b;frame.logic_timer=nullptr;frame.logic_auto=nullptr;frame.queue=&seq_queue;
        // The shared fixture normally visits slots 2,0,1. Preserve it:
        // leaving index zero here would visit the later logic_auto twice.
        const auto saved_index=f.order[0];
        void* const saved_slot=f.slots[0];
        f.slots[0]=sf.seq;f.order[0]=0;
        frame.active=[](void* p,ActiveList* list) noexcept ->int{
            auto& f=*static_cast<Fixture*>(p);assert(f.depth==1);*list={f.order,1,4,f.slots,3};return 1;
        };
        frame.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Fixture*>(p);if(a!=f.slots[0])return 0;*bytes=nm::scripted_sequence_extent;return 1;
        };
        auto tick=[&](){return frame_sequences(true,&state,&frame,&out,&seq_frame,&sr);};
        f.g.curtime=40.f;f.g.tick=320;f.skip=false;single_step=0;
        ServerEntityVoidOutputAction begin{302,303,0,0,99,.125f};ServerEntityNamedEventQueueResult qr{};
        assert(camp.api->enqueue(camp.root,sizeof(camp.root),99,f.g.curtime,0x9001u,0x9001u,&begin,&camp.queue,&qr));
        assert(tick()&&sr.primary_calls==1&&sr.last.no_think&&out.events.future_stopped&&!sr.events.attempted);
        f.g.curtime=40.125f;f.g.tick=321;
        assert(tick()&&sr.last.no_think&&sr.events.scheduled==1&&word(sf.seq,0x74)==321&&sf.notifications==1&&!sr.invoked);
        // Same engine tick: original actor stage runs before event stage.
        assert(tick()&&sr.invoked==1&&sr.retries==1&&sr.last.behavior.rules_blocked&&word(sf.seq,0x74)==329&&sf.notifications==3);
        f.g.curtime=41.f;f.g.tick=328;sf.rules=1;
        assert(tick()&&sr.last.not_due&&!sr.invoked);
        f.g.curtime=41.125f;f.g.tick=329;
        assert(tick()&&sr.invoked==1&&sr.acquired==1&&!sr.retries&&word(sf.npc,0x970)==0x8000u&&word(sf.seq,0x74)==0xffffffffu&&sf.flags==1&&sf.reports==1);
        assert(sf.records==2&&sf.enters==2&&sf.leaves==2&&sf.notifications==4);
        unsigned empty[5]{0,0,0,0xffffffffu,0};
        assert(camp.api->enqueue_direct(camp.root,sizeof(camp.root),99,f.g.curtime,0x8000u,304,0x9001u,0x9001u,empty,sizeof(empty),0.f,99,&camp.queue,&qr));
        assert(tick()&&sr.last.no_think&&sr.events.cancelled==1&&sf.cancels==1&&word(sf.npc,0x970)==0xffffffffu&&word(sf.seq,0x3d0)==0xffffffffu&&camp.event_releases==2&&out.cleanup_calls==3&&!camp.locked&&!f.depth);
        // Real slot 4 executes scheduled SUB_Remove inside the actor
        // scope and releases the marked sequence only after event service.
        const auto frame_remove=reinterpret_cast<InvokeSequenceRemovalBoundary>(GetProcAddress(server,sequence_removal_boundary_export));
        assert(frame_remove&&!GetProcAddress(client,sequence_removal_boundary_export));
        nm::DeferredRemovalBindings deferred{};deferred.context=&sf;deferred.queue=&sf.removal_queue;
        deferred.update=[](void* p,void* a,unsigned handler) noexcept ->int{
            auto& sf=*static_cast<SequenceFrameFixture*>(p);
            assert(a==sf.seq&&handler==0x221446f0u&&sf.frame->depth==1&&!sf.campaign->locked&&sf.seq[0xb8]&1u);
            ++sf.updates;return 1;
        };
        deferred.release=[](void* p,void* net) noexcept ->int{
            auto& sf=*static_cast<SequenceFrameFixture*>(p);
            assert(net==sf.seq+0x160&&!sf.frame->depth&&!sf.campaign->locked&&sf.removal_queue.handle_access_disabled&&sf.updates==1);
            ++sf.releases;sf.frame->slots[0]=nullptr;sf.entries[0].entity=nullptr;return 1;
        };
        nm::ScriptedSequenceRemovalThinkBindings removal{&deferred,&cancel,&sf};
        ScriptedSequenceFrameRemovalResult removed{};
        frame.deferred=&deferred;
        put(sf.seq,0x160,0x22454f00u);put(sf.seq,0x168,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(sf.seq)));
        put(sf.seq,8,0x221c0d40u);put(sf.seq,0x74,330);put(sf.seq,0xb8,0u);
        auto removal_tick=[&](){return frame_remove(true,&state,&frame,&out,&seq_frame,&sr,&removal,&removed);};
        auto wrong=removal;wrong.deferred=nullptr;
        assert(!frame_remove(true,&state,&frame,&out,&seq_frame,&sr,&wrong,&removed)&&!sf.updates&&!sf.releases&&!f.depth);
        assert(removal_tick()&&removed.completed&&!removed.removal_thinks&&sr.last.not_due&&!sf.updates&&!sf.releases);
        f.g.curtime=41.25f;f.g.tick=330;
        assert(removal_tick()&&removed.completed&&removed.removal_thinks==1&&removed.queued==1&&removed.last.cancelled.missing&&removed.last.cancelled.count_reset&&sr.invoked==1&&!sr.last.native_result&&out.native_cleanup_calls==3&&out.native_released==1&&sf.updates==1&&sf.releases==1&&!sf.removal_queue.count&&!f.depth);
        // Original owner bindings were never modified by scoped rebinding.
        assert(removal.cancel==&cancel&&removal.deferred==&deferred);
        assert(!c.entries&&!pos.entries&&c.now==&sf.stale_now&&c.interval==&sf.stale_interval&&sf.stale_now==-100.f);
        f.order[0]=saved_index;f.slots[0]=saved_slot;
        camp.queue.string_nonempty=saved_nonempty;
    }
#include "test_native_game_frame_deferred.inc"
    FreeLibrary(server);FreeLibrary(client);
}
