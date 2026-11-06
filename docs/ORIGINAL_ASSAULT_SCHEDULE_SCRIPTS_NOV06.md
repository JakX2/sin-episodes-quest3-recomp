# Original SiN Episodes MobGrunt assault schedules: verified task/interrupt structure

**November 6, 2026 | Original Windows 2006 `server.dll` reverse engineering**

Original binary SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. The owner-supplied DLL was analyzed directly; no executable binaries, script text dumps or game assets are included in this repository.

## Newly recovered evidence

PR #115 established the nine named and numbered `CAI_AssaultBehavior` schedules. The original PE32 DLL also stores their **complete declarative schedule scripts** as NUL-terminated strings in `.rdata` around `0x223F4508` to `0x223F4D00`. This follow-up recovers the ordered `TASK_*` declarations, each task argument, and the complete `COND_*` interrupt declarations.

The original DLL contains **29 ordered task declarations** and **35 interrupt declarations** across these nine schedules:

| Original schedule | Tasks | Interrupts | Original control-flow role |
|---|---:|---:|---|
| `SCHED_MOVE_TO_RALLY_POINT` | 8 | 3 | Select failure schedule, path to rally, run, stop and face rally, mark arrived, switch to hold |
| `SCHED_HOLD_RALLY_POINT` | 2 | 9 | Wait for assault cue and configured rally delay |
| `SCHED_FAIL_MOVE_TO_RALLY_POINT` | 1 | 3 | Short wait after failed rally movement |
| `SCHED_ASSAULT_FAILED_TO_MOVE` | 1 | 0 | Defer another schedule selection |
| `SCHED_MOVE_TO_ASSAULT_POINT` | 7 | 2 | Choose fail schedule, navigate and face destination, mark assault point reached |
| `SCHED_HOLD_ASSAULT_POINT` | 2 | 11 | Idle and wait, interruptible by encounters and combat conditions |
| `SCHED_WAIT_AND_CLEAR` | 4 | 7 | Face point, idle, await assault timeout, announce clear |
| `SCHED_CLEAR_ASSAULT_POINT` | 1 | 0 | Announce clearance |
| `SCHED_ASSAULT_MOVE_AWAY` | 3 | 0 | Move away, run path, wait for movement completion |

### Verified runtime registration of all nine scripts

A further instruction-level cross-check established that these are **the actual
schedule scripts passed into the original Source runtime parser**, not dead or
unreferenced text in `.rdata`. Original `server.dll` registers all nine
through a common parser at **`0x22042620`**. For each script, the binary
loads its `.rdata` string VA into a stack slot and makes a direct relative x86
`CALL` to the parser. The original pointer and destination bytes have now
been verified independently for **all nine registrations**.

For example, `SCHED_MOVE_TO_RALLY_POINT` uses the original script at
`0x223F4D00`, stores that pointer at `0x220D0694`, and calls the parser
from `0x220D069C`. The `SCHED_MOVE_TO_ASSAULT_POINT` script at
`0x223F4788` is passed from `0x220D07C2` and the parser call occurs
at `0x220D07CA`. Both target `0x22042620`.

The same explicit 8-byte pointer-store and 5-byte relative-call checks exist
in `audit_sin_assault_schedule_scripts.py` for all nine text references.
Synthetic tests corrupt a script pointer and the parser target separately
to ensure the verifier fails closed. This links the recovered task/interrupt
contents to the actual registration/initialization path.

### Specific original transitions now established

- `SCHED_MOVE_TO_RALLY_POINT` ends with `TASK_HIT_RALLY_POINT` followed by `TASK_SET_SCHEDULE` selecting `SCHED_HOLD_RALLY_POINT`. This provides the direct declarative schedule bridge from movement to rally waiting.
- `SCHED_HOLD_RALLY_POINT` executes `TASK_AWAIT_CUE` and `TASK_WAIT_ASSAULT_DELAY`. The original input `Assault(string)`, `m_AssaultCue` and `m_ReceivedAssaultCue` were separately recovered in PRs #113 and #115.
- `SCHED_MOVE_TO_ASSAULT_POINT` ends with `TASK_FACE_ASSAULT_POINT` and `TASK_HIT_ASSAULT_POINT`. The actual hit task is not reimplemented here.
- `SCHED_WAIT_AND_CLEAR` includes `TASK_AWAIT_ASSAULT_TIMEOUT` and `TASK_ANNOUNCE_CLEAR`, which aligns with the `assaulttimeout`, `clearoncontact` and `nextassaultpoint` properties already recovered from `CAssaultPoint`.
- `SCHED_MOVE_TO_ASSAULT_POINT` interrupts on `COND_NO_PRIMARY_AMMO` and `COND_HEAR_DANGER` in this 2006 build. Meanwhile `SCHED_HOLD_ASSAULT_POINT` declares **11** interrupt conditions, including enemy acquisition, ranged/melee attack opportunities, danger, lost enemy and lack of ammunition.
- `SCHED_ASSAULT_MOVE_AWAY` begins with `TASK_MOVE_AWAY_PATH` parameter `120`. The actual units and complete movement behavior of that parameter require the original task executor's interpretation, so the number is not presented as a verified distance unit.

### Why this matters for an authentic SinTEK enemy

We now possess the original *declarative control sequence* that the game's Source AI scheduler executed. Combined with the earlier direct x86 code traces, this identifies which transitions need a native implementation instead of approximating everything with `npc_combine_s`.

However, **a task declaration is not the task executor**. Full native `CNPC_MobGrunt` still requires matching `TASK_*` handlers, navigator behavior, schedule interrupt handling, typed condition IDs, model animation/activity translation, weapon usage and correct engine game-module registration. Merely mounting these declarations into SourceVR will not make the original AI run.

## New code and verification

- `research/tools/audit_sin_assault_schedule_scripts.py`: standard-library read-only auditor. Pins full original DLL hash, reads nine original PE virtual addresses and fails closed if any schedule name, task order, scalar argument or interrupt mask differs. Exports normalized JSON evidence, **not** verbatim text scripts.
- `research/tests/test_audit_sin_assault_schedule_scripts.py`: synthetic format fixtures plus mutations in task order, arguments, interrupt names and schedule syntax, requiring no proprietary DLLs.
- `include/sin_compat/original_assault_tasks.hpp`: portable C++17 constexpr inventory of 29 ordered tasks and 35 interrupts, indexed by the strongly typed original semantic schedule enumeration from PR #115.
- `tests/test_original_assault_tasks.cpp`: C++17 CTest checks all schedules' task counts, zero-based order, key original transitions, and interrupt coverage. This is evidence metadata only, not a SourceVR AI scheduler.
- `.github/workflows/sin-original-decomp-tests.yml` and `CMakeLists.txt` compile/check these references in the existing test pipelines.

Reproduce original-binary audit on the owner's unmodified DLL:

```powershell
py research\tools\audit_sin_assault_schedule_scripts.py --server "D:\SiN\bin\server.dll" --output "D:\SiN_Test_Only\original_assault_schedule_tasks.json"
```

The report is created only if absent and doesn't touch any original asset. Adjust the DLL path to the actual original game's location.

## Next technical gates

1. Trace and label original `TASK_AWAIT_CUE`, `TASK_HIT_RALLY_POINT`, `TASK_HIT_ASSAULT_POINT` and `TASK_AWAIT_ASSAULT_TIMEOUT` executors and schedule interruption behavior in original `server.dll`.
2. Reconcile original `COND_*` schedule interruption names with numeric conditions observed in PR #115, rather than assuming they share one numeric namespace.
3. Build a **small native-independent state-machine test harness** from the now-verified schedule tasks and event predicates; keep engine-dependent movement and navigation callbacks explicit.
4. Integrate those handlers into an actual Source-compatible ARM64 game server once a supported SourceVR game-module loader or equivalent native integration is available.

**Validated scope:** this PR reconstructs the actual schedule descriptions, not a runnable new native NPC. The functioning desktop HL2 one-grunt MobGrunt/AR2 encounter remains the independent integration baseline.
