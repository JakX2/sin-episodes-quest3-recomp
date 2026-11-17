// End-to-end synthetic Win32 lifecycle test. Connect the earlier release,
// contact cleanup, reciprocal unlink, and immediate deletion components.
// No game exports, native retail object layout beyond the edict prefix,
// commercial code, or private DLLs are involved.
#include "server_entity_release_core.h"
#include "server_reciprocal_contact_core.h"
#include "server_entity_immediate_removal_core.h"
#include <array>
#include <cstdio>

class CBaseHandle { public: unsigned value=0; };
class CBaseEntity { public: unsigned flags=1; };
struct edict_t {
    std::uint32_t flags=0xFF, serial=7;
    IServerNetworkable* networkable=nullptr;
    sin_independent::ServerUnknownAbi* unknown=nullptr;
};
static_assert(offsetof(edict_t,unknown)==0x0c,"retail edict prefix");
using namespace sin_independent;
class Unknown final : public ServerUnknownAbi {
public:
    explicit Unknown(CBaseEntity* p):base(p){}
    void __thiscall SetRefEHandle(const CBaseHandle& h) noexcept override { handle=h; }
    const CBaseHandle& __thiscall GetRefEHandle() const noexcept override { return handle; }
    ICollideable* __thiscall GetCollideable() noexcept override { return nullptr; }
    IServerNetworkable* __thiscall GetNetworkable() noexcept override { return nullptr; }
    CBaseEntity* __thiscall GetBaseEntity() noexcept override { return base; }
    CBaseEntity* base;
    CBaseHandle handle;
};
struct Pair {
    ContactNode localRoot{}, remoteRoot{};
    ContactList local{&localRoot,true}, remote{&remoteRoot,true};
    void Populate() {
        localRoot.next=localRoot.previous=new ContactNode{0x2002u,&localRoot,&localRoot,kTouchStarted};
        remoteRoot.next=remoteRoot.previous=new ContactNode{0x1001u,&remoteRoot,&remoteRoot,kTouchStarted};
    }
};
struct World {
    CBaseEntity* departing=new CBaseEntity;
    CBaseEntity partner{};
    Unknown unknown{departing};
    edict_t edict{0xFF,7,nullptr,&unknown};
    Pair touch{}, ground{};
    std::array<EntityHandleEntry,3> entries{};
    ContactCleanupState contacts{2,2,true};
    EntityRemovalRecord record{};
    EntityRemovalEnvironment removal{};
    int stamp=123;
    unsigned stage=0, released=0, ended=0, cleared=0, roots=0, updates=0, destroyed=0;
    bool valid=true;
    World() {
        touch.Populate(); ground.Populate();
        entries[1].entity=departing; entries[1].serial=1;
        entries[2].entity=&partner; entries[2].serial=2;
    }
    Pair& For(ContactKind kind) { return kind==ContactKind::Touch?touch:ground; }
};
static void End(CBaseEntity* receiver,CBaseEntity* argument,void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    if(receiver!=&w.partner || argument!=w.departing || w.stage!=1 ||
       w.contacts.cleanupDataObjects) w.valid=false;
    ++w.ended;
}
static void RemoveFlags(CBaseEntity* e,std::uint32_t mask,void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    if(e!=w.departing || mask!=1 || w.stage!=2) w.valid=false;
    e->flags &= ~mask; ++w.cleared;
}
static void FreeNode(ContactNode* n,ContactKind,void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    ++w.released; delete n;
}
static void DestroyRoot(CBaseEntity*,ContactNode*,ContactKind,void* ctx) noexcept {
    // Roots are fixture-owned sentinels, not heap contact nodes.
    ++static_cast<World*>(ctx)->roots;
}
static void Notify(CBaseEntity* e,CBaseEntity* p,ContactKind kind,void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    if(e!=w.departing || p!=&w.partner) {w.valid=false; return;}
    EntityHandleView view(w.entries.data(),w.entries.size());
    ReciprocalHooks hooks{&End,&RemoveFlags,&FreeNode,&DestroyRoot,&w};
    if(RemoveReciprocalContact(e,p,kind,w.For(kind).remote,w.contacts,view,hooks)
       !=ReciprocalResult::Removed) w.valid=false;
}
static void Cleanup(CBaseEntity* e,ContactKind kind,World& w) noexcept {
    if(e!=w.departing || w.edict.unknown || w.edict.flags ||
       w.stage!=(kind==ContactKind::Touch?0u:1u)) w.valid=false;
    ++w.stage;
    EntityHandleView view(w.entries.data(),w.entries.size());
    ContactCleanupHooks hooks{&Notify,&FreeNode,&DestroyRoot,&w};
    if(RemoveContactList(e,kind,w.For(kind).local,w.contacts,w.stamp,view,hooks)
       !=ContactCleanupResult::Removed) w.valid=false;
}
static void __cdecl Touch(CBaseEntity* e,void* ctx) noexcept {
    Cleanup(e,ContactKind::Touch,*static_cast<World*>(ctx));
}
static void __cdecl Ground(CBaseEntity* e,void* ctx) noexcept {
    Cleanup(e,ContactKind::Ground,*static_cast<World*>(ctx));
}
static void Update(CBaseEntity* e,void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    if(e!=w.departing || w.stage!=3 || w.released!=4 || w.stamp ||
       w.contacts.touchAllocated || w.contacts.groundAllocated ||
       e->flags&kOnGround || !(w.record.extendedFlags&kEntityKillMe) ||
       w.removal.handleAccessDisabled) w.valid=false;
    ++w.updates;
    w.removal.receivedChainedUpdate=true;
}
static void Delete(CBaseEntity* e,void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    EntityHandleView view(w.entries.data(),w.entries.size());
    if(w.updates!=1 || !w.removal.handleAccessDisabled ||
       ResolveLiveEntity(view,0x1001u,w.removal)) w.valid=false;
    w.entries[1].entity=nullptr; w.entries[1].serial=2;
    w.unknown.base=nullptr;
    ++w.destroyed;
    delete e; w.departing=nullptr;
}
static void __cdecl Immediate(CBaseEntity* e,void* ctx) noexcept {
    auto& w=*static_cast<World*>(ctx);
    if(w.stage!=2) w.valid=false;
    ++w.stage;
    EntityRemovalHooks hooks{&Update,&Delete,nullptr,&w};
    if(TryRemoveImmediate(e,w.record,w.removal,hooks)!=ImmediateRemovalResult::Destroyed)
        w.valid=false;
}
int main() {
    World w;
    ServerGameEntsConversionCore conversion;
    EntityReleaseHooks hooks{&Touch,&Ground,&Immediate,&w};
    if(TryReleaseContainingEntity(conversion,&w.edict,hooks)
          !=EntityReleaseStatus::DetachedAndHooksInvoked ||
       !w.valid || w.stage!=3 || w.departing || w.edict.unknown ||
       w.edict.flags || w.edict.serial!=7 || w.released!=4 ||
       w.ended!=1 || w.cleared!=1 || w.roots!=3 || w.updates!=1 ||
       w.destroyed!=1 || w.removal.handleAccessDisabled ||
       w.touch.local.present || w.ground.local.present ||
       !w.touch.remote.present || w.ground.remote.present ||
       w.entries[1].entity || w.entries[1].serial!=2) {
        std::fputs("FAIL full synthetic entity teardown\n",stderr);
        return 1;
    }
    if(TryReleaseContainingEntity(conversion,&w.edict,hooks)
        !=EntityReleaseStatus::NoAssociatedEntity || w.destroyed!=1) return 2;
    std::puts("PASS synthetic full entity teardown: detach, touch/ground reciprocal unlink, UpdateOnRemove, actual delete, no duplicate");
    return 0;
}
