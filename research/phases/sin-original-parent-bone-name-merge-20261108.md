# SiN Episodes: Emergence (2006) parent-linked skeletal bone merge, Phase 114

## Provenance and scope

Original user-supplied private Windows PE32 `server.dll`, SHA-256:
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`;
original image base `0x22000000`. No original DLL, executable fragments,
models, assets or copyrighted disassembly are shipped in this repository.

This phase independently reconstructs **`0x220ED290`**, the parented bone
matrix builder dispatched by Phase 113's original `0x220EF510`. It also
reconstructs its bone-name lookup helper **`0x220FFB30`** and the local
quaternion+position matrix constructor **`0x2226D250`**. These are originally
native x86 binary functions, not a working Quest 3 animation runtime.

## Observed original control flow

1. The builder receives the **parent animating entity** as its seventh
   argument and requests its studio model header via `0x22139A10`. It
   validates the model-cache object type (3) through engine virtual calls.
   The independently authored host treats an invalid parent studio as a
   recoverable error rather than risking undefined pointer behavior.
2. It takes the child's already verified studio header and obtains its bone
   count at header offset `+0x9C` and bone records from `header+int32(+0xA0)`.
   Each bone record is **216 bytes (`0xD8`)**, with name relative-offset
   `+0x00` and parent bone index at `+0x04`.
3. It constructs the child entity's **root world matrix** from its absolute
   angles and origin through `0x2226CC20`.
4. For **every child bone**, it looks up the bone name in the **parent's**
   studio header through `0x220FFB30`. That helper binary-searches a sorted
   **byte** index table at `parentStudio+int32(+0x16C)`, referencing parent
   bone records and comparing names **case-insensitively** via `0x223A5574`.
   A missing name yields `-1`.
5. If the name exists in the parent and its signed short bone-cache remap
   is cached, the builder copies that parent **world-space bone matrix**
   directly via `0x221004C0`/`0x22268EF0`. It does **not** apply the child's
   local pose to that bone on this path.
6. Otherwise it builds the child's local 3×4 transform from its animated
   quaternion and position via `0x2226D250`. For root bones (`parent==-1`),
   it composes `childEntityWorld × localBone`. For nonroot bones, it composes
   `alreadyBuiltChildParentBoneWorld × localBone` via `0x22269520`.
7. Child positions and quaternions are accessed with separate **12-byte**
   and **16-byte** array strides. Output matrix stride is **48 bytes**.

Thus the original uses *bone-name correspondence*, not arbitrary matching
indices between character rigs. If a parent has a named cached bone, that
bone inherits the parent's current world-space result. Bones that do not
match, or whose parent bone is uncached, still follow the child's own pose.

## Read-only byte-level evidence

`research/tools/audit_sin_original_parent_bone_merge.py` verifies the
private original DLL, with six SHA-256-pinned whole function ranges:

| Original function | VA range | SHA-256 |
| --- | --- | --- |
| Parent bone merge | `0x220ED290..0x220ED3C1` | `c6e4dd1d5df71287141c1984818e1d1ba99cbda114d6d4f9e306f4804b7d63e6` |
| Sorted bone-name resolver | `0x220FFB30..0x220FFBB7` | `f3a538d6b42c332d9163373ad4c7366d8f4893ad701585d27288df5ed185f65b` |
| Case-insensitive name compare | `0x223A5574..0x223A55B6` | `d108c0f0291d63e422757a9255fa568f383a772601ba2a66a6f2b79ca9fe7f29` |
| Quaternion rotation matrix | `0x2226AF60..0x2226B03A` | `e0ba6a46c016a127c8304d883923c432b0e46fa567a9613bef593c450b06f2d5` |
| Quaternion + translation | `0x2226D250..0x2226D277` | `5155a696a20f9433b990e8be627790cfb833d545d835a8046647123eb2b7b280` |
| Unparented comparison | `0x220FE0C0..0x220FE209` | `de50e3c4b85d14b4ad7fc12e07cb2be7169c57ef95859622f381f65144ea4666` |

Also **12 exact relative CALL edges** and **20 original instruction anchors**
are checked. A synthetic PE32 unit test covers corrupted headers, cross-
section reads, invalid opcodes and incorrect whole-DLL SHA.

## Independently authored portable implementation

`include/sin_compat/original_parent_bone_merge.hpp` and
`src/original_parent_bone_merge.cpp` implement a bounded studio-bone parser
for copied metadata, a case-insensitive sorted-name binary search, original
quaternion-to-matrix convention, cached-parent-bone reuse, and child-local
hierarchical fallback. An adapter replaces precisely Phase 113's
`compose_parented_bones` callback while all native bone preparation,
model-cache locking, animation overlays, and pose-driver logic remain
host-controlled.

Local CTest validates: matching parent bones, differently ordered name maps,
uncached parent bones, unmatched child bones, root and nonroot compositions,
quaternion rotation, nested child bones, malformed studio offsets, invalid
index permutations, broken hierarchies, cache bounds, NaNs, overflow, and
Phase 113 integration/release of the model-cache lock. The standalone
research ZIP contains only authored source/tests, the read-only verifier and
hash/branch data. Native original binary must be explicitly supplied to run
its audit; ordinary CI uses synthetic inputs only.

## Deliberate safety and accuracy boundaries

The native original assumes a well-formed studio hierarchy with parent bone
matrices previously produced in topological order. Portable code rejects
self-parenting, forward references, invalid sorted tables, invalid offsets,
NaNs and overflowing matrix math rather than reproducing undefined memory
behavior. It uses ASCII case folding; the original CRT comparison may be
locale-sensitive. The C++17 quaternion implementation uses native FP math,
so results may differ in low bits from original x87. This phase does not
implement live `CStudioHdr` model-cache locks, actual pose evaluation,
IK, bone masks, native skinning, cross-ABI vtables, complete game DLLs,
or a playable Meta Quest 3 APK.
