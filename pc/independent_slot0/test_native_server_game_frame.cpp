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
    FreeLibrary(server);FreeLibrary(client);
}
