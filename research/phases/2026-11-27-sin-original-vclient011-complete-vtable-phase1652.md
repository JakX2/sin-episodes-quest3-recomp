# Phase 1652: Full original 2006 VClient011 vtable and replacement Win32 boundary

**Stack:** Phase 1651 PR #1660, exact head `5cc42afe797c5e4300848bfa50c2f8897f45287d`; all six final-head CI families green, Windows MSVC Win32 x86 **250/250**. Phase 1651 recovered the genuine original server `InterfaceReg` constructor, three game interface registrations and seven `ServerGameEnts001` virtual slots. This phase checks the **client** counterpart at a finer ABI boundary.

## Direct original 2006 DLL evidence

Owner-original private `client.dll` (Drive `sin files/SE1/bin.zip`), length **3,592,192**, SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, PE32 Intel i386 image base `0x24000000`. Independently read-only inspected, no original code bytes committed.

The 21-byte original static registration at `0x24268300` constructs a 12-byte `InterfaceReg` node for exact name `VClient011`. The client constructor is `0x24164160`, linked-list head at `0x24369378`, creator `0x240FD840`, and the singleton object is `0x242FA4B8`; its first DWORD is the original vtable pointer `0x24296C6C`. The original module's sole export is `CreateInterface` at `0x24164180` (established by earlier Phase 657 and verified complete body SHA).

The complete original `VClient011` vtable contains **48 consecutive 32-bit function pointers**, original VA range `0x24296C6C..0x24296D28`, total **192 bytes**, SHA-256 of that pointer table: `9d28eccf91315f7a6ecc2394386d7b4077919d8649d34297a951eb7a1cafa664`. The next bytes at `0x24296D2C` begin the independent .rdata string **`Precached\0`**, not a 49th code pointer. All 48 targets resolve to original PE32 `.text` and every target address is checked individually. Exact original pointer values are in `audit_sin_original_vclient011_complete_vtable.py` as derived address metadata, not copied machine code.

Known, separately authenticated slots relevant to the Windows integration boundary:

| Slot | Original function VA | What is actually known |
|---:|---|---|
| 0 | `0x240FE0E0` | `Init`: first slot, complete 0x4C4 method SHA recovered; 21 required engine-factory queries then multiple later gates |
| 1 | `0x240FF3D0` | `Shutdown`: original second slot |
| 33 | `0x240FDCD0` | `DispatchUserMessage`: original wrapper into `CUserMessages::DispatchUserMessage` |
| 47 | `0x240FE670` | Final pointer in original VClient011 table; signature **not inferred** here |

Other slot names/signatures are not inferred solely from pointer-table layout. This is a **vtable shape audit**, not an implementation of 48 original methods.

## Windows replacement DLL checks

The current independent native `pc/independent_slot0/slot0.cpp` already contains 48 declared client virtual methods and intentionally returns **0** from `VClient011::Init` after the 21-interface preflight. Unknown methods fail fast rather than pretending to implement their ABI. That behavior is preserved.

The Windows MSVC x86 loader test `pc/independent_slot0/test_slot0.cpp` now additionally verifies that **all 48 actual compiled virtual function addresses** in the loaded replacement `client.dll` belong to that same module. It also checks that the current replacement factory **does not claim** the four authentic but still-unimplemented original client-facing interfaces `VClientEntityList003`, `VClientPrediction001`, `GameClientExports001`, `ClientLeafSystem001` (each returns null and status 1). The latter is a deliberate current fail-closed expectation to be updated once fully typed, safe implementations exist. It does not imply the original game lacked those interfaces.

## Owner-private and public regression

Run the exact original file audit locally on a trusted, unchanged `client.dll`:

```powershell
py -3 research/tools/audit_sin_original_vclient011_complete_vtable.py --client "C:\Games\SiN Episodes\SE1\bin\client.dll"
```

The example path is illustrative. `--client` validates the complete binary SHA, 21-byte original registration SHA and exact arguments/call target, six-byte singleton thunk SHA, static `VClient011` object/vptr, whole 192-byte original vtable SHA, all 48 original pointer values and their `.text` containment, exact end marker, the complete original `CreateInterface` and registration-constructor body digests, and the complete original `VClient011::Init` digest. It emits only metadata.

`--self-test` constructs an invented table of only pointer DWORD values (not executable binary bytes), rejects mutations in **each of the 192 bytes**, validates explicit first/second/slot33/last indices, table size, next-data boundary, the complete 21-byte fabricated registration wrapper and its bytewise mutations, and refuses a fake original file hash.

The new original-free audit is registered in Linux decomp GitHub Actions and native Windows MSVC x86 CTest. The target Windows regression count increases from **250 to 251** if the final-head CI succeeds. All original game files stay owner-private; CI never downloads or executes them.

## Limits and next original-engine milestone

This is substantive ABI evidence and an executable Win32 replacement-module ownership regression. It is not proof that the original retail engine successfully calls `VClient011::Init` to completion; the replacement intentionally returns failure. A correct complete client still needs exact original startup/shutdown call paths, factory registrations for the other client interfaces, and later gameplay/HUD virtual methods. The engine's retail `ServerGameEnts001` physical ownership contract remains unresolved. Windows native gameplay and Android/Quest 3 startup are **not verified**.

No HL2-equivalent physics, surfaces, props or model loading is added, consistent with the user's priority override.
