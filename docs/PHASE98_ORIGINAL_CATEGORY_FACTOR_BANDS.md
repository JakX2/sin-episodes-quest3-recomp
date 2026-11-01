# Phase 98: Original six-category factor bands, clamped updates and diagnostic logging

**Source:** Owner-held original 2006 Windows i386 SiN Episodes `server.dll`, SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Only independently authored portable C++17 code, short instruction-verification signatures, synthetic test fixtures and evidence metadata are committed. The commercial game file remains private.

## Key correction: helper 0x222C0260 is part of diagnostic logging

Phase96 identified that the six-category collector at `0x222B9E80` calls helper `0x222C0260`. This helper starts by checking a global engine setting through `0x22592400` and its `+0x2C` field, then initializes and writes to engine file-system resources via other functions. The original string at VA `0x2245CF7C` is **dd_info.log**. Calls around `0x222BF5B0`, `0x222BF7B0` and `0x222BFAF0` further support its diagnostic logging role.

**Do not mistake the collector's diagnostic logging helper for its gameplay performance weighting algorithm.** The collector also obtains values through an engine/global interface before emitting the category information. The provenance of those values and actual category weighting remain unverified.

## Real gameplay data: six by two adjustable factors

The original binary contains a global table at VA **0x22521DE0**, organized in six 16-byte rows. Each row contains four consecutive single-precision floats:

| Offset in row | Original field role |
|---|---|
| `+0x00` | Current factor for selector less than 2 |
| `+0x04` | Saved baseline for the first factor |
| `+0x08` | Current factor for selector 2 or greater |
| `+0x0C` | Saved baseline for the second factor |

The original file initializes every float in the six rows to **1.0**. This verifies file-image initial values, **not necessarily every runtime or saved-game starting state**.

The original functions at `0x222C0740` and `0x222C07A0` select or read a band using **selector less than 2 versus selector 2 or greater**. That selection must not be conflated with the five named global/local difficulty levels from Phase96. The original caller can supply 1 or 3 to distinguish the bands.

Two operations modify the current band factors:

- `0x222C07C0`: adds a supplied float delta to the chosen current factor, then clamps to **0.05–2.0**.
- `0x222C0810`: sets the chosen current factor from an absolute float argument, also clamping to **0.05–2.0**.

The exact float bounds are stored at `0x223D9CB0` and `0x223D9CC0`, and the additive routine also writes immediate bit patterns `0x3D4CCCCD` (0.05f) and `0x40000000` (2.0f). The two absolute-setter call sites are at `0x222B926C` and `0x222B9279`; the additive call site is `0x222BAEF8`.

The reset routine at `0x222C0860` copies all six pairs of **saved values directly to their corresponding current values**, rather than applying a clamp. Its original direct call site at `0x222BBA88` is verified.

The additive caller at `0x222BAE60` calculates a magnitude involving an observed difference, `fabs` and factor **0.05**, and consults another helper at `0x222C4BF0` to determine whether to negate that magnitude. **The semantics of that helper and the source of the compared values are not established, so this phase does not reconstruct the original performance-to-delta selection**.

## Independently authored implementation

New `DifficultyCategoryBands` in:

- `include/sin_compat/difficulty_category_bands.hpp`
- `src/difficulty_category_bands.cpp`

It accepts **host-provided** six-row values, offers typed low/high reads, applies bounded set/add updates, and resets current factors from saved baselines. It preserves the original selector split (less than 2 vs 2 or more) and direct-copy reset. It rejects nonfinite input and invalid category indices. The reference uses finite double intermediate arithmetic for safety; exact cross-ISA 2006 x87 rounding/failure parity is **not** promised.

The original categories are six, but no original proprietary per-category runtime values, weights or script data are embedded. This is state manipulation, not AI response or live difficulty feedback.

### Original-free tests

- `tests/test_difficulty_category_bands.cpp`: all six rows, selector split (including unusual selector indices), clamped set/add, signed deltas, saved/current separation, reset without clamp, nonfinite/invalid states.
- `tests/test_difficulty_category_bands_tick.cpp`: **synthetic** host integrates Phase95 `DifficultyFeedbackTick` with independent fabricated deltas per category, verifies due-tick update, publication, and reset. Synthetic magnitude/sign policy is not attributed to Ritual.
- `scripts/verify_difficulty_category_bands.py`: original SHA-256-pinned, read-only audit of **40 original instruction spans**, original table's 24 float values, both clamp constants, and the `dd_info.log` string.
- `research/tests/test_verify_difficulty_category_bands.py`: **11** synthetic PE32 tampering/provenance tests cover fake positive fixture, exact hash enforcement, selector, stores, reset source, additive caller, clamp constants, original row integrity, logger name and malformed PE.

The exact private 2006 DLL passed the 40-instruction audit locally; native GCC C++17 `-Wall -Wextra -Wpedantic -Werror` AddressSanitizer/UBSan tests and 11 synthetic Python tests passed. GitHub CI independently checks all native/Python tests and compiles the existing Android ARM64 and Android Bionic module loader. This is **not** headset or native engine gameplay validation.

Reproduce with the owner's private original DLL:

    python3 scripts/verify_difficulty_category_bands.py /private/server.dll
    python3 -m unittest discover -s research/tests -p 'test_*.py'
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

## Next steps

Trace original caller `0x222BAE60`, including its dynamic comparison state and sign-selection helper `0x222C4BF0`, then seek the source player health/damage/accuracy/headshot/death/time values. Also inspect category-factor consumers at `0x222C07A0` to learn where they influence the real challenge controller. Avoid invented formulas and avoid mistaking logging operations for gameplay calculations.

SourceVR game DLL issue #12, campaign map issue #20 and a playable standalone Quest 3 port remain unresolved. **No physical headset testing, original binary/SDK/API content in public GitHub, or changes to main.**
