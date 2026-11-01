# Phase 84: SDK-grounded difficulty checkpoints and encounter integration

**Evidence levels:** Owner-supplied original SiN Episodes SDK Hammer map settings (static inspection); an independently authored portable C++17 reference runtime; a historical interpretation of the 2006 binary's +/-one spawn-count rule. **No original game runtime, real NPC spawn, Quest game module, or headset tests are claimed.**

## Original SDK settings

The SDK's SinEpisodes.fgd declares trigger_dynamic_difficulty as a solid trigger with EstimatedTime (seconds since last difficulty trigger/map start), EstimatedDifficulty (choices 0–4, default 2), EndOfGame, and spawnflag 1024 (“Don't destroy after triggering”). Original NPC template makers in the sample campaign maps configure StartDisabled, SpawnFrequency and MaxLiveChildren.

The privately inspected owner SDK source VMFs contain these verified values:

| Source map | Checkpoints | Tier frequencies | Template makers | Initially disabled | Repeating checkpoint flag |
|---|---:|---|---:|---:|---:|
| se1_docks04 | 6 | tier 1: 1, tier 2: 5 | 20 | 20 | 0 |
| se1_highrise01 | 9 | tier 1: 2, tier 2: 5, tier 3: 2 | 54 | 54 | 0 |
| arena_office | 0 | none | 1 | 1 | 0 |

All 15 campaign checkpoint entities have ordinary spawnflags 1 and **none** has the optional repeat flag 1024. Every one of the 74 campaign NPC template makers has StartDisabled 1 and explicit MaxLiveChildren. Docks maker limits: 1 (13), 2 (5), 3 (2). Highrise limits: 1 (38), 5 (16). Total EstimatedTime field values are 600 seconds (docks) and 1828 seconds (highrise), designer-authored segment estimates, **not actual player completion times or verified checkpoint ordering**.

## New portable implementation

- include/sin_compat/difficulty_checkpoint_runtime.hpp and src/difficulty_checkpoint_runtime.cpp: DifficultyCheckpointRuntime registers generation-safe checkpoint references, validates tiers/expected duration, reports elapsed time between *accepted* checkpoint touches and upcoming expected tier, supports optional repeat behavior and refuses stale handles or time regression. Hosts own entity lifetime and map-clock inputs.
- SpawnCountDecision adjust_spawn_count uses a supplied [0,1) host random roll and a [-1,1] modifier to deterministically model the previously reported CSE1Rules +/-one enemy-count branching. It cannot create negative spawn counts, overflow int or accept invalid/nonfinite values. **This historical reconstruction is not newly validated against original DLL instructions in Phase 84.**
- Two new native tests cover checkpoint boundaries, one-shot/repeat state, endgame fields, stale generation, expected/actual elapsed values, RNG thresholds, and a synthetic trigger-to-relay-to-checkpoint-to-maker sequence driven by the existing Phase 74 EntityIoRuntime.
- In the second test, the maker is a fake host and SyntheticReached is a test-only event. We do not claim that exact output name is part of any released SiN map.
- The component is built into the portable library, including the official Android NDK target, with **no new exported C ABI functions**.

## Private SDK encounter auditor

research/tools/audit_sin_sdk_encounters.py uses Phase 76's private VPK2 CRC-validated VMF reader. It reports only aggregate numeric checkpoint and maker settings and no map geometry, entity targetnames, NPC template names, output records or original data files. Ten original-free Python fixture tests verify defaults, repeat/endgame flags, tier bounds, durations, maker limits/frequencies and malformed input handling.

Reproduce (the private archive must remain outside git):

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    python3 -m unittest discover -s research/tests -p 'test_*.py'
    python3 research/tools/audit_sin_sdk_encounters.py /private/SinEpisodesSDK.zip --output /private/encounter-summary.json

Local new C++ tests passed with GCC C++17 -Wall -Wextra -Wpedantic -Werror and AddressSanitizer/UBSan, and all ten synthetic Python tests passed. GitHub CI validation is separate.

## Boundaries / next integration work

This implementation does **not** calculate adaptive challenge from player performance, spawn any actual SinTEK NPC, load a BSP, call an original Source trigger, implement Source event save/restore, or prove any gameplay mechanic in Quest. It intentionally separates expected checkpoint metadata from scoring policy until the original CSE1Rules behavior is verified. To progress beyond synthetic encounters: validate the original 2006 game-rule/spawn callsites, implement a real engine host after a legally usable SourceVR-compatible SDK becomes available (issue #12), and connect released map entities to the host (issue #20).

**No VR hardware testing is attempted. No SDK, DLL, APK or map binaries are committed.**
