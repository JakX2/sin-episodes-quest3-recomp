# SiN Episodes: Emergence | Chaingun Spin Cycle, Elite Animation Event 3 & Cover-AI Retry Budget (Deep Dive 14)

**Research date:** 2026-11-04. **Scope:** original 2006 x86 32-bit `server.dll` and `client.dll`, original `scripts.zip`, five user-supplied Half-Life 2 compatibility logs. **Research mode:** read-only static disassembly and source-asset correlation, **not** a compiled replacement or behavioral runtime test. Addresses below are absolute original PE virtual addresses (`0x22xxxxxx`), not file offsets or Quest 3 addresses.

This continues Deep Dives 11–13. Chapter 13 established `CAI_TakecoverBehavior`'s temporary shooting-parameter overrides, cover-schedule dispatch, and two elite animation-event codes. Chapter 14 adds a complete state-readiness mapping for the chaingun, exact timed transitions, an event-3 callback gate, and three documented cover retry paths. It explicitly leaves the original names of numeric animation events and the direct reader of the `cover odds` field unresolved.

## 1. Verifiable sources

| Source | SHA-256 / evidence |
|---|---|
| Original `server.dll` | `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` |
| Original `client.dll` | `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` |
| `scripts.zip` | Hash recorded in `sin_cover_chaingun_state_evidence_14.json` |
| Console logs | Five logs, 1,531 unknown-entity *attempts*, 49 missing classnames; **69** for `npc_druglab_grunt_elite` and **26** for `ai_goal_takecover`. Attempts can recur on reload. |
| Reproducibility | `sin_cover_chaingun_state_audit_14.py`: **90 passing, address-anchored assertions**; writes structured evidence `sin_cover_chaingun_state_evidence_14.json`. |

The original DLL still contains valid `CEntityFactory` registrations for `npc_druglab_grunt_elite` → `CNPC_EliteGrunt`, `weapon_chaingun` → `CWeaponChaingun`, and `ai_goal_takecover` → `CAI_TakeCoverGoal`. Those original factories do not exist in the tested ordinary HL2 game DLL. The Quest 3 port is not modified.

## 2. Exact `CWeaponChaingun` readiness mapping

The original chaingun controller holds its state as a 32-bit word at **`CWeaponChaingun+0x594`**. The original weapon readiness function **`0x22315320`** performs a bound-checked four-way indirect branch through a jump table at **`0x22315374`**.

| State | Jump target | Boolean return | Interpreted role |
|---:|---:|---:|---|
| 0 | `0x22315335` | `false` | Initiates the spin/start state (sets 1), then refuses fire |
| 1 | `0x2231536A` | `false` | Preparatory timed phase |
| **2** | **`0x2231536E`** | **`true`** | **Only fire-ready state** |
| 3 | `0x2231536A` | `false` | Stop/shutdown processing pending |

Crucially, the state numbers and jump-table addresses are verified directly. Labels such as *preparatory* and *shutdown* are analyst descriptions, **not recovered original enum identifiers**.

### Spin-state update: two timed one-second transitions

The controller update at **`0x22315390`** tests states 1, 2, and 3:

1. **0 → 1** occurs inside the readiness query at `0x22315335`: it invokes weapon virtual slot `+0x3C4` with sound/event index **13**, writes state 1 at `0x22315343`, and stamps the game clock into **`+0x590`** and **`+0x58C`** (`0x22315356`, `0x22315364`).
2. **1 → 2** occurs in the tick at `0x22315413` when `curtime - weapon[+0x590]` reaches at least the floating-point constant **`1.0f`** stored at `0x22477C5C`. The tick rewrites both time fields.
3. **2 → 3** occurs in the tick at `0x223153EB` when `curtime - weapon[+0x58C]` reaches at least **`1.0f`**.
4. **3 → 0** occurs on the controller's state-3 processing branch at `0x223153C4`. It invokes virtual slot `+0x3C8` with sound/event index **13** and virtual slot `+0x3C4` with index **14** before setting state 0.
5. **Forced stop**: `0x22315440` sets state **3** immediately. The elite primary event handler calls it at **`0x223554BA`**, on numeric animation event **`0x47`**.

Minimal explanatory pseudocode, **reconstructed, not authored Source C++**:

```cpp
bool QueryChaingunReady(Weapon *w) {
    switch (w->state_594) {
    case 0:
        w->WeaponEvent(/* index */ 13, /* arg */ 0);
        w->state_594 = 1;
        w->time_590 = currentTime;
        w->time_58C = currentTime;
        return false;
    case 2:
        return true;
    default:
        return false;
    }
}

void TickChaingun(Weapon *w) {
    if (w->state_594 == 1 && currentTime - w->time_590 >= 1.0f) {
        w->state_594 = 2;
        w->time_590 = currentTime;
        w->time_58C = currentTime;
    } else if (w->state_594 == 2 && currentTime - w->time_58C >= 1.0f) {
        w->state_594 = 3;
    } else if (w->state_594 == 3) {
        w->StopWeaponEvent(/* index */ 13);
        w->WeaponEvent(/* index */ 14, /* arg */ 0);
        w->state_594 = 0;
    }
}
```

The order of elapsed-time comparisons and state writes is established by the binary. This is **not** proof of a fixed rate of rounds fired per second. Firing still depends on the elite's AI/animation callbacks, weapon ammo and attack functions.

### Original weapon scripts: spin-up/down sound definitions exist

From the original `scripts/weapon_chaingun.txt`:

- `special1` references `Weapon_Chaingun.NPC_SpinUp`.
- `special2` references `Weapon_Chaingun.NPC_SpinDown`.

Both events are declared in `scripts/game_sounds_weapons.txt`, resolving to original `weapons/chaingun/chaingun_spinup.wav` and `weapons/chaingun/chaingun_spindown.wav` assets respectively. **Caveat:** the static trace in this round does not independently prove that numeric weapon event indices **13 and 14** are precisely the script keys `special1` and `special2`; the source names align with the spin behavior, but the intermediate enum-to-KeyValues lookup must still be traced for definitive identification.

## 3. Elite NPC consumes the weapon state machine through animation events

The elite stores `m_pChaingun` (entity handle, `+0xFDC`) and `m_canFire` (boolean, `+0xFD9`). Its primary event handler at **`0x22355480`** branches on event codes **3**, **0x43**, and **0x47**.

- **`0x43`** queries `QueryChaingunReady` (`0x22355503` → `0x22315320`) and stores the returned boolean into elite `+0xFD9` (`0x2235550A`).
- **`0x47`** resolves the chaingun handle and forces state 3 (`0x223554BA` → `0x22315440`), then calls a separate NPC action helper with `1.0f`.
- **Elite per-update hook**: primary vtable `+0x424` leads to `0x223555F0`; once a valid weapon handle is resolved it calls the timed controller (`0x22355645` → `0x22315390`).
- **Second event handler** (`0x223556A0`): the `0x43` branch checks `m_canFire` and may query readiness again. The `0x47` branch includes guarded calls clearing NPC condition IDs **3, 4, and 5** (`0x22355713`, `0x2235571C`, `0x22355725` → `0x220398A0`). These condition IDs' original symbolic names are not yet recovered.

### New finding: numeric animation event 3 has a *three-second cooldown*

Previously, event **3** was left mostly uninterpreted. The primary handler reads a floating-point argument from **`animationEvent+4`**, converts it to integer and follows this specialized path **only when the result equals 1** (`0x2235553F`). It then calls a target resolver (`0x22031A80`), performs virtual guarded checks on that target, and checks elapsed game time against elite offset **`+0x99C`**. The original double constant at `0x223D9EE8` is **`3.0`** (`0x22355587`).

On the accepted branch, the handler:

1. Saves current time back to `elite+0x99C` (`0x2235559B`).
2. Invokes a nontrivial helper on `elite+0xCC0` (`0x223555B2` → `0x220A35C0`) using computed arguments.
3. Calls another NPC helper with floating argument **2.0f** (`0x223555C0` → `0x220325C0`).
4. Calls an elite/NPC virtual slot **`+0x4DC`** with argument 1 (`0x223555CD`).

If a guard fails, the branch instead calls `0x2203D2D0`. The precise gameplay action of those calls, the target's original type, and the **symbolic name of event 3** are not proven in this chapter. The three-second throttle and the event-argument requirement **are** proven.

## 4. Cover behavior: retry budget and failure recovery paths

The `CAI_TakecoverBehavior` object initializes its counter at offset **`+0x7C`** to zero in its constructor (`0x22399D99`). The local schedule-selection function **`0x22398400`** contains:

- A guard on an NPC flag bit at `NPC+0xBF`.
- A comparison of the behavior's `+0x7C` retry count against literal **3** (`0x2239840F`, `0x22398414`).
- A branch that clears the behavior-active byte **`+0x11`** if the counter has reached 3 (`0x22398419`).
- Additional state and virtual schedule checks and an inherited fallback (`0x220CD2D0`).

Three independently verified paths increment the **same retry counter**:

| Address | What the disassembly shows |
|---|---|
| `0x22398653` | Schedule/target fallback increments `behavior+0x7C` |
| `0x22399772` | Failed cover/path result increments counter and arranges a retry |
| `0x2239983D` | A missing target for a special task increments counter |

The special task handler at **`0x22399650`** handles IDs `0x186A0`–`0x186A5`, with six jump-table destinations (mapped in Round 13). For the first task, it invokes a substantial cover-search routine at **`0x22398B80`** (`0x22399749`). On several failed paths it schedules new attempts using a random-float import with **lower bound 2.0f and upper bound 4.0f** (`0x22399757`–`0x22399761`, repeated at `0x2239979F`, `0x22399804`).

Compiled diagnostics provide context:

| String | Original VA | Referencing branch |
|---|---:|---:|
| `No hint node` | `0x223EEB5C` | `0x22399843`, when a target search fails |
| `No longer behind cover` | `0x224AE350` | `0x22399921`, after a trace/visibility-related test |
| `FindCoverPos() - %s has no nearest node!` | `0x224AD760` | `0x22398C08`, inside the cover-search routine |

There is therefore a real retry/abort lifecycle behind `ai_goal_takecover`: *try to find cover → back off for a bounded randomized period when needed → track failures → disable or fall back after repeated unsuccessful attempts under specific conditions*. We have **not** proven this is a guaranteed cap of three attempts for every possible schedule branch or that every failed path increments the counter exactly once.

## 5. Outstanding ambiguity: does `OddsToTakeCover` actually get consulted?

The five compiled aggressiveness presets at `0x2253B600` hold seven values each. The sixth word is the presumed `odds_cover` / cover-odds parameter (e.g. **50** for preset 0 and **0** for preset 4). The original map goal copies all seven words into `CAI_TakecoverBehavior` at **`+0x14`**, so the odds value resides at behavior **`+0x28`**. The seventh word resides at **`+0x2C`**.

In the take-cover-specific functions inspected this round, those fields are directly copied but we did **not** establish a dedicated arithmetic comparison, RNG call, or conditional branch reading the `+0x28` value. A copy into an object **does not prove** the value changes cover selection. It may be read indirectly, passed to shared engine code, or remain unused in this binary. Consequently:

- **Confirmed:** presets contain numeric cover-odds values, and map goal/behavior copies them.
- **Not yet confirmed:** which virtual function, branch, or engine helper consumes the odds, what range the field is normalized to, or any exact chance of taking cover.
- **Not yet confirmed:** the meaning of the seventh float (`AbandonIfEnemyHides` preserves or zeroes it, per Round 13).

This is an intentional research boundary, **not** a guess that the field does nothing.

## 6. Implications for the missing-entity backlog

- **`npc_druglab_grunt_elite`:** 69 logged failures. Its chaingun requires both the NPC-side animation-event gates and the weapon-side timed state transitions; reproducing only `npc_combine_s` with a reskinned model would miss this coupling.
- **`ai_goal_takecover`:** 26 logged failures. Restoring the preset values alone is insufficient. The temporary combat-setting overrides established in Round 13 and the retry/abort paths established here both matter.
- **Weapons/assets:** the original special audio definitions exist. Importing those sound scripts into a plain HL2 mount does not recreate the missing server-side gameplay classes.
- **No claim of completion:** 49 classnames are structurally inventoried, not fully reverse engineered. Weapon damage, actual melee attacks, all animation symbolic IDs, cover-odds reader, map-specific schedule usage, and broader NPC systems remain open.

## 7. Reproduction and next actions

To reproduce the SHA-pinned static checks with the same original binaries, original `scripts.zip`, and five logs in `/mnt/data`:

```bash
python /mnt/data/sin_cover_chaingun_state_audit_14.py /mnt/data
```

This regenerates `sin_cover_chaingun_state_evidence_14.json`, including exact jump-table targets, references, timings, and assertions. **No game executable is launched. No game DLL, Google Drive file, or Quest 3 repo is changed.**

**Next useful questions:** map the elite's numeric animation events onto actual MDL/QC sequence events, identify the event 3 target and side effects, trace the source of the cover preset's sixth/seventh fields, then continue with other high-impact missing classes such as `npc_druglab_jetpack`, `npc_securitybot`, `env_radio_message`, and the full adaptive difficulty controller.