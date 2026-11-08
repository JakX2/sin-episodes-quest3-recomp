# Phase 115: original SiN 2006 ordinary (unparented) masked bone construction

## Provenance

Original, privately held `server.dll`: PE32, image base `0x22000000`,
SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
No proprietary executable, game resources, or original disassembly are distributed.

The primary audited original function is `0x220FE0C0..0x220FE209`,
SHA-256 `de50e3c4b85d14b4ad7fc12e07cb2be7169c57ef95859622f381f65144ea4666`.
This is the **ordinary** bone-matrix constructor selected by the skeletal
setup dispatcher `0x220EF510`, as reconstructed in Phase 113. Parent-linked
construction was handled in Phase 114 (`0x220ED290`).

## Observed control flow and offsets

The constructor receives a studio header, entity absolute angles and origin,
separate local quaternion and translation arrays, requested bone index,
output matrix buffer and bone mask. It builds root model-to-world through
`0x2226CC20`. Each studio bone record is **216 bytes** apart, with parent
index at record `+0x04` and a **32-bit flags field at `+0xA0`**.

- **Bone request `-1`:** the function generates indices `0..count-1` and
  processes them in increasing order.
- **Specific bone request:** the function follows the requested bone's
  parents from leaf to root, stores the chain on its stack, and processes
  it **root to child**, avoiding unrelated bones.
- **Out-of-range request:** the binary clamps an index below `-1` or an
  index at/above the bone count to bone `0`. The portable implementation
  also guards empty models rather than dereferencing a missing bone.
- **Mask:** if `studioBone.flags & requestedMask` is zero, the function skips
  that bone without modifying its world-matrix output slot.
- **Selected bone:** `0x2226D250` converts the animated quaternion and
  translation to a local 3×4 matrix; `0x22269520` concatenates it with the
  entity's root matrix for bones whose parent equals `-1`, or with the
  *already computed or existing* parent's world matrix otherwise.
- The output matrix uses **48 bytes**, positions have 12-byte strides,
  and quaternions have 16-byte strides.

The original uses a finite 0x260-byte stack frame and assumes the input model
has a valid acyclic studio hierarchy. The C++17 reconstruction uses bounded
vectors and rejects malformed parents, NaN, overflowing matrices and missing
previously required parent matrices rather than reproducing unsafe reads.

## Binary witness coverage

`research/tools/audit_sin_original_ordinary_bone.py` is a SHA-pinned read-only
verifier requiring an owner-supplied original DLL. It checks **3 complete
function hashes, 4 original relative CALL destinations and 14 instruction
anchors**. The supporting full functions are `0x2226D250..0x2226D277`
(quaternion plus position) and `0x2226CC20..0x2226CC49` (angles plus origin).
Its synthetic regression test uses fabricated PE32 headers only.

## Independent C++17 reconstruction

- `original_ordinary_bone.hpp/.cpp`: mask-aware, hierarchical bone construction,
  restricted ancestor-chain traversal, 216-byte studio flag parsing, and
  **optional** output matrices to express untouched masked-out slots.
- `OriginalOrdinaryBoneSkeletalAdapter`: replaces Phase 113's
  `compose_unparented_bones` while forwarding the original host callbacks.
  It only exports a complete vector when the caller supplied matrices for
  skipped slots. It does **not** invent their values.
- Parent and ordinary skeletal adapters can be composed so the Phase 113
  dispatcher uses the Phase 114 name-matching merge when appropriate, then
  falls back to Phase 115 for ordinary construction.
- Regression tests cover full skeleton and specific ancestry, flags and
  skipped slots, seeded-parent reuse, rotations, invalid indices, invalid
  skeletons, invalid floats and both skeletal dispatch paths. Local standalone
  harness uses CMake/CTest. Source does not contain real animation pose data.

## Unfinished runtime work

This phase does not implement actual 2006 `client.dll` or `server.dll` DLL
exports, native studio/model-cache and entity ABI, pose evaluation, animation
layers, skinning, game map execution, Android VR rendering, OpenXR input or a
Quest 3 standalone APK. Original game assets remain external to the repository.
The desktop HL2 experiment only establishes a limited BSP/content baseline,
not binary compatibility of the original 2006 game modules.
