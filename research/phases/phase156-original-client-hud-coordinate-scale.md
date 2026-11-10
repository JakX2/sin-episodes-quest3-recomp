# Phase 156: original client.dll shared HUD coordinate scale helper

**10 November 2026.** Continuation of Phase 155, backed by static disassembly and original function-byte hashing of the **actual owner-supplied 2006** SiN Episodes: Emergence Windows x86 game DLLs. This phase recovers **three previously untracked original client functions**, and traces the shared geometry preparation used by both the poison and breath HUD paint paths.

## Exact original binary identity

- Original `client.dll` full SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- Original `server.dll` full SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

The owner-only verification script runs the entire Phase155 client/server binary audit before checking Phase156 evidence. **No original executable bytes or game assets are committed.**

## Three real original functions fully decoded

| Original VA | Length | SHA-256 of complete original method body | Recovery |
|---|---:|---|---|
| `0x24154AB0` | 38 bytes | `50d91b1f479c892647073f2d261f57d59da75ac4bce34301551dac6607ff3e13` | Both float-pointer scale operations and return |
| `0x240FFAD0` | 6 bytes | `a12626a34807f5853399a7d19183b8f64421a56c9825868a16a9f9c3cbe351f3` | 32-bit global read at `0x2433FC14` |
| `0x240FFAE0` | 6 bytes | `80f5c525298aecd4e3077fe952d3bc0977e1f06d65bbd7cef8fe3c80df1f618b` | 32-bit global read at `0x2433FC10` |

Original `0x24154AB0` receives two live `float*` outputs, a third argument that the original method **does not read**, and a fourth `float` divisor. Original x87 performs:

1. Calls getter `0x240FFAD0`. Its entire body is `MOV EAX,[0x2433FC14]; RET`, returning a 32-bit global integer.
2. `FILD` converts that signed integer to the x87 floating-point stack.
3. `FDIV` divides by the **fourth argument**, an original 32-bit float, producing a single common ratio.
4. It multiplies `*first` by that ratio and writes a **float32** result.
5. It multiplies `*second` by that ratio and writes a **float32** result.
6. Returns with `RET 0x10`, consuming four original x86 stack arguments.

The two float stores are **ordered**. If both pointers alias, the second multiplication sees the just-written first result and therefore scales the same float **twice**. The clean C++ model explicitly preserves that case. There is no original zero-divisor check or clamp. Exact x87 extended-precision bit parity on every ARM64 compiler is still not established.

The second getter `0x240FFAE0` returns another signed 32-bit global at `0x2433FC10`, without any transformation. Its likely screen-axis role is suggested by callers but **source-level names are not recoverable** from these two stripped retail methods alone.

## Shared caller evidence

The original client linear-disassembly x86 CALL scan identified **twelve direct calls** to `0x24154AB0`:

`0x2413FE03`, `0x2413FE48`, `0x24147A6D`, `0x241521D2`, `0x241525C4`, `0x2422F228`, `0x2422F332`, `0x2422FCA6`, `0x24230CEE`, `0x24234ED7`, `0x242358F8`, `0x2423618E`.

Crucially, `0x24230CEE` resides in the verified `CHudBreathTimer` panel paint virtual `0x24230C90`, and `0x2423618E` resides in the verified `CHudPoisonTimer` panel paint virtual `0x24236120`. Both paint methods call this scaler **after** their own paint-enable/resource pointer conditions. The complete painter `0x24154D50` is still native-engine dependent and is **not implemented by the reference**.

The original .rdata at `0x2429B69C` contains float32 **1600.0** and at `0x2429B6A0` contains float32 **1200.0**. Both paint methods pass these values as arguments **3 and 4** respectively to `0x24154AB0`. Original scale therefore derives from `int_global_at_0x2433FC14 / 1200.0`. Although callers pass the `1600.0` value, the scaler function **does not read its third argument**. This is valuable for preserving original behavior instead of imposing a generic width/height scaling algorithm.

Both HUD panels also pass an address of `this+0xA3` twice to their subsequent draw routine and read a byte at `this+0xA6`. This adjacency is consistent with a packed color/status subfield but is **not sufficient to prove a source-level member name or discover all original writes**. Retain the neutral name and continue investigating the actual owner and setters.

## New code and verification

- `include/sin_compat/original_client_hud_coordinate_scale.hpp`: host-neutral C++17 reconstruction of both 32-bit getters and the original two-pointer scale math, with the exact original reference 1600/1200 constants. Deliberately separate from engine VGUI ABI.
- `tests/test_original_client_hud_coordinate_scale.cpp`: original getters, unchanged secondary global, both scaled outputs, aliasing order, signed values, zero denominator/NaN/infinity, and integration with Phase153/155 poison/breath paint predicates.
- `research/tools/audit_sin_original_client_hud_coordinate_scale.py`: owner-only original client/server SHA chain, three complete x86 method-region SHA-256 checks, **13 deliberately partial instruction anchors, 13 exact relative CALL targets** (twelve external to the scaler plus its internal getter call) and both original RDATA float constants.
- `research/tests/test_sin_original_client_hud_coordinate_scale.py`: synthetic original-free mutation tests for all opcode witnesses, full region SHA requirements, callsites, data constants and wrong image base.
- Root CMake/CTest registration.
- Phase152 verified-method ledger adds **three client methods** with behavior reconstructed, each separately SHA-pinned against the exact original code region. This expands the reviewed ledger from **34 to 37** (25 server, 12 client), without mistaking those 37 for the total actual decompilation.

Owner-only audit (original commercial files stay private):

```sh
python3 research/tools/audit_sin_original_client_hud_coordinate_scale.py \
  --client /private/client.dll --server /private/server.dll \
  --output /private/phase156-client-hud-scale.json
```

Original-free tests:

```sh
python3 -m unittest discover -s research/tests -p 'test_sin_original_client_hud_coordinate_scale.py' -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_client_hud_coordinate_scale_tests
ctest --test-dir build -R '^sin_original_client_hud_coordinate_scale_tests$' --output-on-failure
```

## Next remaining questions

1. Trace the original owner and writes of shared HUD `+0xA6` separately from unrelated classes using that byte offset. A VGUI color alpha interpretation is *plausible*, not yet proven.
2. Recover the required original renderer `0x24154D50` argument conversion and resource calls, without guessing engine ABI.
3. Follow original singleton sound token resolution and `0x240104B0` side effects used by both timers.
4. Continue client prediction, weapons and HUD decompilation. Native Windows PC game load verification still comes before Android and Quest gameplay.

**This is an exact original function-level behavioral reconstruction. It is NOT a complete `VClient011` replacement, original C++ source recovery, native engine renderer, running Windows game, or Quest standalone build.**
