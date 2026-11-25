#include "native_client_overlay_pool.h"
#include "native_client_overlay_interpolate.h"
#include <cmath>
#include <cstring>
#include <limits>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayInterpolateApi* __cdecl
SiN_GetClientOverlayInterpolateApi(std::uint32_t,std::uint32_t) noexcept;
namespace sin_port::native_weapon_tables {
namespace {
constexpr std::uint32_t stride=sizeof(ClientOverlayHistoryNode),max_nodes=4096;
template<class T> T read(const void* p,unsigned off) noexcept {
    T x{};std::memcpy(&x,static_cast<const unsigned char*>(p)+off,sizeof(x));return x;
}
template<class T> void write(void* p,unsigned off,T x) noexcept {
    std::memcpy(static_cast<unsigned char*>(p)+off,&x,sizeof(x));
}
bool storage_ok(const ClientOverlayPoolStorage* s) noexcept {
    return s&&s->nodes&&s->capacity&&s->capacity<=max_nodes&&s->first_word&&
        (s->first_word&3)==0&&static_cast<std::uint64_t>(s->first_word)+static_cast<std::uint64_t>(s->capacity)*stride<=0x100000000ull;
}
std::uint32_t word(const ClientOverlayPoolStorage& s,unsigned index) noexcept {return s.first_word+index*stride;}
ClientOverlayHistoryNode* node(const ClientOverlayPoolStorage& s,std::uint32_t w) noexcept {
    if(w<s.first_word)return nullptr;
    const auto d=w-s.first_word;
    if(d%stride||d/stride>=s.capacity)return nullptr;
    return s.nodes+d/stride;
}
bool valid(void* watcher,unsigned bytes,const ClientOverlayPool* p,const ClientOverlayPoolStorage* s) noexcept {
    if(!watcher||bytes<0x44||!p||!storage_ok(s)||p->count>s->capacity||
       read<std::uint32_t>(watcher,0x30)!=p->head)return false;
    unsigned char seen[max_nodes]{};
    unsigned count=0;std::uint32_t prev=0;
    for(auto w=p->head;w;){
        auto* n=node(*s,w);
        if(!n||seen[n-s->nodes]||n->previous!=prev||!std::isfinite(n->time))return false;
        seen[n-s->nodes]=1;++count;prev=w;w=n->next;
    }
    if(count!=p->count||prev!=p->tail)return false;
    for(auto w=p->free_head;w;){
        auto* n=node(*s,w);
        if(!n||seen[n-s->nodes]||n->previous!=w)return false;
        seen[n-s->nodes]=1;++count;w=n->next;
    }
    return count==s->capacity;
}
void unlink_recycle(ClientOverlayPool& p,const ClientOverlayPoolStorage& s,std::uint32_t w) noexcept {
    auto* n=node(s,w);
    const auto prev=n->previous,next=n->next;
    if(prev)node(s,prev)->next=next;else p.head=next;
    if(next)node(s,next)->previous=prev;else p.tail=prev;
    --p.count;
    n->previous=w;n->next=p.free_head;p.free_head=w;
}
void link_before(ClientOverlayPool& p,const ClientOverlayPoolStorage& s,std::uint32_t w,std::uint32_t before) noexcept {
    auto* n=node(s,w);
    n->next=before;n->previous=before?node(s,before)->previous:p.tail;
    if(before)node(s,before)->previous=w;else p.tail=w;
    if(n->previous)node(s,n->previous)->next=w;else p.head=w;
    ++p.count;
}
int __cdecl initialize(void* watcher,unsigned bytes,ClientOverlayPool* p,const ClientOverlayPoolStorage* s) noexcept {
    if(!watcher||bytes<0x44||!p||!storage_ok(s)||p->head||p->tail||p->free_head||p->count||read<std::uint32_t>(watcher,0x30))return 0;
    // Port-owned seed of the recycled-node list. Payload is not cleared.
    for(unsigned i=0;i<s->capacity;++i){
        auto& n=s->nodes[i];n.previous=word(*s,i);
        n.next=i+1<s->capacity?word(*s,i+1):0;
    }
    p->free_head=s->first_word;return 1;
}
bool finite(const ClientOverlaySample& x) noexcept {
    return std::isfinite(x.cycle)&&std::isfinite(x.previous_cycle)&&std::isfinite(x.weight);
}
// 240c12b0: allocate before removing newer samples. No grow here.
int __cdecl record(void* watcher,unsigned bytes,ClientOverlayPool* p,const ClientOverlayPoolStorage* s,float time,const ClientOverlaySample* sample,bool replace_future) noexcept {
    if(!sample||!std::isfinite(time)||!finite(*sample)||!valid(watcher,bytes,p,s)||!p->free_head)return 0;
    const ClientOverlaySample copy=*sample;
    const auto w=p->free_head;auto* n=node(*s,w);
    p->free_head=n->next;n->time=time;n->sample=copy;
    std::uint32_t before=p->head;
    if(replace_future){
        while(before&&static_cast<double>(node(*s,before)->time)+0.0001f>=time){
            const auto next=node(*s,before)->next;unlink_recycle(*p,*s,before);before=next;
        }
    }else{
        while(before&&node(*s,before)->time>time)before=node(*s,before)->next;
    }
    link_before(*p,*s,w,before);
    write(watcher,0x30,p->head);return 1;
}
// 240c3230/240c34d0: first sample strictly older than cutoff,
// retain that sample plus two further linked samples, recycle suffix.
int __cdecl prune(void* watcher,unsigned bytes,ClientOverlayPool* p,const ClientOverlayPoolStorage* s,float cutoff,unsigned* removed) noexcept {
    if(!removed)return 0;
    *removed=0;
    if(!std::isfinite(cutoff)||!valid(watcher,bytes,p,s))return 0;
    auto w=p->head;
    while(w&&node(*s,w)->time>=cutoff)w=node(*s,w)->next;
    for(unsigned i=0;i<3&&w;++i)w=node(*s,w)->next;
    while(w){
        const auto next=node(*s,w)->next;unlink_recycle(*p,*s,w);++*removed;w=next;
    }
    return 1;
}
// 240c0d30: release entire history and clear the watcher head.
int __cdecl clear(void* watcher,unsigned bytes,ClientOverlayPool* p,const ClientOverlayPoolStorage* s,unsigned* removed) noexcept {
    if(!removed)return 0;
    *removed=0;
    if(!valid(watcher,bytes,p,s))return 0;
    while(p->head){unlink_recycle(*p,*s,p->head);write(watcher,0x30,p->head);++*removed;}
    return 1;
}
struct ApplyContext {
    ClientOverlayPool* pool;const ClientOverlayPoolStorage* storage;
    unsigned* removed;bool ok=true;
};
int __cdecl apply(void* watcher,unsigned bytes,void* row,unsigned row_bytes,float time,ClientOverlayPool* p,const ClientOverlayPoolStorage* s,int* wrote,unsigned* removed) noexcept {
    if(!wrote||!removed)return 0;
    *wrote=0;*removed=0;
    if(!valid(watcher,bytes,p,s))return 0;
    const auto* interpolate=SiN_GetClientOverlayInterpolateApi(1,sizeof(ClientOverlayInterpolateApi));
    if(!interpolate)return 0;
    ApplyContext c{p,s,removed,true};
    ClientOverlayInterpolateBindings b{};b.history.context=&c;
    b.history.resolve=[](void* ctx,std::uint32_t w) noexcept -> ClientOverlayHistoryNode* {
        return node(*static_cast<ApplyContext*>(ctx)->storage,w);
    };
    b.prune=[](void* ctx,void* watcher,float cutoff) noexcept {
        auto& c=*static_cast<ApplyContext*>(ctx);
        c.ok=prune(watcher,0x44,c.pool,c.storage,cutoff,c.removed)!=0;
    };
    return interpolate->apply(watcher,bytes,row,row_bytes,time,&b,wrote)&&c.ok;
}
const ClientOverlayPoolApi api{1,sizeof(ClientOverlayPoolApi),&initialize,&record,&prune,&clear,&apply};
}
}
#pragma comment(linker,"/EXPORT:SiN_GetClientOverlayPoolApi=_SiN_GetClientOverlayPoolApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayPoolApi* __cdecl
SiN_GetClientOverlayPoolApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ClientOverlayPoolApi)?&api:nullptr;
}
