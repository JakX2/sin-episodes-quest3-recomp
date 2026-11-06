# Original 2006 SiN Assault AI: native task registry IDs and dispatch virtuals

**November 6, 2026 | New instruction-level original DLL analysis**

This work follows the nine verified schedule scripts and the portable task
runner in draft PRs #116 and #117. I used the owner-supplied original
`server.dll` inside `bin.zip`, whose SHA-256 is
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
Each register-call instruction witness below was **checked against the original
2006 PE32 DLL bytes**, not inferred solely from string searches.

## Proven: eleven separate native Assault task IDs

The original game registers its assault-specific `TASK_*` names through an
x86 helper at **`0x2208C590`**. Each call has a verified original task-name
pointer, numeric ID either as an immediate operand or through a register
store, and a direct x86 call edge. These task IDs belong to an entirely
separate registration namespace from the previously recovered `SCHED_*`
schedule IDs.

| Original task symbol | Original 2006 TASK registry ID |
|---|---|
| `TASK_GET_PATH_TO_RALLY_POINT` | `0x186A0` |
| `TASK_FACE_RALLY_POINT` | `0x186A1` |
| `TASK_GET_PATH_TO_ASSAULT_POINT` | `0x186A2` |
| `TASK_FACE_ASSAULT_POINT` | `0x186A3` |
| `TASK_HIT_ASSAULT_POINT` | `0x186A4` |
| `TASK_HIT_RALLY_POINT` | `0x186A5` |
| `TASK_AWAIT_CUE` | `0x186A6` |
| `TASK_AWAIT_ASSAULT_TIMEOUT` | `0x186A7` |
| `TASK_ANNOUNCE_CLEAR` | `0x186A8` |
| `TASK_WAIT_ASSAULT_DELAY` | `0x186A9` |
| `TASK_ASSAULT_DEFER_SCHEDULE_SELECTION` | `0x186AB` |

The observed 2006 task registration block does not map local number
`0x186AA`. That is an unresolved local gap, **not** proof that no task
anywhere in the engine uses that ID.

The numeric collision is real and harmless if IDs remain typed:
**`TASK_AWAIT_CUE = 0x186A6`**, while the independently verified
**`SCHED_WAIT_AND_CLEAR = 0x186A6`**. They are different namespaces.
They must not be implicitly interchanged in a new native module.

The original schedule scripts also invoke inherited/general-purpose
`TASK_RUN_PATH`, `TASK_WAIT`, `TASK_SET_ACTIVITY`, etc. Those are
**not** among the 11 specialized assault-task registrations verified here.

## Proven: two original vtable task dispatch routines

The original `CAI_AssaultBehavior` vtable starts at `0x223F5544`.
At offsets `+0x60` and `+0x64` it contains two distinct original
task-dispatch methods:

| Original C++ vtable offset | Original x86 method VA | Instruction-level witness |
|---|---|---|
| `+0x60` | `0x220CF190` | Reads a passed-in task identifier and compares against `0x186A5` |
| `+0x64` | `0x220CF770` | Reads a passed-in task identifier and compares against `0x186A6` |

Their arrangement and signatures are **consistent with**
`CAI_AssaultBehavior::StartTask` and `RunTask`, respectively. The
exact inherited virtual method names are **not yet ABI-confirmed**, so
the binary auditor calls them “first” and “second task dispatcher.”
This avoids turning a very likely method label into unverified C++ source.

In the original assembly, the dispatch routines have branches for the
task IDs recovered above and make engine calls for AI navigation, state
updates and completion. Those full case bodies are the next decompilation
target. Merely knowing an original numeric task ID is not a native handler.

## Newly committed code

- `research/tools/audit_sin_native_assault_task_ids.py`: read-only,
  hash-pinned PE32 auditor. Verifies **all eleven** distinct task names,
  numeric IDs, original instruction operands and helper call destinations.
  Checks both actual `CAI_AssaultBehavior` vtable pointers and first
  task-dispatch instructions.
- `research/tests/test_audit_sin_native_assault_task_ids.py`: synthetic
  regression tests, deliberately corrupted IDs, register-bound ID loads,
  original helper calls, vtable slots and dispatch opcodes. No proprietary
  game DLL is committed.
- `include/sin_compat/original_assault_task_ids.hpp`: portable C++17
  reference table with `LegacyAssaultTaskId` values; keeps task and
  schedule namespaces separate.
- `tests/test_original_assault_task_ids.cpp`: all eleven IDs, collision
  with the separate schedule namespace, compile-time type separation, and
  distinction between original specialized and generic inherited tasks.
- CI and CMake registration.

## Reproduce using your untouched original DLL

```powershell
py research\tools\audit_sin_native_assault_task_ids.py --server "D:\SiN\bin\server.dll" --output "D:\SiN_Test_Only\original_native_assault_task_ids.json"
```

Adjust the original DLL path if stored elsewhere. The report is created only
if absent and **does not modify the original game**.

## Next decompilation and implementation steps

1. Decode both dispatch routines' switch cases in detail, starting with
   **`TASK_AWAIT_CUE`, `TASK_WAIT_ASSAULT_DELAY`,
   `TASK_HIT_RALLY_POINT`, `TASK_HIT_ASSAULT_POINT`**.
2. Verify task completion/failure call edges and event timing before
   implementing those semantics in the new host-driven runner from #117.
3. Keep original task IDs scoped to their registry. A SourceVR ARM64 adapter
   must receive **new native registered task IDs** and bind the verified
   semantic operations to its own engine AI task interface.
4. The complete Quest standalone game, actual engine game-module factory
   registration, player controller and original game DLL logic remain
   unfinished. The desktop MobGrunt/AR2 finale encounter is still a
   separate visual-combat integration smoke test.
