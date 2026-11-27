# Phase 1654: Original 2006 `GameClientExports001` 4-slot x86 voice-export boundary

**Base:** Phase 1653 [PR #1662](https://github.com/JakX2/sin-episodes-quest3-recomp/pull/1662), SHA `9247ba9e4e1f0f258dcea22bd160ccf6553884a1`, final Windows MSVC Win32 x86 **252/252 native CTest**, with six applicable CI workflow families green. The owner-side original InterfaceReg list observer can inspect **55 client / 10 server** static registrations after trusted Windows loading, but has not yet been executed against commercial DLLs on a retail machine.

## New direct, source-authenticated original evidence

The owner-provided original `client.dll`, SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, is PE32 Intel i386 image base `0x24000000`. It was inspected locally and read-only; proprietary executable bytes are **not committed or copied into GitHub**.

An actual 21-byte original static registration wrapper at `0x242682B0` registers `GameClientExports001` through the original 12-byte `InterfaceReg` constructor `0x24164160`, creator `0x240FD810`, and static registration node `0x2433F7E4`. Its six-byte original creator returns singleton `0x242FA4B0` with vtable pointer `0x24296B18`. All full-function/window SHA hashes are recorded in `audit_sin_original_gameclientexports_abi.py`, which also checks original source bytes are in the expected PE32 .text/.rdata/.data sections.

Unlike the 48-slot `VClient011`, this `GameClientExports001` vtable has **four** entries, exactly 16 pointer bytes, original vtable SHA-256 `23ced2e618d2df47dff35544c12c237d3edf7d7b5a25c823b02d1f433c65dc82`. The next original .rdata bytes at `0x24296B28` begin `g_ParticleMgr.Update\0\0`; they are not a fifth code pointer. Each of the four original targets is in the original .text section.

| Original vtable slot | Original method VA | Proven 2006 x86 ABI | Proven behavior |
| ---: | --- | --- | --- |
| 0 | `0x240FE9B0` | 31-byte **scalar deleting destructor**; `__thiscall`, one 32-bit flag argument, `ret 4` | Tests `flags & 1`, installs base vtable `0x2429664C`, invokes original deletion operator `0x2416FD60` only if the flag requests deletion |
| 1 | `0x240FE950` | 20-byte `__thiscall`, one DWORD, `ret 4` | Fetches voice service through helper `0x241E5C00`, then calls `0x241E51B0`; full return type/semantics **not inferred** here |
| 2 | `0x240FE970` | 22-byte `__thiscall`, one DWORD, `ret 4` | Calls voice service helper `0x241E5C00`, then `0x241E5B20` with the original literal flag **1** |
| 3 | `0x240FE990` | 22-byte `__thiscall`, one DWORD, `ret 4` | Same helper/service, with original literal flag **0** |

The original voice-status implementation at `0x241E5B20` includes the diagnostic string `CVoiceStatus::SetPlayerBlockedState: setting player %d ban to %d`. Thus slots 2/3 are source-backed *block/unblock player voice* forwarding wrappers. Without complete evidence for the object's lifetime, the engine/player-index API and exact return-value types, they are **not safe to export as implemented standalone replacements**.

## New independent tests

- **Original-private file audit:** `python research/tools/audit_sin_original_gameclientexports_abi.py --client <path-to-unmodified-client.dll>`. Reads only the exact full-file SHA and small source-authenticated PE32 ranges, verifies exact 21-byte registration wrapper SHA, 6-byte creator SHA, static singleton vptr, all four original target addresses, full 16-byte table hash and precise original next-data marker. It separately verifies **all four complete original method hashes**, each original relative CALL target and the destructor's flag / delete operator / `ret 4` control-flow shape. Emits **metadata only**; original code never executed or uploaded.
- **Synthetic Python audit:** `--self-test` constructs fake PE32 pointer addresses and synthetic x86 call operands, mutates every vtable, registration wrapper, destructor and forwarding-slot byte, rejects wrong call targets, incorrect flag constants, bad table lengths or boundary markers, and refuses modified/unrecognized owner DLLs by SHA.
- **True native Windows x86 virtual ABI fixture:** `pc/independent_slot0/test_original_gameclientexports_abi.cpp` uses an entirely owned, MSVC-compiled test object with virtual destructor and three single-DWORD `__thiscall` slots. It invokes the three virtual functions by the **actual compiler vtable**, confirms distinct methods and original one-DWORD stack cleanup by ESP before/after, and invokes the scalar deleting destructor with flags 1, verifying exactly one destruction and balanced ESP. No original DLL/process is loaded or invoked.
- The two independent regression cases are registered in MSVC Win32 CTest, expected total **254** (up from the 252-case Phase 1653 baseline), and Python `--self-test` in original-free Linux decomp CI.

## Why not enable the factory yet?

`pc/independent_slot0/slot0.cpp` still correctly rejects `GameClientExports001`; its independent `VClient011::Init` returns **0** until all original startup gates have real behavior. This phase deliberately leaves those safeguards unchanged. A virtual slot 0 deleting destructor has especially sharp ownership consequences: falsely advertising this object to the engine could result in freeing host-owned memory with the wrong allocator. The native fixture proves x86 **calling convention**, not original runtime behavior.

No generic HL2-equivalent physics, surfaces, props or model loading is added. Real original game DLL initialization and playable Windows/Android/Quest 3 startup remain **unverified**. Keep the PR stacked and unmerged.
