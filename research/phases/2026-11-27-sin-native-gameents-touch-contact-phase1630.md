# Phase 1630: Compose original GameEnts touch entry with reciprocal contact graph

## Purpose and overlap check

This is a gameplay-entity *integration* phase, not another original function recovery. The existing original-guided `ServerGameEnts001::MarkEntitiesAsTouching` entry (original server.dll VA `0x221e9380`, 299 bytes, SHA-256 `8352791af429bd1cf0d040a0765b42ec959181b34d67f98f818a8c039389f1c1`) was separately implemented in Phase 1627 as `MarkEntitiesAsTouchingEntry`. The existing independent reciprocal-contact graph `MarkTouchPair`, from `server_touch_mark_core.h`, already owned eligibility, atomic two-node reservation, link updates and StartTouch/Touch callbacks. Keeping these as two disconnected helpers meant the original edict-to-contact path was not exercised end to end.

An overlap audit found concurrent, fully green PRs #1635 (canonicalizing original Minor +0x398) and #1636 (original PE32 `ServerGameEnts001::CheckTransmit` entry). A separate overlapping PR #1637 was closed as superseded rather than merged. Phase 1630 is stacked **directly on #1636** and does not change Minor +0x398 nor CheckTransmit.

## Original owner-side evidence

The original `server.dll`, SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, contains `ServerGameEnts001` virtual +0x08 `0x221e9380`. Its first two source-engine edicts are resolved through edict `+0x0c` networkable virtual `+0x14`; null entities stop before trace construction. The original trace initializer `0x22285a10` writes a 0x54-byte trace with engine-owned defaults. Dirty origins are refreshed **second first, first second**, before storing the float-rounded position midpoint at trace offsets `+0x0c`, `+0x10`, `+0x14`. The wrapper calls original pair helper `0x22221080`, which publishes trace state and dispatches directed helper `0x2221f350` in both directions. These functions and full original machine-code hashes were authenticated in Phase 1627; their bytes are **not counted again** here.

## New independent composition

`pc/independent_slot0/server_gameents_touch_contact_bridge.h` adds `MarkEntitiesAsTouchingWithContactGraph`, a host-facing bridge that:

1. Resolves each supplied original edict through an owner callback into stable, owned `TouchEntity` state. It does not read raw Source `edict_t` or `CBaseEntity` layouts.
2. Calls the existing `MarkEntitiesAsTouchingEntry` for trace initialization, correct second-before-first dirty-origin refresh, and original midpoint computation.
3. Writes back refreshed positions to the wrapper's typed snapshot. It requires the owner refresh callback to clear the underlying dirty flag, avoiding an unintentional second refresh when `MarkTouchPair` is invoked.
4. Hands the resolved typed pair to the **existing** `MarkTouchPair` with caller-owned contact nodes, generation-checked entity registry and complete StartTouch/Touch allocation callbacks. This retains the already-tested atomic two-node reservation, reciprocal links, parent/trigger/deletion eligibility and repeat-contact behavior instead of reimplementing it.
5. Reports `TouchMarkResult::Ignored` as a completed original-style no-op, while allocation failures, stale handles, invalid lists or missing owner callbacks cause an explicit independent failure result. The original void method does not expose those synthetic status values.
6. Checks that both native trace midpoint representations agree after the pair operation. It preserves the initialized 0x54-byte trace on the result; it **does not** claim to publish the retail engine's actual global trace state or implement retail physics.

## Native regression

New Windows CTest executable `independent_server_gameents_touch_contact_bridge_native` performs a complete synthetic `edict -> resolved entity -> initialized trace -> second/first origin refresh -> two contact node allocations -> directed StartTouch` sequence. It also checks repeat calls reuse both links and send Touch rather than StartTouch; same-entity/parent no-ops; null networkable entity; initial trace failure; second and first refresh failures; owner failing to clear a dirty flag; second-node allocation failure rolling back the first unlinked node; stale serial generation preventing mutation; bad edict resolution; and incomplete hooks rejected before work.

Running with a real game engine still requires original entity ownership, compatible networkables, live handles, global trace publication and non-synthetic contact/cleanup callbacks. The original `ServerGameEnts001` factory remains disabled. No proprietary DLL bytes or original code are committed, and no generic physics, surfaces, props, model or rendering systems are reimplemented.

## Branch relationship

Base: `feature/sin-native-gameents-transmit-entry-phase1629-chat-20261127` (green Phase 1629, PR #1636). Main remains untouched. CI pending.
