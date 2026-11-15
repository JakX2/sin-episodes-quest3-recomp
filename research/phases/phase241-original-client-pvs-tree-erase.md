# Phase 241: original 2006 client PVS red-black tree removal frontend

15 November 2026. This work is **stacked on Phase240 PR #295**. Owner-original SiN Episodes: Emergence (2006) Windows PE32 game DLLs are used to authenticate entire native methods. Neither retail DLL bytes nor complete proprietary x86 code bodies or disassemblies are added to GitHub.

## Exact entire owner-original DLL provenance gates

| Owner-original PE32 DLL | Length | Full file SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 bytes | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 bytes | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

## One NEW authenticated whole-original native method

| Original client entry | Function | Complete native x86 body length | Complete original code SHA256 | Return / trailing pad |
| --- | --- | ---: | --- | --- |
| **0x241083C0** | PVS map red-black deletion, successor transplant and fixup dispatch | **693 bytes** | **887ac931d635a7b9e913bb3c68128628eb40a893b626907230f872df0304419d** | **RET 4** at +0x2B2; **11 INT3** at 0x24108675..0x2410867F |

**Reviewed original-method ledger 347 → 348 (server 147; client 200 → 201), +693 full native x86 code bytes.** These are SHA-reviewed complete original method bodies, not complete reconstructed game classes or a game-ready DLL. The original-only auditor requires both complete original retail DLL hashes, the whole 693-byte SHA256, **21 independent sparse x86 witnesses**, native RET4 and 11 trailing INT3, and **27 independently exact original E8 rel32 callsites** (24 original 0x24105F40 map-node resolver calls inside the method, one final 0x24107750 delete-fixup call, and two calls to this method from Phase236 release frontends).

## Native original deletion behavior

The method is x86 thiscall, manager in ECX and one WORD node index argument on the stack, returning with **RET 4**. Map node fields: left WORD+0, right WORD+2, parent WORD+4, color WORD+6 where **0=red, 1=black**. Nodes have 16-byte stride, manager storage pointer DWORD+4, root WORD+0x10, missing-index sentinel **0xFFFF**.

- If the requested node is 0xFFFF, the original exits without modifying the tree.
- If the logical target has at most one child, the physically removed node is the target. Otherwise it selects the in-order successor by following the logical target's right child then descending left links.
- It chooses the physically removed node's non-sentinel child as the replacement (if any), updates that child's parent, and transplants the physically removed node out of its former parent/root.
- If a different successor was selected, it moves the successor into the logical target's original tree position, copies the target's tree links and color, and repairs both child parent pointers. Crucially, it **does not copy the target key into the successor**. The original Phase236 release frontend then returns the logically requested **original target index** to the free list, leaving the successor's index in the tree.
- It retains the **physically removed node's color** before transplant. At native **0x24108657..0x2410866D** it calls original **0x24107750** with the selected replacement index only if **replacement != 0xFFFF AND removed_color == 1 (black)**. If replacement is nil, original code skips this call, even for a black leaf. The host reconstruction mirrors this observed dispatch guard and does NOT manufacture a nil-fixup branch.
- The original function does not decrement the manager's live count or set free-list links. Phase236 native release frontend does both afterward. This distinction is preserved by implementing Phase236 DeletionBackend with an independent NativeTreeErase rather than double-freeing indices.

**Critical unresolved dependency:** Original **0x24107750** is a separate, complex post-delete rebalancer and is not recovered in Phase241. The new host code injects an explicit DeleteBalancingBackend callback for that invocation. A no-op test spy does not imply the game would preserve all red-black invariants without the real balancing implementation. Malformed indices, missing parents and cyclic successor chains are rejected safely in the host; the original x86 code does not guarantee such checks.

## Native exact E8 CALL provenance

All **24** direct in-method calls to original node resolver **0x24105F40** are pinned individually at:

    0x241083DA 0x241083EB 0x241083FD 0x241084AA
    0x241084B7 0x241084C5 0x241084E1 0x24108501
    0x2410850D 0x2410851A 0x2410852F 0x2410854C
    0x24108565 0x2410857F 0x24108593 0x241085AB
    0x241085B7 0x241085C4 0x241085D9 0x241085F6
    0x24108605 0x2410861B 0x2410862B 0x24108642

Three more exact native direct E8 edges:

| Original site | Resolved original target | Meaning |
| --- | --- | --- |
| 0x24108669 | **0x24107750** | conditional black-node, nonnil-replacement delete fixup |
| 0x241087D0 | **0x241083C0** | Phase236 original release frontend variant A |
| 0x24108850 | **0x241083C0** | Phase236 original release frontend variant B |

## Added independent host reference and tests

- research/tools/audit_sin_original_client_pvs_tree_erase.py: dual entire-owner-original-DLL SHA gates, one entire 693-byte native method SHA, original section and RET4 boundaries, eleven INT3 bytes, 21 opcode witnesses, all 27 exact original CALL targets.
- research/tests/test_sin_original_client_pvs_tree_erase.py: synthetic-only PE mutation regressions for every sparse opcode, return operand, INT3 pad, every direct-call E8 byte, section bounds and PE image base. **Synthetic fixtures cannot earn any original method credit.**
- include/sin_compat/original_client_pvs_tree_erase.hpp: independent C++17 PVS successor search, transplants, parent/root rewrites, two-child successor migration and original conditional rebalancer dispatch. The original 0x24107750 implementation remains an opaque injected callback.
- tests/test_original_client_pvs_tree_erase.cpp: sentinel no-op, black single-node root removal, black root with one red child (callback required), direct right-successor transplant (avoid self-cycle), deeper successor with nonnil child (black/red selected-color callback gate), malformed graph guards, and **Phase236–240 insert→lookup→red-leaf erase→free-list reuse→reinsert** integration through the Phase236 release API.
- Root Linux C++17 CTest and genuine native MSVC Win32 x86 host model CTest; original SHA catalog updated from 347 to 348.

Owner original-binary audit:

    python3 research/tools/audit_sin_original_client_pvs_tree_erase.py --archive /private/bin.zip

Synthetic-only evidence tests:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_tree_erase.py -v

## What is not recovered yet

The distinct **0x24107750 post-delete red-black balancing body**, actual game engine-owned PVS map/comparator objects, native allocator internals, dynamic game entities and runnable replacement original Windows gameplay DLLs. The original Python/C++ host tests, proxy forwarding and Android ABI smoke are not complete gameplay emulation or proof of a bootable standalone port.

Platform order remains **Windows PC original retail game first → Android ARM64 → Quest 3 VR**.
