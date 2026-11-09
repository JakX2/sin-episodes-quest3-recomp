# Phase 130: independently reconstructed CSE1Rules gameplay hooks

**Scope:** portable C++ reconstruction of four small rules functions identified in the preserved Phase 5 report. This is not Ritual source, a Source game-rules vtable, or a complete dynamic-difficulty system.

## Evidence provenance and limitations

The user's Library contains the Phase 5 report dated 2026-10-30 and its read-only verifier. The report identifies the original SiN Episodes 2006 `server.dll` by SHA-256:

`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

Preferred image base: `0x22000000`.

The preserved report says the original DLL was statically disassembled, never executed, and that its verifier checks the SHA-256, RTTI hierarchy, 114-entry CSE1Rules/CSingleplayRules vtables, selected function pointers and instruction signatures. The raw `bin.zip` and separate `client.dll` Library records were found during this run, but Project storage denied raw-byte materialization. The Phase 5 verifier was therefore **not rerun** in this run. The functions below are based on that preserved report, not fresh binary inspection.

The report identifies 29 changed entries in the 114-entry primary `CSE1Rules` vtable. It does not prove that those old x86 slots or addresses exist in SourceVR's ARM64 engine.

## Implemented behaviors

| Original CSE1Rules slot | Original VA | Modifier | Reconstructed behavior |
|---:|---|---|---|
| 62 | `0x222D47F0` | index 1, `DAMAGE_DONE_SCALE` | Multiply the passed float |
| 63 | `0x222D5420` | index 0, `DAMAGE_TAKEN_SCALE` | If `damage_type & 0x80024021` is zero, multiply the damage float; otherwise leave it unchanged |
| 100 | `0x222D4A00` | index 7, `PERCENT_NPC_CHANGE_SPAWN_NUM` | Compare random sample with absolute modifier; add one for positive modifier, subtract one for negative modifier, otherwise unchanged |
| 107 | `0x222D4CF0` | index 11, `RANDOM_NPC_WAIT_BEFORE_SHOOT` | Add the modifier to the passed wait float |

The Phase 5 report gives the following original spawn-count instruction locations:

- Modifier index 7 getter call: `0x222D4A00`.
- Increment instruction: `0x222D4A5F`, `add eax, 1`.
- Decrement instruction: `0x222D4A66`, `sub eax, 1`.
- Unchanged-count path: `0x222D4A6D`.

The 32-bit increment/decrement helper preserves the original x86 EAX modulo-2^32 arithmetic for boundary values. The Quest target is also two's-complement ARM64.

The report records modifier 7 endpoints approximately -0.1 and +0.4 at the original table entry near `0x2252353C`. It does not fully establish the runtime RNG provider, its range, every float edge case, or which spawners call slot 100. This implementation accepts the caller-supplied random sample and does not invent an RNG.

## Integration boundary

These functions are pure, portable hooks. They are not wired into `apply_common_alive_damage` because the full original damage-hook ordering and call-site chain have not yet been reconstructed. In particular, this PR does not claim that the damage-taken scale is applied before or after the separate vehicle and class-specific modifiers.

Likewise, the spawn-count function is not yet connected to a real `npc_maker` or SiN spawner. The portable test proves the reconstructed branch behavior only.

## Explicitly not implemented

- The original dynamic-difficulty challenge interpolation and modifier table lifecycle.
- The engine RNG provider and exact random call sites.
- NPC variant selection, helmet probabilities, cover probability, health-kit logic, or secondary-fire probability.
- Individual semantic names for the bits in `0x80024021`.
- A SourceVR game-rules adapter, entity factory, native client/server integration, or Quest execution.

No proprietary DLL, APK, original ELF, BSP, model, map, script, or game asset is included.
