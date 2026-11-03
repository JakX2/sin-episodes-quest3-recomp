# Phase 80: original SiN dd_challenge ConVar and six advisor presets

## Primary evidence

Both owner-held original 2006 Windows x86 game DLLs were independently SHA-256 verified:
- client.dll: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- server.dll: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

Only original machine-code evidence and numeric metadata is committed. Proprietary binaries remain private.

**Correction to Phase 79's unknown-source note:** original address `0x22592168` is the ConVar *parent pointer* at `+0x18` of the static `dd_challenge` object at `0x22592150`, not an unidentified independent global.

## Exact registration and data flow

- `0x2245CDD8`: NUL-terminated name `dd_challenge`; `0x2245CDE8`: literal default `0.2f`; `0x2245CDF0`: `The challenge level.`
- `0x223C2DA0`: original static constructor wrapper passes max=1.0, has-max=1, min=0, has-min=1, name/default/help to object at `0x22592150`. Direct CALL `0x223C2DC1 -> 0x2225BDE0`, which delegates at `0x2225BE25 -> 0x2225B9C0`.
- `0x2225B9C9`: common initializer stores `m_pParent=this` at object `+0x18`; `0x2225BA4B`: stores parsed default float at `+0x28`.
- `0x222BA399` loads `[0x22592168]`, then `0x222BA39F` reads `[parent+0x28]`, caches float bits to difficulty manager `this+0x08`, and passes it as **t** to the recovered fourteen-metric interpolator at CALL `0x222BA3BA -> 0x222D3490`.
- `0x222BA3BF..0x222BA3C8` reloads the **same** `dd_challenge` float and calls `0x222C0B90`, which configures **six advisor presets**.

Also verified `dd_forgiveness`: name `0x2245CE08`, default text `0.5f`, static object `0x22592198`, parent-pointer field `0x225921B0`, and constructor at `0x223C2DE0`. At `0x222BA3A5..0x222BA3AD` the manager stores **`this+0x0C = 1.0f - dd_forgiveness`**.

**Timing boundary:** these are demonstrated *initialization* paths. We do not claim changing a ConVar in the console during gameplay triggers immediate reconfiguration. The precise runtime update lifecycle remains unknown.

## Six original five-float advisor configuration records

Original array at `0x22521F70`; six records, stride `0x14` (20 bytes); dispatcher `0x222C0B90` increments by 20 bytes until `0x22521FE8`. Each record goes to `0x222C0AE0`; four computed output float values are passed to `0x222B9450` via manager instance `0x222BC4A0` and eventually `0x222C4AC0`.

| Index | a | b | c | d | e |
|---:|---:|---:|---:|---:|---:|
| 0 | 80 | 20 | 20 | 0.3 | 0.3333 |
| 1 | 20 | 60 | 10 | 0.3333 | 0.16666 |
| 2 | 70 | 20 | 20 | 0.5 | 0.5 |
| 3 | 30 | 5 | 15 | 0.1 | 0.5 |
| 4 | 10 | 70 | 30 | 0.5 | 0.5 |
| 5 | 0 | 0 | 0 | 0 | 0 |

The original static table's **30 exact IEEE-754 words** are checked individually, including fractional constants which are not exactly representable in binary. The entire 120-byte region is additionally SHA-256 pinned. Advisor identities and meanings of the four outputs remain unknown.

## Recovered original two-branch x87 math

Let `t` be the challenge value; output indices describe *positional pointer arguments*, **not** guessed gameplay field names.

For finite `a < b`:

```text
center = (1-t)*(a+c) + t*b
low = center-c
output_0 = low - d*low
output_1 = low
output_2 = center
output_3 = center + e*(100-center)
```

For finite `a >= b`:

```text
center = (1-t)*a + t*(b+c)
low = center-c
output_0 = center + d*(100-center)
output_1 = center
output_2 = low
output_3 = low - e*low
```

Original `0x222C0AE0` uses `FCOMP`, `FNSTSW`, `TEST AH,5`, `JP` and `100.0f` constant at `0x223D6314`; its instructions are covered by exact opcode and entire-function hash checks. For default challenge `t=0.2`, record 0 yields roughly `[80.4,72,52,34.6684]`, record 1 `[17.3342,26,36,46.66624]`. The C++ reference preserves these finite-input equations, not exact excess-precision x87 rounding, NaN behavior or a complete engine setup.

## Files and verification

- `research/tools/audit_sin_challenge_advisor_presets.py`: chains Phase 79 original dual-DLL SHA-pinned verifier; checks **43 instruction sites, 9 CALL targets, 6 original strings, all 30 float words and 8 original code/table region hashes**. Emits JSON metadata only.
- `research/tests/test_sin_challenge_advisor_presets.py`: 8 synthetic mutation test groups, covering every listed opcode, CALL edge, original text and preset value; rejects fake original DLL fingerprints.
- `include/sin_compat/difficulty_advisor_presets.hpp`: pure C++17 raw configuration constants, both algebraic branches and complement calculation.
- `research/tests/sin_difficulty_advisor_presets_reference.cpp`: C++17 tests of all 30 original bitwise constants, the branches, endpoints, extrapolation and the forgiveness complement.
- `CMakeLists.txt`: native CTest registration.

On the original SHA-256-verified private DLLs the chained auditor passed. Local strict-warning C++17 test passed; original-free synthetic mutations passed **8/8**. GitHub CI status is reported separately.

## Limits and Phase 81 goals

The six advisor class names, semantic meaning of preset arguments, ConVar change-time callback behavior, full manager update/scoring schedule, Source engine message framing/ABI and complete Quest 3 ARM64 gameplay implementation are **not yet recovered**. Next: trace `0x222B9450` and `0x222C4AC0` into the six advisor object constructors and runtime changes. This is genuine original machine-code-grounded gameplay configuration recovery, **not** a playable port or complete source reconstruction.
