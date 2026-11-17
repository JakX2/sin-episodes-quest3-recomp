#pragma once
// Independent, owned-list model of SiN's reciprocal contact removal.
// Retail evidence: partner lookup helpers 0x21FF00 and 0x2200D0,
// removal helpers 0x21F120 and 0x21F2E0. No game object offsets used.
#include "server_contact_cleanup_core.h"

namespace sin_independent {
constexpr std::uint32_t kTouchStarted = 1u;
constexpr std::uint32_t kOnGround = 1u;

enum class ReciprocalResult { NoContact, Removed, InvalidInput, InvalidList };
struct ReciprocalHooks {
    void (*endTouch)(CBaseEntity*, CBaseEntity*, void*) noexcept = nullptr;
    // The retail ground helper invokes a direct remove-flags routine
    // with mask 1 on the entity resolved from the link, NOT a virtual
    // EndGroundContact dispatch. Keep its flag-change side effects explicit.
    void (*removeFlags)(CBaseEntity*, std::uint32_t, void*) noexcept = nullptr;
    void (*releaseNode)(ContactNode*, ContactKind, void*) noexcept = nullptr;
    void (*destroyRoot)(CBaseEntity*, ContactNode*, ContactKind, void*) noexcept = nullptr;
    void* context = nullptr;
    bool Complete(ContactKind kind) const noexcept {
        return releaseNode && destroyRoot &&
            (kind == ContactKind::Touch ? endTouch != nullptr : removeFlags != nullptr);
    }
};

// Require a complete circular doubly linked OWNED list. Unlike bulk cleanup,
// reciprocal unlink needs BOTH neighbors. Bad shape refuses before callbacks.
// All supplied pointers must already designate live independent objects.
inline bool ValidateReciprocalChain(const ContactNode* root,
                                    std::size_t capacity) noexcept {
    if (!root || !root->next || !root->previous) return false;
    const ContactNode* prev = root;
    const ContactNode* node = root->next;
    std::size_t count = 0;
    while (node != root) {
        if (!node || node->previous != prev || !node->next ||
            count == capacity) return false;
        ++count;
        prev = node;
        node = node->next;
    }
    return root->previous == prev;
}

// Remove the FIRST matching reciprocal contact, using live entity identity
// rather than comparing raw handle bits. Callbacks observe the linked node
// before mutation, as in the original. They must not mutate this list,
// destroy either entity or invalidate the registry for this operation.
inline ReciprocalResult RemoveReciprocalContact(
    CBaseEntity* departing, CBaseEntity* partner, ContactKind kind,
    ContactList& remote, ContactCleanupState& state,
    const EntityHandleView& registry, const ReciprocalHooks& hooks) noexcept {
    if (!departing || !partner) return ReciprocalResult::NoContact;
    if (departing == partner || !hooks.Complete(kind))
        return ReciprocalResult::InvalidInput;
    if (!remote.present || !remote.root)
        return ReciprocalResult::NoContact;

    auto& count = kind == ContactKind::Touch ?
        state.touchAllocated : state.groundAllocated;
    auto* const root = remote.root;
    if (!ValidateReciprocalChain(root, count))
        return ReciprocalResult::InvalidList;

    ContactNode* node = root->next;
    for (; node != root; node = node->next) {
        if (registry.Resolve(node->handle) == departing) break;
    }
    if (node == root) return ReciprocalResult::NoContact;

    if (kind == ContactKind::Touch) {
        if (node->flags & kTouchStarted)
            hooks.endTouch(partner, departing, hooks.context);
    } else {
        hooks.removeFlags(departing, kOnGround, hooks.context);
    }

    node->previous->next = node->next;
    node->next->previous = node->previous;
    --count;
    hooks.releaseNode(node, kind, hooks.context);
    // Touch cleanup honors the suppression state; ground cleanup does not.
    if (remote.present && root->next == root && root->previous == root &&
        (kind == ContactKind::Ground || state.cleanupDataObjects)) {
        remote.present = false;
        remote.root = nullptr;
        hooks.destroyRoot(partner, root, kind, hooks.context);
    }
    return ReciprocalResult::Removed;
}

} // namespace sin_independent
