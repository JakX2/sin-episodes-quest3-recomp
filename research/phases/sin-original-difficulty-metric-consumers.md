# Phase 79: original SiN blend input and four gameplay metric consumers

## Original private binary evidence

Original 2006 x86 PE32 client.dll and server.dll were independently SHA-256 verified against the Phase 78 fingerprints. The original DLLs are never uploaded to GitHub. Exact original instruction addresses and constants below are from the matched server.dll.

## Origin and reuse of blend input t

- At 0x222BA399 the original initializer routine 0x222BA360 loads the pointer stored in global 0x22592168; at 0x222BA39F it loads a 32-bit value at pointer+0x28 and caches those bits into manager this+0x08 at 0x222BA3A2.
- Original instructions 0x222BA3B0–0x222BA3BA reload the same global pointer+0x28 value and pass it as the floating point blend parameter to Phase 78's verified function 0x222D3490.
- At 0x222BA3C8 the same source float is also forwarded to 0x222C0B90. The identity and gameplay purpose of that second subsystem are not established here.
- After the manager archive-reader function 0x222BBF50, the original callsite 0x222BBFDA invokes 0x222B9130. Its 0x222B915A comparison checks manager this+0x264. When the flag is clear, it loads manager this+0x08 (0x222B9189), sets this+0x0C to 0.5f (0x222B918D), calls metric blend 0x222D3490 (0x222B9194), and sets this+0x264 to one (0x222B919C). Additional virtual operations in this function remain unmodeled.

**Unproven:** the source object's actual registered ConVar or scripting name, initializer call timing, other state-change triggers, the full manager archive format and the source float's valid gameplay range. No assumption that it is the ConVar dd_challenge has been made.

## Original battle-rule consumers

| Original method | Metric number and name | Observed instruction behavior |
|---|---|---|
| 0x222D5420 | 0 DAMAGE_TAKEN_SCALE | Tests DWORD [damageInfo+0x38] against 0x80024021. On any matching bit the JNE skips the scale; otherwise multiplies [damageInfo+0x2C] by metric 0 and stores the result. |
| 0x222D47F0 | 1 DAMAGE_DONE_SCALE | Gets metric 1 via 0x222D33E0, then uses x87 FMUL to scale a float argument. |
| 0x222D4830 | 2 AMMO_QUANTITY_SCALE | Gets metric 2 via 0x222D33E0, and returns it. |
| 0x222D4CF0 | 11 RANDOM_NPC_WAIT_BEFORE_SHOOT | Gets metric 11 and adds a float argument via x87 FADD. |

The mask 0x80024021 has bits 0, 5, 14, 17 and 31. Tests individually verify these five bypass bits as well as unmasked types. Read-only original function pointers to the four methods appear at 0x2245F848, 0x2245F844, 0x2245F880 and 0x2245F8F8 respectively. This proves actual code references but does not prove every inherited class slot or runtime call frequency.

The original Phase 78 initializer endpoints make metric 0 equal 0.75 at blend t=0 and 1.4 at t=1. Metric 11 is +0.1 at t=0 and -0.1 at t=1. Metrics 1 and 2 both initialize to constant 1.0 across endpoints, so the C++ tests also use modified, post-archive metric values to prove distinct metric indices are used.

## Implemented and checked

- include/sin_compat/difficulty_metric_consumers.hpp: side-effect-limited C++17 numeric helpers accepting explicit current metric arrays, plus original first-value and interpolation accessors.
- research/tests/sin_metric_consumers_reference.cpp: native reference tests of original damage-type mask, each bypass bit, mutable metric indices, ammo and delay arithmetic.
- research/tools/audit_sin_metric_consumers.py: chains prior Phase 78 dual-DLL audit, then verifies 31 exact original instruction sites, 8 direct relative CALL edges, 4 original read-only function pointers, and 6 SHA-256 original instruction regions.
- research/tests/test_sin_metric_consumers.py: original-free synthetic opcode/call/pointer mutation tests, original SHA requirements.
- CMakeLists.txt: BUILD_TESTING CTest target for the native reference tests.

Original DLL audit passed against both SHA-pinned binaries. Locally: 6 grouped Python synthetic tests passed, C++17 compiled with -Wall -Wextra -Wpedantic -Werror, standalone CTest passed 1/1. GitHub Actions CI is checked separately after commits.

## Next steps and limits

Trace the identity of the global object pointed to at 0x22592168, owner and callsites of the battle-rule vtable, metric update scheduling and effects on actual entities. Test x87 finite-input rounding against ARM64 hardware. Connect a real Source-compatible server DLL/ABI and perform gameplay regression tests. No claim of complete Ritual C++ source, full game logic, Source savegame framing or running Quest 3 APK is made.
