#include "native_server_base_animation_frame.h"
#include "native_server_animation_events.h"
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
struct F {
    alignas(4) unsigned char weapon[0x4c0]{},receiver[4]{};
    ServerStudioEvent events[3]{{.2f,12,0,"legacy",0},{.3f,24,0x410,"client",0},{.35f,25,0x401,"server",0}};
    std::vector<int> trace;
    template<class T> void put(unsigned off,T v){std::memcpy(weapon+off,&v,sizeof(v));}
    template<class T> T get(unsigned off){T v{};std::memcpy(&v,weapon+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto gf=reinterpret_cast<GetServerBaseAnimationFrameApi>(GetProcAddress(server,server_base_animation_frame_export));
    auto ge=reinterpret_cast<GetServerBaseAnimationEventsApi>(GetProcAddress(server,server_base_animation_events_export));
    assert(gf&&ge&&!GetProcAddress(client,server_base_animation_frame_export)&&!GetProcAddress(client,server_base_animation_events_export));
    assert(!gf(0,sizeof(ServerBaseAnimationFrameApi))&&!ge(1,0));
    auto* frame=gf(1,sizeof(ServerBaseAnimationFrameApi));auto* events=ge(1,sizeof(ServerBaseAnimationEventsApi));assert(frame&&events);
    F f;ServerBaseAnimationFrameBindings b{};b.context=&f;b.cycle.context=&f;
    b.width_transition=[](void* p,void* a) noexcept -> int {auto& f=F::self(p);assert(a==f.weapon);f.trace.push_back(1);return 1;};
    b.cycle.duration=[](void* p,void* a,int seq,double* out) noexcept -> int {auto& f=F::self(p);assert(a==f.weapon&&seq==7);f.trace.push_back(2);*out=1.;return 1;};
    b.cycle.finish_threshold=[](void* p,void*,int,double* out) noexcept -> int {F::self(p).trace.push_back(3);*out=.9;return 1;};
    b.cycle.movement_distance=[](void* p,void*,int,double* out) noexcept -> int {F::self(p).trace.push_back(4);*out=0;return 1;};
    b.cycle.invalidate=[](void* p,void*,unsigned flags) noexcept -> int {assert(flags==8);F::self(p).trace.push_back(5);return 1;};
    b.cycle.cache=[](void* p,void*,std::uint32_t token,float now) noexcept -> int {assert(token==0x7654&&now==10.125f);F::self(p).trace.push_back(6);return 1;};
    f.put(0x1f0,std::uint8_t{1});f.put(0x28,9.875f);f.put(0x2c,10.f);
    f.put(0x3ec,7);f.put(0x3c0,2.f);f.put(0x3e8,.125f);f.put(0x398,.125f);f.put(0x478,0x7654u);
    f.put(0x494,0x7001u);f.put(0x498,0xffffffffu);f.put(0x4a0,0xffffffffu);
    assert(frame->run(f.weapon,sizeof(f.weapon),10.125f,&b)&&f.get<float>(0x3e8)==.375f);
    assert((f.trace==std::vector<int>{1,2,3,2,4,5,6}));
    ServerAnimationEventBindings eb{};eb.context=&f;
    eb.sequence=[](void* p,void* a,int seq,ServerAnimationEventSequence* out) noexcept -> int {auto& f=F::self(p);assert(a==f.weapon&&seq==7);*out={f.events,3,0,0};return 1;};
    eb.cycle_rate=[](void*,void*,int,double* out) noexcept -> int {*out=1.;return 1;};
    eb.emit=[](void* p,void* recv,int layer,const ServerAnimationEvent* event) noexcept {
        auto& f=F::self(p);assert(recv==f.receiver&&layer==-1&&event->source==f.weapon);f.trace.push_back(event->event);};
    // No overlay storage callback. +494 is a weapon owner handle.
    ServerAnimationEventResult out{};
    assert(events->dispatch(f.weapon,sizeof(f.weapon),f.receiver,10.125f,&eb,&out)&&out.emitted==2&&!out.layers);
    assert(f.get<float>(0x398)==.375f&&f.get<unsigned>(0x494)==0x7001u&&f.get<unsigned>(0x4a0)==0xffffffffu);
    assert((f.trace==std::vector<int>{1,2,3,2,4,5,6,12,25}));
    assert(events->dispatch(f.weapon,0x3f0,f.receiver,10.125f,&eb,&out)&&!out.emitted);
    assert(!events->dispatch(f.weapon,0x3ef,f.receiver,10.125f,&eb,&out));
    FreeLibrary(server);FreeLibrary(client);std::puts("PASS native base-only weapon frame and source/receiver event dispatch");
}
