#pragma once
// Original-guided contact cleanup traversal, using independently owned nodes.
// This is NOT a raw retail touchlink_t/groundlink_t memory overlay.
#include "server_entity_handle_core.h"
#include <cstddef>
#include <cstdint>

namespace sin_independent {

enum class ContactKind { Touch, Ground };
struct ContactNode {
    std::uint32_t handle = kInvalidEntityHandle;
    ContactNode* next = nullptr;
    ContactNode* previous = nullptr;
    std::uint32_t flags = 0;
};
struct ContactList {
    ContactNode* root = nullptr; // root is a sentinel, never a contact allocation
    bool present = false;       // analogue of the respective data-object bit
};
struct ContactCleanupState {
    std::size_t touchAllocated = 0;
    std::size_t groundAllocated = 0;
    bool cleanupDataObjects = true;
};
struct ContactCleanupHooks {
    void (*notifyPartner)(CBaseEntity*, CBaseEntity*, ContactKind, void*) noexcept = nullptr;
    void (*freeNode)(ContactNode*, ContactKind, void*) noexcept = nullptr;
    void (*destroyRoot)(CBaseEntity*, ContactNode*, ContactKind, void*) noexcept = nullptr;
    void* context = nullptr;
    bool Complete() const noexcept {
        return notifyPartner && freeNode && destroyRoot;
    }
};
enum class ContactCleanupResult { NoList, Removed, InvalidInput, InvalidList };

// The original touch and ground data-object cleanup paths use linked
// sentinels and tolerate a null next pointer. Before mutating independent
// state, additionally reject off-sentinel cycles and undercounted lists.
// Only owned, live node pointers may be supplied to this independent API.
inline bool ValidateContactChain(const ContactNode* root,
                                 std::size_t available) noexcept {
    std::size_t count = 0;
    for (const auto* node = root->next; node && node != root;
         node = node->next) {
        if (count == available) return false;
        ++count;
    }
    return true;
}

// Hooks model remaining original subsystem dependencies. They must not
// free another unvisited node in this owner's list. They may clear the
// list's presence bit, matching the original final presence recheck.
inline ContactCleanupResult RemoveContactList(
    CBaseEntity* owner, ContactKind kind, ContactList& list,
    ContactCleanupState& state, int& touchStamp,
    const EntityHandleView& registry, const ContactCleanupHooks& hooks) noexcept {
    if (!owner) return ContactCleanupResult::InvalidInput;
    const bool touch = kind == ContactKind::Touch;
    if (!list.present || !list.root) {
        if (touch) touchStamp = 0;
        return ContactCleanupResult::NoList;
    }
    auto& allocated = touch ? state.touchAllocated : state.groundAllocated;
    if (!hooks.Complete()) return ContactCleanupResult::InvalidInput;
    ContactNode* const root = list.root;
    if (!ValidateContactChain(root, allocated))
        return ContactCleanupResult::InvalidList;

    const bool savedCleanup = state.cleanupDataObjects;
    if (touch) state.cleanupDataObjects = false;
    for (auto* node = root->next; node && node != root;) {
        // Save successor before external notification or release, as the
        // original does. Invalid/stale handles are passed as nullptr.
        auto* next = node->next;
        auto* partner = registry.Resolve(node->handle);
        hooks.notifyPartner(owner, partner, kind, hooks.context);
        --allocated;
        hooks.freeNode(node, kind, hooks.context);
        node = next;
    }
    if (touch) state.cleanupDataObjects = savedCleanup;

    // Original routines recheck the data-object bit before destruction.
    if (list.present) {
        hooks.destroyRoot(owner, root, kind, hooks.context);
        list.present = false;
    }
    list.root = nullptr;
    if (touch) touchStamp = 0;
    return ContactCleanupResult::Removed;
}

} // namespace sin_independent
