#include "server_entity_immediate_removal_core.h"
#include <array>
#include <cstdio>

class CBaseEntity { public: int marker=7; };
using namespace sin_independent;

struct Fixture {
    EntityRemovalRecord record{0x10};
    EntityRemovalEnvironment env{};
    std::array<EntityHandleEntry,2> registry{};
    CBaseEntity* entity=nullptr;
    unsigned updates=0, deletes=0, deferred=0;
    bool valid=true;
    Fixture() {
        entity=new CBaseEntity;
        registry[1].entity=entity;
        registry[1].serial=1;
    }
    ~Fixture() { delete entity; }
};
static constexpr std::uint32_t handle=0x1001u;
static void Update(CBaseEntity* e,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    EntityHandleView view(f.registry.data(),f.registry.size());
    if(e!=f.entity || !(f.record.extendedFlags&kEntityKillMe) ||
       f.env.receivedChainedUpdate || f.env.handleAccessDisabled ||
       ResolveLiveEntity(view,handle,f.env)!=e || f.deletes || f.updates)
        f.valid=false;
    ++f.updates;
    f.env.receivedChainedUpdate=true;
    // Reentrant attempt must see the already-marked bit.
    if(TryRemoveImmediate(e,f.record,f.env,{})!=ImmediateRemovalResult::AlreadyMarked)
        f.valid=false;
}
static void Destroy(CBaseEntity* e,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    EntityHandleView view(f.registry.data(),f.registry.size());
    if(e!=f.entity || !f.env.handleAccessDisabled ||
       !f.env.receivedChainedUpdate || f.updates!=1 ||
       ResolveLiveEntity(view,handle,f.env)!=nullptr)
        f.valid=false;
    ++f.deletes;
    f.registry[1].entity=nullptr;
    f.registry[1].serial=2;
    delete e;
    f.entity=nullptr;
}
static bool Defer(CBaseEntity* e,void* ctx) noexcept {
    auto& f=*static_cast<Fixture*>(ctx);
    if(e!=f.entity || !f.env.deferDepth ||
       f.env.handleAccessDisabled || f.record.extendedFlags!=0x10)
        f.valid=false;
    // A synthetic deferred adapter owns the marker and queue. It is
    // deliberately NOT represented as a reconstructed retail delete list.
    f.record.extendedFlags|=kEntityKillMe;
    ++f.deferred;
    return true;
}
static int fail(int code,const char* msg) {
    std::fprintf(stderr,"FAIL immediate removal %d: %s\n",code,msg);
    return code;
}
int main() {
    Fixture f;
    EntityRemovalHooks hooks{&Update,&Destroy,&Defer,&f};
    if(TryRemoveImmediate(nullptr,f.record,f.env,hooks)!=ImmediateRemovalResult::NoEntity)
        return fail(1,"null guard");
    if(TryRemoveImmediate(f.entity,f.record,f.env,{})!=ImmediateRemovalResult::MissingDependency ||
       f.record.extendedFlags!=0x10 || f.updates || f.deletes)
        return fail(2,"missing callbacks");
    f.env.handleAccessDisabled=true;
    if(TryRemoveImmediate(f.entity,f.record,f.env,hooks)!=ImmediateRemovalResult::DestructionInProgress ||
       f.record.extendedFlags!=0x10 || f.updates)
        return fail(3,"nested destructor guard");
    f.env.handleAccessDisabled=false;
    f.env.receivedChainedUpdate=true;
    if(TryRemoveImmediate(f.entity,f.record,f.env,hooks)!=ImmediateRemovalResult::Destroyed ||
       !f.valid || f.entity || f.updates!=1 || f.deletes!=1 || f.deferred ||
       f.env.handleAccessDisabled || f.record.extendedFlags!=0x11)
        return fail(4,"mark, UpdateOnRemove, guarded destructor");
    EntityHandleView after(f.registry.data(),f.registry.size());
    if(ResolveLiveEntity(after,handle,f.env)!=nullptr)
        return fail(5,"registry invalidated by actual deletion");

    Fixture d;
    EntityRemovalHooks deferHooks{&Update,&Destroy,&Defer,&d};
    d.env.deferDepth=2;
    if(TryRemoveImmediate(d.entity,d.record,d.env,{})!=ImmediateRemovalResult::MissingDependency ||
       d.record.extendedFlags!=0x10)
        return fail(6,"missing deferred adapter");
    if(TryRemoveImmediate(d.entity,d.record,d.env,deferHooks)!=ImmediateRemovalResult::Deferred ||
       !d.valid || !d.entity || d.deferred!=1 || d.updates || d.deletes || d.env.deferDepth!=2)
        return fail(7,"nested semaphore uses deferred path only");
    if(TryRemoveImmediate(d.entity,d.record,d.env,deferHooks)!=ImmediateRemovalResult::AlreadyMarked ||
       d.deferred!=1)
        return fail(8,"deferred entity cannot be queued twice");
    std::puts("PASS immediate removal: mark before update, reentry, destructor handle guard, real delete, deferred routing");
    return 0;
}
