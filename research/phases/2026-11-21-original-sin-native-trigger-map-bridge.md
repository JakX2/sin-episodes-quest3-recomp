# Phase 529: native-owned entity to original SiN trigger map bridge

## The integration gap, without duplicating HL2 engine facilities

The user successfully ported SiN Episodes maps, models, and props into
Half-Life 2. We therefore do not rebuild Source physics, model or prop
readers. The remaining blocker is actual SiN game-code entities being
instantiated and executing their original Spawn/Touch/game-event logic.

Phases 526–528 SHA-authenticated both original 2006
`trigger_dynamic_difficulty` and `trigger_secret` class registrations,
native RTTI/vtable slots 25 and 89, spawn/touch/event helper control
flow, exact original DATADESC KeyValues for dynamic difficulty, and
deferred engine entity deletion. They implemented the gameplay logic
in a **portable** behavior adapter but did not provide a native
Source-compatible entity allocator or any real game DLL Init.

Phase 529 connects that behavior adapter to the existing project
`SourceMapRuntime` through a genuine **host-owned** boundary:

`include/sin_compat/original_sin_trigger_map_bridge.hpp`
`src/original_sin_trigger_map_bridge.cpp`

## How real native integration is required to work

The native game runtime must provide all three callback roles:

1. `ISourceMapEntityHost`: create a genuine engine-owned entity,
   accept Source engine/CBaseEntity/DATADESC KeyValues in their
   original order, initialize ordinary entities, activate and
   reclaim real entity instances.
2. `IOriginalTriggerHost`: real CBaseTrigger base Spawn, Source
   event manager, difficulty manager, queued entity removal,
   actor IsPlayer and special trigger base action.
3. `ITriggerNativeState`: acquire a runtime snapshot from the
   actual *live* trigger object's state, after engine KeyValues,
   rather than interpreting BSP flags as x86 object offsets.

The snapshot captures true source-native fields:
- CBaseEntity world origin `+0x2BC` (used by the dynamic event
  `x`, `y`, `z` payload).
- `this+0xB8` flag bit `0x800`: special dynamic Touch base action.
- `this+0xB4` flag bit `0x400`: original dynamic Spawn warning.
- `this+0x120` flag bit `0x08`: original secret Spawn descriptor11.
- Original global object virtual `+0x88`: skip Spawn and queue
  deletion if already suppressed.

**Do not infer that the +0x120 source field equals any
`spawnflags` map KeyValue without an original DATADESC/source
proof.** The owner-provided synthetic runtime tests deliberately
supply all these fields independently of BSP KeyValues.

The adapter delegates native allocation first, then attaches the
portable `OriginalTriggerEntityFactory::spawn` behavior to that
generation-safe identity. It refuses native factory failure and
does not substitute `prop_dynamic`, `ambient_generic` or any
dummy entity. Native KeyValues must succeed before the behavioral
side parses the known original
`EstimatedTime`, `EstimatedDifficulty`, `EndOfGame`, and
`targetname` fields.

During `dispatch_spawn`, it queries the real native snapshot,
refreshes the behavior's origin and special flags WITHOUT erasing
parsed KeyValues, then dispatches only the appropriate
`run_secret_spawn` or `run_dynamic_spawn`. The native base
`CBaseTrigger::Spawn` is invoked once via the explicit host
callback; the adapter must not then call generic
`native_map.dispatch_spawn` on that trigger a second time.
All non-SiN entities pass through the normal Source host.

On Touch, it refreshes the live runtime state and dispatches the
original SiN event/manager/remove sequence. On removal, it
verifies the host really reclaimed the engine entity before
releasing its portable behavior. If native removal fails,
generation-bound behavior remains available for a later retry.

**The adapter does not call `OriginalTriggerEntityFactory::install`.
That API is an alternative registrar path; combining both paths
would double-attach a behavior.** Native class factory registration
must exist in the real game host before this bridge is usable.

## Actual integrated synthetic tests

`tests/test_original_sin_trigger_map_bridge.cpp` uses the
REAL project SourceMapRuntime and real phase526–528 gameplay
behavior code with a fabricated Source engine host.

- `logic_relay` is created/spawned/activated by the normal host,
  with no SiN behavior attached.
- Native-created `trigger_secret` emits `game` descriptor11
  on Spawn, then `game` descriptor12 on player Touch, and enqueues
  deferred native removal.
- Native-created `trigger_dynamic_difficulty` parses its original
  three map keys, emits original warning from synthetic +0xB4,
  and on player Touch emits its fields and the *native*
  world position while signaling the manager and queueing removal.
- Missing real native factory, fabricated unknown classname,
  missing native-field snapshot, rejected native base Spawn,
  malformed difficulty KeyValue, and failed native removal
  all reject or roll back safely.
- Failed entity deletion retains both engine and portable state
  for a valid later retry. Reused entity indices never borrow
  previous-generation behavior.

Tests can be run through
`sin_original_trigger_map_bridge_tests`. They do NOT run the
original Source executable or game maps; they also do not prove
`ServerGameDLL003::DLLInit` or the real MSVC x86 entity class
ABI. Native Source entity allocation, networkable/server-class
registration, map entity I/O, game rules and player/AI behavior
are still required before a real campaign map can run.

Source integrity: no original game binaries, original asset
payloads, proprietary scripts, textures, model readers or
physics components are committed.
