# Phase 236: original client PVS comparator-tree map frontends

15 November 2026. Stacked draft on Phase235 PR #290. Original owner's SiN Episodes: Emergence (2006) retail Windows x86 game DLLs, not an HL2 or SourceVR surrogate.

**Whole owner-original provenance SHA256 requirements (before native method review credit):**
- client.dll (3,592,192 bytes): 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll (6,049,792 bytes): 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

Do not publish commercial original DLLs, native method-body bytes, full original disassemblies or game assets.

## Four NEW complete original native code SHA reviews

| Original client.dll VA | Function | Whole native bytes | Whole original code SHA256 | Original returns and INT3 |
| --- | --- | ---: | --- | --- |
| **0x24108750** | PVS map insertion frontend | **100** | **3900e2d517db2f4b51ec4166f966baadda6bf68135f01e797df1eb712cf09342** | RET 8 at +0x61, 12 trailing INT3 |
| **0x24106B20** | Comparator binary-tree lookup | **251** | **ad3064ab01086b83eaff75118e1769da0beeabe3439df7f677d7611eb26e59c4** | RET 8 at +0xEF/+0xF8, 5 trailing INT3 |
| **0x241087C0** | Map index release A | **61** | **6de2e1aacee40a88f1d14f69387c17d79b44493ee3c0e8ea2dc67c7c1b28ec77** | RET 4 at +0x3A, 3 trailing INT3 |
| **0x24108840** | Map index release B | **61** | **46609fee9369fed12e4ee0765eacd3b31a98855929d5a94dc7b03bb1bd9d4a01** | RET 4 at +0x3A, 3 trailing INT3 |

**Reviewed catalog 335 → 339: server 147, client 188 → 192. Four separately located and whole-original-SHA-reviewed native functions, 473 original x86 code bytes.** The two release routines have the same observed control flow but distinct original VAs and whole-code SHA fingerprints. They are separate native entries, not proof of two different gameplay systems.

## Map layout and native frontend behavior

Original map manager:
- **+0x00** DWORD comparator function pointer. Lookup dispatches an indirect CALL DWORD PTR [EDI] twice, at **0x24106B53** and **0x24106BB2**.
- **+0x04** DWORD pointer to map records, each **16 bytes**.
- **+0x10** WORD root index, with **0xFFFF** absent.
- **+0x12** WORD live-count, updated by insertion and release; native 16-bit overflow/underflow wraps.
- **+0x14** WORD free-list head, **0xFFFF** if no recycled index.

Each 16-byte map record has left-child WORD at **+0x00**, right-child WORD at **+0x02**, other tree bookkeeping at **+0x04..+0x07**, and **8 bytes of key data at +0x08..+0x0F**. This map's sixteen-byte records are distinct from the Phase233 sixteen-byte **PVS notifier active-list** records. Their word offsets and record ownership must not be confused.

**Insert 0x24108750:** original calls unresolved insertion-location search **0x24106440**, original map index allocator **0x24106700**, and original tree attach helper **0x241076E0**, in that order. It then increments manager+0x12 WORD and copies exactly two DWORDs (8 bytes) from the caller's key into the allocated record at +0x08; returns WORD index in AX with **RET 8**. The tree helper is opaque; the host reference injects callbacks and does not invent the original tree balancing algorithm.

**Lookup 0x24106B20:** takes an 8-byte caller key through the native pointer interface and starts at root manager+0x10. For each current node, calls comparator(key, node.payload); if true follows the node's +0x00 left child. Otherwise calls comparator(node.payload, key); if true follows +0x02 right child. If both comparisons are false, returns the current 16-bit index. An absent root or exhausted branch returns 0xFFFF. The original has a lazy global static nil/sentinel state at 0x2433FDBC, initialized through flag 0x2433FDC4 where relevant. The independent host lookup models the sentinel by absent-index value and does **not** fabricate global CRT lifecycle.

**Release 0x241087C0 and 0x24108840:** both return immediately on index 0xFFFF. Otherwise call original opaque tree deletion **0x241083C0**; then write node.left=index and node.right=previous free-list head, decrement manager live-count WORD, set manager free-list head=index and return using **RET 4**. The native code does not check count underflow. The host adds bounds guards to avoid undefined memory access, but preserves WORD wrap arithmetic.

## Ten authenticated original direct E8 rel32 CALL edges

| Original CALL site | Original native destination | Context |
| --- | --- | --- |
| 0x2410876B | 0x24106440 | Insert: location search |
| 0x24108772 | 0x24106700 | Insert: map index allocation |
| 0x24108786 | 0x241076E0 | Insert: tree attachment |
| 0x241087D0 | 0x241083C0 | Release A: tree deletion |
| 0x24108850 | 0x241083C0 | Release B: tree deletion |
| 0x2410882B | 0x24108750 | Map wrapper inserts record |
| 0x241088F6 | 0x24108750 | Phase232 notifier registration inserts record |
| 0x2410895C | 0x24106B20 | Phase232 notifier unregister looks up key |
| 0x2410756F | 0x24106B20 | Additional original lookup wrapper |
| 0x241089BF | 0x24108840 | Phase232 notifier unregister releases record |

Two additional **original indirect comparator calls** at 0x24106B53 and 0x24106BB2 are checked separately. The source-gated auditor validates four whole original native function hashes, their native RET and INT3 boundaries, **41 sparse instruction witnesses**, these 10 direct E8 CALL targets and both indirect comparator call forms.

## New source, tests and CI

- research/tools/audit_sin_original_client_pvs_map_frontends.py: owner-original dual-full-DLL SHA gate, four whole-native-body hashes, 41 sparse original witnesses, returns, INT3 padding, 12 original call-site checks
- research/tests/test_sin_original_client_pvs_map_frontends.py: synthetic-only mutation regressions. Artificial bytes **never** increment original recovery counts
- include/sin_compat/original_client_pvs_map_frontends.hpp: independent C++17 map frontend reference with typed 16-byte record layout, original double-comparison search order, injected tree find/allocate/attach/delete hooks, safe bounds
- tests/test_original_client_pvs_map_frontends.cpp: synthetic three-node search, call order, left/right comparator direction, missing key, repeated index reuse, the two native release variants, payload preservation, 16-bit count wrap, malformed index and cycle guards
- CMakeLists.txt and pc/windows_x86_proxy/CMakeLists.txt: Linux/native MSVC Win32 x86 CTests for host-reference code, not original game-module loading

Owner-original audit:

    python3 research/tools/audit_sin_original_client_pvs_map_frontends.py --archive /private/bin.zip

Synthetic-only mutations:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_map_frontends.py -v

**Important scope boundary:** Original map location search **0x24106440**, map allocation **0x24106700**, tree insertion/balancing **0x241076E0**, tree deletion/rebalancing **0x241083C0**, engine-owned object/vtables, actual game DLL loading and complete gameplay are not recovered in this phase. Host callbacks are explicit placeholders, not claims about native compiler behavior in the hidden callee bodies. Windows PC gameplay loading remains first, followed by Android ARM64 and Quest 3 VR.
