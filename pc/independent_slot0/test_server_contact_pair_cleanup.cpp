// Integration fixture: wire the bulk contact traversal to the new
// reciprocal remover, rather than merely testing each in isolation.
#include "server_reciprocal_contact_core.h"
#include <array>
#include <cstdio>
class CBaseEntity { public: unsigned flags=0; };
using namespace sin_independent;

struct Pair {
    CBaseEntity departing{1}, partner{1};
    ContactNode ownRoot{}, remoteRoot{};
    ContactList own{&ownRoot,true}, remote{&remoteRoot,true};
    ContactCleanupState state{};
    std::array<EntityHandleEntry,3> registry{};
    int stamp=17;
    unsigned ownFreed=0, remoteFreed=0, ownDestroyed=0, remoteDestroyed=0;
    unsigned endTouched=0, groundCleared=0;
    bool valid=true;
};
static void EndTouch(CBaseEntity* receiver,CBaseEntity* other,void* ctx) noexcept {
    auto& f=*static_cast<Pair*>(ctx);
    if(receiver!=&f.partner || other!=&f.departing ||
        f.state.cleanupDataObjects) f.valid=false;
    ++f.endTouched;
}
static void RemoveFlags(CBaseEntity* e,std::uint32_t mask,void* ctx) noexcept {
    auto& f=*static_cast<Pair*>(ctx);
    if(e!=&f.departing || mask!=kOnGround) f.valid=false;
    e->flags &= ~mask;
    ++f.groundCleared;
}
static void RemoteFree(ContactNode* n,ContactKind,void* ctx) noexcept {
    auto& f=*static_cast<Pair*>(ctx);
    if(n->next!=&f.remoteRoot || n->previous!=&f.remoteRoot ||
       f.remoteRoot.next!=&f.remoteRoot || f.remoteRoot.previous!=&f.remoteRoot)
        f.valid=false;
    ++f.remoteFreed;
    delete n;
}
static void RemoteDestroy(CBaseEntity* e,ContactNode* root,ContactKind,void* ctx) noexcept {
    auto& f=*static_cast<Pair*>(ctx);
    if(e!=&f.partner || root!=&f.remoteRoot) f.valid=false;
    ++f.remoteDestroyed;
}
static void Notify(CBaseEntity* owner,CBaseEntity* partner,ContactKind kind,void* ctx) noexcept {
    auto& f=*static_cast<Pair*>(ctx);
    if(owner!=&f.departing || partner!=&f.partner) { f.valid=false; return; }
    EntityHandleView view(f.registry.data(),f.registry.size());
    ReciprocalHooks hooks{&EndTouch,&RemoveFlags,&RemoteFree,&RemoteDestroy,&f};
    if(RemoveReciprocalContact(owner,partner,kind,f.remote,f.state,view,hooks)
           !=ReciprocalResult::Removed) f.valid=false;
}
static void OwnFree(ContactNode* n,ContactKind,void* ctx) noexcept {
    auto& f=*static_cast<Pair*>(ctx);
    if(f.remoteFreed!=1) f.valid=false;
    ++f.ownFreed;
    delete n;
}
static void OwnDestroy(CBaseEntity* e,ContactNode* root,ContactKind,void* ctx) noexcept {
    auto& f=*static_cast<Pair*>(ctx);
    if(e!=&f.departing || root!=&f.ownRoot || f.ownFreed!=1 ||
        !f.state.cleanupDataObjects) f.valid=false;
    ++f.ownDestroyed;
}
static int run(ContactKind kind) {
    Pair f;
    const bool touch=kind==ContactKind::Touch;
    f.registry[1].entity=&f.departing;
    f.registry[1].serial=1;
    f.registry[2].entity=&f.partner;
    f.registry[2].serial=2;
    f.ownRoot.next=f.ownRoot.previous=
        new ContactNode{0x2002u,&f.ownRoot,&f.ownRoot,kTouchStarted};
    f.remoteRoot.next=f.remoteRoot.previous=
        new ContactNode{0x1001u,&f.remoteRoot,&f.remoteRoot,kTouchStarted};
    f.state.touchAllocated=touch?2:0;
    f.state.groundAllocated=touch?0:2;
    EntityHandleView view(f.registry.data(),f.registry.size());
    ContactCleanupHooks hooks{&Notify,&OwnFree,&OwnDestroy,&f};
    auto result=RemoveContactList(&f.departing,kind,f.own,f.state,f.stamp,view,hooks);
    if(result!=ContactCleanupResult::Removed || !f.valid ||
       f.ownFreed!=1 || f.remoteFreed!=1 || f.ownDestroyed!=1 ||
       f.remoteDestroyed!=(touch?0u:1u) || f.endTouched!=(touch?1u:0u) ||
       f.groundCleared!=(touch?0u:1u) || f.departing.flags!=(touch?1u:0u) ||
       f.own.present || f.own.root || f.remote.present!=touch ||
       f.remote.root!=(touch?&f.remoteRoot:nullptr) || f.remoteRoot.next!=&f.remoteRoot ||
       f.remoteRoot.previous!=&f.remoteRoot || f.state.touchAllocated ||
       f.state.groundAllocated || !f.state.cleanupDataObjects || f.stamp!=(touch?0:17))
        return 1;
    return 0;
}
int main() {
    if(run(ContactKind::Touch) || run(ContactKind::Ground)) {
        std::fputs("FAIL integrated paired contact cleanup\n",stderr);
        return 1;
    }
    std::puts("PASS bulk and reciprocal removal together: two nodes, correct callbacks, retained touch sentinel, destroyed ground sentinel");
    return 0;
}
