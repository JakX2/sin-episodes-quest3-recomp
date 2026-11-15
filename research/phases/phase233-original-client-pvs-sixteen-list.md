# Phase 233: original client PVS sixteen-byte list append/unlink

15 November 2026. Stacked draft on Phase232 PR #287. Owner-original retail 2006 SiN Episodes PE32 DLL pair SHA256-gated:

- client.dll (3,592,192 bytes): 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll (6,049,792 bytes): 568a0086fde607345adf31d7cfb2a5bf4eefe9a86f416faa97076ddf9d97650

Neither original commercial DLL, native proprietary full function-body bytes, disassembly dumps, nor game assets are committed. Whole-original code-region SHA256 values are verified only after BOTH full original file checks match.

## Two NEW complete original native-function reviews

| Original client entry | Role | Complete native x86 bytes | Entire original method SHA256 | Return / INT3 |
| --- | --- | ---: | --- | --- |
| 0x241086F0 | Reserve/append 16-byte PVS list index | 96 | 6d86459297ef74bda6ee895e7bff44215f4895c61d0942c5ab555303e0851197 | Two C3 returns at relative +0x47/+0x5F; next function immediately follows |
| 0x24106CB0 | Unlink 16-byte PVS list index | 140 | 65dbc842dd6a24b1934b0fd24295b119e0e9c7bcf2d67fb4f775e0daaaaca7ff | Two RET 4 at relative +0x6F/+0x89; 4 subsequent INT3 bytes |

**Two new complete original native methods, 236 x86 bytes; reviewed catalog 329 -> 331 (147 server, 184 client).** Both VAs were checked as absent in prior ledger. Full-body hashes are separate from 24 sparse pinned opcode witnesses.

## Original layout and distinction from Phase232

The Phase232 methods 0x24108680/0x24106C20 manipulate EIGHT-byte auxiliary nodes. These new methods manipulate a distinct **16-byte PVS record**; the host reference deliberately keeps them separate.

Original list manager contains head WORD at this+0x0C, tail WORD at this+0x0E, active count WORD at this+0x12, capacity WORD at this+0x14. A node occupies sixteen bytes; first twelve bytes belong to other PVS map/notifier operations and are not written by these list methods; previous index is WORD[node+0x0C], next index WORD[node+0x0E]. Absent index uses 0xFFFF. Freed node uses a different marker: previous and next both equal its own index.

**Native append 0x241086F0** calls original index allocator 0x24106610 with zero argument, then calls 0x24106CB0 to unlink its returned index (if active) BEFORE appending to the tail. It writes node.next=0xFFFF, node.prev=old tail, manager.tail=index and either head=index (empty) or previous_tail.next=index. Then increments WORD active count, returns low 16 bits of index. Original reserve allocator whole body remains opaque in this phase.

**Native unlink 0x24106CB0** takes one stack index, returns via RET 4. If index >= capacity or node.prev==index, it returns unchanged. Otherwise it fixes predecessor.next or manager.head, fixes successor.prev or manager.tail, self-links the removed node (prev=next=index), then decrements WORD active count modulo 2^16.

## Four original direct native CALLs

| E8 callsite | Native target | Meaning |
| --- | --- | --- |
| 0x241086F6 | 0x24106610 | Original 16-byte index allocator, whole body not recovered |
| 0x24108700 | 0x24106CB0 | Unlink before append |
| 0x241088B2 | 0x241086F0 | Phase232 PVS notifier register invokes append |
| 0x241089A2 | 0x24106CB0 | Phase232 PVS notifier unregister invokes unlink |

All exact E8 rel32 displacements, 24 native sparse instruction witnesses, native C3/RET 4 returns and original INT3 region boundaries are authenticated in the owner-original image.

## New code and CI

- research/tools/audit_sin_original_client_pvs_sixteen_list.py: read-only original dual-DLL SHA gate and whole-method/anchor/return/call tests.
- research/tests/test_sin_original_client_pvs_sixteen_list.py: synthetic-only mutation suite. Synthetic bytes NEVER count as recovered original methods.
- include/sin_compat/original_client_pvs_sixteen_list.hpp: clean-room C++17 list, opaque first twelve payload bytes, head/tail/count, 16-bit indexed links, host bounds checking and injected allocator result.
- tests/test_original_client_pvs_sixteen_list.cpp: empty/middle/head/tail/reuse, preserved record payload, count wrap and refused invalid allocation regression.
- Linux and Win32 MSVC x86 CTest registration is included. These tests do NOT demonstrate real retail Source engine executing replacement gameplay DLLs.

Owner-only verification: python3 research/tools/audit_sin_original_client_pvs_sixteen_list.py --archive /private/bin.zip

Synthetic regression: python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_sixteen_list.py -v

Next: real 16-byte index allocator 0x24106610, eight-byte allocator 0x24106570, PVS map insertion 0x24108750, map lookup 0x24106B20, map removal 0x24108840, and original entity virtual callbacks. Then actual Windows PC gameplay DLL loading, followed by Android ARM64 and Quest 3.
