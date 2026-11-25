#include "native_server_npc_run_animation.h"
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
    alignas(4) unsigned char actor[0xe0c]{},rows[0x80]{};
    ServerStudioEvent base[1]{{.55f,20,0,"base",0}},overlay[1]{{.1f,21,0,"overlay",0}};
    std::vector<int> trace;
    bool available=true;int choice=-1;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    static F& self(void* p){return *static_cast<F*>(p);}
    static std::uint32_t word(void* p){return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));}
    void clocks(){put(actor,0x28,9.875f);put(actor,0x2c,10.f);put(actor,0x3e8,.5f);put(actor,0x398,.5f);put(rows,0xc,0.f);put(rows,0x38,0.f);}
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetServerNpcRunAnimationApi>(GetProcAddress(server,server_npc_run_animation_export));
    auto go=reinterpret_cast<GetServerAnimationLayerOwnerApi>(GetProcAddress(server,server_animation_layer_owner_export));
    assert(get&&go&&!GetProcAddress(client,server_npc_run_animation_export)&&!get(0,sizeof(ServerNpcRunAnimationApi))&&!get(1,0));
    auto* api=get(1,sizeof(ServerNpcRunAnimationApi));auto* owner=go(1,sizeof(ServerAnimationLayerOwnerApi));assert(api&&owner);
    F f;ServerNpcRunAnimationBindings b{};b.context=&f;
    b.model_available=[](void* p,void* a) noexcept -> int{auto& f=F::self(p);assert(a==f.actor);f.trace.push_back(1);return f.available;};
    b.debug_update=[](void* p,void* a,std::uint32_t c) noexcept{auto& f=F::self(p);assert(a==f.actor&&c==0x1234);f.trace.push_back(4);};
    b.select_sequence=[](void* p,void* a,int activity,int current,bool,int* out) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&activity==5&&current==9);f.trace.push_back(5);*out=f.choice;return 1;};
    b.set_sequence=[](void* p,void* a,int seq) noexcept -> int{auto& f=F::self(p);assert(a==f.actor&&seq==11);f.trace.push_back(6);f.put(a,0x3ec,seq);f.put(a,0x3e8,0.f);f.put(a,0x398,0.f);return 1;};
    auto& adv=b.advance;adv.context=&f;adv.base.context=&f;adv.layers.context=&f;
    adv.layers.vectors.context=&f;
    adv.layers.vectors.storage=[](void* p,std::uint32_t w,void** out,unsigned* n) noexcept -> int{auto& f=F::self(p);if(w!=F::word(f.rows))return 0;*out=f.rows;*n=sizeof(f.rows);return 1;};
    adv.base.vectors=adv.layers.vectors;
    adv.base.duration=[](void*,void*,int seq,double* out) noexcept -> int{assert(seq==9);*out=2.;return 1;};
    adv.base.finish_threshold=[](void*,void*,int seq,double* out) noexcept -> int{assert(seq==9);*out=.9;return 1;};
    adv.base.movement_distance=[](void*,void*,int seq,double* out) noexcept -> int{assert(seq==9);*out=120.;return 1;};
    adv.base.invalidate=[](void* p,void*,unsigned flags) noexcept -> int{assert(flags==8);F::self(p).trace.push_back(2);return 1;};
    adv.base.cache=[](void*,void*,std::uint32_t,float) noexcept -> int{return 1;};
    adv.layers.cycle_rate=[](void* p,void*,int seq,double* out) noexcept -> int{assert(seq==7);F::self(p).trace.push_back(3);*out=1.;return 1;};
    b.events.context=&f;b.events.storage=adv.layers.vectors.storage;
    b.events.sequence=[](void* p,void* a,int seq,ServerAnimationEventSequence* out) noexcept -> int{auto& f=F::self(p);assert(a==f.actor);if(seq==11){*out={};return 1;}assert(seq==7||seq==9);*out={seq==9?f.base:f.overlay,1,1,0};return 1;};
    b.events.cycle_rate=[](void*,void*,int,double* out) noexcept -> int{*out=1.;return 1;};
    b.events.emit=[](void* p,void* a,int layer,const ServerAnimationEvent* e) noexcept{auto& f=F::self(p);assert(a==f.actor&&e->source==a);assert((layer==-1&&e->event==20)||(layer==0&&e->event==21));f.trace.push_back(e->event);};
    f.put(f.actor,0x494,F::word(f.rows));f.put(f.actor,0x498,2);f.put(f.actor,0x4a0,2);f.put(f.actor,0x4a4,F::word(f.rows));
    f.put(f.rows,0x34,15);f.put(f.rows+0x40,0x34,15);int layer=-1;
    assert(owner->allocate(f.actor,sizeof(f.actor),7,0,10,1,&adv.layers.vectors,&layer)&&layer==0);
    f.put(f.actor,0x3c0,2.f);f.put(f.actor,0x3ec,9);f.actor[0x1f0]=1;f.put(f.actor,0xe08,7u);
    f.put(f.actor,0x8f8,0x1234u);f.put(f.actor,0x850,4);f.put(f.actor,0x878,1);f.put(f.actor,0x87c,5);f.put(f.actor,0x884,9);
    f.clocks();ServerNpcRunAnimationInput in{10.125f,false,true};ServerNpcRunAnimationResult out{};
    f.available=false;assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&!out.advanced&&!out.events_dispatched&&(f.trace==std::vector<int>{1}));
    f.available=true;f.trace.clear();
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.advanced&&out.events_dispatched&&!out.selection_attempted&&out.events.emitted==2);
    assert((f.trace==std::vector<int>{1,2,3,4,20,21})&&f.get<unsigned>(f.actor,0xe08)==7);
    f.clocks();f.trace.clear();f.actor[0x3e0]=1;f.actor[0x3e1]=1;f.put(f.actor,0x850,0);
    in.clear_npc_flags=true;
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.selection_attempted&&!out.sequence_set&&out.events.emitted==2&&f.get<unsigned>(f.actor,0xe08)==4);
    assert((f.trace==std::vector<int>{1,2,3,4,5,20,21}));
    f.clocks();f.trace.clear();f.choice=11;
    assert(api->run(f.actor,sizeof(f.actor),&in,&b,&out)&&out.sequence_set&&out.selected==11&&out.events.emitted==1);
    assert((f.trace==std::vector<int>{1,2,3,4,5,6,21})&&f.get<int>(f.actor,0x3ec)==11);
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS native NPC RunAnimation metadata gate, advancement, idle selection and server events");
}
