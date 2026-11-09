# Phase 151: original CFloatShortTermStat ring-buffer decompilation

**9 November 2026.** Focus: the original SiN Episodes: Emergence (2006) Windows x86 server.dll, as a source for previously recovered short-term difficulty advisor listeners in Phases 149 and 150. No original executable bytes or game assets are uploaded to GitHub.

## Binary evidence

Both owner-supplied original game modules have been verified:
- server.dll SHA-256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client.dll SHA-256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

The exact original MSVC RTTI TypeDescriptor class name at 0x22522E18 is **CFloatShortTermStat**; its CompleteObjectLocator is referenced from 0x2245E6E4, and its primary vtable starts at **0x2245E6E8**. All **13 contiguous virtual-function pointers** and original COL/type-name references have been verified.

The original class is a 20-byte object with virtual pointer at +0, a dirty byte at +0x04, a pointer to float storage at +0x08, configured ring size at +0x0C, and a descending ring cursor at +0x10. The portable C++ class deliberately uses its own memory representation, not this x86 layout.

## Actual recovered behavior

| Original function | Role | C++ model |
|---|---|---|
| 0x222C94C0 | Constructor from an engine-parsed length, initializes zeros | Numeric-capacity constructor |
| 0x222C8EE0 | Allocates/fills ring, sets cursor to length minus one | Constructor and resize |
| 0x222C64A0 | Adds all values, starting at (cursor + 1) mod length | Ordered ring sum |
| 0x222C9050 | Calls ring sum, returns float sum / ring length | read_slot2 |
| 0x222C9500 | Replaces *every* stored sample with supplied float | write_slot3 |
| 0x222C9530 | Writes *one* value at cursor, decrements with wrap, sets dirty | append_slot9 |
| 0x222C9090 | Obtains current mean, rebuilds ring with new capacity filled with that mean | resize_slot4 |
| 0x222C8E40 | Reads inherited modified flag | dirty_slot11 |
| 0x222C8E50 | Returns and clears inherited modified flag | consume_dirty_slot12 |

**Critical distinction:** the original short-term stat gate's source virtual +0x0C (Phase150) calls 0x222C9500, which **fills all history slots**. Individual new gameplay samples enter through virtual slot9, 0x222C9530, which **appends only one entry**. Treating them as the same operation would change adaptive difficulty behavior.

Original initial cursor is length minus one, and each append writes at the cursor then moves it one position downward, wrapping from zero to the final index. The summation helper starts from the next position and iterates all entries. Resizing resets the cursor and repopulates every sample with the *old average*, rather than clearing the history. It does not reset the dirty flag.

The source's virtual slot2 returns its mean to CShortTermStatGate or CShortTermStatGateConjugate; those Phase150 classes clamp/scale the value, and Phase149's CStatAdvisor averages their cached results. The existing Phase76 sender subsequently transmits advisor data to original client.dll over DDAdvisorScores. The original engine-specific event producer that calls slot9 has **not** been fully recovered yet.

## Original method-level provenance

Nine complete code regions have full original x86 byte SHA256 pins in research/tools/audit_sin_original_float_short_term_stat.py (constructor, initialization, sum, mean, fill, append, resize and inherited dirty methods). That auditor also checks 52 exact instruction anchors, four rel32 CALL targets (constructor -> ring init, mean -> sum, resize -> ring init, factory -> constructor), 13 vtable entries and the original RTTI. The owner-only verifier rejects any unexpected original binary fingerprint.

The original source-stat factory invokes its constructor from server.dll callsite **0x222C96E3**, providing another direct connection to the actual game statistics creation code.

## Implemented independently

- include/sin_compat/original_float_short_term_stat.hpp: header-only host-neutral C++17 class implementing the recovered mean, descending ring, fill, append, resize and dirty flag. Inherits the existing Phase150 source-stat abstraction.
- tests/test_original_float_short_term_stat.cpp: ring order, wraparound, mean, full fill, preserving mean on resize, dirty flag, zero-length safety and actual two-gate-to-advisor integration.
- research/tools/audit_sin_original_float_short_term_stat.py: read-only original PE32 dual-DLL SHA verifier and original original-code/RTTI anchors; metadata-only output.
- research/tests/test_sin_original_float_short_term_stat.py: seven synthetic fixture test groups validating every anchored byte, call, virtual slot and RTTI pointer.
- Root CMakeLists.txt: registers new Phase151 CTest executable.

Reproduction using private owner files:

    python3 research/tools/audit_sin_original_float_short_term_stat.py --client /private/client.dll --server /private/server.dll --output /private/phase151-stat.json

Independent non-proprietary tests:

    python3 -m unittest discover -s research/tests -p test_sin_original_float_short_term_stat.py -v
    cmake -S . -B build
    cmake --build build --target sin_original_float_short_term_stat_tests
    ctest --test-dir build -R '^sin_original_float_short_term_stat_tests$' --output-on-failure

## Boundaries and next steps

No original compiler C++ source, complete dynamic-difficulty engine hooks, binary-compatible 32-bit server.dll or client.dll, and no playable PC/Android/Quest build are claimed. In the original x86 implementation, an empty ring's read executes an x87 divide of zero by zero (under masked exceptions, NaN) and attempting an append may dereference invalid storage. The portable implementation safely returns NaN for an empty read and rejects an append on an empty window. Floating-point evaluations are numerically matched where tested; exact x87 extended-precision rounding parity is not claimed.

**Next:** trace the original source-stat factory's parameter-to-class mapping and actual gameplay callsites invoking CFloatShortTermStat slot9; then investigate CAccuracyShortTermStat and the over-time/long-term variants using the same SHA-pinned methodology.
