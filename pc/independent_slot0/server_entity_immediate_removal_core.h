#pragma once
// Owner-original guided, independent entity-lifetime boundary.
// The retail UTIL_RemoveImmediate helper is at RVA 0x286c10.
// No raw retail entity memory or executable code is used here.
#include "server_entity_handle_core.h"

namespace sin_independent {
constexpr std::uint32_t kEntityKillMe = 1u;
struct EntityRemovalRecord {
    std::uint32_t extendedFlags = 0;
};
struct EntityRemovalEnvironment {
    // The SDK uses a nested removal-disable semaphore, not a bool.
    unsigned deferDepth = 0;
    bool receivedChainedUpdate = false;
    bool handleAccessDisabled = false;
};
struct EntityRemovalHooks {
    void (*updateOnRemove)(CBaseEntity*,void*) noexcept = nullptr;
    void (*destroy)(CBaseEntity*,void*) noexcept = nullptr;
    // Original deferred path passes the embedded networkable to its
    // deletion-queue helper. An independent owner supplies that adapter.
    bool (*defer)(CBaseEntity*,void*) noexcept = nullptr;
    void* context = nullptr;
};
enum class ImmediateRemovalResult {
    NoEntity, AlreadyMarked, Deferred, DeferredRefused, Destroyed,
    MissingDependency, DestructionInProgress
};

// No entity/record access after destroy. Callers own the entity and record;
// update must not delete either. The destroy implementation must invalidate
// its registry entry before reclaiming memory. A deferred adapter owns its
// marking, UpdateOnRemove, name reset, and queue-insertion behavior.
inline ImmediateRemovalResult TryRemoveImmediate(
    CBaseEntity* entity, EntityRemovalRecord& record,
    EntityRemovalEnvironment& env, const EntityRemovalHooks& hooks) noexcept {
    if (!entity) return ImmediateRemovalResult::NoEntity;
    if (record.extendedFlags & kEntityKillMe)
        return ImmediateRemovalResult::AlreadyMarked;
    // Independent guard: do not recursively destroy from a destructor.
    if (env.handleAccessDisabled)
        return ImmediateRemovalResult::DestructionInProgress;
    if (env.deferDepth) {
        if (!hooks.defer) return ImmediateRemovalResult::MissingDependency;
        return hooks.defer(entity,hooks.context)
            ? ImmediateRemovalResult::Deferred
            : ImmediateRemovalResult::DeferredRefused;
    }
    if (!hooks.updateOnRemove || !hooks.destroy)
        return ImmediateRemovalResult::MissingDependency;
    record.extendedFlags |= kEntityKillMe;
    env.receivedChainedUpdate = false;
    hooks.updateOnRemove(entity,hooks.context);
    env.handleAccessDisabled = true;
    hooks.destroy(entity,hooks.context);
    env.handleAccessDisabled = false;
    return ImmediateRemovalResult::Destroyed;
}

// Independent resolver guard during destruction. The SDK's original
// diagnostic flag alone does not prove every inlined original lookup
// honored it; this explicit guard is an independent lifetime safeguard.
inline CBaseEntity* ResolveLiveEntity(
    const EntityHandleView& registry, std::uint32_t handle,
    const EntityRemovalEnvironment& env) noexcept {
    return env.handleAccessDisabled ? nullptr : registry.Resolve(handle);
}
} // namespace sin_independent
