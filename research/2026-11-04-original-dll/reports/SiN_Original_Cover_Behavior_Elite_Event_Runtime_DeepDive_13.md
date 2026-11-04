# SiN Episodes: Emergence | Active Take-Cover AI and Elite Chaingun Event Gates (Deep Dive 13)

**Research date:** 2026-11-04  
**Target:** Original 2006 x86 32-bit `server.dll` / `client.dll`, archived HL2 compatibility console logs, and preceding research chapters 10–12.  
**Mode:** Static, read-only partial decompilation. No modification of original binaries, Google Drive content, or the standalone Quest 3 project. Original absolute PE image addresses are used throughout; they are **not file offsets**. This document does **not** represent fully reconstructed original authored C++ or a functional port.

## Verification and provenance

| Evidence | Exact hash / validation |
|---|---|
| Original `server.dll` | SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` |
| Original `client.dll` | SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` |
| Binary audit | `sin_cover_elite_runtime_audit_13.py`; **114 assertions passed**, writing structured evidence JSON |
| Runtime motivation | Five user-provided HL2 compatibility logs: **1,531 failed creation attempts for 49 original SiN classnames**, including **26** `ai_goal_takecover` and **69** `npc_druglab_grunt_elite` attempts. These are occurrences, **not** unique spawned entities. |

The Round 12 evidence established the five cover presets, elite `m_canFire` / `m_pChaingun`, and four-state chaingun timer. Round 13 follows their **actual consumers, behavior lifecycle and event-code branches**.

## 1. `CAI_TakecoverBehavior`: compiled runtime vtable and behavior lifecycle

The original `CAI_TakecoverBehavior` has its primary vtable at **`0x224AE624`**, with **58 function-pointer entries** identifiable by the audit. Its implementation is different from the `CAI_TakeCoverGoal` *map entity*, which has its own class, datamap, and factory.

| Observed vtable slot | Function address | Confirmed behavior (descriptive labels, **not recovered source-symbol names**) |
|---|---:|---|
| `+0x18` | `0x22398250` | Eligibility check involving stored timing, a condition bit and current NPC state |
| `+0x1C` | `0x223982C0` | Enter/start: save and replace combat parameters |
| `+0x20` | `0x22398990` | Leave/cleanup: restore saved combat parameters |
| `+0x24` | `0x223983D0` | Timed condition handling; calls an inherited behavior function |
| `+0x28` | `0x22398390` | Reapply a pair of numeric NPC combat parameters |
| `+0x7C` | `0x22398670` | Map several schedule IDs onto other schedule IDs or delegate |
| `+0xDC` | `0x223984F0` | Checks two numbered conditions and returns `0x31` or `0` |
| `+0xE0` | `0x22398520` | Selects among cover behavior-specific schedule IDs |

**Important corrected precision about offsets:** `C A I_TakeCoverGoal` at `0x22399B80` copies all **seven DWORDs (28 bytes)** of the selected preset to **`CAI_TakecoverBehavior +0x14`**, *not* wholesale into the NPC object. It then writes two **16-bit** shot-count values and two **32-bit** time fields to the NPC's configuration structure based at **`NPC+0x9A8`**:

- `0x220B1A80` writes the two 16-bit fields at `NPC+0x9B0` and `NPC+0x9B2`.
- `0x220B1AA0` writes the two DWORD fields at `NPC+0x9B4` and `NPC+0x9B8`.
- The preset's other three values remain in the behavior-owned 28-byte block and may be used by additional methods. **Their complete downstream use is not yet traced.**

### Begin/exit restores shared NPC state rather than permanently overwriting it

At **`0x223982C0`**, the behavior backs up existing NPC combat data using `0x220B1AE0` and `0x220B1B00`, then installs replacement values using `0x220B1A80` and `0x220B1AA0`.

It also reads `NPC+0xA20` into its own `behavior+0x34` and writes **`0x7F7FFFFF`** (`FLT_MAX` bit pattern) to `NPC+0xA20`, as seen at `0x22398332` and `0x2239833B`.

At **`0x22398990`**, the behavior:

1. Calls a related target cleanup helper `0x223988B0`.
2. Restores the saved `NPC+0xA20` value (`0x2239899E`).
3. Restores the previous shot-count and timing fields through `0x220B1A80` (`0x223989B5`) and `0x220B1AA0` (`0x223989CB`).

This shows the cover behavior **temporarily takes control of a shared NPC shooting configuration**, then hands that state back when it exits. Reproducing only the preset table without this lifecycle would miss important interactions with other NPC schedules and AI behaviors.

### Goal-to-behavior activation

`CAI_TakeCoverGoal` searches the attached NPC for a `CAI_TakecoverBehavior` using the RTTI-verified lookup at `0x22399B00`, called from `0x22399AA0` and `0x22399C20`.

- The beginning callback uses event number **`0x3E8` (1000)**, sets the behavior enabled flag and applies the selected preset (`0x22399AE9`).
- The ending callback uses event number **`0x3E9` (1001)**, clears the enabled flag and still reapplies its configuration (`0x22399C60`).

We verified those numeric event codes, but **not** their original symbolic enum identifiers.

## 2. The `AbandonIfEnemyHides` option has a precise recovered effect

This closes a question from Round 12. In `0x22399B80`:

```asm
0x22399B96  cmp BYTE PTR [eax+0x3CC],0
0x22399B9E  lea esi,[esi*4+0x2253B600]  ; select 28-byte preset
0x22399BAE  rep movsd                       ; seven DWORDs to local block
0x22399BB0  jne 0x22399BBA                ; if flag enabled, keep all values
0x22399BB2  mov DWORD PTR [esp+0x24],0  ; else clear the seventh word
```

Thus **with `AbandonIfEnemyHides = false`, the seventh preset float is forced to `0.0`**; when true, it retains the table's original float. This is a *direct* implementation of the map key affecting a combat parameter.

| Aggressiveness | Original seventh preset float | Value if `AbandonIfEnemyHides=0` |
|---:|---:|---:|
| 0 | 30.0 | 0.0 |
| 1 | 20.0 | 0.0 |
| 2 | 10.0 | 0.0 |
| 3 | 10.0 | 0.0 |
| 4 | 5.0 | 0.0 |

**Caution:** The seventh float's precise consumer and in-game interpretation are not established. It *may* relate to losing sight of the enemy, but that inferred semantic is not substituted for the evidence-backed label.

The six `Custom*` map keys are also present in the original datamap; their *direct* use in the preset-application function is not shown by this path. The behavior's response to out-of-range aggressiveness values remains a separate open problem.

## 3. Cover decision selection: real event/condition checks and schedule IDs

The virtual at **`0x22398520`** checks conditions via `0x220397C0` (numeric IDs **`0x2B` / `0x0C`**) and then calls `0x220398A0` to clear corresponding conditions. It calls the imported random-integer helper with arguments **0 and 10**, comparing its result to **8** (`0x22398560`). Values above 8 select custom schedule ID **`0x186A1`**. The RNG helper's distribution and complete result semantics were **not** independently proven, so an exact probability is intentionally not asserted.

The routine checks a validated entity handle at **`NPC+0xA18`**, testing a 16-bit field at target offset **`+0x3A4`** against **`0x64`, `0x65`, `0x6B`, `0x6C`**. Different matches select further custom schedule IDs `0x186A2`–`0x186A5`. An alternate path yields `0x186A0`. We suspect the handle is the current enemy and the 16-bit field is a classification/type value; the original member/enum names have **not** been recovered from this code alone.

The neighboring function at **`0x22398670`** maps certain schedule IDs to other IDs according to target state and handle validation. `0x22398470` specifically recognizes **`0x186A2` / `0x186A3`**, checks an NPC virtual boolean and optionally changes the behavior-enabled flag. This makes these IDs real, active scheduling paths rather than dead strings.

The compiled behavior includes a dedicated schedule:

`SCHED_COVER_TAKE_COVER_FROM_BEST_SOUND`

It executes `TASK_STOP_MOVING`, `TASK_FIND_COVER_FROM_BEST_SOUND`, `TASK_RUN_PATH`, `TASK_WAIT_FOR_MOVEMENT`, `TASK_WAIT 0.5`, `TASK_WAIT_RANDOM 1.0`, and `TASK_FACE_REASONABLE`.

Other cover behavior uses inherited Source AI tasks/schedules. The presence of a compiled schedule does not prove how frequently it is selected in released maps.

### Six-way task-specific runtime dispatcher

At **`0x22399650`**, a branch normalizes a task ID by subtracting `0x186A0`, then bounds-checks it against **0–5** and uses a jump table at **`0x22399980`**:

| Internal task ID | Dispatch target |
|---|---:|
| `0x186A0` | `0x2239971E` |
| `0x186A1` | `0x2239982D` |
| `0x186A2` | `0x22399940` |
| `0x186A3` | `0x22399955` |
| `0x186A4` | `0x223996E5` |
| `0x186A5` | `0x22399678` |

The `0x186A2` and `0x186A3` cases call the same helper `0x22049C20` with distinct arguments `0x48` and `0x05` respectively. The `0x186A0` case contains additional game-time scheduling and invokes behavior virtual methods. The full Source AI task semantics need more work. These are task dispatch IDs; do **not** equate them with the independently observed schedule IDs merely because they share the same numerical range.

### Eligibility gate

At **`0x22398250`**, the behavior evaluates a stored float at behavior `+0x68`, compares it to current game time, checks an NPC virtual condition mask (`0x08000000`), checks NPC state **3** at `+0x850`, and obtains the current weapon through `0x222AA640`. Together, these show that activation depends on **live NPC state**, not only the static aggressiveness preset. The complete meaning of condition bit `0x08000000` is still open.

## 4. Elite animation events: the chaingun is gated by actual event IDs

The `CNPC_EliteGrunt` owns:

- `m_canFire` at **`+0xFD9`**.
- `m_pChaingun` entity handle at **`+0xFDC`**.

In its animation/event handler at **`0x22355480`**:

- Event **`0x43` (67)** enters the weapon-ready query path at `0x223554D5`, calls **`0x22315320`** (`0x22355503`) and stores the returned flag to `m_canFire` (`0x2235550A`).
- Event **`0x47` (71)** resolves the chaingun handle and calls **`0x22315440`** (`0x223554BA`), forcing the weapon controller into state **3**. It also invokes an NPC action using literal `1.0f`.
- Event **`3`** has a separate branch that reads a numeric event parameter and performs additional NPC action. We have **not** assigned a human-readable symbol to this event.

A second event handler at **`0x223556A0`** also splits on **0x43** and **0x47**. Its 0x43 branch tests `m_canFire` (`0x22355737`) and can re-query the chaingun controller (`0x22355772`). The elite's vtable-driven weapon-tick function at **`0x223555F0`** invokes the state update **`0x22315390`** (`0x22355645`) and then delegates to its inherited NPC tick.

The `CWeaponChaingun` state word **`+0x594`** transitions 0→1→2→3→0 using the existing state controller. Two timed phases compare elapsed time against the **`1.0f`** float at `0x22477C5C`. This corroborates a deliberate spin-up/fire-ready/spin-down control design, but **does not establish exact rate of fire, animation sequence frame timing, or the symbolic names of animation codes 0x43/0x47**.

## 5. Why this matters to the HL2 compatibility experiment

The missing-entity logs for the original maps include **26** failed `ai_goal_takecover` creations and **69** failed `npc_druglab_grunt_elite` creations.

- If `ai_goal_takecover` never spawns, its map-configured combat parameters are never applied to the NPC behavior object. AI will not receive the intended start/stop events through that entity.
- If the elite class is absent, a generic soldier replacement will not inherit its specialized event-driven chaingun readiness gate and state update loop.
- The basic Source scheduler may still function, but that does not reproduce these SiN-specific overrides.

This is *not* evidence that all of HL2's combat AI is broken, nor that the original SiN engine lacks standard Source AI. It is evidence of the precise, game-specific extensions this code adds.

## 6. Remaining unresolved items and next targets

1. Map **`CAI_TakecoverBehavior`'s preset `oddsCover` and seventh-float consumers** to their exact decisions, including any percent/threshold conversions.
2. Identify actual original names of event numbers **0x43 / 0x47** by cross-checking model animation QC/event data, if available from the original MDL/game asset pipeline.
3. Trace the full six-case task dispatcher, especially `0x22398B80` and the alternate branches in `0x22399650`.
4. Trace the NPC weapon selection and ammunition overrides in detail and confirm campaign usage with BSP map entity lumps rather than log occurrence counts alone.
5. Inspect other members of the complete **49-missing-classname** atlas, continuing with `env_radio`, `env_radio_message`, scripted NPCs, vehicles, props, and weapons.

### How to rerun

```bash
python /mnt/data/sin_cover_elite_runtime_audit_13.py /mnt/data
```

This checks original DLL hashes and instruction/call anchors, the table and jump-table structure, the observed AI mapping, and the five supplied console logs, then writes **`sin_cover_elite_runtime_evidence_13.json`**. It does not run proprietary game code or modify any source files.