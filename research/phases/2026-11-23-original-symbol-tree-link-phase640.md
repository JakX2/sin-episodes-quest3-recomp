# Phase640: original SiN symbol tree linker and direct slot accessor

## Authenticated retail evidence

The unchanged owner's original 2006 server.dll and client.dll have SHA256 values 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 and 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

**Two complete original x86 methods, 120 executable bytes:**

| Method | Complete bytes | Entire method SHA256 |
|---|---:|---|
| 0x22281F30 tree insertion and rebalance dispatch | 106 | 42db6c305a6dda67117bfd276134dd609f397af1de0e4e79e7a9d98ae5dcedc5 |
| 0x22280D30 direct 16-byte slot accessor | 14 | d24e0ecdd1504637ccce7f2bc4cbcb165a1773826729ffa8f96748273dadd406 |

The owner auditor checks both original DLL identities, whole method hashes, PE32 .text ownership, 4 authentic rel32 call edges, 33 opcode witnesses, and original post-function INT3 padding. No original game code bytes or DLLs are redistributed.

## Complete original 0x22281F30 write order

Original __thiscall has ECX=table and three stack DWORDs: new slot index, parent index and side flag. RET 0x0C.

1. Read [table+4]; compute slot as array+16*uint16(index).
2. Write four 16-bit fields, in this exact order: [slot+4]=uint16(parent); [slot+2]=0xFFFF; [slot+0]=0xFFFF; [slot+6]=0.
3. If uint16(parent)==0xFFFF, write root WORD [table+0x10]=uint16(index). Otherwise **re-read** [table+4] after the four initial writes to compute parent slot. If the **low byte** of side is nonzero, write [parent_slot+0]=uint16(index); if zero, write [parent_slot+2]=uint16(index).
4. In each branch, call original **0x22281840** with ECX=table and **full original 32-bit index** (ESI), not the truncated slot index. This is a nontrivial, still unrecovered tree-rebalancing function, not a no-op.

The source reference intentionally refuses unsafe guest addresses, exceptions, and reentrance. Earlier writes remain committed when a later host step refuses. The original machine code has no such explicit memory checks.

## Original 0x22280D30 accessor

Return [table+4] + 16*uint16(index) with RET 4. **Does not special-case 0xFFFF.** The distinct 0x22280D40 sentinel-aware accessor remains unrecovered.

## Native call edges

- 0x222822A6 -> 0x22281F30 from Phase639 registrar.
- 0x22281F74 -> 0x22281840 for nonzero side.
- 0x22281F83 -> 0x22281840 for zero side.
- 0x22281F91 -> 0x22281840 for root.

## Tests and proof boundaries

Compiled C++17 tests cover root, child field +0 and +2, full-DWORD-vs-WORD index, BYTE branch vs DWORD, array re-read after four writes, simulated relocation, sequential partial-write refusals, rebalance failure, reentrance, direct slot accessor including 0xFFFF and a synthetic composition of **actual compiled Phase638 Forwarder -> Phase639 RegisterSlot/AcquireSlot -> Phase640 LinkNode**. The synthetic host's tree rebalance callback is not original engine behavior. All symbols/IDs/memory are invented fixtures.

Owner-side audit command:

python research/tools/audit_original_symbol_tree_link_phase640.py --server PATH_TO_ORIGINAL_SERVER_DLL --client PATH_TO_ORIGINAL_CLIENT_DLL

Public --self-test mutates invented bytes only. No actual engine-process invocation or playable Windows/Android/Quest 3 gameplay is claimed. No unrelated Half-Life 2-equivalent physics/materials/surfaces/models/props/rendering work.

**Next original dependency:** 0x22281840, whose entry checks the root at table+0x10 and accesses global scratch at 0x22589138..0x22589140. It must be separately reconstructed before declaring actual tree rebalancing complete.
