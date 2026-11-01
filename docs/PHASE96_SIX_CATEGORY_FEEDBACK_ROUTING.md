# Phase 96: Original six-category feedback routing and global-level override

**Evidence:** Owner-held Windows i386 SiN Episodes 2006 `server.dll`, exact SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. All game binaries and SDK assets stay in private owner storage. This phase reconstructs **the routing logic** of the six-category collector, not category weighting, full player-performance challenge feedback, or a playable Quest module.

## Original machine-code findings

Phase 95 verified that `0x222BA420` invokes the six-slot collector at `0x222B9E80` when a difficulty feedback tick is due. Phase 96 disassembles the collector itself.

The original collector holds:

- The **global difficulty level** as integer field `this+0x40`.
- An array of **six pointers** beginning at `this+0x48`, one per category.
- The **category-local difficulty level** in pointed-to object offset `+0x20`.
- Other category-specific helper data at pointed-to offset `+0x1C`, passed onward to further engine logic.

The original binary's table at `0x22521DC0` names the six slots **Health, Damage, Accuracy, Headshot, Death, Time**, in that order. The five level labels at `0x22521D70` are **Very Easy, Easy, Medium, Hard, Very Hard**, indexed 0 through 4. These short human-readable labels are recorded as metadata; the full proprietary table, binary, and other assets are not committed.

At `0x222B9E90` and `0x222B9E9A`, **slot indices 4 and 5 branch directly past the global-vs-local level comparison**, retaining their category-local level.

For slots **0–3**, the original `CMP`/`Jcc` instructions at `0x222B9EA2...0x222B9ECC` choose either the local level or integer **2** (Medium), based on the global level.

| Global level | Local 0 | Local 1 | Local 2 | Local 3 | Local 4 |
|---|---:|---:|---:|---:|---:|
| 0 (Very Easy) | 0 | 1 | 2 | 2 | 2 |
| 1 (Easy) | 0 | 1 | 2 | 2 | 2 |
| 2 (Medium) | 0 | 1 | 2 | 3 | 4 |
| 3 (Hard) | 0 | 1 | 2 | 2 | 4 |
| 4 (Very Hard) | 0 | 1 | 2 | 2 | 2 |

For **Death** and **Time** slots, the effective level always equals their local level, independent of the global setting.

The collector then calls the original helper `0x222C06F0` to resolve the selected level and `0x222C0950` to resolve the category label, before invoking helper `0x222C0260` with original category data. **This phase does not reconstruct the gameplay logic inside that helper.** The sixth slot is reached by index comparison against 6 at `0x222B9F28`.

## Independent portable implementation

`DifficultyFeedbackRouting` in `include/sin_compat/difficulty_feedback_routing.hpp` and `src/difficulty_feedback_routing.cpp` uses only host-provided levels, original category indices, the observed branch decision table, and a host callback interface. It has **no embedded copyrighted game configuration**.

- Accepts global and six local levels, each in the five-label interval [0, 4].
- Exposes a typed category selector that reports the input level, chosen effective level and whether the global comparison was bypassed.
- Dispatches six selections in original category order to a host-defined callback.
- Rejects invalid levels, indices and uninitialized configuration. This fail-closed validation is **independently authored**; it does not claim to reproduce original malformed-state behavior.

A second **synthetic** integration test routes the selections through the Phase95 `DifficultyFeedbackTick` callback seam. It demonstrates deferred collection, six selected categories on a due tick, and changed host-level selections on a later tick. The test's fabricated one-shot event and timing values are *not* claimed to be original game events.

## Verification and reproducibility

`scripts/verify_difficulty_feedback_routing.py` checks the exact owner-original DLL SHA-256, **27 pinned instruction spans**, six category-name pointers/string contents, and five difficulty-level pointers/string contents. The audit checks exact branch operands at +0x40/+0x20, skip paths for slots 4–5, the integer 2 override, category iteration and helper dispatch instructions.

Ten synthetic original-free PE32 tests in `research/tests/test_verify_difficulty_feedback_routing.py` reject changed global/local field offsets, bypass-branch opcodes, the Medium override integer, category count, misdirected name pointers, corrupt level labels and hash/PE provenance.

Native C++ tests:

1. `sin_difficulty_feedback_routing_tests`: all 25 global/local combinations in **all six categories**, mixed independent levels, indexed callback dispatch, and invalid configuration.
2. `sin_feedback_routing_tick_integration_tests`: scheduled synthetic six-category ticks, callback order, changed input levels, and preserved Death/Time bypass behavior.

These are built with the existing CMake native and Android ARM64 static-library pipeline. No new native Host ABI exports or copyrighted original tables are required.

To reproduce:

    python3 scripts/verify_difficulty_feedback_routing.py /private/server.dll
    python3 -m unittest discover -s research/tests -p 'test_*.py'
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

## Next research targets and limitations

- Read the original `0x222C0260` and downstream gameplay/stat functions to determine the **actual category processing and weighting**.
- Identify writers to the one-shot flag at feedback object offset `+0x05` and how they relate to player health, damage, accuracy, headshots, deaths and time.
- Determine the real active GPV list, source setting updates, savegame lifecycle and the six downstream follow-up stages from Phase95.
- Connect to an actual SourceVR-compatible client/server module and a playable SiN campaign.

**No original game scripts, DLLs, BSPs or APKs are committed. No Quest headset tests. `main` remains unchanged.** Engine interface issue #12 and campaign issue #20 remain open.
