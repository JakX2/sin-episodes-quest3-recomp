#ifdef NDEBUG
#undef NDEBUG
#endif
#include "native_client_overlay_interpolate.h"
#include "native_weapon_datatables.h"
#include <windows.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
static bool close_float(float a,float b){return std::fabs(a-b)<.00001f;}
struct F {
    alignas(4) unsigned char actor[0x808]{},watcher[0x44]{};
    ClientOverlaySample row{};
    ClientOverlayHistoryNode n[3]{};
    ClientOverlayInterpolateBindings b{};
    const ClientOverlayInterpolateApi* api{};
    unsigned prunes=0,repairs=0;
    static F& self(void* p){return *static_cast<F*>(p);}
    template<class T> void put(unsigned char* p,unsigned off,T v){std::memcpy(p+off,&v,sizeof(v));}
    template<class T> T get(unsigned char* p,unsigned off){T v{};std::memcpy(&v,p+off,sizeof(v));return v;}
    void seed(){
        n[0]={3,{7,.25f,.75f,3,0xabc,.75f,0xdead,0xbeef},0,2};
        n[1]={2,{7,.125f,.25f,2,0x123,.25f,0x456,0x789},1,3};
        n[2]={1,{7,0,.1f,1,0,0,0,0},2,0};
    }
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetClientOverlayInterpolateApi>(GetProcAddress(client,client_overlay_interpolate_export));
    auto hg=reinterpret_cast<GetClientOverlayHistoryApi>(GetProcAddress(client,client_overlay_history_export));
    assert(get&&hg&&!GetProcAddress(server,client_overlay_interpolate_export));
    F f;f.api=get(1,sizeof(ClientOverlayInterpolateApi));auto* history=hg(1,sizeof(ClientOverlayHistoryApi));
    assert(f.api&&history&&!get(0,sizeof(ClientOverlayInterpolateApi))&&!get(1,0));
    f.seed();
    ClientOverlayHistorySelection s{f.n+2,f.n+1,f.n,.5f,0};
    ClientOverlaySample o{};
    assert(f.api->evaluate(&s,false,&o)&&close_float(o.cycle,.5f)&&close_float(o.weight,.5f));
    assert(o.sequence==7&&o.order==3&&o.previous_cycle==.25f&&!o.unknown10&&o.unknown18==0xdead&&o.unknown1c==0xbeef);
    s.hermite=1;
    assert(f.api->evaluate(&s,false,&o)&&close_float(o.cycle,.46875f)&&close_float(o.weight,.5f));
    f.n[2].time=.5f;
    assert(f.api->evaluate(&s,false,&o)&&close_float(o.cycle,.45833333f));
    f.seed();f.n[0].sample.cycle=.125f;f.n[1].sample.cycle=.875f;f.n[2].sample.cycle=.75f;
    assert(f.api->evaluate(&s,true,&o)&&close_float(o.cycle,.984375f));
    s.hermite=0;assert(f.api->evaluate(&s,true,&o)&&close_float(o.cycle,0.f));
    f.seed();f.n[2].sample.cycle=.25f;s.hermite=1;
    assert(f.api->evaluate(&s,true,&o)&&close_float(o.cycle,.4375f)); // exact .5 threshold
    f.n[0].sample={1024,-3.f,9.f,77,0x123,4.f,0x456,0x789};s.newer=s.older=f.n;s.hermite=0;
    assert(f.api->evaluate(&s,false,&o)&&o.sequence==1000&&o.previous_cycle==-2.f&&o.cycle==2.f&&o.weight==5.f&&o.order==77&&o.unknown10==0x123);
    f.seed();f.b.history.context=&f;
    f.b.history.resolve=[](void* p,std::uint32_t word) noexcept -> ClientOverlayHistoryNode*{
        auto& f=F::self(p);return word>=1&&word<=3?f.n+word-1:nullptr;};
    f.b.history.looping=[](void* p,void* a,int seq) noexcept -> bool{assert(a==F::self(p).actor&&seq==7);return false;};
    f.b.prune=[](void* p,void* watcher,float cutoff) noexcept {
        auto& f=F::self(p);assert(watcher==f.watcher&&close_float(cutoff,2.45f)&&f.row.sequence==7);++f.prunes;};
    f.put(f.watcher,4,static_cast<void*>(&f.row));f.put(f.watcher,0x2c,1);f.put(f.watcher,0x30,std::uint32_t{1});
    f.put(f.watcher,0x38,1);f.put(f.watcher,0x3c,.25f);
    int wrote=-1;
    assert(f.api->apply(f.watcher,sizeof(f.watcher),&f.row,sizeof(f.row),2.75f,&f.b,&wrote)&&wrote==1&&f.prunes==1&&close_float(f.row.cycle,.46875f));
    f.put(f.watcher,0x2c,0x11);
    assert(f.api->apply(f.watcher,sizeof(f.watcher),&f.row,sizeof(f.row),2.75f,&f.b,&wrote)&&wrote==1&&f.prunes==2&&close_float(f.row.cycle,.5f));
    f.put(f.watcher,0x30,std::uint32_t{0});
    assert(f.api->apply(f.watcher,sizeof(f.watcher),&f.row,sizeof(f.row),2.75f,&f.b,&wrote)&&!wrote&&f.prunes==2&&close_float(f.row.cycle,.5f));
    f.put(f.watcher,0x30,std::uint32_t{99});
    assert(!f.api->apply(f.watcher,sizeof(f.watcher),&f.row,sizeof(f.row),2.75f,&f.b,&wrote)&&!wrote&&f.prunes==2);
    f.seed();f.n[1].sample.sequence=4;f.n[2].sample.sequence=4;
    f.put(f.watcher,0x2c,1);f.put(f.watcher,0x30,std::uint32_t{1});
    f.put(f.actor,0x7b8,LayerVector{f.watcher,1,0,1,f.watcher});f.put(f.actor,0x7cc,9.f);
    // Real loaded Phase1011 repair calls the reconstructed interpolation
    // export, not a fixture-computed numerical result.
    f.b.history.interpolate=[](void* p,void* watcher,float time) noexcept {
        auto& f=F::self(p);int wrote=0;
        assert(f.api->apply(watcher,0x44,&f.row,sizeof(f.row),time,&f.b,&wrote)&&wrote==1);
        assert(close_float(f.get<float>(f.actor,0x7cc),9.f));++f.repairs;
    };
    unsigned changed=0;
    assert(history->correct(f.actor,sizeof(f.actor),2.75f,&f.b.history,&changed)&&changed==1&&f.repairs==1&&f.prunes==3);
    assert(f.n[1].sample.sequence==7&&f.n[2].sample.sequence==7&&close_float(f.row.cycle,.5f)&&close_float(f.row.weight,.75f)&&close_float(f.get<float>(f.actor,0x7cc),.24f));
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS native client overlay linear/Hermite interpolation, clamp and linked history repair");
}
