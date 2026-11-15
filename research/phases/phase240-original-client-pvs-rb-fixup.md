# Phase 240: original client PVS red-black insertion balancing and rotations

15 November 2026. Stacked draft on Phase239 PR #294. This work is grounded in the owner-supplied original retail Windows x86 **SiN Episodes: Emergence (2006)** game binaries. No proprietary whole method bytes, retail DLLs, original disassembly dumps or commercial game assets are committed.

## Mandatory full owner-original DLL provenance

| DLL | Whole file size | Complete original SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

Both entire original DLL hashes must match before the original-only auditor awards method recovery credit.

## Three NEW complete original SHA reviews

| Original client.dll entry | Function | Whole original native body size | Complete original native-code SHA256 | Original exit and alignment |
| --- | --- | ---: | --- | --- |
| **0x241067C0** | Tree left rotation | **231 bytes** | **a32bf577a10663ba6734c3a35a5324e80b3b66141b75f5e8d1a9fefd9a0789db** | RET 4 at +0xE4; 9 trailing INT3 |
| **0x24106D40** | Tree right rotation | **234 bytes** | **9aee654ed5490c3e25fea44e6961666620d7e28493f8cbff80de9dff293981b8** | RET 4 at +0xE7; 6 trailing INT3 |
| **0x24106E30** | Red-black insertion fixup | **1,482 bytes** | **f778c9c8b49e5a2f40cb79694d3c0a7191da19137edea216d456c31705775e81** | RET 4 at +0x5C7; 6 trailing INT3 |

**Reviewed catalog: 344 → 347 (server 147, client 197 → 200), +1,947 complete original native x86 code bytes.** Cryptographically reviewed full method bodies are an evidence metric, not proof of full source equivalence, a decompilation percentage or a runnable replacement gameplay DLL.

### Original x86 behavior recovered

This is a **red-black tree** storing 16-byte map records, **not** a plain unbalanced binary search tree. The reviewed original 0x24106E30 uses the embedded map root WORD at +0x10 and repeatedly examines the parent and grandparent of the newly attached node. It checks original color WORD at node+0x06, where **0 means red** and **1 means black**; the lazily initialized original nil sentinel at 0x2433FDBC is black. It handles red-uncle recoloring and black-uncle inner/outer rotations, and unconditionally paints the final root black before returning.

The original rotation functions repair the pivot, transferred middle subtree, old/new parent and root indexes in 16-byte nodes:
- **0x241067C0**: left rotation at node x; pivot y=x.right; y.left moves into x.right, then y replaces x in the parent/root and y.left=x.
- **0x24106D40**: right rotation at node y; pivot x=y.left; x.right moves into y.left, then x replaces y in the parent/root and x.right=y.
- Node layout is left child WORD+0, right child WORD+2, parent WORD+4, color WORD+6, 8-byte key +8..+15. Missing indexes use 0xFFFF.
- This is specifically insertion fixup. **Original removal/rebalancing 0x241083C0 remains unresolved.**

The native balancer inlines repeated lazy-global-sentinel checks and calls rotations using actual native thiscall + RET4 ABI. The C++17 host model replaces these redundant compiler-generated patterns with a typed nil/black sentinel. This is **behavior-based independent reconstruction**, not a binary decompilation byte-for-byte or a claim to reimplement global CRT lifetime, malformed-pointer crashes, or all compiler-generated register/register-flag effects.

### Eleven whole-original authenticated E8 rel32 call edges

| Original callsite | Original target | Context |
| --- | --- | --- |
| 0x24107137 | 0x241067C0 | Balancer invokes left rotation |
| 0x241071EF | 0x24106D40 | Balancer invokes right rotation |
| 0x24107313 | 0x24106D40 | Balancer invokes right rotation |
| 0x241073CB | 0x241067C0 | Balancer invokes left rotation |
| 0x24107724 | 0x24106E30 | Phase239 left attachment delegates to fixup |
| 0x24107733 | 0x24106E30 | Phase239 right attachment delegates to fixup |
| 0x24107741 | 0x24106E30 | Phase239 root attachment delegates to fixup |
| 0x241067CB | 0x24105F40 | Left rotation uses Phase238 node resolver |
| 0x241067D7 | 0x24105F40 | Left rotation second node resolver |
| 0x24106D4B | 0x24105F40 | Right rotation uses Phase238 node resolver |
| 0x24106D56 | 0x24105F40 | Right rotation second node resolver |

Auditor additionally verifies **19 sparse opcode witnesses**, all three original RET4 paths and 21 INT3 padding bytes after both owner-original full-DLL hashes and three entire native method hashes.

## Code, integration and regression tests

- research/tools/audit_sin_original_client_pvs_rb_fixup.py: original-only two-complete-DLL SHA gates, all three whole-native-method code SHA256s, sparse witnesses, native RET4/INT3 boundaries and 11 E8 rel32 links.
- research/tests/test_sin_original_client_pvs_rb_fixup.py: synthetic-only PE32 mutation regressions for every sparse witness, entire return opcodes, every alignment pad and all 55 bytes from the 11 E8 sites. Artificial witnesses **never count as original methods**.
- include/sin_compat/original_client_pvs_rb_fixup.hpp: independent portable C++17 left/right rotation models and red-black insertion fixup, directly implementing the Phase239 BalancingBackend callback. Explicit checks for nil, missing pivots, broken parent links and cyclic ancestry are host safety divergences from raw original x86.
- tests/test_original_client_pvs_rb_fixup.cpp: original node metadata, inverse left/right rotation preserving middle subtree, nonroot rotations, all four straight/zigzag rotation sequences, red-parent/red-uncle recoloring, **128 ascending and 192 seeded-random distinct-key insertions**, per-insertion red-black invariants, black root, red parent/child exclusions, consistent black height, BST ordering, parent pointer consistency and host invalid-state checks.
- Root CMake and native MSVC Windows x86 CTests, Phase240 research notes and +3 original-SHA-reviewed entries in research/catalog/original_dll_verified_methods.json.

The integration path now comprises recovered independent host models through Phases236–240: map insertion frontend → comparator placement → index allocation → sixteen-byte storage growth/resolver → tree attachment → original-style red-black fixup and rotations → map lookup. Host balancing is invoked **before** original insertion frontend's eight-byte key payload copy, exactly as the native attach call ordering requires.

Private retail SHA audit:

    python3 research/tools/audit_sin_original_client_pvs_rb_fixup.py --archive /private/bin.zip

Synthetic witness suite:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_rb_fixup.py -v

## Scope and next original blockers

Red-black **deletion/rebalancing 0x241083C0** remains unresolved, as do real original engine-owned object lifetimes, native comparator implementations, cross-DLL memory helpers and a genuine standalone replacement gameplay DLL loaded by the original retail Windows engine. Proxy forwarding, host unit tests and Android dlopen smoke tests are **not** a playable native recompilation. Prioritize the original Windows PC gameplay engine, then Android ARM64, then Quest 3 VR.
