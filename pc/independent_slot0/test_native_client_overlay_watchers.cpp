#include "native_client_overlay_watchers.h"
#include "native_weapon_datatables.h"
#include <windows.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace sin_port::native_weapon_tables;
struct Map {int flags;unsigned char* row;unsigned char* watcher;};
static_assert(sizeof(Map)==12);
struct Fixture {
    alignas(4) unsigned char actor[0x7cc]{},rows[15*0x20]{},watchers[15*0x44]{},unrelated[0x20]{};
    Map maps[16]{};
    std::vector<int> trace;
    int grows=0;
    const ClientOverlayWatchersApi* api{};
    ClientOverlayWatcherBindings b{};
    static Fixture& self(void* p){return *static_cast<Fixture*>(p);}
    LayerVector get(unsigned off)const{LayerVector v;std::memcpy(&v,actor+off,sizeof(v));return v;}
    void put(unsigned off,LayerVector v){std::memcpy(actor+off,&v,sizeof(v));}
    int wi(void* p){return static_cast<int>((static_cast<unsigned char*>(p)-watchers)/0x44);}
};
int main(int argc,char** argv){
    assert(argc==3);
    auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetClientOverlayWatchersApi>(GetProcAddress(client,client_overlay_watchers_export));
    assert(get&&!GetProcAddress(server,client_overlay_watchers_export));
    Fixture f;f.api=get(1,sizeof(ClientOverlayWatchersApi));
    assert(f.api&&!get(0,sizeof(ClientOverlayWatchersApi))&&!get(1,0));
    auto* m=reinterpret_cast<unsigned char*>(f.maps);
    f.maps[0]={7,f.unrelated,nullptr};
    f.put(0x788,{m,16,0,1,m});f.put(0x7b8,{f.watchers,15,0,0,f.watchers});
    f.b.context=&f;
    f.b.grow_layers=[](void* p,unsigned char* old,unsigned size) noexcept -> unsigned char*{
        auto& f=Fixture::self(p);assert(!old&&size==15*0x20);++f.grows;return f.rows;};
    f.b.construct=[](void* p,void* w) noexcept {auto& f=Fixture::self(p);assert(f.get(0x788).count==1);f.trace.push_back(100+f.wi(w));};
    f.b.destroy=[](void* p,void* w) noexcept {auto& f=Fixture::self(p);assert(f.get(0x788).count==1);f.trace.push_back(200+f.wi(w));};
    f.b.setup=[](void* p,void* w,void* row,int type) noexcept {
        auto& f=Fixture::self(p);int i=f.wi(w);assert(type==1&&row==f.rows+i*0x20&&f.maps[i+1].row==row&&f.maps[i+1].watcher==w&&f.maps[i+1].flags==1);
        f.trace.push_back(300+i);};
    f.b.type=[](void* p,void* w) noexcept -> int{auto& f=Fixture::self(p);f.trace.push_back(400+f.wi(w));return 1;};
    f.b.amount=[](void* p,void* actor,int type) noexcept -> float{auto& f=Fixture::self(p);assert(actor==f.actor&&type==1);f.trace.push_back(500);return .125f;};
    f.b.set_amount=[](void* p,void* w,float a) noexcept {auto& f=Fixture::self(p);assert(a==.125f);f.trace.push_back(600+f.wi(w));};
    auto getGraph=reinterpret_cast<GetOverlayRecvApi>(GetProcAddress(client,overlay_recv_export));assert(getGraph);
    auto* graph=getGraph(1,sizeof(OverlayRecvApi));assert(graph);
    OverlayRecvBindings rb{&f,[](void* p,void* actor,int off,int stride,int target) noexcept -> int {
        auto& f=Fixture::self(p);assert(actor==f.actor&&off==0x7a4&&stride==0x20);
        return f.api->reserve(actor,sizeof(f.actor),target,&f.b);
    },[](void* p,void* actor,int off,int stride,int target) noexcept -> int {
        auto& f=Fixture::self(p);assert(actor==f.actor&&off==0x7a4&&stride==0x20);
        return f.api->resize(actor,sizeof(f.actor),target,&f.b);
    }};
    OverlayRecvView view{};assert(graph->create(&rb,&view));
    std::memset(f.rows,0xa5,sizeof(f.rows));void* dest=nullptr;
    auto& prop=view.vector_table->props[0];prop.table_proxy(&prop,&dest,f.actor,1);
    assert(dest==f.actor&&f.grows==1&&f.get(0x7a4).count==0);
    for(auto c:f.rows)assert(c==0);
    auto length=[&](int count){auto& lp=view.length_table->props[0];s::DVariant v{};v.words[0]=count;RecvData d{&lp,v,0,1};lp.proxy(&d,f.actor,f.actor);};
    length(2);
    assert((f.trace==std::vector<int>{100,101,300,400,500,600,301,401,500,601}));
    assert(f.get(0x7a4).count==2&&f.get(0x7b8).count==2&&f.get(0x788).count==3);
    f.rows[0]=42;f.rows[0x40]=99;f.trace.clear();
    assert(f.api->reserve(f.actor,sizeof(f.actor),15,&f.b)&&f.rows[0]==42&&f.rows[0x40]==0);
    length(2);assert(f.trace.empty());
    auto shortMap=f.get(0x788);shortMap.capacity=3;f.put(0x788,shortMap);
    assert(!f.api->resize(f.actor,sizeof(f.actor),3,&f.b)&&f.trace.empty()&&f.get(0x7a4).count==2);
    shortMap.capacity=16;f.put(0x788,shortMap);
    length(3);assert((f.trace==std::vector<int>{102,300,400,500,600,301,401,500,601,302,402,500,602}));
    assert(f.rows[0]==42&&f.get(0x788).count==4);
    f.trace.clear();length(1);assert((f.trace==std::vector<int>{202,201,300,400,500,600}));
    assert(f.get(0x7b8).count==1&&f.maps[0].row==f.unrelated&&f.maps[0].flags==7);
    f.trace.clear();length(0);assert((f.trace==std::vector<int>{200}));
    assert(f.get(0x788).count==1&&f.get(0x7a4).count==0&&f.get(0x7b8).count==0&&f.grows==1);
    assert(!f.api->resize(f.actor,sizeof(f.actor),16,&f.b)&&!f.api->resize(f.actor,0,0,&f.b));
    assert(graph->destroy(&view));
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS client overlay receive reserve and interpolation watcher grow/shrink/rebind");
}
