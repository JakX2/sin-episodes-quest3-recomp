# Phase 154: complete original poison HUD fade *decision* path

**10 November 2026.** Continues Phase 153's original SiN Episodes: Emergence (2006) `client.dll` CHudPoisonTimer research. This phase analyzes the **same** 203-byte original function at `0x24236320` in greater detail. It is **not** a newly identified fifth client function.

## Original evidence, actual x86 executable

The user-supplied Windows PE32 pair was checked again:

- `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` `CHudPoisonTimer` secondary vtable slot 93: **`0x24236320`**.
- The entire original 203-byte method has SHA256 `921a342df76c159e6121789f111bcfaf66c4154cb47e4afd8b76fa2c06297150`; this was already in Phase 153, and the SHA was verified again.
- **47 new explicit x86 opcode/control-flow witnesses** tested against that method, including the `FUCOMPP/FCOMP/FNSTSW AX/TEST AH` branches.
- Previously undocumented original constant at `0x24279EEC` is IEEE754 float **`0.25`**, checked by both raw bytes and typed decoding.
- Actual original HUD sound strings: `PoisonTimeFadeIn` at `0x242C8E50` and `PoisonTimeFadeOut` at `0x242C8E3C`.
- Network producer/consumer crosswalk from Phase 153: client `m_HL2Local.m_poisonTimer` at `player+0x11EC`, corresponding original server property at `player+0x13A0`.

## Recovered control-flow order

Original x86 at `0x24236320` executes these branches:

1. Reads global local-player pointer from `0x24338CF0`. If null, returns without changing HUD fields or playing sound.
2. Loads the client player float at `+0x11EC` and the *previous HUD display float* at `+0x10C`. An x87 `FUCOMPP`, `TEST AH,0x44`, `JNP` returns without any update when the two floating-point operands compare **equal**. A comparison involving NaN is unordered and does **not** take this equal-value early exit.
3. Compares player value with 1.0. If **strictly below 1.0** and HUD byte `+0xA6` equals `0x00`, it passes original `PoisonTimeFadeIn` to a global vtable callback at offset `+0x24`, then passes the returned object to original helper `0x240104B0`.
4. Otherwise, the original second path requires the player value to compare **not below 0.25**, **not equal zero**, and **not below 1.0** while being ordered, and requires original HUD byte `+0xA6` to equal `0xFF`. This triggers `PoisonTimeFadeOut` through the same original sound resolver virtual `+0x24` and final helper `0x240104B0`. These three ordered checks collapse to **player value at least 1.0**, but the clean C++ model preserves each original comparison explicitly for documentation/audit.
5. The optional original audio resolver and helper execute **before** the final HUD update: `HUD+0x10C = float(1.0 - player+0x11EC)`. In the actual PE, subtraction uses an original 64-bit double literal at `0x242DE320` through x87 `FSUBR`.

The original byte at `HUD+0xA6` is **read but not written** in this method. Its true semantic meaning and which game events update it remain unknown; do not call it a fully understood fade state machine.

### Boundaries from the original x87 flags

| Player float | Prior HUD float | Status byte | Original outcome |
|---|---|---|---|
| Any | Any | Any, no local player | No update or sound |
| Exactly equal, including +0/-0 | Same value | Any | No update or sound |
| 0.75 | Different | 0x00 | FadeIn then update |
| 0.75 | Different | 0xFF | Update only |
| 1.0 | Different | 0xFF | FadeOut then update |
| 1.25 | Different | 0x00 | Update only |
| NaN | Any | Any | Update to NaN, no sound |
| +infinity | Different | 0xFF | FadeOut then update |
| Negative values | Different | 0x00 | FadeIn then update |

This reveals a real error in the previous Phase 153 portable projection helper: it wrote `1.0-player` even when the original x86 would skip because the incoming player value equaled the previous HUD display float. The helper and its CTest were **corrected in this phase**.

## Independently authored implementation

- `include/sin_compat/original_client_poison_timer_fade.hpp`: clear C++17 controller that preserves the actual condition order, equality and unordered cases, status byte comparisons, exact sound token names, callback-before-HUD-write ordering and original double-subtract projection. It accepts a **host-provided abstract sound-token callback**, **not** a fake native VGUI or Source engine pointer.
- `tests/test_original_client_poison_timer_fade.cpp`: tests sound token order, callback observing the *old* HUD value, no-player/float-equality early exits, signed/unsigned statuses, NaN/inf/zero, branch thresholds and existing CHudPoisonTimer paint-selector integration. Locally passed GCC strict C++17 warnings and Clang AddressSanitizer/UndefinedBehaviorSanitizer.
- `research/tools/audit_sin_original_poison_timer_fades.py`: exact SHA-pinned owner-DLL verifier extending Phase153 audit with 47 new original instruction sites, the extra 0.25 literal and the original `0x240104B0` call destination. Outputs metadata only.
- `research/tests/test_sin_original_poison_timer_fades.py`: synthetic tamper regression and **960-case independent original x87 status-flag emulation** (C0/C2/C3), checking parity and zero flag branches against portable C++ conditions. The mathematical comparison table passed locally.
- Updated Phase153 client HUD helper/test to preserve real equality early exit. Root CMake registers a new CTest.

### Remaining unknowns

The real audio dispatch involves an engine-owned singleton at `0x2437F8A0`, a virtual callback at `+0x24` and the helper `0x240104B0`. Their original interface types, ownership, side effects and audio scheduling are not yet reconstructed. Our callback abstracts these operations but **cannot claim a real sound was played**. We have not identified writers to original HUD byte `+0xA6`. Although the condition-to-event decision and final HUD store are reconstructed, **the entire original engine-dependent method is not fully rebuilt**.

Therefore Phase152 recovery ledger entry `0x24236320` remains `machine_code_verified` with a partial behavior model. Total reviewed entry count remains 30. The original engine ABI and complete `VClient011` replacement remain unresolved; no playable SiN PC, Android or Quest game is claimed.

Owner-only audit command:

```bash
python3 research/tools/audit_sin_original_poison_timer_fades.py \
  --client /private/client.dll --server /private/server.dll \
  --output /private/phase154-poison-fades.json
```

Original-free tests:

```bash
python3 -m unittest discover -s research/tests -p test_sin_original_poison_timer_fades.py -v
cmake -S . -B build && cmake --build build --target sin_original_client_poison_timer_fade_tests
ctest --test-dir build -R sin_original_client_poison_timer_fade_tests --output-on-failure
```

**Next:** trace writers to `CHudPoisonTimer+0xA6`, original sound resolver singleton `0x2437F8A0` and helper `0x240104B0`; continue client prediction/HUD work and file findings in the Phase152 evidence ledger.
