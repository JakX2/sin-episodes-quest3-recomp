# Phase 157: original client HUD renderer preflight and resource rectangle getters

**10 November 2026.** Direct study of owner-held SHA-pinned SiN Episodes: Emergence (2006) x86 `client.dll`, building on the shared coordinate scaler recovered in Phase 156. This phase identifies the common renderer called by poison, breath and five other original HUD code paths. It does **not** claim that the entire native renderer is recreated in C++.

## Source provenance and primary addresses

- Original `client.dll` SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original `server.dll` SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

| Original client code | Verified original byte length | Full original code-region SHA-256 | Actual reconstruction |
|---|---:|---|---|
| `0x241417A0` | 13 | `e398c45791c0ff3cde3fe62f5b117d42ab0a6c4413411187922f938ef65e87f9` | Complete resource dimension A getter, 32-bit wrapped subtraction |
| `0x241417B0` | 13 | `50ee9b1f22450afc4495431683d02390ac4d89474c01aa338d09eae515f13587` | Complete resource dimension B getter, 32-bit wrapped subtraction |
| `0x24154D50` | 1,271 | `73b217a2cde0cd55eaf467aeaa6352fd64e2fba8b844df4ec078d9b445ffaa54` | **Only common input preflight/branch classification**, not entire renderer |

Original resource object getters are simple and fully verified:

- `0x241417A0` returns signed-32-bit `*(this+0xAC) - *(this+0xA8)`.
- `0x241417B0` returns signed-32-bit `*(this+0xB4) - *(this+0xB0)`.

Their original machine subtraction wraps as a 32-bit result. Our portable reference avoids signed C++ overflow while preserving those modulo-32-bit results. The semantic names of these bounds are *inferred* from their usage, not recovered source symbols.

## Native HUD renderer: what is known precisely

Disassembly at `0x24154D50` reveals ten 32-bit stack arguments and a `RET 0x28`. The original native body has multiple internal draw paths and talks to engine-owned VGUI objects.

**Common entry stage, modeled in C++:**

1. Original sixth argument is a primary render-resource pointer. If null, the method branches directly to the return at `0x24155240` without resource reads, floating point preparation or drawing.
2. Original seventh argument is a float fraction. `FCOMP 1.0f/FNSTSW/TEST AH,0x41` clamps *ordered values above 1.0* down to 1.0. `FCOMP 0.0f/TEST AH,0x05/JP` clamps *ordered values below 0.0* up to zero. **NaN remains unordered and is not changed.**
3. The native routine computes `float(1.0f - clamped_fraction)`, storing it before the further drawing code.
4. Original resource getter `0x241417B0` obtains one signed integer extent; getter `0x241417A0` obtains the other. A call to already reconstructed original `0x240FFAD0` obtains a third signed screen dimension from global `0x2433FC14`.
5. The two resource extents are multiplied by the same x87 scale ratio: **`signed_global / original caller argument 4`**. Each store rounds its result into a 32-bit float. Bit-perfect extended-x87 parity on arbitrary ARM64 hardware remains unproven.
6. If original fifth argument is nonnull, the routine first uses the optional second-resource draw path `0x24141DD0`. Its details are deliberately **not implemented** here.
7. Original tenth argument is a draw mode. At `0x24154EA9` unsigned modes above 3 return without entering the four-case main mode switch. Modes 0 through 3 dispatch through the original jump table at `0x24155248`, with verified original targets `0x24155080`, `0x24155137`, `0x24154EB9`, and `0x24154F70`.

`prepare_original_hud_bar` in this phase is a **non-rendering plan**. Its `ValidMode` status means a native mode was *recognized*, not that it executed correctly or drew pixels. The original body contains many further x87 integer-conversion and engine VGUI calls that are not represented by this plan.

### Cross-reference to both original HUD timers

Seven original client `CALL rel32` sites target `0x24154D50`:

`0x2422F2A3`, `0x24230D6F` (**breath**), `0x24233774`, `0x242337FD`, `0x24234F50`, `0x24235973`, and `0x24236208` (**poison**).

Both verified timer paint methods pass the address of their field at `this+0xA3` **twice** to the native renderer. Both also test byte `this+0xA6` in some paths. This is strong evidence that `+0xA6` is the fourth byte of a packed 4-byte color-like value, potentially alpha, but the original owner/type and write sources have **not** been proven. A separate routine at `0x241E6EE0` also writes an offset `+0xA6`, but has a **different surrounding memory layout** and must *not* be attributed to these HUD objects without a class identity bridge. This specifically avoids falsely naming that unrelated store as a HUD-alpha writer.

The Phase 156 coordinate scaler `0x24154AB0` and this drawing function `0x24154D50` are distinct: one prepares a pair of float coordinates; the latter consumes resource pointers, fractions, modes, and native draw callbacks.

## Exact original-byte audit and new source

- `include/sin_compat/original_client_hud_renderer_preflight.hpp`: portable C++17 wrapper for both original signed extent getters, ordered fraction clamp, missing primary-resource early exit, common scale calculations, optional-secondary flag and explicit four-mode classification. No original ABI structure is reproduced.
- `tests/test_original_client_hud_renderer_preflight.cpp`: extent wraparound, null primary short-circuit, zero/infinite/NaN fraction bounds, zero-divisor behavior, four modes, and integration with both Phase 153/155 HUD paint gates.
- `research/tools/audit_sin_original_client_hud_renderer_preflight.py`: SHA-chain owner-only verifier, **3 complete original code hashes**, deliberately sparse instruction witnesses, **15 original CALL edges**, four verified jump-table pointers and exact native branch opcode checks.
- `research/tests/test_sin_original_client_hud_renderer_preflight.py`: synthetic original-free mutation guards for every anchor, call and jump-table entry plus independent x87 status-flag clamp comparison.
- Root CMake/CTest target.
- The Phase 152 recovery ledger now has **40 manually reviewed original functions** (25 server and 15 client). The two 13-byte getters are `behavior_reconstructed`; the 1,271-byte renderer is **only `machine_code_verified`**, precisely because the full native behavior remains unfinished.

To audit exact private owner binaries:

```sh
python3 research/tools/audit_sin_original_client_hud_renderer_preflight.py \
  --client /private/client.dll --server /private/server.dll \
  --output /private/phase157-hud-renderer.json
```

To run original-free tests:

```sh
python3 -m unittest discover -s research/tests \
  -p test_sin_original_client_hud_renderer_preflight.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_client_hud_renderer_preflight_tests
ctest --test-dir build -R '^sin_original_client_hud_renderer_preflight_tests$' --output-on-failure
```

## Remaining work

The original renderer's four mode cases, VGUI pixel/texture callbacks, clipping, packed color ownership, resource lifecycle and compatibility with the original `VClient011` ABI remain unresolved. The next significant increment should trace original `0x24141DD0` (used inside each mode) and investigate the setters for `CHudBreathTimer/CHudPoisonTimer +0xA3..0xA6` by real class ownership. No playable PC/Android/Quest SiN build is claimed.
