# Phase 216: original client.dll snapshot type-1 IEEE754 float comparison and tracing

14 November 2026. **Stacked DRAFT on Phase215 PR #269.** Directly inspected the owner's original `bin.zip` containing the authentic PE32 2006 SiN Episodes: Emergence `client.dll` and `server.dll`. Both full-file original SHA256s match the existing research provenance: client `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, server `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. No proprietary binaries, full original machine code bodies, disassembly dumps or assets are committed.

## Three complete previously opaque original x86 functions

| Original client VA | Complete native bytes | Full native SHA256 | Original Phase212 CALL | RET cleanup |
| --- | ---: | --- | --- | ---: |
| `0x24198AA0` | **152** | `a9234d21d65a6b08005d10cefaa7f76be3733a5cf406734afebd74dc85fa437d` | `0x2419984C` | 12 bytes |
| `0x24197CA0` | **92** | `1a1bf9db4a3e6b585669372ce3f80b0408033bc0d23b0d2ebad627c0b86fd728` | `0x24199866` | 16 bytes |
| `0x24198470` | **37** | `0e047eb6c2d6c90164f88e69be9fe7853f57a7268b48cacfd86ab1023ce0d41d` | `0x24199875` | 16 bytes |

**Three complete SHA-reviewed native entrypoints, 281 bytes**. Real original `INT3` pads are, respectively, 8, 4 and 11 bytes. Exact original callback CALLs: diagnostics `0x24197CD7 → 0x24197910` and `0x24197CEE → 0x241979C0`, preview `0x2419848A → 0x24197FE0`. These targets remain opaque; verifying the CALL is **not** decompiling those functions.

## The recovered comparator's real three-way result

The native 152-byte comparator reads a BYTE at object `this+0x14`, the descriptor pointer `this+0x18`, descriptor BYTE flags at `descriptor+0x13`, and when normal comparison is enabled, a binary32 tolerance at `descriptor+0x30`. Original `fcomp` compares tolerance against authentic binary32 **0.0** constant at `0x242DE2F8` to gate non-exact comparisons.

1. When `this+0x14 == 0`, return **0** without reading the descriptors or arrays.
2. When descriptor flags have bit `0x04` set, return **1** without touching the arrays.
3. When the signed original array count is <= 0, return **1**.
4. Compare pairs of IEEE754 binary32 elements with original x87 `FLD; FLD; FUCOMPP; FNSTSW`. Exact ordered equality advances without changing the initial result **1**. IEEE754 signed +0/-0 and equal infinities compare equal; NaNs are unordered.
5. On a nonexact pair, if tolerance is not **strictly positive and ordered**, return **0**. Otherwise x87 computes absolute difference in extended precision and tests against original binary32 tolerance. Ordered `abs(a-b) <= tolerance` continues, updating status to **2**. If any pair differs beyond tolerance or is unordered (NaN), return **0**.
6. End with **1** when every pair was exactly equal; **2** when at least one was approximate and all were within positive tolerance; **0** on mismatch/disabled.

Crucial x87 nuance: original `TEST AH,0x44; JNP` distinguishes exact equality from unordered comparisons, and the later `TEST AH,0x41; JP` handles outside-tolerance/unordered cases. Do not replace the procedure with one `memcmp` or ordinary epsilon `fabsf` operation. The portable C++17 reference converts exact binary32 inputs to binary64 for subtraction; a subtraction of two finite binary32 values can be represented exactly in binary64. It does not emulate x87 exception status flags, traps or native invalid pointers.

## Diagnostics and preview

- `0x24197CA0`, **92 bytes**, only acts if `this+0x14` is nonzero. For operation/status **0**, calls original `0x24197910` to format `float differs (net %f pred %f) diff(%f)\n`, reading the FIRST float at each pointer and deriving **predicted minus net**. Then in every enabled case calls `0x241979C0` with `float (%f)\n` and the first predicted float. Real source x87 loads binary32 floats and pushes binary64 values to these formatters.
- `0x24198470`, **37 bytes**, tests `DWORD(this+0x34) == DWORD(this+0x18)`; when equal, invokes `0x24197FE0` with `float (%f)` and the first predicted float as an x87-converted double. It does **not** test `this+0x14`.

Phase212's type-1 case calls: **`0x24198AA0` compare → Phase213 `0x24198A70` whole-DWORD forward copy → `0x24197CA0` diagnostics → `0x24198470` preview**. All FOUR native direct callees are now reviewed. A status **1** skips Phase213's copy, but status **0** or **2** may copy if the separate original `this+0x2C` gate is nonzero. The diagnostics run **after** that copy, so the prediction/difference shown by the diagnostic callback can reflect the copied value rather than the pre-copy mismatch.

## What is committed

- `research/tools/audit_sin_original_client_type1_float_specialists.py`: read-only, requires both original whole-DLL hashes, verifies all three entire original method SHA256s, **37** sparse complete native instruction witnesses, three original router CALL sites, three nested formatter CALLs, the original 0.0 float `.rdata` constant, all original diagnostic strings, native `RET` cleanup and `INT3` boundaries.
- `research/tests/test_sin_original_client_type1_float_specialists.py`: **original-free synthetic** mutation tests for opcode, CALL, RET, padding, float constant and strings; synthetic data never increments original verification counts.
- `include/sin_compat/original_client_snapshot_type1.hpp`: clean-room comparison with IEEE754 signed-zero/NaN/infinity tests and host-only input guards, synthetic host callbacks for diagnostics/preview and original four-CALL ordering.
- `tests/test_original_client_snapshot_type1.cpp`: comparison status 0/1/2, tolerance zero/negative/NaN/infinity, unequal arrays, NaNs, signed zero, exact values, operation-specific logging and post-copy effects.
- Root CMake CTest and Windows x86 MSVC registration.

### Owner-local original verification

```sh
python3 research/tools/audit_sin_original_client_type1_float_specialists.py --server path/to/server.dll --client path/to/client.dll
python3 -m unittest discover -s research/tests -p 'test_sin_original_client_type1_float_specialists.py' -v
```

## Honest checkpoint

Reviewed native-function atlas **246 → 249 (server 128, client 118 → 121)**. Phase214's 31 opaque snapshot router destinations have decreased by **six** across Phases 215 and 216 to **25**. This is a narrow recovered-call-target count, not a game decompilation percentage. Original Source engine startup with replacement Windows modules, client prediction full object ownership, campaign playability, Android and Quest 3 remain unproven. **Windows PC first → Android ARM64 → Quest 3**.
