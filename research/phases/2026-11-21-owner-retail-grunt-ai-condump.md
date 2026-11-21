# Owner retail SiN Grunt AI console probe: SE1_U4Lab05

**Evidence:** owner-provided `condump000.txt` from retail SiN Episodes: Emergence (2006), provided after Phase534's owner-side test. The report below records a bounded, manual, read-only interpretation. It is **not** an independent DLL implementation or proof that our reconstructed Windows `server.dll` boots. Original console log, assets, DLL bytes and paths are not committed.

## Preconditions visible in the console

- The game reports `Game .dll loaded for "SiN Episodes"`, then a map transition `map SE1_U4Lab05` and `Spawn Server SE1_U4Lab05` / `Game started`.
- The user sets `developer 2`, then `sv_cheats 1`, followed by `god` (`gdmode ON`).
- `con_logfile sin_grunt_runtime.log` is explicitly **unsupported** (`Unknown command: con_logfile`), while `condump` was invoked successfully at the end of the provided log. Future test instructions must use the 2006-compatible dump path rather than assume `con_logfile`.

## Three actual NPC creation requests

| Console request | Immediate classname recognition evidence | Count |
| --- | --- | ---: |
| `npc_create npc_druglab_grunt_pistol` | `Found npc_druglab_grunt_pistol that isn't in a squad` | 1 |
| `npc_create npc_druglab_grunt_scattergun` | `Found npc_druglab_grunt_scattergun that isn't in a squad` | 2 |
| `npc_create npc_druglab_grunt_assault_rifle` | **Not present** | 0 |

These lines establish that the **retail engine recognizes and reports the requested NPC classnames** on this map. The later task log additionally demonstrates live schedule activity for a pistol Grunt, but these logs do not prove which actual weapon model it carried, projectile/damage results, or whether all native `CAI_*` object pointers were constructed.

## Native Pistol Grunt task/schedule trace

The user's `npc_task_text npc_druglab_grunt_pistol` prints a named entity `npc_druglab_grunt_pistol: 94/<unnamed>` with **three** successive `SCHED_GRUNT_RANGE_ATTACK1` schedule announcements, at raw bracketed console counters `[1315]`, `[1343]`, and `[1378]`. The counter interpretation has not been established; do not relabel them game-time timestamps.

Observed task names, in their repeated order:

1. `TASK_STOP_MOVING`
2. `TASK_FACE_ENEMY`
3. `TASK_ANNOUNCE_ATTACK`
4. `TASK_GAMEPLAY_WAIT_RANDOM`
5. `TASK_RANGE_ATTACK1`

`TASK_GRUNT_IGNORE_ATTACKS` appears immediately before each next attack schedule announcement, including the line preceding the first. Three separate `TASK_RANGE_ATTACK1` task entries occur at bracketed counters `[1336]`, `[1371]`, and `[1406]`. The second instance of the wait task is rendered as `TASK_GAMELAY_WAIT_RANDOM` and an earlier NPC classname as `npc_druglab_grut_pistol`; these are **raw console spellings**, not evidence of different task types or class registrations. Preserve the raw strings rather than silently fix the trace.

This supports an observable retail reference for Phase132/133 Grunt schedule work. It does **not** prove that actual bullets hit the player or that the weapon-mode integer at original owner `+0x3B0` was written to a particular value. Those still require weapon/readback or visual verification.

## Assault navigation failures

Two error pairs occur after the two scattergun creation commands:

```
GetPathToEnemyLKP failed!!
Schedule (SCHED_GRUNT_ASSAULT) Failed at 2!
```

These are **schedule/pathfinding** failure diagnostics, not proof of allocation failure. They are temporally associated with the scattergun spawns but the error lines do not print an NPC entity ID; attribution to either newly created Grunt is probable, not directly established. The map could lack appropriate route/path context at console-selected spawn positions. `Failed at 2` is a raw failure position and must not be assigned a guessed specific task.

Other console warnings (`Missing caption for ...`, `materials/models/Weapons/V_slam/new light1.vtf` not found, absent `maplist.txt`, scene actors during background startup) were present, but this console log does not establish that they caused the Grunt assault failures. Per owner priority, no generic physics/model/material work follows from these warnings.

## How this changes next verification

The console record supports this exact test matrix as the next owner-side baseline:

```text
map SE1_U4Lab05
developer 2
sv_cheats 1
npc_create npc_druglab_grunt_assault_rifle
npc_task_text npc_druglab_grunt_assault_rifle
npc_task_text npc_druglab_grunt_scattergun
condump
```

Confirm which NPCs are visibly present and armed, and whether attack behavior occurs. Repeat each variant in isolation (reload the map between tests) if squad/navigator or map-state effects obscure attribution. Only compare native Grunt scheduling with reconstructed code; do not treat retail load of the original DLLs as independent-port startup.

## Scope boundary

- **Verified by this owner console:** original game+map startup, three `npc_create` requests recognized (pistol once, scattergun twice), named Pistol Grunt task/schedule activity, two assault-path failure pairs.
- **Not verified by this owner console:** assault-rifle variant, weapon model/ammo/damage, all seven CAI pointers, unmodified versus modified DLL SHA, reconstructed DLL runtime boot, or Android/VR gameplay.