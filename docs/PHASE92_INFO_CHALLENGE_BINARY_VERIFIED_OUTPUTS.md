# Phase 92: Original info_challenge float query behavior

**Evidence:** Owner-held original 2006 SiN Episodes server.dll and SinEpisodes.fgd, both SHA-256 pinned. Independently authored C++17 query model, NOT Ritual source or a completed dynamic-difficulty formula. Original binaries and SDK remain private.

## Input/output contract recovered from original FGD and machine code

The FGD's info_challenge entity has three void inputs and three paired float outputs. The pinned DLL's input datamap entries point to actual executable handlers:

| FGD input → float output | Datamap VA | Handler VA | Original observed computation |
|---|---|---|---|
| GetInitialChallenge → OnInitialChallenge | 0x22527D40 | 0x222F4390 | Calls global provider 0x222BC4A0; reads global +0x8, forwards output |
| GetCurrentChallenge → OnCurrentChallenge | 0x22527D74 | 0x222F43B0 | Calls challenge reader 0x222D3730; forwards float output |
| GetChallengeDifference → OnChallengeDifference | 0x22527DA8 | 0x222F43E0 | Calls the same reader, subtracts an entity-local field at +0x3DC, multiplies by constant 100.0f, forwards float output |

The difference handler uses x87 **fsub [this+0x3DC]** and **fmul [0x223D6314]**; the data at 0x223D6314 is exactly the float 100.0. The three handlers use the same output emission helper with distinct output-member offsets +0x394, +0x3AC and +0x3C4.

**Important distinction:** GetInitialChallenge reads a global +0x8 value, but GetChallengeDifference subtracts a cached *entity-local* +0x3DC value. Their synchronization and the cache's initialization are not verified. Do NOT treat these fields as necessarily identical.

## New independently authored C++ component

The InfoChallengeRuntime accepts entity-generation-safe registrations with an externally supplied cached baseline. A host provides the original current and global initial challenge values; the runtime dispatches the exact FGD input names into typed float outputs. For the difference it computes:

    (host_supplied_current - host_supplied_entity_baseline) * 100.0f

It rejects unknown inputs, missing/nonfinite provider values, invalid registration baselines, and stale/recycled handles. It does not impose a speculative 0-100 clamp and cannot calculate challenge from player stats or spawn an engine entity. Actual cross-ISA float bitwise parity with the original x87 implementation is not claimed.

Source and tests:

- include/sin_compat/info_challenge_runtime.hpp
- src/info_challenge_runtime.cpp
- tests/test_info_challenge_runtime.cpp
- scripts/verify_info_challenge_handlers.py
- research/tests/test_verify_info_challenge_handlers.py

The verifier checks the whole DLL SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 and original FGD SHA-256 4e803f2e23e09ee2dd275064ee71be133536549d34321ff2c7237320ac8d60f9. It checks all three real handler registrations, ten short instruction signatures, the exact multiplier, and the FGD void-input/float-output declarations.

Original private files passed the pinned verifier. Seven independently authored synthetic PE32 tampering/provenance tests passed locally. The native C++ test compiled with strict GCC warnings and passed under AddressSanitizer/UBSan. CMake builds this component into the existing native and Android ARM64 static library without changing the public host C ABI.

## Reproduction

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    python3 -m unittest discover -s research/tests -p 'test_*.py'
    python3 scripts/verify_info_challenge_handlers.py /private/server.dll /private/SinEpisodesSDK.zip

## Remaining unknowns

- The original current-challenge calculation at VA 0x222D3730, including its stat inputs and normalization.
- How and when the entity-local +0x3DC baseline is initialized or refreshed.
- The real Source game module, challenge query output wiring and actual campaign/Quest execution.
- Physical VR headset testing, deliberately paused.

The original stats list from Phase 91 is metadata only and does not prove how any of its 62 names feed the adaptive challenge calculation. Engine integration issue #12 and campaign issue #20 remain open. No proprietary DLL, APK, SDK map, game script or asset committed.

**Conclusion:** A real adaptive-difficulty query/output calculation is now grounded in original machine instructions. The complete difficulty controller and original challenge formula are NOT yet reconstructed.
