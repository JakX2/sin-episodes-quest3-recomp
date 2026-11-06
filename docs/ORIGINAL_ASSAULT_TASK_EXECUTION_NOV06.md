# Original SiN assault task executor: native binary control-flow and portable effects

**November 6, 2026.** This analysis used the owner's original, unmodified
2006 PE32 `server.dll`, SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
The original DLL bytes remain private. All reported x86 sites below were
confirmed using the original binary and its decoded instructions. Original
Ritual source code has **not** been recovered or redistributed.

## Verified original task-dispatch table

The previously identified `CAI_AssaultBehavior` task-dispatch vtable at
`0x223F5544` has original members `+0x60 -> 0x220CF190` and
`+0x64 -> 0x220CF770`. Their roles are consistent with
`StartTask` and `RunTask`, but the inherited virtual ABI has not
been independently proven, so those labels remain candidates.

Original `0x220CF190` compares task IDs `0x186A4` /
`0x186A5` directly and uses a **six-entry jump table** from
`0x220CF74C` for IDs `0x186A6` through `0x186AB`.
The original pointer table resolves to:

| Original task ID | Task | Handler VA |
|---|---|---|
| `0x186A6` | `TASK_AWAIT_CUE` | `0x220CF67D` |
| `0x186A7` | `TASK_AWAIT_ASSAULT_TIMEOUT` | `0x220CF631` |
| `0x186A8` | `TASK_ANNOUNCE_CLEAR` | `0x220CF5A2` |
| `0x186A9` | `TASK_WAIT_ASSAULT_DELAY` | `0x220CF600` |
| `0x186AA` | *No registered assault task: inherited fallback* | `0x220CF740` |
| `0x186AB` | `TASK_ASSAULT_DEFER_SCHEDULE_SELECTION` | `0x220CF582` |

This confirms the previously discovered numeric gap at `0x186AA`
is explicitly routed through the table's fallback handler, not a
new native assault-specific task.

## Actual native behavior by task

These findings are based on the executable **instructions**, not merely
the original script declarations or FGD field names.

### `TASK_HIT_RALLY_POINT`

In the original task method, branch `0x220CF4FF` reads the rallypoint
handle from the assault behavior at `+0x18`.
On its successful resolved-point path, it sets the recovered
`m_bHitRallyPoint` flag at behavior `+0x24` to `1`
(`0x220CF539`), takes the rallypoint's
`m_OnArrival` output at `+0x3A4` (`0x220CF550`),
and calls the game's Source output dispatcher
`0x22156900` at `0x220CF556`.

An invalid rally handle takes a different NPC diagnostic/path;
the code is **not** proof that every invocation always fires the output.

### `TASK_HIT_ASSAULT_POINT`

The branch at `0x220CF424` reads behavior
`m_hAssaultPoint` at `+0x14`.
On its successful point path it sets `m_bHitAssaultPoint`
at `+0x25` (`0x220CF44A`), accesses the assaultpoint's
`m_OnArrival` at `+0x3BC` (`0x220CF461`), and calls the
same Source output dispatcher `0x22156900` from
`0x220CF467`. The branch also checks the point's
`m_AssaultHintGroup` member at `+0x394` and may invoke a
further NPC behavior routine if the field is present.

The original full movement/animation ramifications of that extra
call have **not** been reconstructed.

### `TASK_WAIT_ASSAULT_DELAY` and `TASK_AWAIT_ASSAULT_TIMEOUT`

These are **two separately configured intervals**, not one hard-coded
timer:

- The first task's case `0x220CF600` obtains the current rallypoint,
  reads the **`m_flAssaultDelay` float at `+0x39C`**
  (`0x220CF617`), and calls the NPC's wait setup method
  `0x220325C0` at `0x220CF621`.
- The second task's case `0x220CF631` obtains the current
  assaultpoint, reads **`m_flAssaultTimeout` at `+0x3B4`**
  (`0x220CF663`), and calls the same routine
  `0x220325C0` from `0x220CF66D`.
- The second task-dispatch virtual method uses a shared path for
  task IDs `0x186A7` and `0x186A9`, with a wait-completion
  query at `0x220CFA9D` targeting `0x220326D0`.
  A separate branch checks the assaultpoint's
  `m_bClearOnContact` flag `+0x3B8` and an original NPC
  condition ID 10, as documented in PR #114.

The original SDK's default `assaulttimeout=3.0` is a map field
value, while the **4.0-second last-saw-enemy diversion comparison**
in PR #115 is a separate code path. Do not substitute one for another.

### `TASK_ANNOUNCE_CLEAR`

Its task case is at `0x220CF5A2`. It reads the current assaultpoint,
checks the point's `m_bNeverTimeout` (`+0x3BA`) and
`m_NextAssaultPointName` (`+0x398`) in one branch, and calls the
original **next-assaultpoint/terminal-unlock routine**
`0x220CE5B0` at `0x220CF5E9`. This connects the actual
`TASK_ANNOUNCE_CLEAR` script entry to the point-chain progression
function recovered in PR #114.

The assembly contains additional point/flag branches. It is not yet
safe to implement progression as an unconditional call in every case.

### `TASK_ASSAULT_DEFER_SCHEDULE_SELECTION`

At case `0x220CF582`, the original x87 instructions read
global `curtime` (`0x220CF587`), add the task's float
parameter (`0x220CF58C`), and store the future deadline
in the recovered `m_flTimeDeferScheduleSelection` at
behavior `+0x2C` (`0x220CF593`).

### `TASK_AWAIT_CUE`

The task's initial case is at `0x220CF67D`; the second
dispatch method handles it via `0x220CF998`.
Both call a shared original cue-check helper `0x220CEA90`
from `0x220CF67F` and `0x220CF9C1`, respectively.
The task also checks rallypoint validity and other NPC state,
so the full "cue received" predicate cannot yet be replaced with
a guessed boolean or timer.

## New reproducible code

**Hash-pinned original binary auditor**
`research/tools/audit_sin_assault_task_execution.py` verifies
30 original instruction witnesses, 8 direct call destinations,
the exact two vtable slots and all six original jump-table entries.
It returns a normalized JSON report and fails on altered bytes, ID
dispatch, output field, configured timer member or engine call target.
The corresponding synthetic tests are in
`research/tests/test_audit_sin_assault_task_execution.py`.

**Portable field/event effects**
`include/sin_compat/original_assault_task_effects.hpp` is a
small engine-neutral C++17 adapter for the *proven subset*:
successful rally/assault arrival flags and event requests,
forwarding the two distinct configured wait values, and
storing a future schedule selection deadline. The caller must
validate real Source handles and perform real output dispatch
and timer scheduling.

Finite-number checks in the adapter are deliberate **new port-side
safety guards**, not claims about the original x87 execution.
The adapter does **not** automatically declare Source AI tasks
complete. It does **not** implement
`TASK_ANNOUNCE_CLEAR`'s gated branching or the entire
`TASK_AWAIT_CUE` predicate.

CTest coverage in `tests/test_original_assault_task_effects.cpp`
verifies valid and invalid entity cases, correct output selection,
no invented one-shot event suppression, distinct configured waits,
float deadline math, and invalid-number rejection. The existing
C++17 Linux and Android CI builds both reference the new file.

To run the audit against the owner’s unmodified DLL:

```powershell
py research\tools\audit_sin_assault_task_execution.py --server "D:\SiN\bin\server.dll" --output "D:\SiN_Test_Only\original_assault_task_execution.json"
```

The report is created only if absent; the original DLL is read-only.
No original PE binaries, commercial assets or complete original Source
schedule text are committed.

## What remains before real Quest gameplay

The task effect adapter and the host-driven schedule runner from PR #117
are not the original engine's native `CAI_AssaultBehavior`.
Further work must reconstruct the rest of both dispatch routines,
actual movement/pathfinding, NPC combat/conditions,
`TASK_AWAIT_CUE` gate, task completion/failure ABI,
the original `CNPC_MobGrunt` game entity, and a compatible
SourceVR ARM64 native server/client loading path.

The user's working desktop HL2 one-grunt combat test is untouched.
