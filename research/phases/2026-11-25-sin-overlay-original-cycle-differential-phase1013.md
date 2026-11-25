# Phase 1013: original client overlay arithmetic differential

Phase1011 and Phase1012 now share one portable arithmetic header. Owner-only `python research/tools/compare_original_overlay_cycle_x87.py /path/to/client.dll` authenticates the original client hash and compiles a native C++ probe from that exact shared header.

Local Linux x86_64 execution on 2026-11-25: standalone original arithmetic slices 2408afb0..2408b03c (140 bytes, six constant relocations) and 2408c440..2408c554 (276 bytes, ten constant relocations). Original instructions execute on x87 with an x64 argument/return shim and RIP-relative relocated byte-identical constants. Only final RET is replaced by a NOP to fall through to the return shim. Direct PE32 ELF execution was unavailable on this host. No original DLL bytes committed, no retail game or engine launched.

| Routine | Cases | Bitwise float mismatches |
|---|---:|---:|
| Loop linear | 10,343 | 0 |
| Loop Hermite | 12,401 | 0 |
| Total | 22,744 | 0 |

Coverage: seeded random normalized cycle inputs and fractions [-2,2], exact 0/.125/.25/.5/.75/.875/1 endpoints, half-cycle boundary, negative and positive fractions. This verifies only the two isolated arithmetic helpers on this host against compiled shared C++ math, not whole client history, x86 MSVC output, runtime interpolation, or retail engine integration. Windows CI remains pending.
