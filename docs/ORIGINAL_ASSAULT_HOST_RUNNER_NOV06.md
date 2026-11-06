# Original SiN assault schedule execution harness: host callback boundary

**November 6, 2026 | New, engine-neutral C++17 code**

This is a focused implementation step building on original-DLL evidence already checked
in draft PRs [#113 through #116](https://github.com/JakX2/sin-episodes-quest3-recomp/pull/116).
It is **not** a recovered C++ source file from Ritual, **not** a native Source engine
`CAI_AssaultBehavior`, and **not** an independently running Quest 3 game.

## Input evidence, separated from our implementation

The prior verified original server.dll work found nine `CAI_AssaultBehavior`
schedules, **29 task declarations**, **35 declared interrupt conditions**, actual
Source runtime schedule-parser registration callsites, original rally-point
ownership, scripted assault-point links, and original MobGrunt input handlers.

The new `include/sin_compat/assault_schedule_runner.hpp` consumes the source-derived
`original_assault_tasks.hpp` table. Its behavior is a **new deterministic
host-driven interpretation** of the original declarative plans:

- Start one of the nine **semantic** `AssaultSchedule` values, not a raw x86 numeric
  Source task or schedule identifier.
- Resolve exactly one declared task per `tick()`, in its original order.
- Check that schedule's original `COND_*` interrupt declarations before running a
  task, and report the first matching declaration. **Do not guess** what
  native combat schedule the interrupted NPC would choose.
- Return `TaskPending` and retry the same task on a subsequent tick, e.g. when
  a host-provided navigation or assault-cue callback has not completed.
- Return `TaskCompleted` only if the host explicitly reports completion.
- Interpret the two explicit `TASK_SET_FAIL_SCHEDULE` and
  `TASK_SET_SCHEDULE` metadata instructions into a semantic fallback or schedule
  switch. This models the intended declarative contract, **not** the exact
  original task executor, and cannot be used as an engine-level ABI.
- On host-reported failure, choose the declared fallback schedule if one exists;
  otherwise stop with `TaskFailedNoFallback`. Clear stale fallbacks on restart
  or new schedule.
- End a completed schedule explicitly with `ScheduleFinished`, without
  automatically deciding the next Source engine AI schedule.
- Stop on an invalid schedule reference instead of executing unknown metadata.
- Leave `RallyPointLease` ownership, map-entity selection, navigation, physics,
  animation activities, game time, damage, networking and Quest engine integration
  **outside** the runner.

The host receives each non-control `AssaultTaskWitness` and returns exactly
`Completed`, `Pending` or `Failed`. A real Source-compatible adapter must
supply the original meaning of `TASK_GET_PATH_TO_RALLY_POINT`,
`TASK_WAIT_FOR_MOVEMENT`, `TASK_AWAIT_CUE`,
`TASK_WAIT_ASSAULT_DELAY`, `TASK_HIT_ASSAULT_POINT`,
`TASK_AWAIT_ASSAULT_TIMEOUT`, and the remaining task handlers. A callback
can reject/stop if native semantics are not yet known.

### Worked source-derived rally path

The original `SCHED_MOVE_TO_RALLY_POINT` declares these steps:

```text
SET_FAIL_SCHEDULE -> SCHED_ASSAULT_FAILED_TO_MOVE
GET_PATH_TO_RALLY_POINT
RUN_PATH
WAIT_FOR_MOVEMENT
STOP_MOVING
FACE_RALLY_POINT
HIT_RALLY_POINT
SET_SCHEDULE -> SCHED_HOLD_RALLY_POINT
```

The runner's test first holds the GET_PATH step as **Pending**, checks that an
original declared `COND_HEAR_DANGER` interrupt pauses before dispatch, resumes
the original task, and verifies the final switch to
`SCHED_HOLD_RALLY_POINT`. Its host then leaves `TASK_AWAIT_CUE` pending
until explicitly acknowledged, and independently completes
`TASK_WAIT_ASSAULT_DELAY`.

For `SCHED_MOVE_TO_ASSAULT_POINT`, if the host reports a failure from
`TASK_GET_PATH_TO_ASSAULT_POINT`, the runner follows the original
`TASK_SET_FAIL_SCHEDULE` declaration to `SCHED_ASSAULT_FAILED_TO_MOVE`.
The subsequent `TASK_ASSAULT_DEFER_SCHEDULE_SELECTION` is still **host-driven**;
no arbitrary 1-second timer or Source state transition is invented.

## Tests and reproducibility

New files:

- `include/sin_compat/assault_schedule_runner.hpp`
- `tests/test_assault_schedule_runner.cpp`
- `CMakeLists.txt` test registration.

Run locally:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --target sin_assault_schedule_runner_tests
ctest --test-dir build -R sin_assault_schedule_runner_tests --output-on-failure
```

The tests cover all nine schedules, actual source-derived task order,
rally-to-hold transition, pending/resume, interrupt declaration ordering and
exact matches, original fail-schedule fallback, exhaustion, explicit stopping,
unknown schedule names, and failure without fallback. CI cross-builds the
reference on Android ARM64 as part of the existing test target suite.

**Success means the portable host-driven task harness is testable.** It
does **not** prove that the Quest SourceVR binary accepts replacement
`server.dll`/`client.dll`, that MobGrunt uses the authentic AI in the
working desktop HL2 prototype, or that the original game has launched.

## Synthetic two-MobGrunt integration proof

`tests/test_assault_simulated_encounter.cpp` connects the three separate
source-derived references already in this stacked PR series:

1. The original rally selector ranks the highest-priority available point
   before nearer lower-priority points.
2. Each rally point's separately reconstructed `RallyPointLease` refuses a
   second living NPC owner. The mock host supplies entity-generation-aware
   liveness and explicitly releases a stale owner's point. No raw original
   Windows x86 EHANDLE is exposed.
3. A **fake** navigation/cue/delay host drives the newly created task runner.
   The initial rally schedule pauses on original declared danger interrupts;
   the host reports one pending movement step before completion; the task
   sequence explicitly switches to hold-rally, then waits separately for a
   cue and for the fake assault-delay completion.

The test verifies two competing simulated grunts reserve different points,
a second living owner cannot acquire/release the first point, and completing
the hold schedule does **not** silently release the rally lease or invent
native follow-on AI. This scenario uses synthetic positions and actor IDs,
**not** actual finale map coordinates or physical Source AI navigation.

The test is built as `sin_assault_simulated_encounter_tests` in CTest. This
integration is useful precisely because its host obligations remain explicit:
a future Source-compatible engine adapter must implement real navigation,
entity handles, scripted outputs and schedule selection before the same
behavior can be claimed for Quest.

## Next engineering work

1. Recover the instruction-level implementations and enum meanings of the
   task callbacks from the **original** `server.dll`, especially cue/arrival,
   movement/failure, point timeout and the schedule's contact interruptions.
2. Implement a separate mock host that connects the previously verified
   rally-rank/lease/assault-point chain rules to this task harness, without
   claiming engine movement.
3. Establish an authorized native SourceVR custom game module loading
   interface and test original entity factory registration on ARM64.
4. Preserve the one-grunt HL2 finale BSP smoke-test baseline, with **no**
   automatic rewriting of the user's original SiN assets.

Every original behavior claim must remain pinned to actual original 2006 DLL
evidence. The harness's convenience state semantics are an explicit new
engineering choice and may need adjustment as more original instructions
are reconstructed.
