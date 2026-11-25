#ifdef NDEBUG
#undef NDEBUG
#endif
#include "native_client_overlay_history.h"
#include "native_client_overlay_watchers.h"
#include "native_weapon_datatables.h"
#include <windows.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
struct F {
    alignas(4) unsigned char actor[0x808]{},rows[15*0x20]{},watchers[15*0x44]{},mapping[15*12]{};
    ClientOverlayHistoryNode nodes[4]{};
    ClientOverlayHistoryBindings b{};
    int loops=0,calls=0;bool looping=false;
    static F& self(void* p){return *static_cast<F*>(p);}
    template<class T> void put(unsigned char* p,unsigned off,T value){std::memcpy(p+off,&value,sizeof(value));}
    template<class T> T get(const unsigned char* p,unsigned off){T v{};std::memcpy(&v,p+off,sizeof(v));return v;}
    void seed(float from=.25f,float to=.5f) {
        nodes[0]={3,{7,from,.75f,2,0xabc,to,0xdef,0x123},0,2};
        nodes[1]={2,{4,.11f,.2f,1,0x11,.3f,0x22,0x33},1,3};
        nodes[2]={1,{4,.12f,.1f,0,0x44,.2f,0x55,0x66},2,0};
        nodes[3]={0,{},0,0};
        loops=calls=0;
        put(watchers,0x30,std::uint32_t{1});put(watchers,0x2c,1);put(watchers,0x3c,.25f);
        put(watchers,0x34,std::uint32_t{0xabcdef00});put(actor,0x7cc,9.f);
    }
};
static bool near(float a,float b){return std::fabs(a-b)<.00001f;}
int main(int argc,char** argv){
    assert(argc==3);
    auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetClientOverlayHistoryApi>(GetProcAddress(client,client_overlay_history_export));
    auto wg=reinterpret_cast<GetClientOverlayWatchersApi>(GetProcAddress(client,client_overlay_watchers_export));
    assert(get&&wg&&!GetProcAddress(server,client_overlay_history_export));
    auto* api=get(1,sizeof(ClientOverlayHistoryApi));auto* wa=wg(1,sizeof(ClientOverlayWatchersApi));
    assert(api&&wa&&!get(0,sizeof(ClientOverlayHistoryApi))&&!get(1,0));
    F f;
    f.put(f.actor,0x7a4,LayerVector{f.rows,15,0,0,f.rows});
    f.put(f.actor,0x7b8,LayerVector{f.watchers,15,0,0,f.watchers});
    f.put(f.actor,0x788,LayerVector{f.mapping,15,0,0,f.mapping});
    ClientOverlayWatcherBindings wb{};wb.context=&f;
    wb.construct=[](void*,void* w) noexcept{std::memset(w,0,0x44);};
    wb.destroy=[](void*,void*) noexcept{};
    wb.setup=[](void*,void*,void*,int) noexcept{};
    wb.type=[](void*,void*) noexcept -> int{return 1;};
    wb.amount=[](void*,void*,int) noexcept -> float{return .25f;};
    wb.set_amount=[](void* p,void* w,float a) noexcept{F::self(p).put(static_cast<unsigned char*>(w),0x3c,a);};
    assert(wa->resize(f.actor,sizeof(f.actor),1,&wb));
    f.b.context=&f;
    f.b.resolve=[](void* p,std::uint32_t word) noexcept -> ClientOverlayHistoryNode*{
        auto& f=F::self(p);return word>=1&&word<=4?&f.nodes[word-1]:nullptr;};
    f.b.looping=[](void* p,void* a,int sequence) noexcept -> bool{
        auto& f=F::self(p);assert(a==f.actor&&sequence==7);++f.loops;return f.looping;};
    f.b.interpolate=[](void* p,void* w,float time) noexcept {
        auto& f=F::self(p);assert(w==f.watchers&&near(time,2.75f)&&f.nodes[1].sample.sequence==7&&near(f.get<float>(f.actor,0x7cc),9.f));
        assert(f.get<std::uint8_t>(f.watchers,0x34)==f.looping);++f.calls;};
    f.seed();
    ClientOverlayHistorySelection s{};
    assert(api->select(1,1,2.75f,.25f,&f.b,&s)&&s.newer==f.nodes&&s.older==f.nodes+1&&s.earlier==f.nodes+2&&s.hermite&&near(s.fraction,.5f));
    assert(api->select(1,0x11,2.75f,.25f,&f.b,&s)&&!s.earlier&&!s.hermite);
    assert(api->select(1,1,4.f,0.f,&f.b,&s)&&s.newer==s.older&&s.newer==f.nodes);
    assert(api->select(1,1,.5f,0.f,&f.b,&s)&&s.newer==s.older&&s.newer==f.nodes+2);
    assert(api->select(0,1,2.f,0.f,&f.b,&s)&&!s.newer&&!s.older);
    assert(api->select(4,1,2.f,0.f,&f.b,&s)&&s.newer==f.nodes+3&&s.older==s.newer);
    f.nodes[0].time=2.00005f;
    assert(api->select(1,1,2.000025f,0.f,&f.b,&s)&&s.newer==s.older&&s.newer==f.nodes);
    f.seed();
    float cycle=-1;
    assert(api->loop_lerp(.5f,.75f,.25f,&cycle)&&near(cycle,0.f));
    assert(api->loop_lerp(-1.f,.875f,.125f,&cycle)&&near(cycle,.625f));
    assert(api->loop_lerp(-1.f,.125f,.375f,&cycle)&&near(cycle,.875f));
    unsigned changed=99;
    assert(api->correct(f.actor,sizeof(f.actor),2.75f,&f.b,&changed)&&changed==1&&f.loops==2&&f.calls==1);
    assert(f.nodes[1].sample.sequence==7&&near(f.nodes[1].sample.cycle,.25f)&&near(f.nodes[1].sample.weight,.75f));
    assert(f.nodes[2].sample.sequence==7&&near(f.nodes[2].sample.cycle,0.f)&&near(f.nodes[2].sample.weight,.75f));
    assert(f.nodes[1].sample.order==1&&near(f.nodes[1].sample.previous_cycle,.11f)&&f.nodes[1].sample.unknown10==0x11&&f.nodes[2].sample.unknown1c==0x66);
    assert(near(f.get<float>(f.actor,0x7cc),.24f)&&f.get<std::uint32_t>(f.watchers,0x34)==0xabcdef00);
    assert(api->correct(f.actor,sizeof(f.actor),2.75f,&f.b,&changed)&&!changed&&f.calls==1);
    f.seed(.875f,.125f);f.looping=true;
    assert(api->correct(f.actor,sizeof(f.actor),2.75f,&f.b,&changed)&&changed==1&&near(f.nodes[2].sample.cycle,.625f)&&f.get<std::uint32_t>(f.watchers,0x34)==0xabcdef01);
    f.seed();f.put(f.watchers,0x2c,0x11);
    assert(api->correct(f.actor,sizeof(f.actor),2.75f,&f.b,&changed)&&changed==1&&f.loops==1&&f.nodes[2].sample.sequence==4);
    f.seed();f.nodes[1].next=999;
    assert(!api->correct(f.actor,sizeof(f.actor),2.75f,&f.b,&changed)&&!changed&&!f.calls&&f.nodes[1].sample.sequence==4);
    f.seed();assert(!api->correct(f.actor,0x7cc,2.75f,&f.b,&changed)&&!changed);
    assert(wa->resize(f.actor,sizeof(f.actor),0,&wb));
    assert(api->correct(f.actor,sizeof(f.actor),2.75f,&f.b,&changed)&&!changed);
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS native client linked overlay history selection, sequence repair and cycle wrapping");
}
