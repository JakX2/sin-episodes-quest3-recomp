# Phase 1637: Generation-aware GameEnts association validation

Stacked on Phase 1636 PR #1644, which authenticated original-engine consumers of the seven-slot ServerGameEnts001 ABI. This is independent x86 diagnostic safety work; it does not register the object with the retail engine.

## Original-lifetime mismatch

The bounded owner contract from Phases 1634–1635 attests that edicts, IServerUnknown dispatch targets, CBaseEntity instances and TouchEntity storage are individually live under an owner lease. However, an individually valid entity can belong to **another** valid edict, or to an earlier generation of the *same* edict address. This is a material gap in FreeContainingEntity (wrong owned target could be destroyed), entity conversion (wrong pointer could be exported) and MarkEntitiesAsTouching (wrong contact graph could be mutated).

## Implementation

- GameEntsBoundedOwner now requires valid_pair(owner_context,edict,entity) in Complete() alongside valid_edict, valid_entity, valid_unknown, valid_touch and the owner lease callbacks. The *trusted host* must authenticate the same live edict/entity association and generation via its registry while its access lease is held. Pointer equality between separately authorized allocations is insufficient.
- BaseEntityToEdict validates the returned edict **and** entity association after reading the authenticated source entity +0x16c. EdictToBaseEntity likewise validates the returned entity **and** association after the unknown interface virtual call. Mismatch maps to Unauthorized and returns null.
- FreeContainingEntity invokes the association callback after authorizing the result entity, but **before** clearing edict flags / unknown or invoking any cleanup stage. Mismatch maps to UnauthorizedEntity. The standalone release core retains an optional default hook to preserve existing focused fixtures; the complete bounded vtable always supplies it.
- The contact bridge takes an optional raw-edict association hook and owner context. The bounded vtable supplies the mandatory typed owner callback through a non-allocating adapter. It checks each resolved TouchEntity's embedded entity against the **actual source edict** at resolution, rechecks during live origin refresh, before trace publication, **and after** the external trace publication callback before directed contact mutation.
- No changes to the original seven x86 virtual signatures, call order, lease token semantics or diagnostic instance vptr.

## Native regression

The existing full Windows MSVC x86 vtable fixture now maintains edict serials and registry generations in its owner association callback. New negative cases reject: a valid entity pointing to the other valid edict, another authorized unknown returning a foreign-but-owned entity, foreign owned target in release, foreign owned target in touch, a recycled serial with unchanged addresses and unknown, and a valid TouchEntity containing the wrong valid CBaseEntity. After restoring identities it retests positive conversion. A further regression simulates an owner callback recycling a pair during trace publication; publication remains an allowed external side effect, but no directed contact links may be allocated or inserted after the mismatch. Earlier failures must not detach edicts, run cleanup, allocate contacts or publish a trace. Existing original-vtable ESP checks, owner-lease balancing, end-to-end touch/transmit/release and forged pointer cases remain.

## Limitations

This validates a **synthetic** host-supplied generation registry, not the original game's unmodified live entity manager. Exact retail edict generation, ownership of unknown interfaces, contacts, and CCheckTransmitInfo bitsets still require owner-backed integration. The engine is not booted. Retail CreateInterface remains disabled. No generic Source physics, surface or model/prop work.

CI verification: check the final commit after push; all synthetic probes are original-free.
