# SiN Episodes: Emergence — Security-Bot Pulse Projectiles and Jetpack AI (Deep Dive 16)

**Research date:** 2026-11-05. **Method:** original 2006 x86 server/client PE static analysis and five SiN-map-under-HL2 console logs; 56 independently re-run, SHA-pinned assertions. No original game binary executed and no gameplay port code changed.

- Original server SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- Original client SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- Logs: 1,531 unknown-entity creation attempts across 49 distinct classnames. These include reload repeats, not unique map placements.

## 1. An important hidden dependency: `pulse` → `CPulseShot`

The console logs identify `npc_securitybot` (16 failed creations) but never attempt `pulse`. Nevertheless, the original server has a **real `pulse` entity factory** with C++ class `CPulseShot` at registration VA **`0x223C7777`**. The security-bot animation-event handler at **`0x2237F280`** selects its alternating gun attachment and calls **`0x2237ED60`** at call site **`0x2237F3B7`**. That helper requests the literal classname `pulse` and creates the entity through original factory methods at **`0x2237ED69`** and **`0x2237ED78`**. The event handler then scales a launch vector by a compiled float **1500.0** (data VA `0x224A3198`) and calls a velocity-related function at **`0x2237F408`**.

Security bot event ID **`0x43`** increments a selector at `+0xD20`, wrapping it to zero after one, and indexes attachment IDs held at `+0xD18/+0xD1C`. The attachment strings are `gun_right` and `gun_left`. A separate event ID **`0x95`** uses a random interval between 5.0 and 6.0 seconds to set warning time at `+0xD24`; the code also names `NPC_CScanner.PreShoot`. The shot path names `NPC_CScanner.Shoot`. **These numeric event IDs are not yet recovered as original symbolic names.**

The original `CPulseShot` RTTI hierarchy begins `CPulseShot -> CBaseCombatCharacter -> CBaseFlex`, rather than a standalone inheritance from a named projectile base. Its primary vtable is **`0x224A32D4`**, with **7 changed primary-vtable slots** compared to `CBaseCombatCharacter`. Its datamap declares three records, but all three need static-constructor decoding.

### The projectile has its own lifecycle

- `CPulseShot::Spawn` candidate at **`0x223803F0`**.
- Precache function at **`0x2237EB80`**.
- Spawn registers a think thunk at **`0x22380200`** (which forwards to **`0x22380010`**) and touch callback at **`0x2237F4C0`**.
- The first think is scheduled after a compiled **0.25 seconds** (`0x224A31A8`), not evidence of a 0.25-second projectile lifetime.
- The touch callback checks the contacted entity before entering a complicated response path. Its damage/effect constants, collision filters, and removal timing require further decoding.
- The think path resolves a target handle at `+0x790` and applies several calls to that target; the exact original effect names remain unresolved.

**Implication for recompilation:** a future `npc_securitybot` implementation must account for `CPulseShot` as a transitive gameplay dependency. Restoring its model and schedule names alone will not restore shooting.

## 2. Three original jetpack registrations, not one

| Original map classname | Compiled C++ class | Registration VA | Differing primary-vtable slots versus common jetpack base |
|---|---|---|---:|
| `npc_druglab_jetpack` | `CNPC_DruglabJetpack` | `0x223C68B7` | 5 |
| `npc_druglab_elite_jetpack` | `CNPC_EliteJetpack` | `0x223C6A97` | 4 |
| `npc_sintek_jetpack` | `CNPC_SintekJetpack` | `0x223C77C7` | 4 |

Each uses the parent `CNPC_JetpackGruntBase`, whose RTTI parent is `CNPC_GruntBase`, followed by `CAI_BaseHumanoid`. The parent changes **31** slots versus the standard Grunt base. The original code also creates an attached **`prop_dynamic`** entity during common spawn at **`0x22367FF9`** and stores its handle at **`+0x1028`**. This is original binary behavior; its exact parent attachment and physical effect need more work.

The original compilation includes dedicated schedule and task descriptions for:

- `SCHED_JETPACK_PATROL`
- `SCHED_JETPACK_CHASE_ENEMY`
- `SCHED_JETPACK_FOLLOW_HOVER`
- `SCHED_JETPACK_RANGE_ATTACK1`
- `SCHED_JETPACK_MOVE_TO_INSPECT`
- `SCHED_JETPACK_SHOOT_COVER`

The patrol schedule text contains `TASK_GET_PATH_TO_RANDOM_NODE 1024` and `TASK_SET_TOLERANCE_DISTANCE 32`; chase has `TASK_SET_TOLERANCE_DISTANCE 120`; follow hover uses `TASK_WAIT 0.1`. This is **compiled schedule definition data**, not a live runtime trace of the NPC actually choosing the schedules. The code also contains `g_jetpack_new_anims`, `g_debug_jetpack`, `g_debug_jetpack_path_distance` and `sk_grunt_jetpack_health` identifiers.

The logs only include `npc_druglab_jetpack` among the three, with **29** failed creation attempts. The other variants and `pulse` do not appear in those logs. The 49-classname count is therefore a **console-observed minimum**, not a complete map of every original entity we may need.

## 3. Reproducibility and limitations

The accompanying [read-only verifier](../verifiers/sin_jetpack_securitybot_pulse_audit_16.py) checks the original DLL hashes, 5 entity factories, 6 comparative vtable layouts, RTTI parents, call edges, instruction anchors, compiled floats, jetpack schedule strings and captured console counts. Local run: **56 assertions passed**.

Original game DLLs and assets are intentionally **not** redistributed. Verification requires user-owned original DLLs and the five privately supplied logs, with the existing atlas helper script on `PYTHONPATH`. Addresses are PE VAs, not Quest/ARM64 addresses.

**Unresolved:** the actual jetpack flight physics/movement control loop; security-bot projectile collision and damage rules; exact symbolic names of `0x43` and `0x95` animation events; runtime conditions that select the compiled jetpack schedules; the three `CPulseShot` datamap records. Distinguish original compiled capability from observed in-game behavior.

**No Quest 3 port code was modified.**