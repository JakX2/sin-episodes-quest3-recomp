# Phase 1646: Epoch-scoped authorization after same-address edict and entity reuse

**Stack:** Phase 1645 PR #1654, original reciprocal 2006 PE32 network-property verification under simultaneous trusted host pins. The retail factory remains disabled.

## The ABA problem

In the existing independent registry, pointer-only methods such as ValidUnknownPair(edict, unknown), ValidPair(edict, entity), ValidEntity(entity) and ValidCandidate(table, index, entity) checked live ownership but did not accept a *caller's expected owner epoch*. A real allocator can reuse a 0x14-byte edict slot; a game allocator can also reconstruct CBaseEntity at the same address. After such recycling, stale caller addresses may equal the *new* pair and pass simple live pointer equality even though the old owner was retired. A registry-internal generation does not solve this ambiguity when the caller fails to supply its own expected generation.

The original retail edict network serial is uint32, subject to wrapping, and is not this host lifetime epoch. These added authorizers are independent fail-closed host-side safety APIs, **not claims about original 2006 Source virtual methods or ABI changes**.

## Implemented

Phase 1646 adds six optional, epoch-scoped, lease/thread-owned registry queries:

- ValidEdictAtEpoch(ptr, expected_epoch): accepts owned pending or activated edict only for its current epoch.
- ValidEntityAtEpoch(ptr, expected_epoch): requires a live entity and that exact epoch.
- ValidUnknownPairAtEpoch(edict, unknown, expected_epoch): requires live exact-base original unknown pair plus exact epoch.
- ValidPairAtEpoch(edict, entity, expected_epoch): requires live pair plus epoch.
- ValidTouchAtEpoch(ptr, extent, expected_epoch): requires the touch range at that exact current epoch.
- ValidCandidateAtEpoch(table, index, entity, expected_epoch): checks both the bound row generation and the current entity generation.

Every method requires the original thread-owned lease and rejects null/zero inputs. The pointer-only legacy methods remain unchanged for existing unit fixtures, but are **not safe as stand-alone cross-lifetime authorization** when addresses may be reused.

## Native regression

The owner lifetime regression now registers one original-style CBaseEntity == IServerUnknown alias at epoch 11, binds candidate table/index, validates all epoch-scoped fields, retires the entity through the authorized original slot-3 lease, reserves **the same exact edict address** at epoch 12, and reactivates **the same exact entity and touch addresses** as a new lifetime. Pointer-only pair authorization succeeds against the current owner, demonstrating that stale addresses alone have no lifetime identity. All six scoped queries must accept epoch 12 while rejecting epoch 11. Wrong/zero epochs, expired lease, wrong touch extent and stale candidate row generations also fail.

The Phase 1645 pinned-original-link attacher test now validates the epoch-scoped pair after successful reciprocal readback.

No new original SHA evidence or commercial binaries are added. These APIs cannot themselves physically pin memory. Trusted host and live Windows engine ownership remain unresolved, as does retail ServerGameEnts001 registration and playable Windows/Android/Quest startup. Do not revive HL2-equivalent physics, surfaces, props or model loading.
