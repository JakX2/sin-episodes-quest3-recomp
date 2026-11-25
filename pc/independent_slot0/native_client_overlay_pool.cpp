#include "native_client_overlay_pool.h"
#include "native_client_overlay_entity.h"
#include "native_client_overlay_event_frame.h"
#include "native_weapon_datatables.h"
#include "native_client_overlay_interpolate.h"
#include <cmath>
#include <cstring>
#include <limits>
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayInterpolateApi* __cdecl
SiN_GetClientOverlayInterpolateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayHistoryApi* __cdecl
SiN_GetClientOverlayHistoryApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayClockedInterpolateApi* __cdecl
SiN_GetClientOverlayClockedInterpolateApi(std::uint32_t,std::uint32_t) noexcept;
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayEventsApi* __cdecl
SiN_GetClientOverlayEventsApi(std::uint32_t,std::uint32_t) noexcept;
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

// Original 240c2230: unlike interpolation prune, retain first three
// entries irrespective of timestamps and recycle any later <= cutoff.
int __cdecl trim(void* watcher,unsigned bytes,ClientOverlayPool* p,const ClientOverlayPoolStorage* s,float cutoff,unsigned* removed) noexcept {
    if(!removed)return 0;
    *removed=0;
    if(!std::isfinite(cutoff)||!valid(watcher,bytes,p,s))return 0;
    unsigned index=0;
    for(auto w=p->head;w;){
        auto* n=node(*s,w);const auto next=n->next;
        if(index>=3&&n->time<=cutoff){unlink_recycle(*p,*s,w);++*removed;}
        ++index;w=next;
    }
    return 1;
}
bool bound_row(void* watcher,unsigned bytes,const void* row,unsigned row_bytes) noexcept {
    return watcher&&bytes>=0x44&&row&&row_bytes>=sizeof(ClientOverlaySample)&&
        read<const void*>(watcher,4)==row&&read<int>(watcher,0x38)==1;
}
// Original 240c21c0/240c21e0. Caller provides the observed original
// current and network clocks; the sample timestamp is independent.
int __cdecl capture(void* watcher,unsigned bytes,const void* row,unsigned row_bytes,float sample_time,float current_time,float network_time,ClientOverlayPool* p,const ClientOverlayPoolStorage* s,unsigned* removed) noexcept {
    if(!removed)return 0;
    *removed=0;
    if(!bound_row(watcher,bytes,row,row_bytes)||!std::isfinite(sample_time)||
       !std::isfinite(current_time)||!std::isfinite(network_time))return 0;
    const float delay=read<float>(watcher,0x3c);
    const float cutoff=static_cast<float>(static_cast<double>(current_time)-delay-2.);
    const auto sample=read<ClientOverlaySample>(row,0);
    if(!std::isfinite(delay)||!std::isfinite(cutoff)||!record(watcher,bytes,p,s,sample_time,&sample,true))return 0;
    write(watcher,8,sample);write(watcher,0x28,network_time);
    return trim(watcher,bytes,p,s,cutoff,removed);
}
// Original 240c1250: clear, three ordinary records at the same current
// clock, copy networked layer to snapshot. Preserve the network time.
int __cdecl reset(void* watcher,unsigned bytes,const void* row,unsigned row_bytes,float current_time,ClientOverlayPool* p,const ClientOverlayPoolStorage* s) noexcept {
    if(!bound_row(watcher,bytes,row,row_bytes)||!std::isfinite(current_time)||!valid(watcher,bytes,p,s)||s->capacity<3)return 0;
    const auto sample=read<ClientOverlaySample>(row,0);
    if(!finite(sample))return 0;
    unsigned removed=0;
    if(!clear(watcher,bytes,p,s,&removed))return 0;
    for(int i=0;i<3;++i)if(!record(watcher,bytes,p,s,current_time,&sample,false))return 0;
    write(watcher,8,sample);return 1;
}
// Original 240c11d0: restore all 32 bytes, not just interpolated fields.
int __cdecl restore(void* watcher,unsigned bytes,void* row,unsigned row_bytes) noexcept {
    if(!bound_row(watcher,bytes,row,row_bytes))return 0;
    const auto snapshot=read<ClientOverlaySample>(watcher,8);write(row,0,snapshot);return 1;
}
const ClientOverlayCaptureApi capture_api{1,sizeof(ClientOverlayCaptureApi),&trim,&capture,&reset,&restore};
const ClientOverlayPoolApi api{1,sizeof(ClientOverlayPoolApi),&initialize,&record,&prune,&clear,&apply};
}

namespace {
struct EntityVectors {
    unsigned char* layers;
    unsigned char* watchers;
    unsigned count;
};
bool range_overlaps(std::uint64_t a,std::uint64_t an,std::uint64_t b,std::uint64_t bn) noexcept {
    return a<b+bn&&b<a+an;
}
bool ptr_overlap(const void* a,unsigned an,const void* b,unsigned bn) noexcept {
    return range_overlaps(reinterpret_cast<std::uintptr_t>(a),an,reinterpret_cast<std::uintptr_t>(b),bn);
}
enum class EntityOp {Capture,Reset,Render,Restore};
// Validate all active chains, bindings and exclusive storage before any
// mutation. In particular, do not allow two pools to resolve the same word.
bool preflight(void* actor,unsigned bytes,const ClientOverlayEntityBindings* b,EntityOp op,
    EntityVectors& v,ClientOverlayEntityResult& result) noexcept {
    if(!actor||bytes<0x7cc||!b||b->count>overlay_limit||(!b->slots&&b->count))return false;
    const auto layers=read<LayerVector>(actor,0x7a4),watchers=read<LayerVector>(actor,0x7b8);
    if(layers.count<0||layers.count!=watchers.count||static_cast<unsigned>(layers.count)!=b->count||
       layers.capacity<layers.count||watchers.capacity<watchers.count||
       (b->count&&(!layers.memory||!watchers.memory))||
       bytes<0x7cc+b->count*4)return false;
    v={layers.memory,watchers.memory,b->count};
    if(!v.count)return true;
    if(ptr_overlap(v.layers,v.count*0x20,v.watchers,v.count*0x44)||
       ptr_overlap(actor,bytes,v.layers,v.count*0x20)||ptr_overlap(actor,bytes,v.watchers,v.count*0x44))return false;
    for(unsigned i=0;i<v.count;++i){
        result.failed_slot=static_cast<int>(i);
        const auto& s=b->slots[i];auto* w=v.watchers+i*0x44;auto* row=v.layers+i*0x20;
        if(!bound_row(w,0x44,row,0x20)||!valid(w,0x44,s.pool,&s.storage)||
           !std::isfinite(read<float>(w,0x3c)))return false;
        const unsigned extent=s.storage.capacity*stride;
        if(ptr_overlap(s.storage.nodes,extent,s.pool,sizeof(*s.pool))||
           ptr_overlap(actor,bytes,s.storage.nodes,extent)||ptr_overlap(actor,bytes,s.pool,sizeof(*s.pool))||
           ptr_overlap(v.layers,v.count*0x20,s.storage.nodes,extent)||ptr_overlap(v.layers,v.count*0x20,s.pool,sizeof(*s.pool))||
           ptr_overlap(v.watchers,v.count*0x44,s.storage.nodes,extent)||ptr_overlap(v.watchers,v.count*0x44,s.pool,sizeof(*s.pool)))return false;
        for(unsigned j=0;j<i;++j){
            const auto& t=b->slots[j];const unsigned other=t.storage.capacity*stride;
            if(range_overlaps(s.storage.first_word,extent,t.storage.first_word,other)||
               ptr_overlap(s.storage.nodes,extent,t.storage.nodes,other)||
               ptr_overlap(s.pool,sizeof(*s.pool),t.pool,sizeof(*t.pool))||
               ptr_overlap(s.pool,sizeof(*s.pool),t.storage.nodes,other)||
               ptr_overlap(s.storage.nodes,extent,t.pool,sizeof(*t.pool)))return false;
        }
        if(op==EntityOp::Capture&&(!s.pool->free_head||!finite(read<ClientOverlaySample>(row,0))))return false;
        if(op==EntityOp::Reset&&(s.storage.capacity<3||!finite(read<ClientOverlaySample>(row,0))))return false;
        if(op==EntityOp::Render){
            for(auto word=s.pool->head;word;){
                auto* n=node(s.storage,word);if(!finite(n->sample))return false;word=n->next;
            }
        }
    }
    result.failed_slot=-1;return true;
}
bool begin(ClientOverlayEntityResult* out) noexcept {
    if(!out)return false;
    *out={};out->failed_slot=-1;return true;
}
int __cdecl entity_capture(void* actor,unsigned bytes,float sample_time,float current_time,float network_time,
    const ClientOverlayEntityBindings* b,ClientOverlayEntityResult* out) noexcept {
    if(!begin(out)||!std::isfinite(sample_time)||!std::isfinite(current_time)||!std::isfinite(network_time))return 0;
    EntityVectors v{};
    if(!preflight(actor,bytes,b,EntityOp::Capture,v,*out))return 0;
    for(unsigned i=0;i<v.count;++i){
        if(!std::isfinite(static_cast<float>(static_cast<double>(current_time)-read<float>(v.watchers+i*0x44,0x3c)-2.))){
            out->failed_slot=static_cast<int>(i);return 0;
        }
    }
    for(unsigned i=0;i<v.count;++i){
        const auto& s=b->slots[i];unsigned removed=0;
        if(!capture(v.watchers+i*0x44,0x44,v.layers+i*0x20,0x20,sample_time,current_time,network_time,s.pool,&s.storage,&removed)){
            out->failed_slot=static_cast<int>(i);return 0;
        }
        ++out->processed;out->recycled+=removed;
    }
    return 1;
}
int __cdecl entity_reset(void* actor,unsigned bytes,float time,const ClientOverlayEntityBindings* b,ClientOverlayEntityResult* out) noexcept {
    if(!begin(out)||!std::isfinite(time))return 0;
    EntityVectors v{};
    if(!preflight(actor,bytes,b,EntityOp::Reset,v,*out))return 0;
    for(unsigned i=0;i<v.count;++i){
        const auto& s=b->slots[i];const unsigned old=s.pool->count;
        if(!reset(v.watchers+i*0x44,0x44,v.layers+i*0x20,0x20,time,s.pool,&s.storage)){out->failed_slot=static_cast<int>(i);return 0;}
        ++out->processed;out->recycled+=old;
    }
    return 1;
}
int __cdecl entity_restore(void* actor,unsigned bytes,const ClientOverlayEntityBindings* b,ClientOverlayEntityResult* out) noexcept {
    if(!begin(out))return 0;
    EntityVectors v{};
    if(!preflight(actor,bytes,b,EntityOp::Restore,v,*out))return 0;
    for(unsigned i=0;i<v.count;++i){
        if(!restore(v.watchers+i*0x44,0x44,v.layers+i*0x20,0x20)){out->failed_slot=static_cast<int>(i);return 0;}
        ++out->processed;++out->writes;
    }
    return 1;
}
struct EntityRender {
    EntityVectors v;
    const ClientOverlayEntityBindings* b;
    const ClientOverlayInterpolationClock* clock;
    const ClientOverlayClockedInterpolateApi* clocked;
    ClientOverlayEntityResult* result;
    bool done[overlay_limit]{};
    bool ok=true;
};
bool render_one(EntityRender& c,unsigned i,float time) noexcept {
    const auto& s=c.b->slots[i];
    unsigned removed=0;int wrote=0,raw=0;
    ApplyContext local{s.pool,&s.storage,&removed,true};
    ClientOverlayInterpolateBindings bindings{};bindings.history.context=&local;
    bindings.history.resolve=[](void* p,std::uint32_t w) noexcept -> ClientOverlayHistoryNode* {
        return node(*static_cast<ApplyContext*>(p)->storage,w);
    };
    bindings.prune=[](void* p,void* w,float cutoff) noexcept {
        auto& c=*static_cast<ApplyContext*>(p);c.ok=prune(w,0x44,c.pool,c.storage,cutoff,c.removed)!=0;
    };
    if(!c.clocked->apply(c.v.watchers+i*0x44,0x44,c.v.layers+i*0x20,0x20,time,c.clock,&bindings,&wrote,&raw)||!local.ok){
        c.ok=false;c.result->failed_slot=static_cast<int>(i);return false;
    }
    c.done[i]=true;++c.result->processed;c.result->writes+=wrote;c.result->raw_copies+=raw;c.result->recycled+=removed;
    return true;
}
int entity_interpolate(void* actor,unsigned bytes,float time,const ClientOverlayInterpolationClock* clock,
    const ClientOverlayEntityBindings* b,ClientOverlayEntityResult* out,bool complete) noexcept {
    if(!begin(out)||!std::isfinite(time)||!clock||!std::isfinite(clock->last_network_time)||!b||!b->looping)return 0;
    EntityVectors v{};
    if(!preflight(actor,bytes,b,EntityOp::Render,v,*out))return 0;
    for(unsigned i=0;i<v.count;++i){
        const auto delay=read<float>(v.watchers+i*0x44,0x3c);
        if(!std::isfinite(static_cast<float>(static_cast<double>(time)-delay-0.05f))){out->failed_slot=static_cast<int>(i);return 0;}
    }
    const auto* history=SiN_GetClientOverlayHistoryApi(1,sizeof(ClientOverlayHistoryApi));
    const auto* clocked=SiN_GetClientOverlayClockedInterpolateApi(1,sizeof(ClientOverlayClockedInterpolateApi));
    if(!history||!clocked)return 0;
    EntityRender c{v,b,clock,clocked,out};
    ClientOverlayHistoryBindings hb{};hb.context=&c;
    hb.resolve=[](void* p,std::uint32_t w) noexcept -> ClientOverlayHistoryNode* {
        auto& c=*static_cast<EntityRender*>(p);
        for(unsigned i=0;i<c.v.count;++i)if(auto* n=node(c.b->slots[i].storage,w))return n;
        return nullptr;
    };
    hb.looping=[](void* p,void* a,int seq) noexcept -> bool {
        auto& c=*static_cast<EntityRender*>(p);return c.b->looping(c.b->context,a,seq);
    };
    // Original 240c19a0 calls the changed watcher's interpolation virtual
    // immediately, then updates the event baseline. Preserve that boundary.
    hb.interpolate=[](void* p,void* w,float t) noexcept {
        auto& c=*static_cast<EntityRender*>(p);
        if(!c.ok)return;
        const auto i=static_cast<unsigned>((static_cast<unsigned char*>(w)-c.v.watchers)/0x44);
        if(i>=c.v.count){c.ok=false;return;}
        render_one(c,i,t);
    };
    if(!history->correct(actor,bytes,time,&hb,&out->repairs)||!c.ok)return 0;
    // Port-owned frame composition: evaluate each other active watcher once.
    if(complete)for(unsigned i=0;i<v.count;++i)if(!c.done[i]&&!render_one(c,i,time))return 0;
    return 1;
}
int __cdecl entity_render(void* actor,unsigned bytes,float time,const ClientOverlayInterpolationClock* clock,
    const ClientOverlayEntityBindings* b,ClientOverlayEntityResult* out) noexcept {
    return entity_interpolate(actor,bytes,time,clock,b,out,true);
}

struct EventFrameLock {
    const ClientOverlayEventFrameBindings* b;
    void* model;
    ~EventFrameLock(){b->unlock(b->context,model);}
};
int __cdecl event_frame(void* actor,unsigned bytes,float frame_time,float time,const ClientOverlayInterpolationClock* clock,
    const ClientOverlayEventFrameBindings* b,ClientOverlayEventFrameResult* out) noexcept {
    if(!out)return 0;
    *out={};out->repair.failed_slot=out->events.failed_slot=out->events.aborted_slot=-1;
    if(!actor||bytes<0x7cc||!std::isfinite(frame_time))return 0;
    if(frame_time==0.f){out->skipped=EventFrameZeroTime;return 1;}
    if(!b||!b->resolve_model||!b->lock||!b->unlock||!b->sequence_count||!b->base_events||
       !b->entity.looping||!b->events.sequence||!b->events.emit||
       !clock||!std::isfinite(clock->last_network_time)||!std::isfinite(time))return 0;
    void* model=nullptr;
    const int ready=b->resolve_model(b->context,actor,&model);
    if(ready==0){out->skipped=EventFrameNoModel;return 1;}
    if(ready!=1||!model)return 0;
    // Port safety guard before invoking external engine services.
    EntityVectors v{};
    if(!preflight(actor,bytes,&b->entity,EntityOp::Render,v,out->repair))return 0;
    b->lock(b->context,model);
    EventFrameLock guard{b,model};
    int count=0;
    if(!b->sequence_count(b->context,model,&count)||count<0)return 0;
    b->base_events(b->context,actor);
    // 240c19a0 only interpolates changed sequences; do not evaluate the
    // rest of the watchers again, as the general frame API would.
    if(!entity_interpolate(actor,bytes,time,clock,&b->entity,&out->repair,false))return 0;
    const auto* events=SiN_GetClientOverlayEventsApi(1,sizeof(ClientOverlayEventsApi));
    return events&&events->dispatch(actor,bytes,frame_time,count,&b->events,&out->events);
}
const ClientOverlayEventFrameApi event_frame_api{1,sizeof(ClientOverlayEventFrameApi),&event_frame};

const ClientOverlayEntityFrameApi entity_api{1,sizeof(ClientOverlayEntityFrameApi),&entity_capture,&entity_reset,&entity_render,&entity_restore};
}

}
#pragma comment(linker,"/EXPORT:SiN_GetClientOverlayPoolApi=_SiN_GetClientOverlayPoolApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayPoolApi* __cdecl
SiN_GetClientOverlayPoolApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ClientOverlayPoolApi)?&api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetClientOverlayCaptureApi=_SiN_GetClientOverlayCaptureApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayCaptureApi* __cdecl
SiN_GetClientOverlayCaptureApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ClientOverlayCaptureApi)?&capture_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetClientOverlayEntityFrameApi=_SiN_GetClientOverlayEntityFrameApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayEntityFrameApi* __cdecl
SiN_GetClientOverlayEntityFrameApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ClientOverlayEntityFrameApi)?&entity_api:nullptr;
}

#pragma comment(linker,"/EXPORT:SiN_GetClientOverlayEventFrameApi=_SiN_GetClientOverlayEventFrameApi")
extern "C" __declspec(dllexport) const sin_port::native_weapon_tables::ClientOverlayEventFrameApi* __cdecl
SiN_GetClientOverlayEventFrameApi(std::uint32_t version,std::uint32_t size) noexcept {
    using namespace sin_port::native_weapon_tables;
    return version==1&&size==sizeof(ClientOverlayEventFrameApi)?&event_frame_api:nullptr;
}
