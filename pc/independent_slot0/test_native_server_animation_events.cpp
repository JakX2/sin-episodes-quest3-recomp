#include "native_server_animation_events.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0x4a8]{},receiver[4]{},rows[3*0x40]{};
    ServerStudioEvent events[7]{{0.f,10,0,"start",0},{.125f,5000,0,"client legacy",0},{.25f,20,0x400,"server",0},{.375f,21,0x410,"client",0},{.5f,22,0,"middle",0},{.875f,23,0,"tail",0},{1.f,24,0,"end",0}};
    std::vector<int> ids;std::vector<float> times;std::vector<int> layers;unsigned queries=0;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
};
static bool close_float(float a,float b){return std::fabs(a-b)<.00001f;}
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerAnimationEventsApi>(GetProcAddress(server,server_animation_events_export));
    assert(get&&!GetProcAddress(client,server_animation_events_export)&&!get(0,sizeof(ServerAnimationEventsApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerAnimationEventsApi));assert(api);
    F f;ServerAnimationEventSequence view{f.events,7,1,.1f};ServerAnimationEvent event{};
    assert(api->next(&view,.25f,.5f,0,2,&event)==3&&event.event==20&&event.time==2&&!event.source&&event.options==f.events[2].options);
    assert(api->next(&view,.25f,.5f,3,2,&event)==0);
    assert(api->next(&view,.875f,.25f,0,2,&event)==1&&event.event==10);
    assert(api->next(&view,.875f,.25f,1,2,&event)==6&&event.event==23);
    assert(api->next(&view,.875f,.25f,6,2,&event)==7&&event.event==24);
    view.flags=0;assert(api->next(&view,.875f,.25f,0,2,&event)==0);
    f.put(f.actor,0x28,1.8f);f.put(f.actor,0x2c,2.f);f.put(f.actor,0x3c0,1.f);
    f.put(f.actor,0x3ec,7);f.put(f.actor,0x3e8,.5f);f.put(f.actor,0x398,.25f);
    f.put(f.actor,0x494,0x1000u);f.put(f.actor,0x498,3);f.put(f.actor,0x4a0,3);
    auto* a=f.rows;auto* c=f.rows+0x80;
    f.put(a,0,1u);f.put(a,5,std::uint8_t{1});f.put(a,8,8);f.put(a,0xc,.25f);f.put(a,0x14,1.f);f.put(a,0x38,.875f);
    f.put(c,0,1u);f.put(c,8,9);f.put(c,0xc,.95f);f.put(c,0x14,1.f);f.put(c,0x38,.875f);
    ServerAnimationEventBindings b{&f,
        [](void* p,void* owner,int seq,ServerAnimationEventSequence* v) noexcept -> int {
            auto& f=*static_cast<F*>(p);assert((seq==7&&owner==f.actor)||((seq==8||seq==9)&&owner==f.receiver));
            ++f.queries;*v={f.events,7,seq==8?1u:0u,.1f};return 1;},
        [](void* p,void* owner,int seq,double* v) noexcept -> int {
            auto& f=*static_cast<F*>(p);assert(owner==(seq==7?static_cast<void*>(f.actor):static_cast<void*>(f.receiver)));*v=1.;return 1;},
        [](void* p,std::uint32_t word,void** out,unsigned* n) noexcept -> int {
            auto& f=*static_cast<F*>(p);assert(word==0x1000);*out=f.rows;*n=sizeof(f.rows);return 1;},
        [](void* p,void* receiver,int layer,const ServerAnimationEvent* e) noexcept {
            auto& f=*static_cast<F*>(p);assert(receiver==f.receiver&&e->source==f.actor);f.ids.push_back(e->event);f.layers.push_back(layer);f.times.push_back(e->time);}
    };
    ServerAnimationEventResult out{};
    assert(api->dispatch(f.actor,sizeof(f.actor),f.receiver,2.1f,&b,&out)&&out.layers==2&&out.emitted==5&&out.failed_layer==-2);
    assert((f.ids==std::vector<int>{20,10,23,24,23}));
    assert((f.layers==std::vector<int>{-1,0,0,0,2}));
    assert(close_float(f.times[0],1.85f)&&close_float(f.times[1],1.85f)&&close_float(f.times[2],1.725f)&&close_float(f.times[3],1.85f)&&close_float(f.times[4],2.025f));
    assert(f.get<float>(f.actor,0x398)==.5f&&f.get<float>(a,0x38)==.25f&&f.get<float>(c,0x38)==1.f&&f.get<std::uint8_t>(c,4)==1);
    f.ids.clear();f.times.clear();f.layers.clear();
    f.put(f.actor,0x3e0,std::uint8_t{1});f.put(f.actor,0x3e8,.99f);f.put(f.actor,0x398,.875f);
    f.put(a,0,0u);f.put(c,0,0u);
    assert(api->dispatch(f.actor,sizeof(f.actor),f.receiver,2.1f,&b,&out)&&out.emitted==2&&!out.layers);
    assert((f.ids==std::vector<int>{23,24})&&close_float(f.get<float>(f.actor,0x398),1.01f));
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS server studio event filtering, base/overlay traversal, timestamps and completion");
}
