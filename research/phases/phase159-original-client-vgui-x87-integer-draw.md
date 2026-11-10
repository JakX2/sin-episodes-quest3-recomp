# Phase 159: original 2006 client x87 integer HUD draw adapter

**10 November 2026.** Independent decompilation of SiN Episodes: Emergence original PE32 client.dll. This continues the native HUD renderer recovered in Phases 157 and 158. No original DLLs or game assets are committed.

## Original binary evidence

- client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- Original function entry: **0x241420D0**, 327 bytes, ending in RET 0x14 at 0x24142214.
- Original entire function-region SHA256: e0717404d4d24787028e5db0f7cef56b8c9c0f33f1f31b468db975a80e573c75.
- 34 sparse original x86 opcode witnesses, including several x87 control-word overrides and FISTP sites.
- Original getters called from 0x24142106 -> 0x240FFAE0 and 0x2414210B -> 0x240FFAD0.
- Original native worker called at 0x2414220B -> 0x24141DD0 (Phase158).
- Seven separately verified external callers: 0x2413FE27, 0x2413FE6E, 0x2422F355, 0x2422FCC9, 0x242358B6, 0x242359D8 and 0x24236993.
- **10** exact original direct CALL edges in total.

## The recovered complete conversion algorithm

The original native helper takes five 32-bit caller words and a resource object as this. Its first two words are float32 coordinates, the third is not read in the function body, the fourth is a float32 scale denominator, and the fifth is an opaque 32-bit value forwarded unchanged.

The resource rectangle extents are W = signed32(this+0xAC) - signed32(this+0xA8), H = signed32(this+0xB4) - signed32(this+0xB0), with original 32-bit wrapping subtraction.

The original executes FILD then FSTP float32 separately for W and H, rounding the large signed integer extents to IEEE float32. This affects later integer conversion, especially numbers greater than 2^24.

The original calls secondary dimension getter **0x240FFAE0 first**, then primary dimension getter **0x240FFAD0**. The secondary getter return is overwritten unused. The primary returns signed integer global at 0x2433FC14 and creates x87 ratio R = primary_dimension / float32(arg4).

Before each signed integer conversion the original saves the x87 control word and modifies it through OR AH,0x0C, selecting **round toward zero**, then uses FISTP. With invalid-operation exceptions masked, NaN, infinity, and out-of-range inputs write x87 signed integer-indefinite 0x80000000. The portable model explicitly handles those cases without undefined C++ casts.

The nine raw 32-bit words supplied to native worker 0x24141DD0 are:

| Callee word | Value |
|---:|---|
| 1 | FISTP(arg1 float32) |
| 2 | FISTP(arg2 float32) |
| 3 | FISTP(float32(W) * R) |
| 4 | FISTP(float32(H) * R) |
| 5 | 0 |
| 6 | 0 |
| 7 | FISTP(float32(W)) |
| 8 | FISTP(float32(H)) |
| 9 | original arg5 raw bits |

For W=200, H=400, arg1=12.9, arg2=-3.9, primary dimension=900, divisor=1200, and arg5=0x7FC01234, the output is:

    [12, -3, 150, 300, 0, 0, 200, 400, 0x7FC01234]

The native helper ends in RET 0x14, consuming five original caller arguments. Unlike the previous seven-to-nine adapter, this five-to-nine adapter **truncates floating point values** and chooses two zero constant arguments. They are distinct original methods, not interchangeable wrappers.

## Correctness boundaries

- Original int32 16777217 first rounds to float32 16777216, and the returned unsigned word reflects 16777216.
- Original int32 2147483647 rounds to float32 2147483648, then FISTP returns 0x80000000 with invalid operation masked; a naive direct conversion would not reproduce this.
- Negative finite values truncate toward zero, not floor or round-nearest.
- Zero denominators/NaNs and overflow are preserved as x87 invalid integer results, without invented safeguards.
- The reference uses long double to approximate the x87 80-bit ratio. Exact last-bit behavior on non-x86 targets, when long double has different precision, is not claimed.
- The real native worker 0x24141DD0 remains a separate partially reconstructed engine function. This phase returns the nine-word packet and **does not draw pixels or emulate VGUI interface calls**.

## Code and tests

- include/sin_compat/original_client_vgui_integer_draw_adapter.hpp: independent C++17 five-to-nine adapter with original getter order, staged float32 extents, signed FISTP truncation and masked-invalid sentinel.
- tests/test_original_client_vgui_integer_draw_adapter.cpp: strict GCC C++17 and Clang AddressSanitizer/UndefinedBehaviorSanitizer tests for stack layout, negative values, NaN, infinities, denominator zero, 2^24 and 2^31 boundaries, unused arg3 and preserved arg5 payload.
- research/tools/audit_sin_original_client_vgui_integer_draw_adapter.py: exact owner-only client/server SHA chain and complete original code-region verification.
- research/tests/test_sin_original_client_vgui_integer_draw_adapter.py: synthetic original-free corruption tests for all 34 opcode sites and 10 original call edges, plus 320 independent Python float32/x87 integer-conversion cases.
- Root CMake/CTest registration.
- Phase152 recovery atlas grows from 42 to **43** reviewed original functions (25 server / 18 client), with this complete argument adapter marked behavior_reconstructed. This does not imply only 43 real functions have been decompiled across all research phases.

Owner-only original binary audit:

    python3 research/tools/audit_sin_original_client_vgui_integer_draw_adapter.py --client /private/client.dll --server /private/server.dll --output /private/phase159.json

Original-free tests:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_vgui_integer_draw_adapter.py -v
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --target sin_original_client_vgui_integer_draw_adapter_tests
    ctest --test-dir build -R sin_original_client_vgui_integer_draw_adapter_tests --output-on-failure

## Next

Verify original class ownership of the native draw-resource object, the VGUI surface virtual callback contracts, and HUD color alpha setter provenance. Broaden client prediction and weapon code recovery. This phase does not produce a playable original Windows PC replacement DLL, Android build of the actual game, or Quest 3 game port.
