#include "native_client_overlay_pool.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cmath>
#include <cstdio>
using namespace sin_port::native_weapon_tables;
static bool close_float(float a,float b){return std::fabs(a-b)<.00001f;}
struct Fixture {
    alignas(4) unsigned char watcher[0x44]{};
    ClientOverlaySample row{};
    ClientOverlayHistoryNode nodes[8]{};
    ClientOverlayPool p{};
    ClientOverlayPoolStorage s{nodes,8,0x1000};
    const ClientOverlayPoolApi* api{};
    template<class T> void put(unsigned off,T v){std::memcpy(watcher+off,&v,sizeof(v));}
    std::uint32_t head(){std::uint32_t w=0;std::memcpy(&w,watcher+0x30,4);return w;}
    ClientOverlayHistoryNode& node(std::uint32_t w){assert(w>=s.first_word&&(w-s.first_word)%0x2c==0&&(w-s.first_word)/0x2c<8);return nodes[(w-s.first_word)/0x2c];}
    void record(float time,bool replace=false){
        ClientOverlaySample x{7,.125f,.5f,2,0,time*.1f,0x1111,0x2222};
        assert(api->record(watcher,sizeof(watcher),&p,&s,time,&x,replace));
    }
    void order(const float* times,unsigned count){
        assert(head()==p.head&&p.count==count);
        auto w=p.head;std::uint32_t prev=0;
        for(unsigned i=0;i<count;++i){assert(w&&close_float(node(w).time,times[i])&&node(w).previous==prev);prev=w;w=node(w).next;}
        assert(!w&&prev==p.tail);
    }
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetClientOverlayPoolApi>(GetProcAddress(client,client_overlay_pool_export));
    assert(get&&!GetProcAddress(server,client_overlay_pool_export)&&!get(0,sizeof(ClientOverlayPoolApi))&&!get(1,0));
    Fixture f;f.api=get(1,sizeof(ClientOverlayPoolApi));assert(f.api);
    assert(f.api->initialize(f.watcher,sizeof(f.watcher),&f.p,&f.s)&&!f.p.count&&f.p.free_head==0x1000);
    assert(!f.api->initialize(f.watcher,sizeof(f.watcher),&f.p,&f.s));
    for(int i=1;i<=8;++i)f.record(static_cast<float>(i));
    const float all[]={8,7,6,5,4,3,2,1};f.order(all,8);
    const auto snapshot=f.p;
    assert(!f.api->record(f.watcher,sizeof(f.watcher),&f.p,&f.s,9,&f.row,true)&&!std::memcmp(&snapshot,&f.p,sizeof(snapshot)));
    unsigned removed=99;
    const auto original_tail=f.p.tail;
    assert(f.api->prune(f.watcher,sizeof(f.watcher),&f.p,&f.s,5,&removed)&&removed==1);
    const float seven[]={8,7,6,5,4,3,2};f.order(seven,7);
    assert(f.p.free_head==original_tail&&f.node(original_tail).previous==original_tail&&!f.node(original_tail).next);
    f.record(5.5f);const float inserted[]={8,7,6,5.5f,5,4,3,2};f.order(inserted,8);
    assert(f.api->prune(f.watcher,sizeof(f.watcher),&f.p,&f.s,9,&removed)&&removed==5);
    const float top[]={8,7,6};f.order(top,3);
    f.record(7.f,true);const float rewind[]={7,6};f.order(rewind,2);
    assert(f.api->prune(f.watcher,sizeof(f.watcher),&f.p,&f.s,9,&removed)&&!removed);
    f.record(7.00005f,true);const float equal_epsilon[]={7.00005f,6};f.order(equal_epsilon,2);
    assert(f.api->clear(f.watcher,sizeof(f.watcher),&f.p,&f.s,&removed)&&removed==2&&!f.head()&&!f.p.tail);
    assert(f.api->clear(f.watcher,sizeof(f.watcher),&f.p,&f.s,&removed)&&!removed);
    for(int i=1;i<=8;++i)f.record(static_cast<float>(i));
    f.put(4,static_cast<void*>(&f.row));f.put(0x2c,0x11);f.put(0x38,1);f.put(0x3c,.25f);
    int wrote=0;
    // Render time 5.75, adjusted time 5.5, cutoff 5.45.
    // Keep old samples 5,4,3; release 2,1.
    assert(f.api->apply(f.watcher,sizeof(f.watcher),&f.row,sizeof(f.row),5.75f,&f.p,&f.s,&wrote,&removed)&&wrote==1&&removed==2&&close_float(f.row.cycle,.55f));
    const float after[]={8,7,6,5,4,3};f.order(after,6);
    auto& tail=f.node(f.p.tail);tail.next=f.p.head;
    const auto saved=f.p;const auto saved_row=f.row;
    assert(!f.api->apply(f.watcher,sizeof(f.watcher),&f.row,sizeof(f.row),5.75f,&f.p,&f.s,&wrote,&removed)&&!wrote&&!removed&&!std::memcmp(&saved,&f.p,sizeof(saved))&&!std::memcmp(&saved_row,&f.row,sizeof(saved_row)));
    tail.next=0;
    auto& free=f.node(f.p.free_head);const auto oldprev=free.previous;free.previous=0;
    assert(!f.api->prune(f.watcher,sizeof(f.watcher),&f.p,&f.s,9,&removed)&&!removed);
    free.previous=oldprev;
    assert(f.api->clear(f.watcher,sizeof(f.watcher),&f.p,&f.s,&removed)&&removed==6);
    assert(!f.api->prune(f.watcher,0,&f.p,&f.s,9,&removed));
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS native bounded overlay history recording, recycled node pool, retention and interpolation");
}
