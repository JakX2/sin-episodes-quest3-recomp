# Phase 1632: Original GameEnts touch entry through full FreeContainingEntity lifecycle

## Exact retail evidence

Original owner `server.dll` SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. All reads are from the authenticated commercial binary, not generic Source engine inference.

| Region | Original VA | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| ServerGameEnts001 +0x08 `MarkEntitiesAsTouching` | `0x221e9380` | 299 | `8352791af429bd1cf0d040a0765b42ec959181b34d67f98f818a8c039389f1c1` |
| ServerGameEnts001 +0x0c `FreeContainingEntity` thunk | `0x221e82e0` | 14 | `0b0caf62afd15971ebdd8778c492e03519e45a2c93c03b42aceec070630cb0ac` |
| Original `FreeContainingEntity` helper | `0x22266e60` | 65 | `a502a1f89195150a94d7a02b8e224090d543047fb3a95c384d62260289435090` |

The helper at `0x22266e60` starts with an edict argument, checks null edict, reads unknown/networkable from `edict+0x0c`, invokes its virtual `+0x14` to get `CBaseEntity*`, and exits if no entity. If resolved, it first clears edict unknown `+0x0c`, then clears the first edict dword, and invokes **three owner helpers in this order**: `0x2221ffd0` (touched list), `0x22220190` (ground list), `0x22286c10` (immediate entity removal). The original helper returns `void`, with `ret` at `0x22266ea0`. Both wrappers and original helper are already known to earlier stages; this phase does **not** count them as newly recovered functions.

## Implementation / missing integration

The repository already implements original-guided, independently owned `MarkEntitiesAsTouchingWithContactGraph`, `TryReleaseContainingEntity`, `RemoveContactList`, `RemoveReciprocalContact`, and `TryRemoveImmediate`. Earlier `test_server_entity_full_teardown.cpp` had to preseed touched and ground nodes by hand, while Phase 1631's contact graph test exercised creation and unlink without calling the original PE32 `FreeContainingEntity`/edict detachment path. That left a significant cross-subsystem lifecycle gap.

The new `test_server_gameents_touch_release_lifecycle.cpp` covers the **real composition** of already implemented modules:

1. Define an x86 edict prefix with unknown pointer at original +0x0c and a real `ServerUnknownAbi` virtual +0x14 `GetBaseEntity` implementation, using deliberately owned fixtures only.
2. Resolve original-style edicts to typed `TouchEntity` views through the existing `ServerGameEntsConversionCore`, then call `MarkEntitiesAsTouchingWithContactGraph` to obtain two live contact nodes and a host-initialized, host-published 0x54-byte trace.
3. Repeat the same pair and verify existing-node `Touch` callbacks, not duplicate allocations or `StartTouch`.
4. Verify a missing cleanup dependency causes `TryReleaseContainingEntity` to refuse **before** touching the edict or either contact list.
5. Execute `TryReleaseContainingEntity` with complete host callbacks. The original edict association and first dword are both cleared **before** touched-list callbacks. Those callbacks remove the partner's reciprocal node, trigger `EndTouch` and release both nodes, with suppression of the remote touch sentinel while unlinking.
6. Exercise the original ground cleanup stage (a valid empty list) **after** touched cleanup, then the native immediate-removal component's `UpdateOnRemove`, deletion guard, registry invalidation and final heap entity destruction.
7. Verify a second `FreeContainingEntity` call on the same detached edict is a safe no-op; the old edict cannot be used to generate new touch contacts, and its unknown conversion resolves null rather than a freed entity.

This adds a new independent Windows x86 CTest executable, `independent_server_gameents_touch_release_lifecycle_native`. The expected test suite grows from 237 to 238; it does not mean 238 original game functions have been decompiled.

The important non-goal remains unchanged: the actual retail `ServerGameEnts001` factory is **not** exported from the incomplete independent server.dll. These original-guided typed components still run with synthetic owner callbacks, not live Source edicts or actual in-engine entity deletion. No generic Source physics, surface/material, prop, model-loading or rendering work and no proprietary executable bytes are committed.
