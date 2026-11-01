# Phase 97: Original feedback-report deadline and one-shot flag writer

**Evidence:** Privately owner-held 2006 SiN Episodes Windows x86 `server.dll`, SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. This reconstructs a specific, independently verified executable-path subset, not the complete player-performance feedback controller or a Quest-ready engine.

## Key discovery: the missing writer to the Phase95 pending flag

Original DLL instructions at `0x222BC3E3` through `0x222BC493` establish:

| Field or address | Observed execution |
|---|---|
| feedback object `+0x04` | If the active byte is zero, skip this entire timer/feedback section |
| object `+0x74` | Scheduled report deadline, compared against current game time |
| object `+0x68` | Report interval, added to current time when the report is due |
| `0x222BC475` | Writes **1** to the one-shot pending publication flag at object `+0x05` |
| `0x222BC47F` | Loads a separate engine-setting pointer from global address `0x225923A8` |
| engine-setting object `+0x2C` | If zero, skip the downstream update, **after** the pending flag has already been raised |
| `0x222BC48E` | Calls `0x222BC0E0`, the Phase95 update wrapper, **even when the report deadline was not reached**, provided the object is active and the engine setting is enabled |

The original routine's deadline condition is inclusive: when `next_report_time <= current_time`, it raises the flag and calculates `new_report_time = current_time + interval`. The timing is anchored to current time, not to an older deadline.

This directly links to the previous Phase95 binary evidence:

- `0x222BC0E0` calls the independent update gate at `0x222BA420`.
- If its own gate is due, that routine runs six category samples.
- It then checks object `+0x05`, clears the flag at **`0x222BA460`** if set, and reports whether the six follow-up stages should execute.
- Therefore the report flag may be raised while an independent feedback-sample deadline is **not yet due**. The bit persists until a later qualifying sample consumes it.

The full enclosing function performs other work before checking the report timer, including engine, time and actor state checks. **Those other paths, their prerequisites and full consequences are not reconstructed here.**

## New host-neutral C++17 model

`DifficultyPublicationDeadline` in `include/sin_compat/difficulty_publication_deadline.hpp` and `src/difficulty_publication_deadline.cpp` links directly to the Phase95 `DifficultyFeedbackTick`.

- Its configuration provides externally supplied active status, report deadline and interval. This is *not* original savegame parsing.
- On a due active report it advances the deadline and raises the Phase95 tick's pending-publication flag before consulting the engine-setting provider.
- If the setting is enabled, it calls Phase95's feedback gate regardless of whether the report timer itself was due. The second gate retains its own independent deadline.
- A disabled engine setting stops update-gate invocation but does not drop a newly raised report flag.
- Finite values, monotonic time, positive intervals and overflow are checked for safety. These validation/error outcomes are independently authored and **not claimed to match the original x87 engine under malformed state**.

## Tests and original evidence verification

`tests/test_difficulty_publication_deadline.cpp` uses a synthetic host to exercise:

1. A report deadline that fires *before* the sampling deadline, leaving the flag pending.
2. A later sampling update that consumes the flag and invokes follow-up stages.
3. Two independent report periods and the six callback stages.
4. A disabled global setting that leaves the report flag latched until updates resume.
5. Inactive state, regressing clock, invalid/overflowing intervals, and an unconfigured downstream sample gate.

The new read-only `scripts/verify_difficulty_publication_deadline.py` pins the original DLL's whole-file hash, **17 instruction spans**, exact report flag writer, its consumer at `0x222BA460`, report interval/deadline field offsets, and direct call target `0x222BC0E0`. Ten original-free fabricated PE32 tests in `research/tests/test_verify_difficulty_publication_deadline.py` reject changed branches, offsets, opcodes, call targets, consumer clearing behavior and incorrect provenance.

The code builds alongside the existing desktop and Android ARM64 compatible-library code without changing the exported host C ABI. CI covers native CTests, original-free Python tests, Android NDK compilation, and an Android emulator module-loading smoke test. This is **not** native game-module or headset validation.

Reproduction (private original DLL only for direct evidence check):

    python3 scripts/verify_difficulty_publication_deadline.py /private/server.dll
    python3 -m unittest discover -s research/tests -p 'test_*.py'
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

## Remaining limitations

The full feedback calculation inside six collector categories, the exact player-performance metrics/weights, other original update branches, real SourceVR client/server game-module compatibility, campaign BSP execution and Quest 3 headset testing remain unresolved. The original game's engine-setting name and precise startup/save-state handling also remain to be verified.

No original commercial game files, binaries, scripts or numeric tables are committed. Engine issue #12 and campaign issue #20 remain open. **No headset testing; main unchanged.**
