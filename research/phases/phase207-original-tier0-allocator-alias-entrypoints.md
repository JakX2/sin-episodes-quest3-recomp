# Phase 207: original client/server tier0 allocator alias entrypoints

13 November 2026. **Draft stacked on Phase206 PR #260.** Owner-supplied original 2006 SiN Episodes: Emergence Windows 32-bit `server.dll` and `client.dll` were privately inspected at their original virtual addresses. Both complete whole-file SHA256 hashes match:

- Original `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- Original `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

No original commercial DLLs, full native function bytes, disassembly output, maps, or game assets are committed.

## Eight new verified native functions

| Original function | Server native VA | Client native VA | Size each |
| --- | --- | --- | ---: |
| Repeated imported tier0 vtable `+0x20` wrapper | `0x2226E5E0` | `0x2416FB40` | 18 |
| Primary lazy-owner stack allocation, alias A | `0x2226E600` | `0x2416FB60` | 79 |
| Primary lazy-owner stack allocation, alias B | `0x2226E650` | `0x2416FBB0` | 79 |
| Opaque two-argument tier0 vtable `+0x1C` call | `0x2226E6A0` | `0x2416FC00` | 23 |

**Eight unique original function entrypoints, 398 new complete original x86 bytes (four per DLL).** The baseline original 79-byte allocator at server `0x2226E390` and client `0x2416F8F0` and the earlier vtable `+0x20` wrapper at server `0x2226E5C0` / client `0x2416FB20` were already cataloged in Phases202/205/206 and are **reverified, not counted again**.

Each of the four new server/client method pairs has an entire body that becomes exactly byte-identical after normalizing ONLY the precise original imported allocator and module-local owner pointer/flag DWORD operands. No native function is declared recovered solely from a symbol or a guessed size.

| Entry pair | Server complete-original SHA256 | Client complete-original SHA256 |
| --- | --- | --- |
| Repeated `+0x20` | `895cd1c5f870dd7ff343225897583863df86dcbe96010ecf12a3c3760a52e041` | `3407dec8c90462f67a8084470e5d362fc741ed9ae0bf24befeb54b269ee70dc1` |
| Stack alias A | `631b6917d8e62170590843a21f22c22893e5609169125b67b2a0cbb623636e66` | `49018cbed857fb9700e1514c35d82b715c2458c212371295863becb5b6ce1d50` |
| Stack alias B | `5317deb75ca3c33c3bd1167bcb04e9353970665a5a59f40b5fb8cb5d5342effa` | `ca40a5c6f99fb923ff0e69a34763d74a9a6e78d2d4bbc239d5abf48df5c3840c` |
| Opaque `+0x1C` | `c6b2156b641595c0db8146e1b6461ef5a1668b5ee350d7959958ef70ae9f2b94` | `7041a7c7eb1e3fabe53b9c4ca9b807b08ca01531ebdce8cddf8d7978d6cf0f27` |

## Native allocator alias identity beyond matching labels

- The 79-byte alias A and alias B wrappers in **each DLL** have the exact same control flow and static owner/flag addresses as the original Phase202/205 79-byte `stack_alloc` entrypoint. They differ only in the `CALL rel32` displacement to the exact same 90-byte module-local context initializer. The new auditor normalizes those four call-displacement bytes at `+0x14` along with six exact absolute DWORD operands in each entire method body. All **six distinct** stack allocation entrypoints (three in each module) then normalize byte-for-byte identical, complete SHA256 **`976227759a29da6c6e7521a12fec6c5796d2575d7c4636dbaa62cf70e406a2a3`**.
- The new 18-byte `+0x20` wrapper is an exact **raw byte-for-byte duplicate within each DLL** of the already recovered original Phase206 `+0x20` function. Distinct native entrypoints are preserved even when a compiler produced duplicated code.
- The 23-byte native wrapper reads the original **second** stack DWORD first and pushes it, then reads/pushes the original **first** DWORD using the changed ESP offset. It dispatches the imported `tier0.dll!g_pMemAlloc` allocator object via its vtable byte offset **`+0x1C`**, then returns. The externally implemented target method name and semantics are not established by this original code. The host model preserves original argument order without speculating about allocation ownership, reference counting or engine callbacks.

The original lazy primary owner globals are **separate for each DLL**: server pointer `0x225885DC` and flag `0x225885E0`, client pointer `0x24372908` and flag `0x2437290C`. Each aliased allocation function ORs flag bit0 **before** calling the local resolver, and a returned zero owner is cached. Within a module the three stack allocation entrypoints and Phase205 register-based entrypoints share those native statics, rather than having separate per-wrapper copies.

## Implementation, validation and limits

- `include/sin_compat/original_tier0_allocator_aliases.hpp`: independent C++17 model reuses the Phase205 **single 16-byte per-DLL native lazy cache state** and Phase206 opaque `+0x20` dispatcher. It adds the distinct native entrypoint configurations and a new two-argument opaque `+0x1C` callback, deliberately without inventing its meaning. All memory pointers in tests are synthetic tokens, not original 32-bit game pointers.
- `tests/test_original_tier0_allocator_aliases.cpp`: checks first-call initialization once across both aliases and prior stack allocator, cached-null/no-retry after enabling contexts, unrelated flag bits retained, tagged owner path, module-local state isolation, and opaque `+0x20` versus `+0x1C` argument ordering and return propagation.
- `research/tools/audit_sin_original_tier0_allocator_aliases.py`: definitive dual-original whole-DLL SHA gate, eight complete native function-body SHA256s and four normalized complete-body hashes, **28 exact absolute module/IAT DWORDs**, **64 sparse x86 opcode witnesses**, **six exact original relative context-helper calls** (four new and two previously counted), exact original `ret`/INT3 code boundaries, both tier0 allocator IAT slots, six-entrypoint cross-alias full-body normalized equality and old/new `+0x20` original-body equality.
- `research/tests/test_sin_original_tier0_allocator_aliases.py`: ten synthetic-only negative-test groups that mutate sparse x86 opcodes, exact addresses/calls/boundaries, the baseline and repeated wrapper functions, and PE/IAT and whole-original hash gates.
- Root Linux CTest and Win32 x86 MSVC CTest targets added. CI status must be checked on the **exact Phase207 head** before reporting pass.

This phase increases the unique original-method research catalog **213 → 221 (120 server, 101 client)**. Reconstructing multiple allocator aliases is useful for linking the actual game call graph and stabilizing memory ownership boundaries, but **is not evidence that a full replacement SiN module runs the campaign**. The most valuable unsatisfied milestone is still owner-only original Windows engine process loading actual reconstructed game-code interfaces and executing real Init/gameplay. Windows PC first, Android next, Quest 3 third.
