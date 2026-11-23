# Gameplay Phase634: original Elite event47 flag clearing and conditional state 4

## Original binary evidence

The owner's original, unchanged 2006 **SiN Episodes: Emergence** `server.dll` is SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, with original `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Phase632's **complete 232-byte native Elite second event handler** `0x223556A0` reaches two terminal helpers not previously implemented; Phase633 filled its clock, serial and dirty-write helpers. Phase634 authenticates and reconstructs the **remaining two complete source methods** (the original ID mapping routines *called by them* are still native dependencies).

| Original 2006 method | Entire method bytes | Full original SHA256 |
| --- | ---: | --- |
| `0x220398A0`, flag clear | 91 | `1c8c2465a759e27e7eead05d5d31074fc077765f60b8ed55ed44617a86f97327` |
| `0x2203D2D0`, conditional state 4 | 93 | `045a0f4fa001ae8c01ae319839067cec5aed1facaa6452174c9cda2d6eece679` |

This contributes **184 distinct complete original executable bytes** and **two** additional original functions, not 184 bytes of original code redistributed in the repository. Each method was verified against the owner's actual PE32 method bytes, and the instruction flow was independently reviewed with x86 disassembly.

## Source-backed method `0x220398A0`: clear a particular original flag bit

The source receives signed integer event-ID parameter from `[ESP+4]`; Phase632 calls it successively with **3**, **4**, **5** at exact source callsites `0x22355713`, `0x2235571C`, `0x22355725`.

1. Compare input **as signed** against literal **1,000,000,000 (`0x3B9ACA00`)**. If the value is less, or is exactly `-1`, call original virtual slot **`+0x760`** on the Elite, then convert event ID using `0x22073700` with mapping context `virtual760_result+0x34`. Otherwise keep the input as the index code.
2. If converter returns `-1`, exit without modifying any flags.
3. Calculate original mapped **signed 32-bit** index `delta = mapped_code - 1,000,000,000` using 32-bit wraparound. If `delta == -1`, exit.
4. Original x86 `SAR 5` produces DWORD word index, and `AND 31` produces the target bit. Native `AND [Elite + 0x7CC + word_index*4], ~(1<<bit)` **clears** the corresponding flag.
5. Original has **no general nonnegative index check** before the final memory access. A mapper returning a code below the base (other than base-1) can produce a negative original word index and a potential native invalid access. The portable model reports `OriginalUnsafeFlagIndex` and refuses rather than touching an unchecked target.

The exact `+0x760` object class and `0x22073700` ID range-map backing data are **not yet reconstructed**. The model requires an explicit host callback for both. It never assumes that IDs 3/4/5 refer to flags 3/4/5: that simple mapping is **synthetic test data only**.

## Source-backed method `0x2203D2D0`: maintenance then conditional state

The source receives a byte-like scalar argument (read as original `BYTE [ESP+8]` after one `push ESI`).

1. **Unconditionally** invoke original `0x2204E420` with the Elite. Its actual body is a thin thunk (`add ECX,0x9E4; jmp 0x220720C0`) and has real native side effects controlled by the host. Even a nonzero argument must trigger this action before returning.
2. If the byte argument is **nonzero**, skip the ID translation and bit test and write `Elite DWORD +0x7A4=4` directly.
3. If the byte argument is **zero**, invoke Elite virtual slot **`+0x760`**, resolve the same context `+0x34), and call original `0x22073700` with **literal ID `0x21`**.
4. If this mapping returns `-1`, or subtraction of 1,000,000,000 yields signed `-1`, go directly to the state write.
5. Otherwise compute the original `SAR 5` word index and bit. The original `TEST DWORD [Elite+0x7CC+word_index*4]` **checks** the bit; if it is **set**, return **without** writing `+0x7A4`. If the bit is **clear**, write `Elite DWORD+0x7A4=4`.
6. Again the original does not bound-check a negative mapped index. The source reference marks `OriginalUnsafeFlagIndex` if it cannot safely represent a valid nonnegative word index.

Both source models preserve exact external call order, have per-reference reentrancy guards, and fail closed on host exceptions, a missing mapping context or an unproven index. Host failure does not invent rollback of an earlier real maintenance effect.

## Exactly verified original rel32 edges

The Phase634 auditor requires complete original DLLs and checks **eight** direct source-call relationships: method-internal flag-map call `0x220398C3 → 0x22073700`; unconditional maintenance call `0x2203D2D3 → 0x2204E420`; state-map call `0x2203D2EE → 0x22073700`; three Phase632 flag-clear edges `0x22355713 / 0x2235571C / 0x22355725 → 0x220398A0`; and two state-method edges `0x2235572E / 0x2235577F → 0x2203D2D0`.

Additionally, **39** individual original x86 instruction witnesses cover the signed threshold, virtual `+0x760` dispatch, conversion arguments, bit indexing/shifting, flag AND, unconditional maintenance, literal `0x21`, conditional TEST, source field write `+0x7A4=4` and native return. Every witness and whole-method digest was manually checked against the actual owner's original local DLL. The original-free `--self-test` runs SHA256 single-byte mutations, x86 rel32 corruption, opcode corruption, and refusal of non-retail DLL identity.

## Implementation and complete event path tests

- `include/sin_compat/original_elite_event47_terminal_phase634.hpp`: original VA constants, IDs, offsets and `IHost` operations; explicit portable `ClearFlag` and `StateZero` references.
- `src/original_elite_event47_terminal_phase634.cpp`: original 32-bit wrap/SAR bit derivation, mapped vs direct ID path, original flag clear, unconditional maintenance, ID 0x21 bit test, and conditional state 4.
- `tests/test_original_elite_event47_terminal_phase634.cpp`: authentic path tests, wrong/invalid mapping, negative-bit hazard, nonzero direct ID, state flag set/clear/unmapped, mandatory maintenance, injected host failures, exceptions and reentrant callbacks.
- The same C++ test constructs **actual compiled references** for the Phase632 original secondary handler, the Phase633 original clock/serial/+0x4EC dirty-setter helpers and both Phase634 terminal helpers. A single synthetic **event `0x47`** goes through the true source-ordered C++ chain: clock+Elite deadline predicate, first original serial resolution, second original serial resolution and virtual `+0x404` value, third serial resolution and actual +0x4EC dirty-write, three independent ID 3/4/5 translation calls and flag clears, original maintenance action and event `0x21` flag test before conditional state `4`. It also verifies event `0x43` source gate allowed and rejected states, reusing Phase634 state-zero helper.
- These tests build as genuine MSVC x86 Win32 and portable Linux C++17 executables; no downloaded or proprietary native game DLL is needed to run them.

### Owner audit

```powershell
python research\tools\audit_original_elite_event47_terminal_phase634.py `
  --server "D:\SiN-Original\server.dll" `
  --client "D:\SiN-Original\client.dll"
```

## Honest integration boundary and next research

**This is a complete reconstruction of the two original terminal methods' control-flow decisions but not a fully standalone original-engine implementation.** The genuine `+0x760` virtual dispatch and `0x22073700` ID map contents are still engine-owned. The unconditional `0x2204E420 → 0x220720C0` maintenance side effect is also not yet ported. The complete `0x22073700` converter method is only **58 bytes** and is a promising next original-specific research target when paired with live owner evidence for the mapping table.

There is no confirmed original-game callback into reconstructed DLL code, no verified original real-game Windows test of this model, and no Android/Quest playable test. The user already has SiN maps/props rendering when ported to HL2, so this phase **does not** spend any research on rendering, model loading, props, physics, surfaces or materials. Phase634 is strictly original SiN Elite event-state behavior.
