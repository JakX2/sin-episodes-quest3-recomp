# Phase 134: original SiN grunt `TASK_GRUNT_HELP_UP` task paths

## Scope and provenance

This is an independently authored, engine-neutral C++ reconstruction based on the preserved Phase 10 static-analysis report and Phase 11 original datamap evidence for the owner-supplied 2006 `server.dll` (PE32/x86, SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`). The original DLL was statically inspected, never loaded or executed. No fresh disassembly or original-binary verifier run is claimed in this phase.

## Recovered task dispatch

The Phase 10 report identifies `CNPC_GruntBase` vtable slot 273 at `0x2235BFD0` as the StartTask-style dispatcher and slot 275 at `0x2235CCE0` as the RunTask-style dispatcher. Those names are high-confidence behavioral inferences, not recovered debug symbols.

The registered grunt-local task `TASK_GRUNT_HELP_UP` has ID `0x94`:

- Start case: `0x2235C734`.
- Update case: `0x2235CF4E`.

The task ID is local to this 2006 grunt registry. It is not a Source SDK task ID, a schedule ID, or a task ID for another NPC class.

## Confirmed effects in both paths

Both case bodies read `this + 0xCA4`, call helper `0x2235A0F0` on the pointed-to object only when non-null, clear `this + 0xCA4`, and reach the common task-completion-style helper `0x2203D2D0` with argument zero.

Phase 11's datamap extraction names the field at `+0xCA4` `m_pWounded` with field type code 12 (`FIELD_CLASSPTR` in the historical Source field-type numbering). It is not the `EHANDLE` field type. The portable host therefore supplies an opaque host-owned token, not a raw pointer or a generation-checked entity handle.

The instruction anchors preserved by the Phase 10 verifier include:

- `0x2235C734`: `8b 8e a4 0c 00 00` (load `m_pWounded`).
- `0x2235C747`: `6a 00 8b ce c7 86 a4 0c 00 00 00 00 00 00` (shared null/complete path, including clear and zero argument).
- `0x2235CF4E`: `8b 8e a4 0c 00 00` (load `m_pWounded`).
- `0x2235CF5D`: `c7 86 a4 0c 00 00 00 00 00 00` (clear `m_pWounded`).

The portable implementation preserves the observable ordering: optional target callback, unconditional target-field clear, then task-completion callback with zero. It deliberately does not assign a semantic name to `0x2235A0F0` or claim the wounded actor is revived, healed, animated, or otherwise changed in a particular way.

## Inherited behavior

For every task ID other than `0x94`, the portable dispatcher explicitly delegates to the host's inherited humanoid start/run path. This includes update task IDs outside the grunt update dispatcher's explicit `0x01..0x9A` range; those IDs are not treated as no-ops.

## Tests

The C++ regression test covers both start and update paths with null and non-null targets, callback ordering, unconditional clear, zero completion argument, parent fallback for other task IDs, and the `0x9B` update ID's inherited fallback.

These are portable host-contract tests. They do not execute the original DLL, instantiate an engine NPC, verify the unresolved helper's side effects, or constitute Quest 3 gameplay.

## Remaining work

- Trace helper `0x2235A0F0` and all writers/readers of `m_pWounded` / `m_pHelper`.
- Reconstruct the actual wounded-squadmate state transition and animation/task interaction.
- Connect the portable task host to a supported native game-code host; no SourceVR custom game-module loader has been established.
