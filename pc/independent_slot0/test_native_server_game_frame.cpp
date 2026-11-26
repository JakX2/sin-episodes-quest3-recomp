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
    bool fail=false;
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
    b.scope=[](void* p,const char* n,bool enter) noexcept ->int{auto& q=*static_cast<Fixture*>(p);if(!std::strcmp(n,"CServerGameDLL::GameFrame"))q.sequence+=enter?'[':']';else if(!std::strcmp(n,"Physics_RunThinkFunctions"))q.sequence+=enter?'(':')';else{assert(!std::strcmp(n,"ServiceEventQueue()"));q.sequence+=enter?'{':'}';}return 1;};
    b.cleanup=[](void* p) noexcept ->int{auto& q=*static_cast<Fixture*>(p);++q.cleanup;q.sequence+='C';return 1;};
    b.pre_systems=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='S';return 1;};
    b.pre_actors=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='P';return 1;};
    b.manager=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='M';return 1;};
    b.engine_begin=[](void* p) noexcept ->int{static_cast<Fixture*>(p)->sequence+='B';return 1;};
    b.active=[](void* p,ActiveList* l) noexcept ->int{auto& q=*static_cast<Fixture*>(p);assert(q.depth==1);*l={q.order,3,4,q.slots,3};return 1;};
    b.extent=[](void*,void*,unsigned* n) noexcept ->int{*n=0x3b0;return 1;};
    b.edict_simulate=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<Fixture*>(p);assert(a==q.edict_actor&&q.g.curtime==10&&q.depth==1);++q.edict;q.sequence+='E';q.g.curtime=999;q.slots[1]=nullptr;return 1;};
    b.other_think=[](void* p,void* a) noexcept ->int{auto& q=*static_cast<Fixture*>(p);assert(a==q.foreign&&q.g.curtime==10);++q.other;q.sequence+='T';q.g.curtime=888;return !q.fail;};
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
    FreeLibrary(server);FreeLibrary(client);
}
