# Phase 232: original client PVS notifier and auxiliary-list helpers

15 November 2026. STACKED DRAFT on Phase231 PR #286. Research is gated by BOTH complete owner-original SiN Episodes: Emergence 2006 Windows PE32 files: client.dll SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d (3,592,192 bytes); server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 (6,049,792 bytes).

No original commercial DLL, game asset, full original x86 method byte sequence or disassembly dump is published. SHA values below refer to the complete native original function regions, not synthetic data.

## Four newly SHA-reviewed whole-original client methods

| Entry VA | Native purpose | Original code bytes | Complete original native SHA256 | RET 4 exits | INT3 pad |
| --- | --- | ---: | --- | ---: | ---: |
| 0x24108680 | Auxiliary 8-byte list reserve/append | 105 | e6e8903a2ca41cf82c90d08ec01862893525edbb459db00db9de2ee6f3b05175 | 1 | 7 |
| 0x24106C20 | Auxiliary 8-byte list unlink | 137 | 17cb2bddf5ae89c032798869d9063ba42412e9a018f176e38928409529a3e743 | 2 | 7 |
| 0x24108880 | PVS notifier map/16-byte list registration | 132 | 30770d1648b0a0d71d62752ee3a9d108ac91bbf9cdccca479ab1a9a644c4796e | 1 | 12 |
| 0x24108910 | PVS notifier map/16-byte list unregister | 190 | 51ce5be9512ec947d5fdfb5b87de3314d651657195ae12717ca7fe418cfdf394 | 2 | 2 |

Total: 4 new complete original native methods, 564 x86 bytes. These VAs were not in Phase231's 325-method ledger. New ledger target is **329 complete-function SHA reviews (server 147, client 182)**.

## Two distinct native list organizations

The auxiliary manager handled by 0x24106C20 and 0x24108680 uses 8-byte records. DWORD this+0 points at backing nodes; WORD this+0x0C is head, WORD this+0x0E is tail, WORD this+0x12 is active count, WORD this+0x14 is capacity. Each node has a DWORD payload at offset +0, previous WORD at +4 and next WORD at +6. Absent index sentinel is 0xFFFF.

Native 0x24106C20 receives a 16-bit index, ignores out-of-capacity indices and self-linked free nodes (node.prev == its own index), unlinks active nodes from both neighboring links or head/tail, self-links the removed node and decrements active count with 16-bit wrap.

Native 0x24108680 calls 0x24106570 (unreviewed index allocator), unconditionally calls the original 0x24106C20 to unlink the returned index if active, appends the index to the tail, increments active count, copies one caller-provided DWORD into the node, and returns the index. The C++17 host model provides a bounded vector and injected allocator index; it does NOT reproduce original heap capacity/growth.

The related PVS register/unregister methods operate additional **16-byte** manager records at parent+0x18044 and parent+0x18060. These are separate from the eight-byte auxiliary data structure. The host models their outer event sequence only. It does not conflate the two structures or claim complete PVS map reconstruction.

## Native PVS registration and removal

0x24108880 calls an entity virtual at vtable byte offset +0x14. A null result exits. Otherwise it calls notifier virtual +0x48; a null identity exits. The active branch calls 0x241086F0 to acquire a 16-byte node, writes the identity and notifier address, clears byte+8 and stores the node index into WORD+0x0A. A nonnull parent+0x1807C pointer is copied into original absolute global 0x2433FDB8. It then calls 0x24108750 to insert the map association.

0x24108910 repeats the +0x14 and +0x48 virtual gates. It optionally writes the same original global pointer. Then calls 0x24106B20 for map lookup and 0x24105F40 to inspect its node. If node.first WORD equals the looked-up index it prints original native warning text:

    PVS notifier not in m_PVSNotifierMap

The original literal is at .rdata VA 0x242973C4 including trailing newline and zero terminator. It calls tier0.dll!Warning through IAT 0x242761E0, NOT tier0.dll!Msg. The original warning PUSH immediate and imported-call IAT word both have original PE32 HIGHLOW relocations.

On success the native function reads a 16-byte node's WORD at offset +0x0C, calls 0x24106CB0 to unlink a 16-byte node, repairs list links, and calls 0x24108840 to remove the map item. The complete nested map routines are still unresolved. The C++17 host models only the call/event order, optional global write and Warning vs normal branches.

## Eight original direct nested CALL edges

| Call instruction | Native target |
| --- | --- |
| 0x24108686 | 0x24106570 |
| 0x24108690 | 0x24106C20 |
| 0x241088B2 | 0x241086F0 |
| 0x241088F6 | 0x24108750 |
| 0x2410895C | 0x24106B20 |
| 0x24108966 | 0x24105F40 |
| 0x241089A2 | 0x24106CB0 |
| 0x241089BB | 0x24108840 |

Both absolute original PVS-global store instructions at 0x241088EA and 0x2410894E have HIGHLOW relocations on their 32-bit operands. All eight E8 rel32 CALLs, 34 sparse opcode witnesses, original return/padding bounds, the exact original warning string and Warning imported-call operands are rechecked by source-grounded Python verification.

## Committed source, tests and limits

- research/tools/audit_sin_original_client_pvs_aux_helpers.py: read-only original dual-full-DLL SHA gate, four entire original native method hashes, exact native RET/INT3, opcode, CALL, warning string/IAT and PE relocations.
- research/tests/test_sin_original_client_pvs_aux_helpers.py: synthetic-only mutation suite for evidence. Synthetic bytes NEVER count as original method recoveries.
- include/sin_compat/original_client_pvs_aux_helpers.hpp: portable C++17 bounded model of 8-byte active-list nodes, plus injected callbacks for PVS notifier native call ordering.
- tests/test_original_client_pvs_aux_helpers.cpp: verifies head/middle/tail unlink, self-linked free markers, reusing active slots, list count consistency, PVS early exits, global write, Warning branch, and native call event ordering.
- Root Linux and native Win32 MSVC x86 CTest targets are registered. These host references do not run game-owned original C++ objects or original engine loaders.

Original owner-private audit: python3 research/tools/audit_sin_original_client_pvs_aux_helpers.py --archive /owner-private/bin.zip

Original-free tests: python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_aux_helpers.py -v

Next source-driven work: original index allocator 0x24106570, 16-byte manager allocate 0x241086F0 / map link 0x24108750, map lookup 0x24106B20 / 0x24105F40, 16-byte unlink 0x24106CB0 / map remove 0x24108840, native virtual entity implementations, and actual retail-engine Windows gameplay module loading.

Windows PC first, Android ARM64 second, Quest 3 VR third. Reviewed original functions do not equal complete game decompilation or playability.
