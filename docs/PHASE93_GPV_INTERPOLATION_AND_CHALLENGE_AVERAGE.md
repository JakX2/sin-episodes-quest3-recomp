# Phase 93: Original 2006 gameplay-variable interpolation and challenge-value formula

**Evidence level:** Exact SHA-256-pinned owner-held 2006 Windows x86 SiN Episodes server.dll, instruction-level disassembly of two routines, plus independently authored portable C++17 code and original-free tests. No actual engine/Quest runtime is executed, and the player's game data remains outside the repository.

## Discovery: actual normalized challenge calculation

At **VA 0x222D3730**, the 2006 game:

1. Obtains a runtime list of GPV (gameplay-variable) average entries from the object reached via a global at 0x225927CC.
2. If the active-list count is zero, returns a floating-point zero.
3. For each tracked entry, resolves the case-sensitive name through a **14-name table** at VA **0x2245F04C**, using the name lookup routine at 0x222D3370. On lookup failure, logs an invalid GPV name and skips its contribution.
4. On a match, reads the 28-byte row at VA **0x22523478 + 28 * id**. The mutable current float resides at offset +0x00, rating lower bound at +0x14, and upper bound at +0x18. It calculates `(current - lower) / (upper - lower)` and clamps the contribution to [0,1] via x87 comparisons.
5. Sums the contributions, divides by the **total active entry count**, and multiplies by a float **100.0**, from VA 0x223D6314.

Thus the reconstructed formula is:

    rating = 100 * sum(clamp((current_i - lower_i) /
                             (upper_i - lower_i), 0, 1)) / active_entry_count

An unrecognized name contributes **zero**, but still occupies an active entry in the denominator. Repeated names, if supplied, are independently visited. If the active list is empty, the original routine returns zero.

**Important:** Some original bounds are **descending**. For example `AMMO_QUANTITY_SCALE` has rating endpoints 1.5 then 0.5; `PERCENT_HEALTH_KITS_DROP` is 0.9 then 0.0. The model preserves signed denominators instead of sorting the range.

The original file has 14 registered variable names, including `DAMAGE_TAKEN_SCALE`, `DAMAGE_DONE_SCALE`, `AMMO_QUANTITY_SCALE`, `NUM_FORCED_HEALTH_KITS`, `PERCENT_HEALTH_KITS_DROP`, `PERCENT_NPC_2ND_FIRE`, `PERCENT_NPC_MAKE_DIFFERENT`, `PERCENT_NPC_CHANGE_SPAWN_NUM`, `PERCENT_NPC_WEARS_HELMET`, `PERCENT_NPC_HELMET_SHOT_OFF`, `PERCENT_NPC_TAKES_COVER`, `RANDOM_NPC_WAIT_BEFORE_SHOOT`, `PERCENT_NPC_ACCURATE`, and `PERCENT_NPC_MOBILE`.

## Companion GPV modifier interpolation: 0x222D3490

The original routine at **0x222D3490** loops over the same 14 rows (in two unrolled blocks of seven) and updates their current value at +0x00 from static endpoint floats at +0x04 and +0x08:

    new_current = (1 - challenge_parameter) * low_target
                + challenge_parameter * high_target

The disassembled routine does not itself clamp the supplied parameter to [0,1]. The parameter's **caller, input schedule, and relation to player performance are not established by these two routines**. The C++ model accepts any finite parameter and fails safely on overflow, without silently clamping it.

All 14 original static `current` values match the midpoint of their original endpoint pair, within binary float precision. An owner-private TSV generated from the original DLL successfully configured the model, and an owner-only C++ probe confirmed those midpoint relationships.

## New portable implementation

`GameplayDifficultyVariables` lives in:

- `include/sin_compat/dynamic_difficulty_gpv.hpp`
- `src/dynamic_difficulty_gpv.cpp`

The host supplies row names, current values, target endpoints and rating bounds. Nothing from the proprietary original 14-row numeric table is embedded in the public code. `configure()` checks uniqueness, count and finite nonzero range; `interpolate()` updates all rows atomically using finite arithmetic; `current_challenge(active_names)` computes the verified name-based normalized average and reports recognized/visited counts.

This reference code uses double intermediates for numerical safety. **It does not promise bit-for-bit parity with the 2006 x87 floating-point precision/rounding behavior**, and its fail-closed handling of invalid input is deliberately stronger than the observed original code.

A new synthetic integration test connects the result to Phase 92's `InfoChallengeRuntime`, implementing a host that supplies `GetCurrentChallenge` and the separate entity-cached baseline for `GetChallengeDifference`. This is **a test harness**, not proof that original runtime initialized that cache in the same way.

## Independent evidence and tests

`scripts/verify_dynamic_difficulty_gpv.py` checks the entire exact original `server.dll` SHA-256:

    568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

It verifies **21 original x86 instruction spans**, two clamp constants 0.0 and 1.0, the 100.0 multiplier, all 14 name pointers, and each row's finite/nonzero rating range. Optional `--private-tsv` writes an extracted parameter table to the **owner's own storage**, never to GitHub.

Nine synthetic PE32 tests cover valid fake tables and tampering of names, ranges, constants, instructions, architecture/provenance. Two native CTests cover normalized values including descending ranges/unknown names, boundary conditions, interpolation and integration with Phase 92; a third optional, compiled-only owner-file executable checks private TSV consistency. Original file and synthetic tests passed locally with GCC C++17 strict warnings, ASan and UBSan. GitHub CI runs synthetic/native/Android builds independently.

Reproduce:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    python3 -m unittest discover -s research/tests -p 'test_*.py'
    python3 scripts/verify_dynamic_difficulty_gpv.py /private/server.dll --private-tsv /private/gpv.tsv
    ./build/sin_dynamic_difficulty_private_audit /private/gpv.tsv

The original GPV table initially produces roughly **42.45** if the **hypothetical selection contains all 14 names**. The real active GPV average list is a separate runtime object. **Do not present 42.45 as the game's default difficulty score or any verified live challenge value.**

## Not yet verified

- Exactly which GPV names the game selects dynamically for its averages, or how that list changes over a playthrough.
- Which AI/skill/checkpoint routines set the interpolation parameter, when 0x222D3490 is invoked, and how actual player statistics influence that parameter.
- Full original `CDynamicDifficulty` feedback control, checkpoint updates, NPC type/spawn decisions and stats save/restore.
- SourceVR game-module compatibility, a loaded SiN BSP, native Quest gameplay or Quest 3 headset testing.

**Phase 93 verifies two real numerical routines, not the complete adaptive-difficulty controller.** No game DLL, APK, FGD, original numeric table or other commercial content is committed. Engine integration #12 and campaign issue #20 remain open.
