# Phase 230: original client entity-list EHandle resolution and dispatch

15 November 2026. **STACKED DRAFT on Phase229 PR #284.** Inspected the owner's original retail 2006 Windows PE32 client.dll and server.dll, unpacked from private bin.zip. Original complete DLL provenance:

- Original client.dll, 3,592,192 bytes, SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
- Original server.dll, 6,049,792 bytes, SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.

The read-only auditor requires **both complete original DLL SHA256 matches** before crediting new recovered methods. No original commercial DLL, full raw proprietary native method body, disassembly dump, or game resource is committed.

## Seven newly SHA-reviewed complete original x86 native methods

The Phase228 investigation established that the genuine client VClientEntityList003 interface sits at **containing object 0x2433FDC8 + 0x10014 = 0x2434FDDC**. Phase229 authenticated slots 0, 5, 6, 7, and 8 of its final embedded-interface vtable at 0x24297390. This phase recovers the missing slots **1, 2, 3, and 4**, plus three original parent-entity-list helpers.

| Original client.dll native method | Role | Complete x86 bytes | SHA256 of entire original native body | INT3 pad |
| --- | --- | ---: | --- | ---: |
| 0x24105AF0 | VClientEntityList003 slot 3, signed-index dispatch, entity virtual +0x18 | 36 | 010be7e7997efd85df59f59260fba5498a35eea7f7d00be9592578987ee80506 | 12 |
| 0x24105B20 | Parent object recompute highest occupied entity slot | 48 | c0ca3b059853bb32b91236c0dcdecc0f47d98f1a7157657368071418c704d29a | 0 |
| 0x24105B50 | Parent object signed-index entity virtual +0x1C | 34 | 8c6620646e8ab5c7aa066bd27631d9dc448144ee40543c27470c4f14c7eaf68c | 14 |
| 0x24105B80 | Parent object signed-index entity virtual +0x0C | 34 | eba653c74bdaa08a8fa0d4210f0747e6a49f902711e86965d6e0ed37801cbb79 | 14 |
| 0x24105BB0 | VClientEntityList003 slot 1, handle resolve then entity virtual +0x10 | 33 | f7b0f8459b8668297a723f983ccb184b7db31427a00e9ab4d28a16493bbd2c7b | 15 |
| 0x24105BE0 | VClientEntityList003 slot 4, handle resolve then entity virtual +0x18 | 33 | b905524f103a461177b4c02f4541100c4425c7663b68d8783d7f4870466da56d | 15 |
| 0x241074D0 | VClientEntityList003 slot 2, native EHandle slot+serial resolution | 44 | 4ab8dfccb518e758d2af09b81bed3fcabe80219b0890d885392cc42ac7f55520 | 4 |

**Seven whole-original native function SHA reviews, 262 x86 instruction bytes.** The original native return boundaries, including every success/failure RET 4 path and both normal RET C3 branches in the scan helper, are verified separately. The original code padding and 47 sparse complete-opcode witnesses are checked, with an independent SHA256 over each full method body.

The following four original vtable DWORD pointer entries were independently checked against their **exact native function addresses**, and each has an original PE32 HIGHLOW relocation record:

| Original VClientEntityList003 vtable slot | Native method | Function |
| --- | --- | --- |
| 1 | 0x24105BB0 | EHandle → entity virtual +0x10 |
| 2 | 0x241074D0 | EHandle → resolved entity pointer |
| 3 | 0x24105AF0 | signed index → entity virtual +0x18 |
| 4 | 0x24105BE0 | EHandle → entity virtual +0x18 |

Three **actual native direct game-code CALL rel32** sites further connect the non-interface helpers to real client gameplay logic, beyond neighboring code similarity:

- 0x24108AA3 calls highest-slot recomputation 0x24105B20 after an entity-list deletion/slot accounting branch.
- 0x2406F3FD calls parent indexed virtual +0x1C helper 0x24105B50.
- 0x24132B54 independently calls that same 0x24105B50 helper.

## Handle resolution behavior and the crucial index-count distinction

Original native slot 2 at 0x241074D0 uses DWORD EHandle from the first stack argument. If handle is **0xFFFFFFFF**, it immediately returns null. Otherwise:

- Original unsigned slot index: handle & 0xFFF, a **12-bit range [0, 4095]**.
- Original serial: handle >> 12, upper 20 bits.
- Native table-entry address: **embedded interface this + ((index - 0x1001) * 16)**, computed using original i386 wrapping arithmetic.
- Because the embedded interface lies at *containing object +0x10014*, this equals **containing object +4 + index*16**, for the original object layout.
- Compares DWORD[slot+4] against handle serial. If equal, returns DWORD[slot+0] as original native entity pointer; stale serial returns null.

No separate index 0..2047 check occurs inside this resolver. It can address an index as high as 4095 under the 12-bit mask. The special all-ones sentinel bypasses all table access. The clean-room host refuses invalid byte-buffer arithmetic rather than emulating out-of-range i386 crashes or pointer wrap.

**Different algorithm:** the original parent helper at 0x24105B20 writes 0xFFFFFFFF to DWORD[parent+0x10024], then loops **2047 down to 0** checking DWORD[parent+4+index*16] for a nonzero entity pointer. It writes the highest occupied index or retains -1. This helper's 2048-entry scan range is NOT the full 4096-slot range addressable by the native EHandle resolver. C++ tests explicitly exercise handle index 4095 and scan index 2047 independently.

## Native virtual dispatch behavior

- Original interface **slot 1** receives EHandle, calls its own virtual slot 2 (which consumes the stack handle), and if the returned entity pointer is nonzero invokes that entity's vtable **byte offset +0x10**. On failed resolution it returns zero.
- Original interface **slot 4** does the same, but invokes the entity's vtable **byte offset +0x18**.
- Original interface **slot 3** receives a **signed 32-bit index, not an EHandle**, returns zero for negative index; otherwise directly loads the pointer from the corresponding 16-byte slot without serial validation and calls entity vtable **byte offset +0x18** if nonzero.
- Original parent helper at **0x24105B50** receives signed index, loads DWORD[parent+4+index*16] for nonnegative index, and calls entity vtable **byte offset +0x1C** if the pointer is nonzero. Original client gameplay call sites are pinned.
- Original parent helper at **0x24105B80** uses the same parent-pointer indexing, but entity virtual byte offset **+0x0C**.

The invoked **entity virtual-method bodies are NOT recovered in this phase**. The portable reference exposes these calls through an explicit injected callback receiving (entity pointer identity token, original vtable byte offset). No fabricated C++ entity class or actual original runtime pointer is dereferenced.

## Committed source and original-free regression

- research/tools/audit_sin_original_client_entitylist_handle_dispatch.py: whole-original dual-DLL SHA gate; seven complete native code SHA256s, 47 opcode witnesses, all original native RET 4/C3 branches and INT3 pads; four selected embedded vtable entries and original HIGHLOW relocations; three exact actual game-code CALL rel32 sites; and the original index/serial/stride constants.
- research/tests/test_sin_original_client_entitylist_handle_dispatch.py: synthetic-only mutation suite checking all opcode witnesses, native returns and INT3, vtable slots and relocations, game CALLs, base/section. Artificial fixture bytes cannot be credited as whole-original native reviews.
- include/sin_compat/original_client_entitylist_handle_dispatch.hpp: independent portable C++17 byte-buffer model of slot+serial resolution, virtual callback routing, signed-index dispatch, parent index helpers, and 2048-slot highest-live scan. Uses pointer identity tokens and host bounds/overflow guards only.
- tests/test_original_client_entitylist_handle_dispatch.cpp: stale/valid/all-ones/null entity handles, two different raw handles resolving to identical entity pointer, full 12-bit index 4095, signed negative indexing, four virtual call offsets, no callback on null, helper lookup semantics, high-water scan index 2047 and empty state, host-only malformed buffer refusal.
- Root Linux CTest and native Win32 MSVC x86 CTest registrations test the independent reference. These tests do NOT prove rebuilt gameplay DLLs load inside the original retail game.

Owner-private original verifier:

    python3 research/tools/audit_sin_original_client_entitylist_handle_dispatch.py --archive /private/bin.zip

Original-free synthetic mutation suite:

    python3 -m unittest discover -s research/tests -p 'test_sin_original_client_entitylist_handle_dispatch.py' -v

## Scope and remaining work

Original individually complete SHA-reviewed method ledger **315 → 322**, now **147 server / 175 client**. The previously examined final VClientEntityList003 vtable's slots 0 through 8 have their original target method bodies SHA-reviewed across Phases229 and230. That is a strictly local nine-slot interface region, not all entries in the complete entity-list or parent-object vtables.

Unresolved: original native callee bodies of all independently dispatched entity methods, complete engine-owned entity lifecycle and class ABIs, CRT startup order, and real Windows retail Source engine loading replacement gameplay client/server DLLs. The project priority remains **Windows PC first, Android ARM64 second, Quest 3 VR last**.
