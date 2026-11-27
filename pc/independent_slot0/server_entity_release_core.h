#pragma once
// Independent, fail-closed reconstruction of a bounded portion of
// ServerGameEnts001::FreeContainingEntity from the owner's original PE32.
//
// Authenticated original server.dll (SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650):
//   entity virtual slot 3 at RVA 0x1e82e0 delegates to RVA 0x266e60.
// The helper checks edict->unknown->GetBaseEntity; if valid it clears
// edict +0x0c and the first edict dword before invoking three separate
// cleanup routines on that same base-entity pointer, in sequence.
//
// Source SDK 2004 mapentities.cpp names the corresponding helpers as
// PhysicsRemoveTouchedList, PhysicsRemoveGroundList, UTIL_RemoveImmediate.
// Exact SiN implementations/side effects are NOT yet reconstructed.
// Callers must explicitly provide all three independently implemented
// callbacks. Missing callbacks cause a non-mutating refusal. This component
// does NOT advertise the incomplete ServerGameEnts001 through CreateInterface.
#include "server_entity_conversion_core.h"
#include <cstdint>
#include <cstring>

namespace sin_independent {

using OriginalEntityCleanupStage =
    void (__cdecl *)(CBaseEntity* entity, void* context) noexcept;

struct EntityReleaseHooks {
    OriginalEntityCleanupStage removeTouchedList = nullptr;
    OriginalEntityCleanupStage removeGroundList = nullptr;
    OriginalEntityCleanupStage removeImmediate = nullptr;
    void* context = nullptr;

    bool Complete() const noexcept {
        return removeTouchedList && removeGroundList && removeImmediate;
    }
};

enum class EntityReleaseStatus : unsigned char {
    NoAssociatedEntity,
    MissingCleanupHooks,
    DetachedAndHooksInvoked,
    UnauthorizedEntity
};

// A safe, test-only core, NOT a drop-in implementation of the void virtual
// FreeContainingEntity method. The original game always has its teardown
// helpers; this independent contract must refuse work without them.
inline EntityReleaseStatus TryReleaseContainingEntity(
    const ServerGameEntsConversionCore& conversion,
    edict_t* edict,
    const EntityReleaseHooks& hooks,
    bool (*authorized_entity)(void*,const CBaseEntity*) noexcept = nullptr,
    void* authorization_context = nullptr,
    bool (*authorized_pair)(void*,const edict_t*,const CBaseEntity*) noexcept
        = nullptr) noexcept {
    if (!edict) return EntityReleaseStatus::NoAssociatedEntity;
    if (!hooks.Complete()) return EntityReleaseStatus::MissingCleanupHooks;

    CBaseEntity* entity = conversion.EdictToBaseEntity(edict);
    if (!entity) return EntityReleaseStatus::NoAssociatedEntity;
    // Reject a callback-returned foreign/retired entity before the first
    // irreversible edict write or any cleanup callback. Optional only for
    // legacy standalone fixtures; the complete vtable always supplies it.
    if (authorized_entity && !authorized_entity(authorization_context, entity))
        return EntityReleaseStatus::UnauthorizedEntity;
    // An individually authorized but cross-wired/stale-generation entity
    // must not be deleted on behalf of some other still-live edict.
    if (authorized_pair && !authorized_pair(authorization_context, edict, entity))
        return EntityReleaseStatus::UnauthorizedEntity;

    auto* bytes = reinterpret_cast<unsigned char*>(edict);
    // Original order is important: unknown is cleared FIRST, then the
    // first state dword, before invoking any of the three teardown helpers.
    ServerUnknownAbi* nullUnknown = nullptr;
    std::memcpy(bytes + kOriginalEdictUnknownOffset,
                &nullUnknown, sizeof(nullUnknown));
    const std::uint32_t clearedState = 0;
    std::memcpy(bytes, &clearedState, sizeof(clearedState));

    hooks.removeTouchedList(entity, hooks.context);
    hooks.removeGroundList(entity, hooks.context);
    hooks.removeImmediate(entity, hooks.context);
    return EntityReleaseStatus::DetachedAndHooksInvoked;
}

}  // namespace sin_independent
