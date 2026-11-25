#include "native_client_overlay_events.h"
#include "native_weapon_datatables.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace sin_port::native_weapon_tables;
struct Fixture {
    alignas(4) unsigned char actor[0x808]{},layers[3*0x20]{};
    ClientOverlayEvent events[9]{
        {0.f,5000,0,"start",0},{.125f,4999,0,"legacy excluded",0},
        {.25f,42,0x410,"new client",0},{.375f,6000,0x400,"new server",0},
        {.5f,5001,0,"mid",0},{.625f,5002,0,"later",0},
        {.75f,5003,0,"boundary",0},{.875f,43,0x410,"tail",0},{1.f,5004,0,"end",0}};
    std::vector<int> trace;
    unsigned queries=0;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    void row(unsigned i,int seq,float cycle,float baseline,int order=0){
        put(layers+i*0x20,0,seq);put(layers+i*0x20,0xc,order);put(layers+i*0x20,0x14,cycle);put(actor,0x7cc+i*4,baseline);
    }
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetClientOverlayEventsApi>(GetProcAddress(client,client_overlay_events_export));
    assert(get&&!GetProcAddress(server,client_overlay_events_export)&&!get(0,sizeof(ClientOverlayEventsApi))&&!get(1,0));
    const auto* api=get(1,sizeof(ClientOverlayEventsApi));assert(api);
    Fixture f;
    f.put(f.actor,0x7a4,LayerVector{f.layers,3,0,3,f.layers});
    ClientOverlayEventBindings b{&f,[](void* p,void* actor,int seq,ClientOverlayEventView* view) noexcept -> int {
        auto& f=*static_cast<Fixture*>(p);assert(actor==f.actor&&seq>=0&&seq<3);++f.queries;
        if(seq==2){*view={};return 1;}*view={f.events,9};return 1;
    },[](void* p,void* actor,unsigned layer,const ClientOverlayEvent* e) noexcept {
        auto& f=*static_cast<Fixture*>(p);assert(actor==f.actor&&layer<3&&e>=f.events&&e<f.events+9);
        // Trace layer and event separately; callback sees old baseline.
        f.trace.push_back(static_cast<int>(layer)*10000+e->event);
        if(layer==0&&e->event==5000)assert(f.get<float>(f.actor,0x7cc)==-.01f);
    }};
    ClientOverlayEventResult o{};
    f.row(0,0,.5f,-.01f,2);f.row(1,1,.25f,0.f,0);f.row(2,2,.5f,.125f,1);
    assert(api->dispatch(f.actor,sizeof(f.actor),0,3,&b,&o)&&!o.visited&&!f.queries);
    assert(api->dispatch(f.actor,sizeof(f.actor),.016f,3,&b,&o)&&o.visited==3&&o.emitted==4&&!o.wrapped&&o.aborted_slot==-1);
    assert((f.trace==std::vector<int>{5000,42,5001,10042}));
    assert(f.get<float>(f.actor,0x7cc)==.5f&&f.get<float>(f.actor,0x7d0)==.25f&&f.get<float>(f.actor,0x7d4)==.125f);
    f.trace.clear();
    assert(api->dispatch(f.actor,sizeof(f.actor),.016f,3,&b,&o)&&!o.emitted&&f.trace.empty());
    // Raw index 0 wraps despite order=2 and weight=0.
    f.row(0,0,.125f,.75f,2);f.row(1,1,.5f,.25f,0);
    assert(api->dispatch(f.actor,sizeof(f.actor),.016f,3,&b,&o)&&o.wrapped==1&&o.emitted==4);
    assert((f.trace==std::vector<int>{43,5004,5000,15001})); // .75 boundary excluded
    f.trace.clear();const auto q=f.queries;
    f.row(0,0,.25f,.75f);f.row(1,1,.875f,.5f); // exact .5 backwards -> abort all
    assert(api->dispatch(f.actor,sizeof(f.actor),.016f,3,&b,&o)&&o.aborted_slot==0&&o.visited==1&&!o.emitted&&f.queries==q+1);
    assert(f.get<float>(f.actor,0x7cc)==.75f&&f.get<float>(f.actor,0x7d0)==.5f);
    f.row(0,3,.1f,.9f); // upper-invalid sequence skipped
    assert(api->dispatch(f.actor,sizeof(f.actor),.016f,3,&b,&o)&&o.visited==3&&o.emitted==3&&o.aborted_slot==-1);
    assert((f.trace==std::vector<int>{15002,15003,10043}));
    f.row(0,-1,.1f,.9f);
    assert(!api->dispatch(f.actor,sizeof(f.actor),.016f,3,&b,&o)&&o.failed_slot==0);
    assert(!api->dispatch(f.actor,0x7cc,.016f,3,&b,&o));
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS native overlay event client filtering, interval bounds, wrap and whole-pass abort");
}
