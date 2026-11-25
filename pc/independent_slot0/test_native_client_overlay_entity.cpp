#include "native_client_overlay_entity.h"
#include "native_weapon_datatables.h"
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
    alignas(4) unsigned char actor[0x808]{},watchers[2*0x44]{};
    ClientOverlaySample rows[2]{};
    ClientOverlayHistoryNode nodes[2][8]{};
    ClientOverlayPool pool[2]{};
    ClientOverlayEntitySlot slots[2]{{pool,{nodes[0],8,0x1000}},{pool+1,{nodes[1],8,0x2000}}};
    ClientOverlayEntityBindings b{slots,2,this,nullptr};
    int queries=0;
    template<class T> void put(void* p,unsigned off,T v){std::memcpy(static_cast<unsigned char*>(p)+off,&v,sizeof(v));}
    template<class T> T get(void* p,unsigned off){T v{};std::memcpy(&v,static_cast<unsigned char*>(p)+off,sizeof(v));return v;}
    unsigned char* watcher(unsigned i){return watchers+i*0x44;}
    void sample(int n){
        rows[0]={n==3?7:4,.25f,n==3?.75f:.25f,0,0,n==3?.75f:n*.1f,0xabc,0xdef};
        rows[1]={9,.1f,.5f,1,0,n*.2f,0x111,0x222};
    }
};
int main(int argc,char** argv){
    assert(argc==3);auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);assert(client&&server);
    auto get=reinterpret_cast<GetClientOverlayEntityFrameApi>(GetProcAddress(client,client_overlay_entity_frame_export));
    auto gp=reinterpret_cast<GetClientOverlayPoolApi>(GetProcAddress(client,client_overlay_pool_export));
    assert(get&&gp&&!GetProcAddress(server,client_overlay_entity_frame_export)&&!get(0,sizeof(ClientOverlayEntityFrameApi))&&!get(1,0));
    const auto* api=get(1,sizeof(ClientOverlayEntityFrameApi));const auto* pool=gp(1,sizeof(ClientOverlayPoolApi));assert(api&&pool);
    Fixture f;auto* row_bytes=reinterpret_cast<unsigned char*>(f.rows);
    f.put(f.actor,0x7a4,LayerVector{row_bytes,2,0,2,row_bytes});
    f.put(f.actor,0x7b8,LayerVector{f.watchers,2,0,2,f.watchers});
    f.b.looping=[](void* p,void* actor,int seq) noexcept -> bool{
        auto& f=*static_cast<Fixture*>(p);assert(actor==f.actor&&seq==7);++f.queries;return false;};
    for(unsigned i=0;i<2;++i){
        auto* w=f.watcher(i);f.put(w,4,static_cast<void*>(f.rows+i));f.put(w,0x2c,1);f.put(w,0x38,1);f.put(w,0x3c,.25f);
        assert(pool->initialize(w,0x44,&f.pool[i],&f.slots[i].storage));
        f.put(f.actor,0x7cc+i*4,9.f);
    }
    ClientOverlayEntityResult out{};
    f.sample(1);assert(api->reset(f.actor,sizeof(f.actor),1,&f.b,&out)&&out.processed==2&&f.pool[0].count==3&&f.pool[1].count==3);
    f.sample(2);assert(api->capture(f.actor,sizeof(f.actor),2,2,2,&f.b,&out)&&out.processed==2);
    f.sample(3);assert(api->capture(f.actor,sizeof(f.actor),3,3,3,&f.b,&out)&&out.processed==2);
    ClientOverlaySample snapshots[2];std::memcpy(snapshots,f.rows,sizeof(snapshots));
    ClientOverlayInterpolationClock clock{true,3.f};
    assert(api->render(f.actor,sizeof(f.actor),2.75f,&clock,&f.b,&out));
    assert(out.processed==2&&out.writes==2&&out.repairs==1&&out.raw_copies==0&&f.queries==2);
    assert(f.rows[0].sequence==7&&close_float(f.rows[0].cycle,.5f)&&close_float(f.rows[0].weight,.75f));
    assert(f.rows[1].sequence==9&&close_float(f.rows[1].cycle,.5f)&&close_float(f.rows[1].weight,.5f));
    assert(close_float(f.get<float>(f.actor,0x7cc),.24f)&&f.get<float>(f.actor,0x7d0)==9.f);
    assert(api->restore(f.actor,sizeof(f.actor),&f.b,&out)&&out.writes==2&&!std::memcmp(snapshots,f.rows,sizeof(snapshots)));
    assert(api->render(f.actor,sizeof(f.actor),4,&clock,&f.b,&out)&&out.raw_copies==2&&out.writes==2&&!out.repairs);
    assert(!std::memcmp(snapshots,f.rows,sizeof(snapshots)));
    auto before=f.pool[0];const auto head=f.pool[1].head;
    f.put(f.watcher(1),0x30,head+1);
    assert(!api->capture(f.actor,sizeof(f.actor),4,4,4,&f.b,&out)&&out.failed_slot==1&&!out.processed&&!std::memcmp(&before,&f.pool[0],sizeof(before)));
    f.put(f.watcher(1),0x30,head);
    auto saved=f.slots[1];f.slots[1]=f.slots[0];f.put(f.watcher(1),0x30,f.pool[0].head);
    assert(!api->render(f.actor,sizeof(f.actor),4,&clock,&f.b,&out)&&out.failed_slot==1&&!out.processed);
    f.slots[1]=saved;f.put(f.watcher(1),0x30,head);
    assert(api->reset(f.actor,sizeof(f.actor),5,&f.b,&out)&&out.processed==2&&f.pool[0].count==3&&f.pool[1].count==3);
    unsigned removed=0;
    for(unsigned i=0;i<2;++i)assert(pool->clear(f.watcher(i),0x44,&f.pool[i],&f.slots[i].storage,&removed)&&removed==3);
    f.put(f.actor,0x7a4,LayerVector{row_bytes,2,0,0,row_bytes});
    f.put(f.actor,0x7b8,LayerVector{f.watchers,2,0,0,f.watchers});
    f.b.count=0;
    assert(api->render(f.actor,sizeof(f.actor),5,&clock,&f.b,&out)&&!out.processed&&!out.writes);
    FreeLibrary(server);FreeLibrary(client);
    std::puts("PASS native multi-watcher overlay capture, history repair, clocked interpolation and restore");
}
