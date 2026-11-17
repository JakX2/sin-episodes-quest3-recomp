#pragma once
// Independent gameplay-state model for the original MarkEntitiesAsTouching
// wrapper and its two directed touch-link registration calls. Not an ABI
// overlay of retail CBaseEntity, trace_t, or touchlink_t.
#include "server_reciprocal_contact_core.h"
#include <limits>

namespace sin_independent {
struct ContactPoint { float x=0,y=0,z=0; };
struct TouchTraceState {
    ContactPoint end{};
    float fraction=1.0f;
};
struct TouchEntity {
    CBaseEntity* entity=nullptr;
    std::uint32_t handle=kInvalidEntityHandle;
    std::uint32_t parent=kInvalidEntityHandle;
    bool dirtyOrigin=false;
    bool dontTouch=false;
    bool trigger=false;
    bool solid=true;
    bool volumeContents=false;
    bool markedForDeletion=false;
    ContactPoint origin{};
    int touchStamp=0;
    ContactList contacts{};
};
struct TouchMarkHooks {
    bool (*refreshOrigin)(TouchEntity&,void*) noexcept=nullptr;
    ContactNode* (*createNode)(void*) noexcept=nullptr;
    void (*freeUnlinkedNode)(ContactNode*,void*) noexcept=nullptr;
    void (*startTouch)(CBaseEntity*,CBaseEntity*,void*) noexcept=nullptr;
    void (*touch)(CBaseEntity*,CBaseEntity*,void*) noexcept=nullptr;
    void* context=nullptr;
    bool Complete() const noexcept {
        return refreshOrigin && createNode && freeUnlinkedNode && startTouch && touch;
    }
};
enum class TouchMarkResult { Recorded, Ignored, InvalidInput, InvalidList, AllocationFailed, OriginUnavailable };

// Callbacks operate on live owned state and must not mutate either list,
// registry or entity lifetime during this operation. Instead of the
// original data-object allocator, both roots are supplied by the owner.
inline TouchMarkResult MarkTouchPair(
    TouchEntity& first, TouchEntity& second, ContactCleanupState& state,
    const EntityHandleView& registry, TouchTraceState& trace,
    const TouchMarkHooks& h) noexcept {
    if (!first.entity || !second.entity || !h.Complete() ||
        registry.Resolve(first.handle)!=first.entity ||
        registry.Resolve(second.handle)!=second.entity)
        return TouchMarkResult::InvalidInput;

    // The retail wrapper refreshes the second origin before the first.
    if ((second.dirtyOrigin && !h.refreshOrigin(second,h.context)) ||
        (first.dirtyOrigin && !h.refreshOrigin(first,h.context)))
        return TouchMarkResult::OriginUnavailable;
    trace = {{(first.origin.x+second.origin.x)*0.5f,
              (first.origin.y+second.origin.y)*0.5f,
              (first.origin.z+second.origin.z)*0.5f},1.0f};

    // The inner retail registration routine independently applies these
    // symmetric eligibility tests before creating or refreshing links.
    if (first.entity==second.entity ||
        registry.Resolve(first.parent)==second.entity ||
        registry.Resolve(second.parent)==first.entity ||
        first.dontTouch || second.dontTouch ||
        (first.trigger && second.trigger && !first.solid && !second.solid) ||
        first.markedForDeletion || second.markedForDeletion)
        return TouchMarkResult::Ignored;

    if (!first.contacts.present || !second.contacts.present ||
        first.contacts.root==second.contacts.root ||
        !ValidateReciprocalChain(first.contacts.root,state.touchAllocated) ||
        !ValidateReciprocalChain(second.contacts.root,state.touchAllocated))
        return TouchMarkResult::InvalidList;

    auto lookup = [&](TouchEntity& owner, CBaseEntity* partner) noexcept {
        auto* root=owner.contacts.root;
        for(auto* n=root->next;n!=root;n=n->next)
            if(registry.Resolve(n->handle)==partner) return n;
        return static_cast<ContactNode*>(nullptr);
    };
    auto* foundA=lookup(first,second.entity);
    auto* foundB=lookup(second,first.entity);
    const unsigned needed=(foundA?0u:1u)+(foundB?0u:1u);
    if (state.touchAllocated>(std::numeric_limits<std::size_t>::max)()-needed)
        return TouchMarkResult::InvalidInput;
    // Independent safeguard: reserve both nodes before changing either
    // list. The original can leave a unilateral link on allocation failure.
    auto* newA=foundA?nullptr:h.createNode(h.context);
    if (!foundA && !newA) return TouchMarkResult::AllocationFailed;
    auto* newB=foundB?nullptr:h.createNode(h.context);
    if (!foundB && !newB) {
        if(newA) h.freeUnlinkedNode(newA,h.context);
        return TouchMarkResult::AllocationFailed;
    }
    auto mark = [&](TouchEntity& owner, TouchEntity& other,
                    ContactNode* existing, ContactNode* fresh) noexcept {
        if (existing) {
            existing->stamp=owner.touchStamp;
            h.touch(owner.entity,other.entity,h.context);
            return;
        }
        auto* root=owner.contacts.root;
        const bool start=((owner.solid && !owner.volumeContents) || owner.trigger)
                         && !other.trigger;
        *fresh={other.handle,root->next,root,start?kTouchStarted:0u,owner.touchStamp};
        root->next->previous=fresh;
        root->next=fresh;
        ++state.touchAllocated;
        if(start) h.startTouch(owner.entity,other.entity,h.context);
    };
    mark(first,second,foundA,newA);
    mark(second,first,foundB,newB);
    return TouchMarkResult::Recorded;
}
} // namespace sin_independent
