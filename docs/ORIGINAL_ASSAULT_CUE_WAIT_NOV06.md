# Original SiN Episodes: assault cue switch and NPC decisecond wait clock

**November 6, 2026 | Direct reverse engineering of the original game DLL**

This research builds on original MobGrunt/AssaultBehavior tasks recovered in
PRs #113 through #121. The owner-supplied 2006 Windows x86 `server.dll`
hashes to:
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

All values and x86 sites in this report were independently verified against
the original PE32 DLL. The original SDK `SinEpisodes.fgd` was inspected to
cross-reference documented cue modes. **No original game binaries, original
source code, or gameplay assets are redistributed.**

## Actual `CAI_AssaultBehavior` cue predicate at 0x220CEA90

The previous task analysis proved that `TASK_AWAIT_CUE` invokes helper
`0x220CEA90` both when the task starts and while it runs.
This investigation reconstructs its complete five-way dispatch, based
on a byte-verified x86 jump table at `0x220CEAD0`.

The helper reads two original data-map members:
`m_ReceivedAssaultCue` at original behavior object `+0x20` and
`m_AssaultCue` at `+0x1C`. It short-circuits to success if
`m_ReceivedAssaultCue == 4`. Otherwise:

| `m_AssaultCue` | Original branch | Meaning corroborated by SDK |
|---:|---|---|
| 0 | Return false | Not a documented FGD choice |
| 1 | Return true iff `m_ReceivedAssaultCue == 1` | “Entity System Input” |
| 2 | Query owning NPC condition **numeric `0x33`** via `0x220397C0`; return that value | “Gunfire” |
| 3 | Write **`m_ReceivedAssaultCue=3`**, return true | “Don't wait for a cue.” |
| 4 | Return false unless received-cue override is 4 | Not a documented FGD choice |

A corrupt or unsupported requested cue value above 4 also returns false,
unless the received value was already 4. **Updated original-binary proof:** the November 7 condition-registry audit establishes that native condition `0x33` is **`COND_HEAR_COMBAT`**. The SDK's “Gunfire” description and the x86 instruction both agree. The exact combat-hearing sensor's implementation still requires a separate trace.

This resolves the actual `TASK_AWAIT_CUE` gate and lets the future
native AI wait for real input/condition signals rather than for a
fabricated delay.

## Original NPC wait-clock implementation

The original tasks `TASK_WAIT_ASSAULT_DELAY` and
`TASK_AWAIT_ASSAULT_TIMEOUT` both pass a configured duration
into the NPC wait-setup routine **`0x220325C0`**. The task
calls pass **zero spread**, selecting the helper's deterministic
fixed-duration path. The original waiting routine:

1. Multiplies the input float duration by the float literal **10.0**
   at `0x223D8F14`.
2. Explicitly sets the x87 rounding-control mode to **round upward,
   toward positive infinity** (`0x220325E1..0x220325E6`).
   Thus fractional tenths are rounded **up** rather than
   truncated. The integer conversion uses `fistp`.
3. Multiplies the integer duration by the original double
   constant **0.1** at `0x223D8FE8`.
4. Adds the engine's `curtime`, then stores the absolute
   deadline as a float in the **original NPC object `+0x9DC`**
   (`0x22032624`).
5. A second optional branch with nonzero spread can choose a
   randomized duration; the assault tasks' zero-spread invocation
   does not select that path.

The follow-on helper **`0x220326D0`**, previously found
in the original `RunTask` dispatch, compares global game
time with this `+0x9DC` deadline and reports completion when
the finite current time has reached or passed it.

Readable reconstruction for the original zero-spread path:

```cpp
// Semantic reference only: exact 2006 x87 intermediate rounding differs
// from ordinary portable C++ floating-point evaluation at edge cases.
float tenths = ceil(original_float(duration_seconds * 10.0f));
float deadline = game_curtime + tenths * 0.1f;
bool complete = game_curtime >= deadline;
```

The **original configurable assault timeout** (FGD default 3.0 seconds),
the **configured rally assault delay**, and the unrelated
4.0-second “last enemy seen” diversion threshold must **not**
be merged into one timer. Each has a distinct original source field
and distinct usage.

## Portable implementation and tests

- `research/tools/audit_sin_assault_cue_wait.py`:
  read-only SHA256-pinned original DLL verifier. Confirms **26**
  original instruction witnesses, all **five** original cue
  jump-table targets, NPC condition call to `0x220397C0`,
  `10.0f` duration scaling, `0.1` double quantum,
  up-rounding instructions, deadline field and time-completion comparison.
- `research/tests/test_audit_sin_assault_cue_wait.py`:
  synthetic original-x86 fixtures and corruption cases for each
  independently important cue and deadline dataflow component.
- `include/sin_compat/original_assault_cue_wait.hpp`:
  **new engine-neutral C++17 reference**, with the actual
  `m_ReceivedAssaultCue` state mutation for mode 3,
  explicit host-provided `COND_HEAR_COMBAT` (`0x33`),
  deterministic decisecond deadline calculations and time
  completion check.
- `tests/test_original_assault_cue_wait.cpp`:
  validates every cue mode including value 4 override,
  gunfire condition querying, tenth-second upward rounding,
  no guessed random-spread behavior, invalid-number rejection
  and a complete **synthetic** hold-rally cue-then-delay flow
  through the host-driven task runner from PR #117.

The C++ implementation rejects nonfinite inputs and float/int
overflow. This is a **new port-side safety policy**, not a claim
about every original x87 corner case. The native fixed path uses
original x87 precision and conversion rules; the portable reference
is suitable for semantic testing, not bit-exact deterministic replay
in every boundary case.

Run the read-only original-DLL evidence report with your unchanged
game installation:

```powershell
py research\tools\audit_sin_assault_cue_wait.py --server "D:\SiN\bin\server.dll" --output "D:\SiN_Test_Only\original_assault_cue_wait.json"
```

The output is created only if absent. It does not rewrite the
original game DLL, BSP or any installed engine files.

## Remaining native integration gaps

- Trace the original conditions that **set/clear `COND_HEAR_COMBAT`** (`0x33`) in the NPC hearing code. Its enum name and registration ID are now proven by the original condition-registration instructions.
- Verify the engine's exact entity-input path that sets
  `m_ReceivedAssaultCue=1`, and the source event that sets 4.
- Implement a real engine host that supplies entity handles,
  NPC conditions, navigation, task completion, timer storage
  and Source I/O output dispatch for `CNPC_MobGrunt`.
- Solve the SourceVR ARM64 native game-module ABI/loader and
  separately port the required server/client game systems.
  None of this research alone makes the original SiN Episodes
  campaign playable in standalone Quest 3.

The existing owner-confirmed desktop HL2 MobGrunt visual/AR2
combat test remains untouched and is not counted as native
Ritual AI runtime validation.
