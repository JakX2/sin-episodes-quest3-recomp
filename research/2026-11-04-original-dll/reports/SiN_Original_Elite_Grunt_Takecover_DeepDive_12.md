# SiN Episodes: Emergence | Elite Grunt, Chaingun State Machine & Take-Cover AI, Deep Dive 12

**Research date:** 2026-11-04. **Scope:** Original 2006 32-bit PE `server.dll`/`client.dll`, archived `scripts.zip` and five HL2 compatibility console logs supplied for the SiN Episodes: Emergence standalone Quest 3 research project. **Read-only static reverse engineering, not a complete source recovery; no edits to game DLLs, Google Drive files or the Quest 3 project.**

This chapter continues [Deep Dive 11](SiN_Original_Mob_Grunt_Combat_DeepDive_11.md). The Python auditor `sin_elite_cover_audit_12.py` reruns **98 SHA-pinned static assertions** and outputs `sin_elite_cover_evidence_12.json`, including original virtual addresses, datamaps, field/parameter records, and asset/log cross-references. Addresses are absolute **original PE image virtual addresses**, not disassembly file offsets.

## 1. Source and diagnostics

- `server.dll` SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- Factories verified by RTTI: `npc_druglab_grunt_elite` → `CNPC_EliteGrunt`, `weapon_chaingun` → `CWeaponChaingun`, `ai_goal_takecover` → `CAI_TakeCoverGoal`. Regular pistol/scattergun/assault variants map to `CNPC_MobGrunt` (from the previous round).
- In the five collected HL2 console logs, **69** attempts to create `npc_druglab_grunt_elite` and **26** attempts to create `ai_goal_takecover` failed. These are failed *attempts*, not unique entities. The cumulative dataset still contains **1,531** failed creations for 49 names.

## 2. Elite Grunt: separate weapon reference and firing gate

`CNPC_EliteGrunt` declares exactly two of its own named datamap entries, in addition to the inherited `CNPC_GruntBase` data:

| Original member | Offset | Data type | Role established |
|---|---:|---|---|
| `m_canFire` | `+0xFD9` | Boolean | Set from a weapon-control query, then tested by another elite handler |
| `m_pChaingun` | `+0xFDC` | Entity handle | References the currently equipped `CWeaponChaingun` when available |

The primary elite vtable is at `0x2248DF04`, and its spawn override, slot `+0x64`, is at **`0x22355820`**.

**Verified spawn chain:**

1. Checks and, when appropriate, configures inherited weapon name state (`+0xA24`).
2. Invokes the inherited `CNPC_GruntBase` spawn function at `0x2235F2A0` (call at `0x223558CF`).
3. Clears `m_canFire` at `0x223558DA` and uses a configuration-controlled value to set both current health (`+0x9C`) and maximum health (`+0x98`). The exact runtime health value was **not** established.
4. Obtains the current equipped weapon using `0x222AA640` (call at `0x223559E4`), and performs the RTTI type test at **`0x223559EA`**. The original RTTI descriptors name `CBaseCombatWeapon` as source and `CWeaponChaingun` as destination.
5. If the type check succeeds, it captures the weapon's entity handle at `+0xFDC` (`0x223559FF`); if the test fails, it writes `0xFFFFFFFF` (invalid handle) (`0x22355A0A`). The elite constructor initializes this handle to invalid at `0x22355D1C`.

The handle is then repeatedly checked using Source's entity-handle index and serial-generation fields, including generation validation, before the elite invokes weapon-specific methods. This is not simply a raw weapon pointer.

**Elite dispatchers (partial decompilation):**

- At `0x22355480`, an animation/event-related override branches on event values **`3`, `0x43` (67), and `0x47` (71)**. The `0x43` branch resolves the chaingun, invokes its control-query function `0x22315320`, then stores the returned byte in `m_canFire` at `0x2235550A`. The `0x47` branch reaches `CWeaponChaingun` function `0x22315440`, which forces internal state **3**.
- A separate override at `0x223556A0` also distinguishes `0x43` and `0x47`; the `0x43` path reads `m_canFire` and re-queries the weapon controller. These numeric event codes are recovered; their original human-readable symbolic names are **not independently proven**.
- The elite vtable slot `+0x424` points to `0x223555F0`; it checks the handle, calls the weapon's state update `0x22315390` at `0x22355645`, then tail-jumps to the inherited update at `0x2235DF10`. This is an important verified coupling between NPC logic and weapon logic.

## 3. Chaingun: compiled four-state spin/control sequence

`CWeaponChaingun` includes an internal state word at **`+0x594`** and two timing fields at **`+0x58C`**, **`+0x590`**.

- `0x22315320`: a state-dependent query. The exact four-entry jump table at `0x22315374` dispatches states **0 → `0x22315335`**, **1 → `0x2231536A`**, **2 → `0x2231536E`**, **3 → `0x2231536A`**. The state-0 path enters state 1. The state-2 path returns true; states 1/3 return false.
- `0x22315390`: updates state progression using current game time minus the stored timestamp. For state 1, after the threshold it writes **2**. For state 2, after the threshold it writes **3**. For state 3 it performs virtual weapon actions and resets to **0**.
- `0x22315440`: unconditionally sets state **3**.
- The threshold referenced by both timed comparisons is the exact floating-point constant **`1.0f`** at `0x22477C5C`.

**Conservative pseudocode (only confirmed branches):**

```cpp
// Partial recovery from CWeaponChaingun, not original authored source.
bool QueryFireReadiness() {
    switch (state_0x594) {
        case 0: StartWeaponAction(); state_0x594 = 1; return false;
        case 1: return false;
        case 2: return true;
        case 3: return false;
    }
    return false;
}

void UpdateWeaponState() {
    switch (state_0x594) {
        case 1:
            if (gameTime - timestamp_0x590 >= 1.0f) {
                state_0x594 = 2;
                timestamp_0x590 = gameTime;
                timestamp_0x58C = gameTime;
            }
            break;
        case 2:
            if (gameTime - timestamp_0x58C >= 1.0f) state_0x594 = 3;
            break;
        case 3:
            InvokeWeaponResetActions();
            state_0x594 = 0;
            break;
    }
}
```

The original `scripts/weapon_chaingun.txt` supplies `clip_size=30`, `default_clip=30`, `primary_ammo=CHAIN`, `ITEM_FLAG_NOPLAYERPICKUP=1`, and separate sound references `Weapon_Chaingun.NPC_SpinUp` and `Weapon_Chaingun.NPC_SpinDown`. This is compatible with a spin-control interpretation, **but the precise animation-event-to-sound mapping is not yet recovered**. The script also has its own `Weapon_Chaingun.NPC_Single` and `Weapon_Chaingun.NPC_Reload` references. `scripts/npc_sounds_elite.txt` provides separate elite NPC sound definitions.

The compiled AI schedule text contains exactly these two elite-specific schedule spellings:

| Elite schedule | Tasks from compiled schedule description |
|---|---|
| `SCHED_ELITE_MELEE_ATTACK1` | Stop, face enemy, announce attack, perform melee attack |
| `SCHED_ELITE_RANGE_ATTACK1` | Stop, face enemy, announce attack, **four consecutive `TASK_RANGE_ATTACK1` steps**, then `TASK_GRUNT_IGNORE_ATTACKS` with parameter `0.1` |

The ranged schedule's interrupt list includes new/dead enemy, heavy damage, low/no primary ammo, blocked-by-friend, too-close-to-attack, give-way, danger/move-away sounds, and `COND_GRUNT_NO_FIRE`. This is task data; we still need to trace the actual scheduler's selection logic and animations to determine observed combat cadence.

## 4. Take-cover goal: an actual behavior configurator, not a decorative trigger

The factory `ai_goal_takecover` creates **`CAI_TakeCoverGoal`**, whose primary vtable is `0x224AE37C` and own datamap is at `0x2253B68C` (8 statically readable records).

| Map key | Member | Object offset |
|---|---|---:|
| `Aggressiveness` | `m_aggressiveness` | `+0x3C8` |
| `AbandonIfEnemyHides` | `m_bAbandonIfEnemyHides` | `+0x3CC` |
| `CustomCoverOnReload` | `m_customParams.fCoverOnReload` | `+0x3D0` |
| `CustomMinTimeShots` | `m_customParams.minTimeShots` | `+0x3D4` |
| `CustomMaxTimeShots` | `m_customParams.maxTimeShots` | `+0x3D8` |
| `CustomMinShots` | `m_customParams.minShots` | `+0x3DC` |
| `CustomMaxShots` | `m_customParams.maxShots` | `+0x3E0` |
| `CustomOddsCover` | `m_customParams.oddsCover` | `+0x3E4` |

At `0x223999F0`, the constructor sets `m_aggressiveness=0`, `m_bAbandonIfEnemyHides=false`, and copies a 28-byte default custom parameter structure from `0x2253B5A8`. That custom default contains `coverOnReload=1`, `minTimeShots=1.5`, `maxTimeShots=2.5`, `minShots=1`, `maxShots=3`, `oddsCover=25`, and an unidentified seventh word equal to zero.

### Five compiled aggressiveness presets

At **`0x22399B80`**, the goal uses `m_aggressiveness` as an index into a table beginning at **`0x2253B600`**, with **`0x1C` (28) bytes per entry**. Five complete rows are identifiable before unrelated data begins.

| Index | Cover on reload | Min shot time | Max shot time | Min shots | Max shots | Cover odds value | Unnamed float |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 0 | 1 | 4.0 | 8.0 | 2 | 4 | 50 | 30.0 |
| 1 | 1 | 2.0 | 5.0 | 3 | 5 | 25 | 20.0 |
| 2 | 1 | 0.6 | 2.5 | 3 | 6 | 25 | 10.0 |
| 3 | 1 | 0.2 | 1.5 | 5 | 8 | 10 | 10.0 |
| 4 | 0 | 0.0 | 0.0 | 100 | 100 | 0 | 5.0 |

`minTimeShots`/`maxTimeShots` and `oddsCover` above are labels grounded in the actual datamap members. **Do not assume cover odds are literal percentages** without following the consumer code. The unnamed seventh float is retained as unresolved, even though the goal has an independently named `AbandonIfEnemyHides` field. A complete validation check for out-of-range `Aggressiveness` values has not yet been located.

### Actual goal-to-NPC behavior connection

- At **`0x22399B00`**, the goal queries associated AI objects and uses RTTI to find a **`CAI_TakecoverBehavior`** derived from `CAI_BehaviorBase` (RTTI type descriptors at `0x2253BDEC` and `0x224E4BC4`). This is the missing link between the map entity and an NPC behavior object.
- At `0x22399B80`, it selects a preset, copies **7 DWORDs / 28 bytes** into a temporary buffer, and copies those parameters into the resolved behavior object's substructure (`+0x14`).
- If `m_bAbandonIfEnemyHides` is **false**, it explicitly zeroes the last DWORD in the temporary preset (`0x22399BB2`) before copying it. Thus the flag demonstrably changes configuration transmitted to the behavior, although the downstream semantic interpretation of the seventh field remains to be established.
- The routine calls two further helpers, **`0x220B1A80`** and **`0x220B1AA0`**, when the behavior is associated with an actor, and a further vtable-mediated actor routine.
- Two enclosing callbacks (`0x22399AA0`, `0x22399C20`) send the numeric event values **`0x3E8` (1000)** and **`0x3E9` (1001)** respectively through the associated actor/AI interface. Their exact named enumeration and full effects require additional tracing.

Partial pseudocode, **not verbatim C++ source**:

```cpp
// Inside CAI_TakeCoverGoal, partial reconstruction.
CAI_TakecoverBehavior* behavior = FindAttachedTakecoverBehavior(goalActors);
if (behavior) {
    Param7 params = presetTable[m_aggressiveness];
    if (!m_bAbandonIfEnemyHides)
        params.unnamedSeventhField = 0.0f;
    behavior->params = params;
    UpdateActorCombatTuning(behavior);
}
```

**Important limitation:** The `Custom*` fields are real, compiled mapper keys, but **their exact application path has not yet been identified** in `0x22399B80`. The preset-table path is confirmed; don't assume the custom override path just because the fields exist.

## 5. Why this matters for HL2 and Quest 3

The entity failure logs show that stock HL2 can't construct **either** the elite NPC **or** its `ai_goal_takecover` behavior controller. Replacing the elite with a generic Combine NPC and treating its weapon as a rapid-fire hitscan gun would omit the recovered handle-validation path, custom event dispatch, timed chaingun state machine, and special AI schedules. Similarly, replacing `ai_goal_takecover` with an inert map entity would lose its per-NPC cover/burst configuration.

Those are now verified functional dependencies. They are **not** evidence that every arena encounter or campaign map exercises every schedule or preset, and the compiled code has not been recompiled into a playable port.

## 6. Next research targets

1. Resolve the elite's symbolic animation event IDs and its `SCHED_ELITE_RANGE_ATTACK1` selection conditions, including the four repeated fire tasks.
2. Trace the **consumer** of `CAI_TakecoverBehavior` parameters: how often it chooses cover, shot intervals, burst lengths, and what the unnamed seventh float controls.
3. Decode the `Custom*` takeover path and `Aggressiveness` index validation, then correlate it with actual map keyvalues from BSP entity lumps if available.
4. Continue through the 49 missing classnames by behavioral dependency: Mob Grunt/Elite, jetpack, securitybot, worker/civilian, weapons, radio, vehicles, exploding props, secret/dynamic-difficulty triggers.

Everything here is based on the original binary and shipped assets, and all original files remain untouched.