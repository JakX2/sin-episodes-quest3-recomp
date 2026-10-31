# Early SiN game-code reconnaissance (historical conversation excerpt)

**Evidence level:** Historical findings from a preserved earlier project conversation. The original script, full RTTI dump, and Phase 1 evidence bundle are not retained here. This file records those findings without claiming a new audit.

## Original SiN binary inventory (reported at the time)

- `server.dll`: Windows 32-bit x86, 6,049,792 bytes.
- `client.dll`: Windows 32-bit x86, 3,592,192 bytes.

## NPC class tree (RTTI-derived in earlier work)

```text
CAI_BaseNPC
  -> CAI_BaseHumanoid (intermediate Source behavior-host classes omitted)
     -> CNPC_GruntBase
        -> CNPC_SintekGrunt
        -> CNPC_JetpackGruntBase
           -> CNPC_SintekJetpack
```

This is **not** a claim that SiN grunts inherit directly from HL2's `CNPC_Combine`; they reuse Source AI foundations with their own hierarchy.

## Schedules and behavior inventory

Earlier work reported **38 grunt schedules**. Examples preserved in the chat excerpt:

| Identifier | Descriptive interpretation (not implementation) |
|---|---|
| `SCHED_GRUNT_SUPPRESS` | Suppression |
| `SCHED_GRUNT_DODGE` | Dodge |
| `SCHED_GRUNT_KICK_GRENADE` | Kick a grenade |
| `SCHED_GRUNT_HELP_STUNNED` | Assist stunned squadmate |
| `SCHED_GRUNT_HELMET_FLINCH` | Helmet-hit reaction |
| `SCHED_GRUNT_TAKE_COVER1` | Take cover |

The same conversation reported **seven named jetpack-specific tasks**, covering such behaviors as flying patrol, chase and dive. **Task names alone do not establish branching logic or combat correctness.**

## Dynamic difficulty labels observed earlier

`dd_enabled`, `dd_challenge`, `dd_forgiveness`, `dd_nosaveallowed`, `dd_arenamode`, `g_debug_dynamic_difficulty`; `CTriggerDynamicDifficulty` was reported as inheriting Source trigger types. Reconstructing exact modifier rules requires more than these names.

## Open work

Recover and verify full grunt schedule/condition/task registration, behavior selection functions and jetpack decision chains. Implement AI on a compatible Source branch and validate against game behavior. Do not present the schedule inventory as complete AI source reconstruction.
