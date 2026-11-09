# Phase 128: Versioned entity-I/O save/restore

**Status:** Independently authored portable C++17 snapshot model and codec. This is not original SiN save-game parity, a SourceVR save subsystem adapter, or a Quest gameplay test.

## What is persisted

The EntityIoRuntime snapshot captures:

- Registered output definitions grouped by stable source ID and output name.
- Each connection's target, input, parameter, configured delay, maximum fire count and remaining fire count.
- Pending events' target/input/parameter, remaining delay, caller/activator stable IDs and insertion ordinal.
- The next insertion ordinal, preserving deterministic same-time queue ordering.

Pending due times are stored as **remaining game-time durations**, not wall-clock timestamps. The host supplies a new monotonic clock origin during restore, so time spent in menus or outside the process does not silently consume a map delay.

## Stable entity identity

Raw EntityRef index/generation pairs are deliberately not serialized as persistent identity. The engine host implements IEntityIoPersistenceMap:

- stable_id_for maps a currently live entity to a unique, nonzero game-owned ID. A host must not alias distinct live sources to one ID or reuse IDs while a save can reference them.
- entity_for_stable_id maps those IDs to newly recreated live entities after the host has restored its entity set.

Zero is the serialized null-handle sentinel. Expired or unresolved weak caller/activator references become null while their pending event survives. A missing output-definition source is different: restore fails because silently dropping an entity's registered outputs would produce an incomplete map runtime.

Restore builds temporary output and event containers, validates every required source mapping and due-time calculation, then swaps them into the runtime. A failed restore leaves the old runtime untouched. Save/restore is rejected during an active input callback to avoid a mid-dispatch snapshot.

## Binary format

The codec uses a bounded, versioned little-endian format:

- Four-byte SIOQ magic.
- 32-bit format version (currently 1).
- 64-bit next event ordinal.
- 32-bit output-group and pending-event counts.
- Fixed-width IDs, ordinals, signed fire counters and IEEE-754 binary64 delays.
- Length-prefixed byte strings.

The decoder rejects unsupported versions, oversized records/strings, more than 65,536 connections, invalid counters or delays, duplicate output groups/ordinals, missing required fields, truncated data and trailing bytes. Encode/decode leave the destination unchanged on failure. The current cap is 16 MiB; this is a compatibility save chunk, not a general-purpose unbounded archive.

## Verification added

The dedicated sin_entity_io_snapshot_tests target covers:

1. Codec round-trip and transactional failure on truncation/trailing bytes.
2. Output connection remaining-fire-count preservation.
3. Pending-event ordering and delivery after a new clock origin.
4. Stable-ID remapping across different entity indices and generations.
5. Missing weak activator resolution to null without dropping the event.
6. Stale caller/activator handles serialized as null.
7. Stale output-definition sources omitted at capture.
8. Required source remapping failure without mutating the destination runtime.
9. Stable-ID collision rejection, unsupported versions, duplicate ordinals, invalid fire counters and non-finite restore time.

## Evidence boundary

The event queue's weak caller/activator lifetime model was previously compared with public Source SDK 2013 reference behavior. This snapshot format is a new project-owned portable format. It is **not** claimed to match the original SiN 2006 save-game block layout, Source Save/Restore field descriptors, SourceVR 0.5.5 serialization or any commercial game's save format.

The host still must integrate this chunk with its real game save system, recreate entities and stable IDs before restore_state, and prove an actual map transition/save/load in an engine that supports game-code modules. trigger_changelevel, info_landmark, BSP entity parsing, NPC state, world state, player state and asset access remain separate campaign requirements.

No proprietary DLLs, APKs, original ELF, BSPs, models, maps, scripts or game assets are included. No placeholder native module is produced.
