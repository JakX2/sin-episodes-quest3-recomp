# Retail SiN Episodes `SE1_U4Lab05`: Assault Rifle Grunt schedule and failure recovery (`condump001.txt`)

## Source and confidence

Owner-supplied read-only console dump `condump001.txt`, SHA-256 `1dd519e3ad2f5f8f3722f8e303238d515ac1214af5afcaa04fa07c6a1a89f6b5`, 22,758 bytes / 304 text lines. This was collected after the Phase 534 owner test; only this analysis is committed, not the raw log or original game assets. All observations below concern the **retail game** running the original SiN gameplay modules, **not** the independent rebuilt DLLs. The console states `Game .dll loaded for "SiN Episodes"` but the dump alone does not cryptographically attest the loaded DLLs. Bracketed counters like `[1513]` are treated as opaque counters, not seconds or wall-clock times.

The console text itself contains dropped or misspelled letters (`Found npc_druglab_gunt_assault_rifle`, `Shedule:`, `SCHE_MELEE_ATTACK1`, `TASK_GAMELAY_WAIT_RANDOM`, shortened bracketed numbers). Count only exact spellings when giving audited exact-match totals; contextual near-matches are recorded explicitly but **not silently normalized**.

## Map and spawn sequence

- `map SE1_U4Lab05` loads and reports `Game started`.
- `sv_cheats 1`, `developer 2`, `god` / `godmode ON` precede NPC creation.
- First `npc_create npc_druglab_grunt_assault_rifle` prints `SUB_Remove called on entity with health > 0` and `Can't create npc_druglab_grunt_assault_rifle.  Bad Position!`. No post-failure NPC task sequence immediately follows. Do **not** infer a broken AI constructor or memory ownership from this single spawn-placement rejection.
- The second identical `npc_create` prints `Found npc_druglab_gunt_assault_rifle that isn't in a squad` (literal console spelling) and subsequent trace lines consistently identify the NPC as `npc_druglab_grunt_assault_rifle: 94/<unnamed>`.
- The source contains **no** `npc_create npc_druglab_grunt_scattergun` command in this run. Entering `npc_task_text npc_druglab_grunt_scattergun` later did **not** produce a Scattergun NPC task trace; output continued for Assault Rifle Grunt entity 94. Do not attribute those task lines to the Scattergun.

## Scheduling evidence

Strict intact-prefix schedule-entry counts in the raw console:

| Exact displayed schedule | Intact `Schedule:` entries | Additional visibly damaged near-match |
|---|---:|---|
| `SCHED_GRUNT_RANGE_ATTACK1` | 24 | Two `Shedule:` / `Schdule:` entries with same remaining schedule name |
| `SCHED_GRUNT_MOBILITY_SIDESTEP` | 2 | None required |
| `SCHED_MELEE_ATTACK1` | 4 | One `Schedule: SCHE_MELEE_ATTACK1` entry |
| `SCHED_GRUNT_COMBAT_FAIL` | 1 | None |

There are **25** exact `Task: TASK_RANGE_ATTACK1` entries and **5** exact `Task: TASK_MELEE_ATTACK1` entries; one extra ranged-task line has `Task: TAK_RANGE_ATTACK1`. Because task tracing was enabled mid-combat and output is imperfect, these are occurrences in this dump, not total attacks fired or a precise performance rate. They do not independently establish bullet impact/damage or confirm the gun's visible model.

Repeated ranged schedule structure (clearest uncorrupted block at raw counter `[1520]`):

1. `TASK_STOP_MOVING`
2. `TASK_FACE_ENEMY`
3. `TASK_ANNOUNCE_ATTACK`
4. `TASK_GAMEPLAY_WAIT_RANDOM`
5. `TASK_RANGE_ATTACK1`
6. `TASK_GRUNT_IGNORE_ATTACKS` appears before the next schedule block.

Ranged attacks are visible at `[1513]`, `[1548]`, `[1569]`, etc.; this independently corroborates the Pistol Grunt's repeated `SCHED_GRUNT_RANGE_ATTACK1` from owner `condump000.txt`. Multiple melee-schedule blocks at `[1912]`, `[1961]`, `[2010]`, `[2059]` then occur, each showing `TASK_STOP_MOVING`, `TASK_FACE_ENEMY`, `TASK_ANNOUNCE_ATTACK`, `TASK_MELEE_ATTACK1`. Another damaged melee label appears at `[2108]`. The console also emits `Missing caption for 'npc_grunt.gunmelee'`; this is a caption warning, not proof of melee damage.

## Two distinct mobility outcomes

**First sidestep around `[1639]`:** `SCHED_GRUNT_MOBILITY_SIDESTEP`, `TASK_GRUNT_SIDESTEP_GOAL`, `TASK_RUN_PATH`, `TASK_WAIT_FOR_MOVEMENT`, followed by `TASK_FACE_ENEMY` around `[1723]` and another ranged schedule. No explicit task failure is shown for this first sidestep. This suggests navigation task progress, but the log cannot prove route completion or distance traveled.

**Second sidestep around `[2157]`:**

```text
Schedule: SCHED_GRUNT_MOBILITY_SIDESTEP
Task: TASK_GRUNT_SIDESTEP_GOAL
TaskFail -> Don't have a route
TaskFail -> Don't have a route
(failed)
Schedule (SCHED_GRUNT_MOBILITY_SIDESTEP) Failed at 0!
Schedule: SCHED_GRUNT_COMBAT_FAIL
Task: TASK_STOP_MOVING
Task: TASK_SET_ACTIVITY
Task: TASK_WAIT_FACE_ENEMY
Break condition -> COND_CAN_RANGE_ATTACK1
Schedule: SCHED_GRUNT_RANGE_ATTACK1
```

This is a positive retail observation of **fallback and recovery**: an unavailable sidestep route leads to a combat-fail schedule and then an interrupt condition that resumes ranged combat. The original displayed failure index is `0`, unlike the separate `SCHED_GRUNT_ASSAULT` failure index `2` in `condump000.txt`. Do not conflate them.

The portable Phase 133 Grunt translator recognizes a combat-failure translation branch as a *separate* original-static-analysis result. This retail trace corroborates that the **named combat-failure schedule occurs**, but alone does not identify the exact numerical translator input, virtual branch guard, vtable call, or class member responsible for selecting it. Similarly, it does not directly verify Phase 534's seven native CAI component pointers, only functional downstream AI output from the original engine.

## Concurrent unrelated output and remaining questions

Map I/O for `item_ammo_magnum` / `item_ammo_buckshot` and spawner `ForceSpawn` fires during combat; no evidence ties these to Grunt weapon selection or spawn ownership. Missing captions for `solidmetal.bulletimpact`, `flesh.bulletimpact`, grunt footstep sounds and `npc_grunt.gunmelee` are localization diagnostics, not confirmed failures of Grunt attacks. `Soundscape: docks.beach` is not an AI discovery. The text includes several likely console-rendering glitches, so exact counts must retain that caveat.

**Next reproducible retail check:** isolate the three Grunt variants across map reloads in an open navigable part of `SE1_U4Lab05`; confirm visually whether each carries the intended weapon and whether shots produce visible effects. If targeting a single entity with `npc_task_text`, validate which actual NPC's classname/ID appears in each printed task line. Record the `condump` without presuming a named command parameter causes filtering. No need to change Source physics, surface properties, models or props based on this dump.

**Current scope verdict:** original-engine NPC spawn confirmed on second attempt; repeated ranged and melee AI confirmed; successful-looking and failed sidestep paths recorded; explicit `COMBAT_FAIL -> COND_CAN_RANGE_ATTACK1 -> RANGE_ATTACK1` recovery observed. Independent Windows `server.dll` runtime and Android/Quest port remain unproven.