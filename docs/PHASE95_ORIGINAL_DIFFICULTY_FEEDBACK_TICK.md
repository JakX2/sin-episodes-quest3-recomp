# Phase 95: Original timed adaptive-difficulty feedback gate

**Evidence:** Private 2006 SiN Episodes Windows i386 `server.dll`, exact SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Independently authored portable C++17 code and original-free tests. **No original DLL, game/SDK data, or copyrighted implementation copied to the repository.**

## Machine-code recovery

The original routine at `0x222BA420` is called directly at `0x222BC0E8` inside the wrapper starting at `0x222BC0E0`. We verified:

| Original object field / code | Behavior visible in x86 |
|---|---|
| `this+0x60` | Scheduled next time for checking feedback |
| Input float argument | Current time; due when `next_due <= now` |
| `this+0x64` | Base update interval |
| `this+0x3C` | If positive, decrement and use half the interval for this scheduling step |
| Constant `0x223D9778` | Exact float `0.5f` for temporary interval multiplier |
| `0x222BA455` | Calls `0x222B9E80` every time the gate is due |
| `0x222B9E80` | Iterates **six** feedback-category slots (indexed 0–5) and invokes other engine subsystems |
| `this+0x05` | Tested **after** the six-category collector; if nonzero it is cleared and function returns true, otherwise returns false |
| `0x222BC0E0` caller | When true, calls six distinct downstream routines; zeroes two fields at `+0x94` and `+0x98` between first and second downstream calls |

The six follow-up call targets in original order are:
`0x222BAE60`, `0x222BBBD0`, `0x222BB0B0`, `0x222BF510`, `0x222BBAC0`, and `0x222BA4E0`. These are **verified addresses**, not recovered semantic names.

There is one direct `CALL rel32` to the gate (`0x222BC0E8`) and one direct `CALL rel32` to its collector (`0x222BA455`) within the pinned DLL's executable `.text` section. This is not proof that no indirect callers exist.

### Reconstructed scheduling subset

If the next time is still in the future, the gate skips collection. If it is due, the original updates the next deadline relative to **current time**, not by repeatedly adding intervals to the old deadline:

    due = next_due <= now
    next_due = now + base_interval * (half_interval_count > 0 ? 0.5 : 1.0)
    if half_interval_count > 0: half_interval_count -= 1
    sample category 0 ... category 5
    if pending_report:
        pending_report = false
        return true
    return false

The wrapper acts on a true result by calling follow-up stage 0, zeroing two counters, then calling stages 1–5. A due tick without the pending-report flag still collects all six categories but **does not** trigger the downstream stage sequence.

**Unknown:** what raises the pending flag, what inputs/weights each category collector uses, whether the true return means a challenge *increase* specifically, and the exact semantics of every follow-up routine. This phase therefore does **not** claim the full performance-to-difficulty feedback formula.

## Portable C++17 component

- `include/sin_compat/difficulty_feedback_tick.hpp` and `src/difficulty_feedback_tick.cpp` add `DifficultyFeedbackTick`, with a host interface exposing six indexed sample callbacks, six indexed follow-up callbacks and a counter-reset callback.
- Its `configure()` accepts explicitly host-provided schedule state; it is **not** an original savegame loader.
- `advance_to(now)` checks due equality, counts down half-interval updates, records the next deadline, invokes six host samples, consumes a one-shot report flag, and preserves the observed follow-up/reset ordering.
- A host can call `request_publication()` during the category callbacks, mirroring the original flag check after collector execution. This is an independent host integration interface, not an original named engine input.
- The reference rejects nonfinite/regressing time, invalid intervals/counts and arithmetic overflow. Original x87 comparison/float rounding and malformed-state failure semantics are **not** guaranteed identical.

## Verification

`scripts/verify_difficulty_feedback_tick.py` verifies the exact original DLL SHA, **21 x86 instruction spans**, the 0.5 constant, exact gate/collector direct-call sites, six downstream call targets, two counter resets and the six-slot loop.

`research/tests/test_verify_difficulty_feedback_tick.py` contains **nine original-free synthetic PE32 tests** for verification success, hash enforcement, modified timing fields, counter branch, constant, collector target, downstream target, extra caller and invalid PE data.

Two new native CTests:

1. `sin_difficulty_feedback_tick_tests`: equality boundary; no emission before due; two consecutive accelerated half intervals; regular interval afterward; exact six sample events; one-shot publication after category sampling; follow-up/counter reset order; invalid clocks and overflow.
2. `sin_difficulty_feedback_challenge_integration_tests`: a **synthetic** host using Phase 93's GPV interpolation and Phase 92's `InfoChallengeRuntime`. It deliberately provides fabricated category values and challenge input to show end-to-end orchestration without claiming those weights or events came from the game.

The pinned original DLL, all nine synthetic PE tests, and both C++17 tests passed locally; native C++ tests used strict GCC warning flags, AddressSanitizer and UBSan. GitHub CI independently reruns portable native/Python suites, Android ARM64 cross-build and Bionic emulator module smoke. No real Quest 3 test is performed.

### Reproduction

    python3 scripts/verify_difficulty_feedback_tick.py /private/server.dll
    python3 -m unittest discover -s research/tests -p 'test_*.py'
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

## Next evidence needed

To reconstruct **actual adaptive performance feedback**, continue backward from the six category call sites in `0x222B9E80`, identify their player statistic inputs, identify writers to the one-shot flag `+0x05`, and trace the follow-up functions at their verified addresses. The Phase93 interpolation and rating calculation are already available, but this tick gate does not itself prove how game performance changes the challenge slider.

SourceVR native game module interface (#12), real campaign map execution (#20), and VR gameplay remain unresolved. **No headset testing; `main` is unchanged.**
