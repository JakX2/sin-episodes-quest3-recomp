// Synthetic tests of original-guided independent touch/ground cleanup.
#include "server_contact_cleanup_core.h"
#include <array>
#include <cstdio>

class CBaseEntity { public: int id; };

using namespace sin_independent;
struct Trace {
    ContactCleanupState* state;
    ContactList* list;
    ContactKind kind;
    CBaseEntity* owner;
    std::size_t expectedCount;
    bool initialCleanup;
    bool clearPresence = false;
    bool valid = true;
    unsigned notifications = 0;
    unsigned releases = 0;
    unsigned destroyed = 0;
    std::array<CBaseEntity*,3> partners{};
};

static void Notify(CBaseEntity* owner, CBaseEntity* partner,
                   ContactKind kind, void* context) noexcept {
    auto& t = *static_cast<Trace*>(context);
    auto count = kind == ContactKind::Touch ?
        t.state->touchAllocated : t.state->groundAllocated;
    if (owner != t.owner || kind != t.kind ||
        count != t.expectedCount - t.releases ||
        t.state->cleanupDataObjects !=
            (kind == ContactKind::Touch ? false : t.initialCleanup) ||
        t.notifications != t.releases ||
        t.notifications >= t.partners.size()) {
        t.valid = false;
        return;
    }
    t.partners[t.notifications++] = partner;
}

static void Release(ContactNode* node, ContactKind kind,
                    void* context) noexcept {
    auto& t = *static_cast<Trace*>(context);
    auto count = kind == ContactKind::Touch ?
        t.state->touchAllocated : t.state->groundAllocated;
    if (kind != t.kind || ++t.releases != t.notifications ||
        count != t.expectedCount - t.releases)
        t.valid = false;
    // Destroy the current node; traversal must already have saved its next.
    delete node;
    if (t.clearPresence) t.list->present = false;
}

static void DestroyRoot(CBaseEntity* owner, ContactNode* root,
                        ContactKind kind, void* context) noexcept {
    auto& t = *static_cast<Trace*>(context);
    if (owner != t.owner || kind != t.kind || root != t.list->root ||
        t.notifications != t.releases ||
        t.state->cleanupDataObjects != t.initialCleanup)
        t.valid = false;
    ++t.destroyed;
    // Sentinel is separately owned; it is not included in node count.
}

static int fail(int code, const char* reason) {
    std::fprintf(stderr,"FAIL contact cleanup %d: %s\n",code,reason);
    return code;
}

int main() {
    CBaseEntity owner{1}, partner{2}, reused{3};
    std::array<EntityHandleEntry,4> entries{};
    entries[1].entity = &partner;
    entries[1].serial = 3;
    entries[2].entity = &reused;
    entries[2].serial = 7;
    EntityHandleView registry(entries.data(),entries.size());

    for (auto kind : {ContactKind::Touch,ContactKind::Ground}) {
        ContactNode root{};
        auto* last = new ContactNode{kInvalidEntityHandle,&root};
        auto* stale = new ContactNode{0x6002u,last};
        auto* first = new ContactNode{0x3001u,stale};
        root.next = first;
        ContactList list{&root,true};
        ContactCleanupState state{5,6,true};
        int stamp = 42;
        Trace t{&state,&list,kind,&owner,
                kind == ContactKind::Touch ? 5u : 6u,true};
        ContactCleanupHooks hooks{&Notify,&Release,&DestroyRoot,&t};
        if (RemoveContactList(&owner,kind,list,state,stamp,registry,{})
                != ContactCleanupResult::InvalidInput ||
            list.root != &root || !list.present || stamp != 42 ||
            t.notifications != 0)
            return fail(1,"missing hooks must not mutate list");
        if (RemoveContactList(&owner,kind,list,state,stamp,registry,hooks)
                != ContactCleanupResult::Removed ||
            !t.valid || t.notifications != 3 || t.releases != 3 ||
            t.destroyed != 1 || t.partners[0] != &partner ||
            t.partners[1] != nullptr || t.partners[2] != nullptr ||
            list.root || list.present || !state.cleanupDataObjects ||
            state.touchAllocated != (kind == ContactKind::Touch ? 2u : 5u) ||
            state.groundAllocated != (kind == ContactKind::Ground ? 3u : 6u) ||
            stamp != (kind == ContactKind::Touch ? 0 : 42))
            return fail(2,"ordered touch/ground removal and stale handles");
        if (RemoveContactList(&owner,kind,list,state,stamp,registry,hooks)
                != ContactCleanupResult::NoList || t.releases != 3)
            return fail(3,"empty list repeat");
    }

    // Missing list follows the original touch-stamp reset; ground leaves it.
    ContactList absent{};
    ContactCleanupState state{0,0,false};
    int stamp = 100;
    if (RemoveContactList(&owner,ContactKind::Ground,absent,state,stamp,registry,{})
            != ContactCleanupResult::NoList || stamp != 100 ||
        RemoveContactList(&owner,ContactKind::Touch,absent,state,stamp,registry,{})
            != ContactCleanupResult::NoList || stamp != 0)
        return fail(4,"absent list stamp behavior");

    // A non-sentinel cycle or insufficient count refuses before any mutation.
    ContactNode root{}, loop{};
    root.next = &loop;
    loop.next = &loop;
    ContactList list{&root,true};
    state.touchAllocated = 1;
    stamp = 12;
    Trace t{&state,&list,ContactKind::Touch,&owner,1,false};
    ContactCleanupHooks hooks{&Notify,&Release,&DestroyRoot,&t};
    if (RemoveContactList(&owner,ContactKind::Touch,list,state,stamp,registry,hooks)
            != ContactCleanupResult::InvalidList ||
        state.touchAllocated != 1 || stamp != 12 ||
        t.notifications || !list.present || list.root != &root)
        return fail(5,"reject cycle before mutation");
    loop.next = &root;
    state.touchAllocated = 0;
    if (RemoveContactList(&owner,ContactKind::Touch,list,state,stamp,registry,hooks)
            != ContactCleanupResult::InvalidList || t.notifications)
        return fail(6,"reject undercounted chain");

    // Empty sentinel handled without freeing it; prior suppression restored.
    root.next = &root;
    t.expectedCount = 0;
    if (RemoveContactList(&owner,ContactKind::Touch,list,state,stamp,registry,hooks)
            != ContactCleanupResult::Removed || !t.valid || t.destroyed != 1 ||
        state.cleanupDataObjects || stamp != 0)
        return fail(7,"empty sentinel / prior cleanup suppression");

    // A callback can clear presence. The original final recheck avoids a
    // second data-object destruction; a null link also terminates traversal.
    root.next = new ContactNode{0x3001u,nullptr};
    list = {&root,true};
    state.groundAllocated = 1;
    stamp = 11;
    Trace final{&state,&list,ContactKind::Ground,&owner,1,false,true};
    hooks.context = &final;
    if (RemoveContactList(&owner,ContactKind::Ground,list,state,stamp,registry,hooks)
            != ContactCleanupResult::Removed || !final.valid ||
        final.releases != 1 || final.destroyed || list.root ||
        state.groundAllocated || stamp != 11)
        return fail(8,"presence recheck and null termination");

    std::puts("PASS independent contact cleanup traversal, handle resolution, release order, counters, suppression and guards");
    return 0;
}
