# Phase 94: Original GPV interpolation callers and initialization guard

**Evidence tier:** Exact original 2006 SiN Episodes Windows x86 server.dll, SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. The owner holds the original binary privately. This phase reconstructs **observed call-site behavior**, not the complete dynamic-difficulty controller, Save/Restore system or a Quest-native Source game module.

## New x86 evidence: two direct calls to 0x222D3490

We scanned the original PE32 executable .text section for direct E8 relative calls whose computed target is the Phase93 GPV interpolation routine. Exactly **two direct rel32 call sites** occur in this pinned DLL. This does not exclude indirect calls through function pointers or dynamic dispatch.

| Caller | Original machine-code evidence |
|---|---|
| 0x222B9194, in routine at 0x222B9130 | Checks DWORD this+0x264 against zero and branches away if nonzero; gets the cached float parameter at this+0x8; stores 0.5f to this+0x0C; calls 0x222D3490 with cached +0x8; marks this+0x264 = 1 |
| 0x222BA3BA, in routine at 0x222BA360 | Sets this+0x264 = 1; reads an engine ConVar-like setting from global-pointer storage at 0x22592168, object offset +0x28; caches it at this+0x8; passes the setting to 0x222D3490; then invokes other distinct gameplay initialization calls |

The guarded routine at 0x222B9130 itself has a **verified direct caller at 0x222BBFDA**. That call follows several additional object and game-state operations which this phase does not reconstruct.

The full initialization routine also sets other fields, calls other functions, and branches on gameplay mode. We verify only the shown GPV-related subset. In particular, we **do not** claim 0x222BA360 is the sole source of slider changes, or that a ConVar value changes automatically cause re-interpolation on every frame.

### Why this matters

Phase93 recovered `current=(1-slider)*low+slider*high`, and `challenge=100*mean(normalized active GPV contributions)`. The original DLL now shows **where the interpolation parameter is read and when a guarded path skips the interpolation**.

The ConVar-like source object is read via an indirect global pointer and offset +0x28. The precise user-facing console-variable name, controller feedback loop, update frequency, external save/restore state synchronization, and all later runtime modifications to that setting remain **unverified**.

## Portable implementation

- `include/sin_compat/dynamic_difficulty_activation.hpp` and `src/dynamic_difficulty_activation.cpp` add an independent `GameplayDifficultyActivation` state model attached to the Phase93 `GameplayDifficultyVariables`.
- A host implements `IGameplayDifficultySettingSource::read_setting(float&)`, so the commercial ConVar/engine ABI is not hardcoded.
- `initialize_from_setting()` models only the GPV-related part of the direct call site. It obtains a current host setting, interpolates all configured rows and updates a cached value/initialized flag.
- `restore_if_uninitialized()` models the guarded call site. If already initialized, it does nothing; otherwise it interpolates from the cached saved parameter, records a 0.5f marker and marks initialization complete.
- `seed_cached_state()` is an **independently authored testing/host synchronization interface**. The original mechanism that loads its fields remains unknown.
- For safety, the portable model validates finite inputs and performs interpolation **before** committing state. Original x86 writes the initialization flag earlier in the direct path. Thus **failure semantics differ intentionally**; this is not strict original-object memory parity.

No direct mapping to original on-disk Save/Restore structures, real game ConVar, dynamic challenge feedback schedule or gameplay AI is implemented.

## Verification

Original verification: `scripts/verify_dynamic_difficulty_activation.py` confirms the exact original DLL SHA, both call target rel32 displacements, **15 instruction signatures**, the guarded routine's own caller, and the complete list of matching direct .text calls. The original owner-held server.dll passes. Nine fabricated PE32 Python tests reject corrupted branch instructions, slider sources, ConVar address, modified direct targets, extra matching callers and invalid provenance.

Native C++17 tests:

1. `sin_dynamic_difficulty_activation_tests`: direct setting initialization, repeated guarded restore without extra modification, cached restore independent of the latest setting, 0.5f marker, missing/nonfinite settings, missing GPV config and arithmetic overflow.
2. `sin_dynamic_difficulty_activation_challenge_tests`: **synthetic** end-to-end sequence connecting the Phase93 GPV interpolator, this activation model, and Phase92's info_challenge typed query results. Verifies that guard suppresses unintended changes, direct initialization applies a changed setting, and independently cached challenge baselines remain separate.

Both local native tests passed GCC C++17 `-Wall -Wextra -Wpedantic -Werror`, AddressSanitizer and UBSan. All nine original-free Python tests passed locally. GitHub CI independently runs the full existing host/Android ARM64 matrix and Bionic emulator smoke.

Reproduce against the owner's original DLL:

    python3 scripts/verify_dynamic_difficulty_activation.py /private/server.dll
    python3 -m unittest discover -s research/tests -p 'test_*.py'
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

## Still unresolved

- When and how original player stats feed a challenge adjustment and the ConVar-like parameter.
- The user-facing ConVar identity and exact initialization/restore schedule.
- What every additional call in the full initialization routine does, and indirect callbacks to the GPV routine.
- Actual original SourceVR client/server integration, SiN campaign BSP gameplay, animation/AI, and Quest native execution.

**No physical headset testing.** No original DLL, APK, SDK assets, maps or commercial data committed. Engine interface issue #12 and campaign issue #20 remain open; `main` unchanged.
