#pragma once
// Owner-original guided deletion queue, with an independently owned,
// bounded array. The original stores IServerNetworkable pointers;
// the independent queue uses live entity identity plus handle generation
// and an explicit release adapter. No game binary addresses are accessed.
#include "server_entity_immediate_removal_core.h"
#include <array>
#include <cstddef>

namespace sin_independent {

enum class QueueRemovalResult {
    Queued, NoEntity, AlreadyMarked, Busy, BadHandle, Full,
    MissingDependency, InvalidatedDuringUpdate
};
struct QueueRemovalHooks {
    void (*update)(CBaseEntity*,void*) noexcept = nullptr;
    void (*clearName)(CBaseEntity*,void*) noexcept = nullptr;
    void* context = nullptr;
};
struct QueueDrainHooks {
    void (*cleanupPhysics)(void*) noexcept = nullptr;
    void (*release)(CBaseEntity*,void*) noexcept = nullptr;
    void* context = nullptr;
};
enum class QueueDrainStatus { Completed, Busy, MissingRelease };
struct QueueDrainResult {
    QueueDrainStatus status;
    std::size_t released = 0;
    std::size_t stale = 0;
};

template <std::size_t Capacity>
class EntityDeletionQueue final {
public:
    EntityDeletionQueue() noexcept = default;
    EntityDeletionQueue(const EntityDeletionQueue&) = delete;
    EntityDeletionQueue& operator=(const EntityDeletionQueue&) = delete;
    EntityDeletionQueue(EntityDeletionQueue&&) = delete;
    EntityDeletionQueue& operator=(EntityDeletionQueue&&) = delete;

    std::size_t Count() const noexcept { return count_; }
    bool InCleanup() const noexcept { return cleaning_; }

    // Recovered deferred branch: mark for deletion, reset the chained
    // update state, update the entity and clear its name before the
    // enqueue. Preflight handle/capacity and reject reentrant mutation
    // in this bounded independent implementation.
    QueueRemovalResult Enqueue(
        CBaseEntity* entity, std::uint32_t handle, EntityRemovalRecord& record,
        EntityRemovalEnvironment& env, const EntityHandleView& registry,
        const QueueRemovalHooks& hooks) noexcept {
        if (!entity) return QueueRemovalResult::NoEntity;
        if (record.extendedFlags & kEntityKillMe) return QueueRemovalResult::AlreadyMarked;
        if (enqueuing_ || cleaning_ || env.handleAccessDisabled)
            return QueueRemovalResult::Busy;
        if (registry.Resolve(handle) != entity)
            return QueueRemovalResult::BadHandle;
        if (count_ == Capacity) return QueueRemovalResult::Full;
        if (!hooks.update || !hooks.clearName)
            return QueueRemovalResult::MissingDependency;

        enqueuing_ = true;
        record.extendedFlags |= kEntityKillMe;
        env.receivedChainedUpdate = false;
        // Independent callbacks must not destroy the entity or its record.
        hooks.update(entity,hooks.context);
        hooks.clearName(entity,hooks.context);
        if (registry.Resolve(handle) != entity) {
            enqueuing_ = false;
            // Update/name effects are not reversible. Never queue a stale
            // generation or claim success in this unexpected case.
            return QueueRemovalResult::InvalidatedDuringUpdate;
        }
        entries_[count_++] = {entity, handle};
        enqueuing_ = false;
        return QueueRemovalResult::Queued;
    }

    // Original frame cleanup invokes physics cleanup first, then disables
    // normal handle access and releases networkables in queue order.
    // This independent owner additionally checks generation at drain time
    // and refuses reentrant queue mutation while callbacks execute.
    QueueDrainResult Drain(
        EntityRemovalEnvironment& env, const EntityHandleView& registry,
        const QueueDrainHooks& hooks) noexcept {
        if (enqueuing_ || cleaning_ || env.handleAccessDisabled)
            return {QueueDrainStatus::Busy};
        if (!hooks.release) return {QueueDrainStatus::MissingRelease};
        cleaning_ = true;
        if (hooks.cleanupPhysics) hooks.cleanupPhysics(hooks.context);
        env.handleAccessDisabled = true;
        QueueDrainResult result{QueueDrainStatus::Completed};
        for (std::size_t i=0; i<count_; ++i) {
            auto entry = entries_[i];
            // Registry lookup here is privileged queue bookkeeping;
            // ordinary code must go through ResolveLiveEntity.
            if (registry.Resolve(entry.handle) == entry.entity) {
                hooks.release(entry.entity,hooks.context);
                ++result.released;
            } else {
                ++result.stale;
            }
            entries_[i] = {};
        }
        env.handleAccessDisabled = false;
        count_ = 0;
        cleaning_ = false;
        return result;
    }

private:
    struct Entry {
        CBaseEntity* entity = nullptr;
        std::uint32_t handle = kInvalidEntityHandle;
    };
    std::array<Entry,Capacity> entries_{};
    std::size_t count_ = 0;
    bool enqueuing_ = false;
    bool cleaning_ = false;
};

} // namespace sin_independent
