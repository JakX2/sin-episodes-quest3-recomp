# SiN assault AI: original condition witnesses versus runtime registration

**November 7, 2026.** Stack after the original `ai_goal_assault` input and condition-registry research (#124 to #126).

## Why this boundary is necessary

The owner's 2006 Windows x86 `server.dll` establishes the original names and
IDs of conditions used by assault behavior. In particular,
`COND_PROVOKED = 0x18` is set by `ai_goal_assault::InputBeginAssault`,
and `COND_HEAR_COMBAT = 0x33` is queried for the mode-2 Gunfire cue.
Those facts are documented with original instruction addresses in
[ORIGINAL_ASSAULT_CONDITION_IDS_NOV07.md](ORIGINAL_ASSAULT_CONDITION_IDS_NOV07.md).

It is **not** established that future SourceVR/ARM64 condition registration
will assign those same integers. Passing `0x18` or `0x33` to native engine
condition APIs would therefore be unjustified, even though the original
2006 IDs are proven.

## New portable bridge

- `begin_goal_assault_semantic` preserves per-actor original operation
  order (reset owner fields, store received cue 1, signal Provoked) while
  supplying a typed semantic condition to a host callback.
- The existing `begin_goal_assault` numeric reference function is
  **preserved** for compatibility and original-DLL test evidence.
- `AssaultRuntimeConditionBindings` binds verified semantic condition
  names to host-chosen `RuntimeAssaultConditionId` values. Legacy
  IDs and runtime IDs are different C++ types. Unknown semantic keys,
  conflicting rebinds, duplicate runtime IDs and the invalid sentinel are
  rejected. An identical rebind is idempotent.
- `begin_goal_assault_with_runtime_conditions` refuses to touch actors
  when Provoked is not registered. Otherwise it translates the semantic
  event to the host's runtime ID.
- `check_assault_cue_with_runtime_conditions` asks the host about HearCombat
  only on original cue mode 2, excluding the received-cue-4 override.
  Missing registration returns no result instead of querying a guessed
  engine ID. Modes 1 and 3 use the original cue gate and do not call
  the host's combat-hearing sensor.

Tests cover duplicate and invalid mapping, fail-before-mutation,
per-actor callback order, skipped/unresolved actors, native ID translation,
and precise mode-2 query gating.

## Concrete next integration gate

Once a compatible engine is available, the real NPC adapter must register
native conditions by name, record the returned native IDs in the binding
registry, connect `COND_PROVOKED` to actual NPC state updates and
`COND_HEAR_COMBAT` to the engine sensor, then verify behavior with a real
SiN map. This bridge **does not** implement or prove those interfaces.

## Evidence boundaries

Original code: only the already audited **2006 x86** condition names,
numeric IDs, cue-mode branch and goal-input ordering.

New port design: strongly typed native IDs, duplicate-registration
policy, missing-registration safety, callback shape and tests.

No original DLLs, source assets, BSPs, engine pointers or real SourceVR
game modules are included or altered. This change is intentionally a
portable reference step, not a claim of working Quest 3 gameplay.
