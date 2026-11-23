# Phase641: original SiN tree rotations and sentinel-aware slot accessor

## Scope and original evidence

This phase continues the actual 2006 SiN Episodes: Emergence server.dll symbol-tree recovery after Phase640's complete tree-node insertion method. The user explicitly prohibited working on generic HL2-like physics, surface properties, models/props, rendering or materials because those already work in ported maps; none of that work is included.

**Unchanged owner's retail DLL SHA256:**
- server.dll: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client.dll: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

**Three fully authenticated executable methods: 546 original x86 bytes total.**

| Function VA | Complete bytes | Original full SHA256 |
|---|---:|---|
| 0x22281180 left tree rotation | 231 | 9092a623c08c10b6537a42c5ef8d50ad803fb44e1c5170821b1ca91b73a28888 |
| 0x22281270 right tree rotation | 234 | 96aead04d0b3c209f367fbd5fee136a2e9d593ba7e1db18c401f5f20ad87a47a |
| 0x22280D40 sentinel-aware slot accessor | 81 | af140a33de10123b9607b600d5d08d64bcf62bdd995d02d6779580a4f6d23305 |

**Source links audited:**
- Full original **1,482-byte** 0x22281840 rebalancer SHA256: 4ef6465536a6a442a842531f09f0d8f0a37f274be6403d540528b24059527ede.
- Four direct calls from it to the original rotations: 0x22281B47 and 0x22281DDB to left; 0x22281BFF and 0x22281D23 to right.
- Both rotations make exactly **nine static rel32 calls each** to the original 0x22280D40 accessor, including calls in conditional branches. All 18 sites are verified.
- In all **22 original call edges**, and **83 address-pinned x86 instruction witnesses** in the three complete methods. All three native function ends followed by INT3 padding.

## Recovered original 0x22280D40 accessor behavior

On first access, if bit 0 of the original BYTE at absolute guest address 0x22589140 is unset, the native method first ORs bit 0 into the full DWORD at 0x22589140, then initializes the shared sentinel node at 0x22589138:
- WORD +0, +2, +4 = FFFF;
- WORD +6 = 1.

Each subsequent call observes the bit and skips these writes. Its index arg is truncated to a 16-bit WORD. For FFFF it returns **absolute VA 0x22589138** (the shared sentinel), otherwise it returns the fresh table slot-array pointer [table+4] + 16*uint16(index). Original thiscall returns through RET 4.

**This is a global side effect**, not an ordinary slot lookup. Earlier phases left 0x22280D40 opaque; Phase641 now preserves its original first-call initialization and sentinel routing in a portable reference. No original-binary contents are shipped.

## Original left rotation 0x22281180

Native thiscall(table,index32), RET 4. The method reads the target node's right child pivot (node +2), copies the pivot's left child (pivot +0) into the target's right field, and, if that transferred child is not FFFF, changes its parent to the target node. If the pivot is non-FFFF, it receives the target's parent; then either the tree root table+0x10 or the relevant field of the target's parent node is updated to the pivot. Finally pivot.left becomes the target and target.parent becomes pivot.

**Exact repeat lookups matter:** the native routine repeatedly calls 0x22280D40 and **re-reads [table+4]** before direct slot writes. The reference preserves that ordering instead of caching the array pointer or the pivot's transferred child value.

## Original right rotation 0x22281270

Native thiscall(table,index32), RET 4. Mirror of left:
- pivot = target.left (+0)
- target.left = pivot.right (+2)
- if transferred child non-FFFF, change its parent
- attach pivot to old parent or replace table root
- pivot.right = target and target.parent = pivot.

When deciding whether the target is its old parent's right child, compare the original WORD at parent+2 and select parent+2 if equal, parent+0 otherwise. Original node indices and slot fields are 16-bit; the initial argument may contain arbitrary high 16 bits, which the original accessor truncates.

## Portable model and tests

`SentinelSlotAccessor` and `RotateNode` are independent portable C++17 classes, using a typed owner-supplied guest-memory interface. They reproduce explicit ordered memory writes with first-error refusal, wrapping 16-bit indices and repeated pointer reads. Safety guards for null pointers, address overflow, read/write failure, reentrant host operations and exceptions are **added portability protections**, not native exception semantics.

The standalone synthetic tests cover:
- One-time sentinel initialization, full DWORD flag preservation, FFFF index routing, and partial initialized state when a write fails.
- Root and non-root rotations, both possible attachment sides for each direction.
- Transferred-child present and FFFF absent cases, and full-DWORD index -> low-16-bit truncation.
- Read-after-write slot-array relocation, original partial-write provenance, nested reentrance refusal and host exceptions.
- Original-only owner audit against exact server/client DLL bytes and original-free synthetic mutation tests.

Local C++17 compiled test and ASan+UBSan pass; public CI must separately confirm its uploaded commit. Tests operate exclusively on invented guest memory/indices, not a running retail game.

## Limitations and next work

The full 1,482-byte original `0x22281840` routine is **authenticated but not yet reconstructed**. Its branch-specific color checks, rotations, and tree-fixup loop must be independently recovered before these rotations can be driven faithfully by the native original rebalancer. There is **no original Windows engine in-level execution**, no Android/Quest 3 playable engine, and no claim of a completed decompilation. The original DLLs and other proprietary game binaries were not committed.

Owner verification command:

`python research/tools/audit_original_symbol_rotations_phase641.py --server ORIGINAL_SERVER_DLL --client ORIGINAL_CLIENT_DLL`

Public synthetic self-test:

`python research/tools/audit_original_symbol_rotations_phase641.py --self-test`
