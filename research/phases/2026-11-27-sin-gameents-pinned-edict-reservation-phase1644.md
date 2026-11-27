# Phase 1644: Edict-only generation reservation under engine owner pin

**Stack:** Phase 1643 PR #1652, original CBaseEntity/IServerUnknown alias recovery. The original factory is still unregistered.

## Motivation

Phase 1643 tested enrollment of complete entity identities only **after** the synthetic engine allocator had released its physical edict pin. But engine.dll allocates/restamps its 0x14-byte edict before server.dll constructs/binds a corresponding CBaseEntity. Inventing an entity pointer at allocation is incorrect, while delaying the edict epoch reservation until after unpin leaves an ownership-metadata handoff gap.

The original 2006 PE32 evidence remains the SHA-pinned engine select/free/restamp path from Phases 1638-1642 and server CBaseEntity/IServerUnknown alias path from Phase 1643. This phase makes **no new original binary claims**.

## New lifecycle

- **ReserveEdict(slot, edict, 0x14, epoch)** records an edict-only allocation at an increasing host ownership epoch. It is called by the trusted host **inside** its physical pin/count-commit callback. No entity, unknown, or touch pointer is manufactured and none is authorized while pending.
- A reserved edict cannot be replayed, overwritten while pending, enrolled at a stale epoch, switched to a different address, or made to overlap another live owner allocation. An existing edict can be re-reserved only after its old entity is retired, with a higher epoch.
- **ActivateReservedEntity(slot, identity)** later accepts the complete original-game entity/unknown identity only when its slot, edict address, edict extent and reserved epoch match exactly. It reuses Phase 1643's source-backed exact-base alias bounds and checks cross-owner range disjointness. Separate owners may create entities out of epoch order.
- **ForgetEdict** also cancels a never-activated reservation when an authorized owner reconciles an abandoned slot. Existing Enroll remains compatible for legacy fully owned fixtures.
- The independent pinned allocator regression now exercises host-count plus metadata reservation in one synthetic commit callback, deferred binding, a duplicate journal collision that quarantines the allocator, and failed unpin after committed reservation. The owner-registry regression independently covers wrong strides, zero/stale/replayed epochs, overlap, lease refusal, exact-address matching, deferred original unknown aliases, retirement/reuse and cancellation.

## Limits and safety

These are original-free synthetic C++17 ownership contracts. ReserveEdict holds the registry mutex but **does not itself acquire an engine physical pin**: the trusted host must already hold its exclusive, stable pin and must not fail after successfully staging the reservation unless it quarantines and reconciles the owner. ActivateReservedEntity similarly depends on separate trusted host attestation and lifetime protection for later entity/unknown/touch allocations. A registry logical lease never pins those objects.

This step closes the *synthetic metadata ordering* gap, not the live-engine ownership integration. The unmodified retail engine's allocator globals, host-protected count and physical pin mechanism, constructor-to-entity attachment, and Windows load/startup path remain unverified. Keep ServerGameEnts001 unregistered with retail CreateInterface; do not present native Windows gameplay, Android gameplay or Quest 3 startup as proven. Do not revive HL2-equivalent physics, surface, model or prop work.
