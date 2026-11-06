# Original SiN Episodes assault diversion: x86 vtable and conditional scheduler evidence

**6 November 2026 | Original `server.dll` and `SinEpisodesSDK.zip`**  
Research target: `CAI_AssaultBehavior`, which receives `CNPC_MobGrunt::InputAssault` through the verified `CAI_AssaultBehavior::SetParameters` path from PR #113. This is a static decompilation milestone, **not** a functioning new Quest 3 NPC module.

## Provenance and independent byte validation

The user-provided original 2006 `server.dll` has SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
This release's PE32 image base is `0x22000000`. The short instruction witnesses in
`research/tools/audit_sin_assault_diversion.py` were checked directly against that
original DLL: **24 of 24 instructions, seven of seven calls, original RTTI, three
vtable entries, and two float constants matched**. Original `SinEpisodes.fgd` also
matched both configuration contracts. The complete DLL or any game asset is not
included in this repository.

## 1. Verified `CAI_AssaultBehavior` virtual method ownership

The C++ class has an MSVC RTTI Complete Object Locator at `0x224B8C24`;
the associated type descriptor names `CAI_AssaultBehavior`. Its virtual table
begins at `0x223F5544`.

| Original vtable offset | Original Windows x86 method VA | Confirmed observations |
|---|---|---|
| `+0x18` | `0x220CEAF0` | Condition and elapsed-time gate reading `allowdiversion` |
| `+0x20` | `0x220CEBF0` | Reset arrival flags and release a stored rally owner through original unlock |
| `+0x78` | `0x220CED00` | Schedule-dependent processing which checks `allowdiversion` and clears NPC conditions |

These are grounded through the *actual class RTTI and virtual table pointers*.
We do not infer C++ method names or a public ABI beyond those witnesses.

## 1a. Actual class member names, recovered from 52-byte data maps

The original DLL's `CAI_AssaultBehavior` typedescription records now establish
the member names and locations **directly**, so the previous anonymous byte
offsets can be identified:

| Original 2006 member | Original x86 behavior object offset |
|---|---:|
| `m_hAssaultPoint` | `0x14` |
| `m_hRallyPoint` | `0x18` |
| `m_AssaultCue` | `0x1C` |
| `m_ReceivedAssaultCue` | `0x20` |
| `m_bHitRallyPoint` | `0x24` |
| `m_bHitAssaultPoint` | `0x25` |
| `m_bDiverting` | `0x26` |
| `m_flLastSawAnEnemyAt` | `0x28` |
| `m_flTimeDeferScheduleSelection` | `0x2C` |

The verifier checks each actual member name, field-type ordinal, member
offset and original 52-byte descriptor layout, including the recorded
one-byte/32-bit field sizes. They are facts about this **specific Windows
x86 build**, not offsets for a rebuilt Quest ARM64 class.

## 1b. All nine original assault schedules recovered by name

The original `server.dll` also embeds named, textual AI schedule
definitions. Crucially, their **registered numeric IDs are recoverable
from the code** at `0x220D0675` through `0x220D085D`, including the
schedules whose numeric IDs are stored indirectly through registers.

| Original schedule ID | Registered original name |
|---|---|
| `0x186A0` | `SCHED_MOVE_TO_RALLY_POINT` |
| `0x186A1` | `SCHED_ASSAULT_FAILED_TO_MOVE` |
| `0x186A2` | `SCHED_FAIL_MOVE_TO_RALLY_POINT` |
| `0x186A3` | `SCHED_MOVE_TO_ASSAULT_POINT` |
| `0x186A4` | `SCHED_HOLD_RALLY_POINT` |
| `0x186A5` | `SCHED_HOLD_ASSAULT_POINT` |
| `0x186A6` | `SCHED_WAIT_AND_CLEAR` |
| `0x186A7` | `SCHED_ASSAULT_MOVE_AWAY` |
| `0x186A8` | `SCHED_CLEAR_ASSAULT_POINT` |

This resolves the three numbers tested by virtual routine
`0x220CED00`: **move toward the rally point**, **move toward the
assault point**, and **hold at the rally point**. The routine checks
whether those schedules are active when deciding whether to clear
conditions under the `allowdiversion` gate.

**Important:** The Source AI framework also assigns numeric IDs to
`TASK_*` names in a *separate task registry*, which can reuse the
same numeric values. Do not mix task ID `0x186A3` with schedule
ID `0x186A3` during reimplementation. The original textual schedule
definitions are not being copied wholesale into this repository.

## 2. The diversion flag and internal elapsed-time condition

Inside `0x220CEAF0` the original method:

1. Checks an NPC virtual method at vtable `+0x450`, then numeric condition
   `0x3B` by calling `0x220397C0`.
2. Uses behavior field `+0x2C` in a comparison with global game time, and
   checks behavior state `+0x1C`.
3. Validates the current assault point and rally references.
4. Reads the **original assault-point `m_bAllowDiversion` at `+0x3B9`**, independently
   proven by the `CAssaultPoint` data map in PR #113.
5. Within the enabled path, compares a previously stored behavior timestamp
   `+0x28` and the elapsed expression `curtime - field(+0x28)` against an
   original static **`4.0f`** literal at `0x223DDCF4`. The code also compares
   against `0.0f` at `0x223D2590`, with x87 status-flag branches.
6. One branch sets a behavior byte `+0x26` to `1`; the alternate path may
   call `0x22039760` with NPC numeric condition `0x18` and resets that
   byte to `0`. Invalid/stale reference paths also clear state `+0x1C`
   and arrival flags `+0x24`/`+0x25`.

**Crucial limitation:** The dataflow and the 4-second literal are verified;
the `+0x26` field is now definitively `m_bDiverting`, but the NPC condition enumeration and the game's
x87 floating-point corner cases are not yet decoded. The literal must not
be reported as “damage,” “grenade fuse,” or a generic four-second NPC reaction
time.

## 3. Separate schedule-state branch uses the same flag

The function at `0x220CED00` is a distinct virtual method (`+0x78`).
It verifies a current assault-point handle and checks the same
`m_bAllowDiversion` at `+0x3B9`. It tests three original schedule IDs via
`0x220CD3B0`: `SCHED_MOVE_TO_ASSAULT_POINT` (`0x186A3`),
`SCHED_MOVE_TO_RALLY_POINT` (`0x186A0`) and
`SCHED_HOLD_RALLY_POINT` (`0x186A4`).
In the recognized cases, it calls `0x22039C70` twice with NPC numeric
condition IDs `0x19` and `0x0A`. This is *schedule-dependent condition
cleanup*, not proof of specific named player-contact events.

The original `+0x20` virtual routine `0x220CEBF0` is also associated
with rally ownership cleanup and is a useful verification bridge with PR #114.

## 4. SDK corroboration and two different time constants

The *original* `SinEpisodesSDK/bin/SinEpisodes.fgd` defines:

| Map property | SDK default | SDK-authoritative meaning |
|---|---:|---|
| `allowdiversion` | `0` | An assaulting NPC may divert to fight encountered enemies and resume the assault after contact is lost |
| `assaulttimeout` | `3.0` seconds | Clear an assault point after the configured interval without seeing enemies |
| `SetAllowDiversion(integer)` | Input | Change the point's diversion flag |
| `SetClearOnContact(integer)` | Input | Change the point's contact-clear flag |

The internal `4.0f` comparison in `0x220CEAF0` and the FGD-configured
`assaulttimeout=3.0` are **distinct**. Neither should overwrite the other
in a reconstructed native AI system. The original SDK describes the desired
high-level behavior; only the machine code establishes what each executable
branch actually reads/writes.

## Repeatable original-DLL/SDK audit

```powershell
py research\tools\audit_sin_assault_diversion.py `
  --server "D:\SiN\bin\server.dll" `
  --sdk-zip "D:\SinEpisodesSDK.zip" `
  --output "D:\SiN_Test_Only\original_assault_diversion.json"
```

Adjust paths to the actual original local files. A mismatching DLL hash,
changed instruction, virtual method target, float constant, or original SDK
contract fails verification. The tool writes **new evidence JSON only** and
refuses overwrite. Python synthetic tests under
`research/tests/test_audit_sin_assault_diversion.py` and the existing GitHub
Actions workflow exercise the verifier without embedding proprietary binaries.

## Next decompilation targets

1. Identify the remaining numeric NPC condition IDs `0x3B`, `0x18`,
   `0x19` and `0x0A`. The nine registered `SCHED_*` IDs are now resolved
   independently from actual schedule-registration instructions.
2. Trace `0x220CEAF0` callers through the verified vtable and determine
   precisely when `+0x26` enables/resets.
3. Trace original assault-point output firing (`OnArrival`,
   `OnAssaultClear`) and how the full task scheduler advances through
   `nextassaultpoint`.
4. Only then implement native equivalent behavior in an independently
   buildable Source-compatible game server, with a real supported ARM64
   loader/factory ABI. Stock SourceVR content-only mounting remains
   insufficient for new native `CNPC_MobGrunt` entities.
