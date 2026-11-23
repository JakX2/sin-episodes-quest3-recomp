# Phase639: Original SiN symbol-slot registration and acquisition

## Retail evidence

Original owner-supplied SiN Episodes 2006 PE32 DLLs verified:
- server.dll SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Two complete original native methods, **200 executable bytes**:

| Method | Bytes | Complete original SHA256 |
|---|---:|---|
| 0x22282270 symbol-slot registrar | 102 | 7bf859a991bd9008ccff924d24c8aabb677a8ce8a2452cfeaf61c5b51a31e014 |
| 0x222810E0 slot acquire/reuse | 98 | 0e31663049d5343559829cd95e2a8ab56ee5d5509d9393bedb615d26cd783e05 |

Six verified original rel32 edges: 0x2228228B -> 0x22280FB0; 0x22282292 -> 0x222810E0; 0x222822A6 -> 0x22281F30; 0x222810FC -> 0x22280DD0; 0x22281113 -> 0x22280D40; 0x222823A0 -> 0x22282270.

## Registration at 0x22282270

Original __thiscall ECX=table; first stack argument points to original 8-byte symbol record, second is a DWORD flag. Returns index in AX; RET 8.

First, preserve the record pointer in EBX; call 0x22280FB0 with pointer to the record, **addresses of both incoming stack argument words**, and the flag. The helper may rewrite either stack argument. The registrar calls 0x222810E0 to acquire an index, then re-reads the mutated argument words and calls 0x22281F30 to link the slot. The link helper remains an explicit host callback, NOT an assumed no-op.

**After linking**, the native function reads current [table+4], increments the **WORD [table+0x12]** (wraparound), computes slot_array+16*uint16(index)+8, and copies WORD [original_record] into slot+8 then DWORD [original_record+4] into slot+0xC. Earlier writes are not rolled back on later faults.

## Slot acquisition at 0x222810E0

- A free-head WORD at table+0x14 different from FFFF selects reuse. The original 0x22280D40 resolves the slot and its next-free WORD from +2. That WORD replaces the head.
- Otherwise compare next unused WORD at table+0x16 with capacity DWORD at table+8. If equal, call original 0x22280DD0 (ECX=table+4, argument 1) to grow. Re-read the next index, return its prior value, increment the counter **as a WORD**.
- Then compute current [table+4] + 16*handle + 8, write WORD FFFF, **re-read** table+4 and cache in table+0x18. Original code has no safe memory checks; the C++17 host reference declines invalid addresses without undoing earlier writes.

## Reconstructed host model, tests, and limits

The new AcquireSlot and RegisterSlot C++17 references preserve all source-ordered state changes and the native widths. Compiled tests cover new index, reuse, growth relocation, WORD wraparound, mutable arguments, after-link pointer relocation, partial writes, host exceptions/refusals and reentrance. An integration test invokes the real compiled Phase638 Forwarder and its allocation hook, which calls the compiled Phase639 code. Test memory, symbols, IDs and helper behavior are **fabricated**, not retail gameplay observations.

The original 0x22280FB0 enumeration (255 B), 0x22281F30 link/fixup (106 B), 0x22280D40 free-slot reader, 0x22280DD0 array-growth helper are **NOT** reconstructed here. Their side effects remain mandatory host callbacks. The original DLLs are not redistributed. No original engine in-game DLL execution or playable Windows/Android/Quest 3 gameplay is claimed. No generic Half-Life 2 physics/surfaces/models/props/materials/rendering work.

Owner audit: python research/tools/audit_original_symbol_slot_allocation_phase639.py --server PATH_TO_ORIGINAL_SERVER_DLL --client PATH_TO_ORIGINAL_CLIENT_DLL. It verifies both complete method SHA256s, six exact original call edges, **61 opcode witnesses**, source section ownership and the two retail DLL SHA256s. Public --self-test uses invented byte mutations.

Next priorities: reconstruct original 0x22281F30 link/fixup and its downstream 0x22281840 helper; then original 0x22280FB0 enumeration with argument rewrites.
