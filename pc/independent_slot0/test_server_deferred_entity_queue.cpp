#include "server_deferred_entity_queue.h"
#include <array>
#include <cstdio>

class CBaseEntity { public: unsigned index; unsigned name; };
using namespace sin_independent;
struct World {
    EntityDeletionQueue<2> queue;
    EntityRemovalEnvironment env{};
    std::array<EntityHandleEntry,4> registry{};
    std::array<EntityRemovalRecord,4> records{};
    std::array<CBaseEntity*,4> objects{};
    std::array<unsigned,4> updates{}, named{};
    std::array<unsigned,3> releasedOrder{};
    unsigned released=0, physics=0;
    bool valid=true;
    World() {
        for(unsigned i=1;i<objects.size();++i) {
            objects[i]=new CBaseEntity{i,100+i};
            registry[i].entity=objects[i];
            registry[i].serial=1;
            records[i].extendedFlags=0x20;
        }
        env.deferDepth=2;
    }
    ~World() { for(auto* e:objects) delete e; }
    EntityHandleView View() const noexcept {
        return EntityHandleView(registry.data(),registry.size());
    }
    std::uint32_t Handle(unsigned i) const noexcept {
        return (registry[i].serial<<12)|i;
    }
};
static void Update(CBaseEntity* e,void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    auto i=e->index;
    if(w.env.receivedChainedUpdate || w.env.handleAccessDisabled ||
       !(w.records[i].extendedFlags&kEntityKillMe) ||
       w.updates[i] || w.named[i] ||
       ResolveLiveEntity(w.View(),w.Handle(i),w.env)!=e)
        w.valid=false;
    ++w.updates[i];
    w.env.receivedChainedUpdate=true;
    // Reentrant mutation must not append a second entry.
    if(w.queue.Drain(w.env,w.View(),{}).status!=QueueDrainStatus::Busy)
        w.valid=false;
}
static void ClearName(CBaseEntity* e,void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    auto i=e->index;
    if(w.updates[i]!=1 || w.named[i] || !w.env.receivedChainedUpdate)
        w.valid=false;
    e->name=0;
    ++w.named[i];
}
static bool Defer(CBaseEntity* e,void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    auto i=e->index;
    QueueRemovalHooks hooks{&Update,&ClearName,&w};
    return w.queue.Enqueue(e,w.Handle(i),w.records[i],w.env,w.View(),hooks)
        ==QueueRemovalResult::Queued;
}
static ImmediateRemovalResult Request(World& w,unsigned i) {
    EntityRemovalHooks hooks{nullptr,nullptr,&Defer,&w};
    return TryRemoveImmediate(w.objects[i],w.records[i],w.env,hooks);
}
static void Physics(void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    if(!w.queue.InCleanup() || w.env.handleAccessDisabled || w.released)
        w.valid=false;
    ++w.physics;
}
static void Release(CBaseEntity* e,void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    const auto i=e->index;
    if(!w.queue.InCleanup() || !w.env.handleAccessDisabled ||
       w.physics!=1 || w.updates[i]!=1 || w.named[i]!=1 || e->name ||
       ResolveLiveEntity(w.View(),w.Handle(i),w.env)!=nullptr ||
       w.released>=w.releasedOrder.size())
        w.valid=false;
    w.releasedOrder[w.released++]=i;
    // Independent registry owner invalidates before reclaiming the object.
    w.registry[i].entity=nullptr;
    ++w.registry[i].serial;
    delete e;
    w.objects[i]=nullptr;
}
static int fail(int code,const char* msg) {
    std::fprintf(stderr,"FAIL deferred queue %d: %s\n",code,msg);
    return code;
}
int main() {
    World w;
    QueueRemovalHooks enqueueHooks{&Update,&ClearName,&w};
    if(w.queue.Enqueue(w.objects[1],kInvalidEntityHandle,w.records[1],w.env,w.View(),enqueueHooks)
       !=QueueRemovalResult::BadHandle || w.records[1].extendedFlags!=0x20)
        return fail(1,"invalid handle refused without mutation");
    if(w.queue.Enqueue(w.objects[1],w.Handle(1),w.records[1],w.env,w.View(),{})
       !=QueueRemovalResult::MissingDependency || w.records[1].extendedFlags!=0x20)
        return fail(2,"missing teardown dependencies");
    if(Request(w,1)!=ImmediateRemovalResult::Deferred ||
       Request(w,2)!=ImmediateRemovalResult::Deferred ||
       Request(w,1)!=ImmediateRemovalResult::AlreadyMarked ||
       w.queue.Count()!=2 || !w.valid || !w.objects[1] || !w.objects[2] ||
       w.objects[1]->name || w.objects[2]->name || w.released)
        return fail(3,"deferred routing, mark, update/name and no duplicate");
    if(Request(w,3)!=ImmediateRemovalResult::DeferredRefused ||
       w.records[3].extendedFlags!=0x20 || w.updates[3] || !w.objects[3]->name)
        return fail(4,"full queue must not mark entity");
    if(w.queue.Drain(w.env,w.View(),{}).status!=QueueDrainStatus::MissingRelease ||
       w.queue.Count()!=2) return fail(5,"missing release does not discard queue");
    QueueDrainHooks drain{&Physics,&Release,&w};
    auto result=w.queue.Drain(w.env,w.View(),drain);
    if(result.status!=QueueDrainStatus::Completed || result.released!=2 ||
       result.stale || !w.valid || w.physics!=1 || w.released!=2 ||
       w.releasedOrder[0]!=1 || w.releasedOrder[1]!=2 || w.queue.Count() ||
       w.queue.InCleanup() || w.env.handleAccessDisabled ||
       w.objects[1] || w.objects[2] || !w.objects[3] ||
       w.View().Resolve(0x1001u) || w.View().Resolve(0x1002u))
        return fail(6,"physics-first FIFO actual deletion and guard");

    World stale;
    if(Request(stale,1)!=ImmediateRemovalResult::Deferred ||
       Request(stale,2)!=ImmediateRemovalResult::Deferred)
        return fail(7,"stale fixture queue");
    // Model a separately destroyed entry, then reuse the same index.
    delete stale.objects[1];
    stale.objects[1]=new CBaseEntity{1,777};
    stale.registry[1].entity=stale.objects[1];
    stale.registry[1].serial=2;
    drain.context=&stale;
    auto staleResult=stale.queue.Drain(stale.env,stale.View(),drain);
    if(staleResult.status!=QueueDrainStatus::Completed || staleResult.stale!=1 ||
       staleResult.released!=1 || !stale.valid || stale.releasedOrder[0]!=2 ||
       !stale.objects[1] || stale.objects[1]->name!=777 || stale.objects[2] ||
       stale.queue.Count() || stale.env.handleAccessDisabled)
        return fail(8,"stale generation never deletes replacement");

    std::puts("PASS deferred removal queue: mark/update/name, capacity refusal, FIFO release, physics order, stale generation protection");
    return 0;
}
