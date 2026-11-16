# Phase 247: original client entity-list base destructor and handle removal

16 November 2026. Stacked draft on Phase246 PR #302. Source is the owner-provided original **SiN Episodes: Emergence (2006)** retail Windows x86 client/server DLL pair. Do not upload the original DLLs, whole commercial method bodies, original disassemblies or copyrighted assets.

## Mandatory whole-original DLL provenance

| Original PE32 DLL | Entire bytes | Whole SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

Both entire DLL digests are required before authentic native method credit.

## Four NEW SHA-reviewed complete native client methods

| Original VA | Original role | Entire native bytes | Whole original code SHA256 | Native exit |
| --- | --- | ---: | --- | --- |
| **0x24127E20** | Remove a 16-byte slot from current linked list if linked, then append to list tail | 91 | a7a4fce2f5f71f02f28ed914f974c0eb6bc10f525b511336c7bfe170c506fcc3 | Two RET4; five trailing INT3 |
| **0x24127FC0** | Remove bound entity by raw slot index, dispatch virtual callbacks, bump serial, repair lists | 176 | 3dda702afe19b7f523be5d57e8abc1cd947dc94a0feb315e01f7e1df8b284af5 | RET4; no trailing INT3 |
| **0x24128070** | Base entity-list destructor, base vtable store and repeated bound-slot removal | 48 | 60ccb6988afda1e4d8aae4497391ef6ec5f27d504aaa0a8526e205fce9fad7cb | RET; no trailing INT3 |
| **0x24128180** | Mask packed handle with 0xFFF then tail-JMP to raw remover | 18 | fe23a8e377538a49ae4a22f04ca9d4057a0a356e92151e00597b9f245b17bad5 | E9 tail JMP; 14 trailing INT3 |

**New Phase247 whole-original native code: four functions, 333 bytes.** Whole-SHA verifier additionally confirms 24 distributed sparse opcode witnesses, all original native exits, 19 trailing INT3 bytes, three direct E8 CALLs and two E9 tail JMP relationships. Synthetic fixtures are never counted as authentic original code.

## Phase246 ledger reconciliation (not a new Phase247 discovery)

Preceding Phase246 PR #302 already reviewed native client **0x24105FE0**, 39 bytes, SHA256 **305edbceeb790e6a91f91fa5815f31c11f606f71e1831384350bb5c265186fe2**, but did not update the shared JSON original-method catalog. The Phase247 audit rechecks that original complete method and free-call/RET/INT3, then adds one **carry-forward method entry** alongside the four new methods. An additional fix corrects doubly escaped PE-signature and INT3 byte literal checks in the Phase246 private auditor.

Thus the actual JSON method catalog changes **368 → 373**: **+1 Phase246 carry-forward and +4 Phase247 new**, with 147 server and 226 client. The additional 39 original bytes must not be attributed to Phase247 discovery.

## Observed native data and control flow

The original base entity-list contains **4096** primary slots of **16 bytes each**. Each slot has entity pointer DWORD+0, serial DWORD+4 and doubly linked pointers DWORD+8/+C. The active list head/tail are parent DWORD+0x10004/+0x10008; the distinct high-index free list head/tail are DWORD+0x1000C/+0x10010.

- **0x24128180** masks the input DWORD with **0xFFF** and E9 tail-calls **0x24127FC0**. The raw helper itself does not mask. The native stack is cleaned by the raw helper RET4.
- **0x24127FC0** returns immediately if the slot entity pointer is null. Otherwise it calls entity-owned vtable byte offset +4 with an out-token initialized to 0xFFFFFFFF, then parent-owned vtable byte offset +4 with the entity pointer and original packed handle **(old_serial << 12) | index**. Only after callbacks does it write **(serial+1)&0x7FFF**, clear entity pointer, repair active prev/next links and self-mark the freed node.
- Raw indexes **>=2048** are appended to the distinct high-slot free chain via original **0x24127E20**. Indexes less than 2048 are not appended there. Original 0x24127E20 unlinks an already linked node if needed before appending to the current chain tail.
- **0x24128070** writes base vtable **0x24298EB4**, then repeatedly **saves the current active node NEXT pointer before** removing it via 0x24127FC0. It does not invoke the allocator: Phase245 containing-object destructor **0x24107430** tail-JMPs to this base destructor.
- Synthetic host uses safe *indices* rather than dereferencing native process pointers and delegates still-opaque native virtual callbacks. Its bounds, cycle and graph-consistency guards are not claims about the retail x86 implementation.

## Exact original direct code relationships

| Original instruction | Target | Role |
| --- | --- | --- |
| **0x24128065 E8** | **0x24127E20** | Raw remover to high free-list tail helper |
| **0x24128092 E8** | **0x24127FC0** | Base destructor to raw remover |
| **0x24106256 E8** | **0x24128180** | Prior Phase245 drain to masked-handle remover |
| **0x2412818D E9** | **0x24127FC0** | Masked wrapper tail-JMP to raw remover |
| **0x241074C7 E9** | **0x24128070** | Prior Phase245 containing parent destructor tail-JMP to base |

The original x86 code has no INT3 between raw removal/base destructor or between base destructor/the following unrelated function. Complete whole-method SHA, exact RET location and native call structure establish these boundaries.

## New source and tests

- research/tools/audit_sin_original_client_entitylist_base_removal.py: dual entire original-DLL SHA gates, four complete method code SHA256s, predecessor SHA, 24 original sparse instruction witnesses, original returns/padding and five native E8/E9 edges.
- research/tests/test_sin_original_client_entitylist_base_removal.py: synthetic-only independent mutation tests for each witness, return/JMP byte, padding, E8/E9 instruction, image base, text-section boundaries and predecessor evidence.
- include/sin_compat/original_client_entitylist_base_removal.hpp: C++17 reference of original 4096-slot entity-list, raw/masked removal, serial wrap, virtual event ordering, high free-list append, reuse and base teardown.
- tests/test_original_client_entitylist_base_removal.cpp: masks, callbacks, old handles, serial wrap, low/high split, tail-reorder, high-index reuse, save-next destructor, null slot behavior and host index guards.
- Portable Linux root CTest, native MSVC Windows x86 CTest, original catalog with Phase246 carry-forward, and byte-literal bugfix in Phase246 private-only audit.

Private original retail binary audit:

    python3 research/tools/audit_sin_original_client_entitylist_base_removal.py --archive /private/bin.zip

Synthetic-only mutation regression:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_entitylist_base_removal.py -v

## Remaining milestones

Native entity and parent virtual methods, actual Source object ownership, CRT lifecycle and allocator code are still outside this host reference. A Windows x86 compatibility/proxy test does not prove a rebuilt independent retail gameplay client/server DLL runs in the original game. Platform order: **original Windows PC engine first; Android ARM64 next; Quest 3 VR last**.
