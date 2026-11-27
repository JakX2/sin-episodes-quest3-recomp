# Phase 1634: Guard callback-returned GameEnts pointers before dispatch and cleanup

Base: Phase 1633 commit `18dddcb0019a087c40b02c6fe148b9fc3953be51` (PR #1641). This is a separate, stacked diagnostic-only branch. No retail `CreateInterface` registration, gameplay claim, physics/surface/model changes, or original proprietary executable bytes.

## Motivation

The Phase 1633 seven-slot object validated the original edict and entity arguments and transmit buffer extents. However the original edict +0x0c unknown-interface pointer and several host callbacks return *new* objects. Attesting only the initial edict pointer does not attest those nested interfaces or the entity pointer returned by `GetBaseEntity`. Existing `FreeContainingEntity` could detach the edict and call three teardown hooks with an unvalidated returned entity. Transmit policy similarly accepted a callback-returned entity pointer. A touch resolver could return an unvalidated synthetic `TouchEntity*` before the contact bridge dereferenced its fields.

## New bounded ownership contracts

- Require `valid_unknown` and `valid_touch` in addition to `valid_edict` / `valid_entity` before the complete diagnostic bridge reports `Ready()`.
- Before `EdictToBaseEntity` or `FreeContainingEntity` invokes the `IServerUnknown` virtual method, check the edict's +0x0c interface pointer through the owner. Return `Unauthorized` with no virtual dispatch on rejection.
- The independent release core now supports a trailing optional entity-authorizer callback, leaving old standalone fixture contracts unchanged. In bounded mode, authorize the *single resolved entity* **before clearing edict +0x0c/+0x00 or invoking touched-list, ground-list and immediate-removal callbacks**.
- Attest both the synthetic `TouchEntity` storage and embedded `CBaseEntity` before reading origin/contact fields. Reattest around refresh and before pair publication.
- Attest callback-returned transmit entities before sending them to transmission policy. Distinguish unauthorized returned pointers from other host callback failures in diagnostics.

All validation callbacks must themselves be safe to call with arbitrary untrusted pointers. The host must attest live storage, virtual dispatch targets and stable lifetime for the duration of a call; no generic runtime heap-probing or engine-wide synchronization is claimed.

## Regression additions

Extend `independent_server_gameents_bounded_dispatch_native` (same existing CTest target) to test missing nested attestations, a foreign `IServerUnknown` with a zero-call assertion, release with a foreign resolved entity and unchanged edict, a forged touch storage address (0x1), a valid touch container with foreign embedded entity, and a forged transmit entity (0x1) with zero transmission side effects. Existing full vtable x86 stack-balance and touch→transmit→release checks remain.

Verification: **not yet run at commit creation**. GitHub Windows MSVC x86 CI and Android reference builds must be checked separately. No retail engine boot claim.
