# Phase 234: original client PVS index allocator methods (8 and 16 bytes)

15 November 2026. Stacked draft on Phase233 PR #288.

Owner-original **SiN Episodes: Emergence (2006)** PE32 retail binaries, authenticated **before** method credit:

- client.dll, 3,592,192 bytes: SHA256 **6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d**
- server.dll, 6,049,792 bytes: SHA256 **568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650**

No original commercial binary, original complete body bytes, original function disassembly dump or game assets are committed.

## Two newly authenticated complete original client methods

| Native entry | Original method | Whole code length | Complete original code SHA256 |
| --- | --- | ---: | --- |
| **0x24106570** | Auxiliary 8-byte record index allocator | **154 bytes** | **3f972fba67fca64b208624a577230cec867af88128418bce19a6df633314097a** |
| **0x24106610** | PVS 16-byte record index allocator | **154 bytes** | **c03f2c59e9484db8a76c31258d43d12272493ec83813bcac23f5ef179a3e594e** |

Both methods contain two actual **RET 4** paths at method-relative offsets **+0x77** and **+0x97**, with **six native INT3 bytes** after the complete 154-byte body. Each has 12 independent sparse opcode witnesses (24 total), separately checked from the complete whole-code SHA256.

**Reviewed ledger: 331 → 333; server 147; client 184 → 186. New original native bytes: 308.** This is not DLL percentage completeness and not an executable game-module reimplementation.

## Original control flow and layout

Both methods use **ECX/thiscall**, one stack argument interpreted by a byte comparison, and **RET 4**. Both operate on a manager with:

- **+0x00 DWORD**: record-storage pointer; **+0x04 DWORD**: capacity
- **+0x10 WORD**: free-chain head, **0xFFFF** when absent
- **+0x14 WORD**: next never-before-allocated index
- **+0x18 DWORD**: cached record-storage pointer, rewritten on each successful return

The **eight-byte auxiliary record** has link WORD offsets **+0x04 (previous)** and **+0x06 (next)**. The distinct **16-byte PVS record** has link WORD offsets **+0x0C (previous)** and **+0x0E (next)**. The leading four or twelve payload bytes belong to other helper functions.

When the free-chain head is not **0xFFFF**, original code returns that head index and advances the free head from the selected record's next WORD. When the free-chain is empty, it uses the next-unused WORD, first requesting capacity growth with argument **1** when next-unused equals capacity. It then increments next-unused using native 16-bit arithmetic.

For a false/zero flag, the selected record's two link WORDs are both set to its own index. For nonzero, both are set to **0xFFFF**. Each path updates cached storage pointer at manager+0x18. A virgin index of **0xFFFF** reaches the native error path: literal **"CUtlLinkedList overflow!\n"** at **0x242D7E40**, via original **tier0.dll!Error** imported at IAT **0x242761FC**. This is not tier0 Warning or Msg.

## Direct original x86 E8 rel32 calls

| Native callsite | Target | Meaning |
| --- | --- | --- |
| 0x2410658A | **0x24106010** | Auxiliary eight-byte growth helper; complete body still unreviewed |
| 0x2410662A | **0x24106090** | PVS sixteen-byte growth helper; complete body still unreviewed |
| 0x24108686 | **0x24106570** | Phase232 auxiliary append uses allocator |
| 0x241086F6 | **0x24106610** | Phase233 PVS append uses allocator |

The source-gated verifier checks all four exact rel32 calls as well as the whole-method SHA256s, 24 opcode witnesses, RET 4 endings, INT3 boundaries, error import callsite and error string.

## Source and tests

- research/tools/audit_sin_original_client_pvs_index_allocators.py: original dual-DLL SHA gate, whole-method SHA verification, sparse witnesses and call edges
- research/tests/test_sin_original_client_pvs_index_allocators.py: artificial PE mutation tests; never confer original-recovery credit
- include/sin_compat/original_client_pvs_index_allocators.hpp: clean-room C++17 allocator reference with distinct 8/16-byte layouts, injected growth and bounds checks
- tests/test_original_client_pvs_index_allocators.cpp: virgin allocations, free-chain reuse, both flag choices, payload preservation, growth and 16-bit sentinel cases
- Root Linux CTest and native Windows MSVC **Win32 x86** CTest registrations

Owner-original verification:

    python3 research/tools/audit_sin_original_client_pvs_index_allocators.py --archive /path/to/private/bin.zip

Synthetic mutation regression:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_index_allocators.py -v

**Boundary:** The portable C++ model deliberately refuses invalid indices or missing growth callbacks instead of reproducing unsafe native memory operations. It does not instantiate original engine objects, implement the original tier0 allocator, recover either growth-helper body, or prove original Windows game loading reconstructed DLLs. Next evidence targets: original growth helpers **0x24106010/0x24106090**, PVS map insertion **0x24108750**, and map lookup/removal **0x24106B20/0x24108840**. Windows PC gameplay integration remains the first platform gate; Android ARM64 second, Quest 3 last.
