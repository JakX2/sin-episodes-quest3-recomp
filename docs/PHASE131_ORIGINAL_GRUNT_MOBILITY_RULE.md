# Phase 131: original SiN grunt mobility rule and call-site factors

**Scope:** independently authored portable reconstruction of CSE1Rules slot 104 and four receiver-verified call sites in the original SiN grunt code. This is not a complete grunt schedule selector or native Source NPC.

## Evidence provenance

The user's Library retains the Phase 6 report, `grunt_rules_call_sites.csv`, and a separate `mobility_reference.cpp` sketch. They identify the same original server.dll SHA-256 used by the Phase 5 report:

`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

The Phase 6 report describes a receiver-aware x86 static inspection. The raw owner-supplied `bin.zip` was located during this run, but raw-byte materialization was denied. No fresh binary disassembly or verifier rerun is claimed.

## Recovered rule

CSE1Rules slot 104 points to original function VA `0x222D4B70`. The preserved Phase 6 report describes this calculation:

`threshold = modifier[13] * caller_factor * 0.2f`

- If the computed threshold is exactly zero, return false.
- Request a random float in the range [0, 1].
- Return true when `random <= abs(threshold)`.
- The threshold is not clamped to one; a sufficiently large factor can make the decision effectively guaranteed.

The portable helper accepts the random sample from its caller. It does not invent or replace the original engine RNG.

## Receiver-verified grunt call sites

| Original call site | Grunt-side caller | Factor | Context preserved in the code |
|---|---:|---:|---|
| `0x223587A3` | vtable slot 292 | 1.0 | Ordinary mobility decision |
| `0x223596A9` | vtable slot 530 | 4.0 | Branch selected by `COND_GRUNT_IN_PLAYER_SIGHTS` (grunt-local ID `0x4D`) |
| `0x223596B8` | vtable slot 530 | 1.0 | Alternative branch |
| `0x2235D889` | vtable slot 271 | 20.0 | Integer/schedule translation switch |

The later Phase 7 registration extraction identifies grunt-local condition `0x4D` as `COND_GRUNT_IN_PLAYER_SIGHTS`. The earlier Phase 6 report had only the numeric ID. The factor-20 call is inside a schedule-related translation path; it is not described as ordinary movement speed.

A separate scan hit at `0x223568E7` uses the NPC's own vtable slot 107, not the game-rules pointer. It is deliberately excluded from this rules-call-site mapping.

## Implementation

`original_grunt_mobility_rules.hpp` provides:

- `should_npc_be_mobile(modifier13, factor, random_sample)`.
- `GruntMobilityCallSite` values for the four observed contexts.
- `grunt_mobility_factor()` and `grunt_mobility_decision()`.

Tests cover inclusive threshold equality, the false zero-threshold path, negative modifiers, the 1/4/1/20 factors and the unclamped factor-20 case.

## Still unknown

- The shared condition ID `0x37` used by the dodge branch; its semantic name is not recovered in the grunt-local registry.
- The complete `CNPC_GruntBase::SelectSchedule` control flow and all rules decisions.
- Exact x87 NaN/denormal edge behavior and in-game random-seed comparisons.
- A real NPC host, game-rules interface, engine entity factory and campaign encounter.

This code does not transplant the original x86 vtable slots into SourceVR. It is a portable behavior function for a future legal host adapter. No proprietary DLL, APK, original ELF, BSP, model, map, script or game asset is included.
