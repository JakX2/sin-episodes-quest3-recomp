# Phase 117: Original SiN pose-driver recursive bone matrix construction

## Primary, owner-supplied binary evidence

2006 x86 PE32 `server.dll` SHA-256:
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
All addresses below are the original fixed preferred base `0x22000000`.
No original executable code, disassembly dumps or assets are packaged.

Within `0x22103A60`, the pose driver's second per-track loop begins at
`0x22103D70`. It reads track influence at track `+0x5C` and skips the track
when its original x87 comparison decides it is inactive. The loop advances
by **`0xE4` bytes** per record and returns at `0x221048D5`. Its active
track path first calls `0x22100F30` at `0x22103DC8`, then performs quaternion
matrix setup and concatenation. The later kinematic/effect code after
`0x22103E3A` is **not implemented** in this phase.

### Recursive helper `0x22100F30..0x22101026`

The complete original routine is only 247 bytes. Inspection of its x86
argument use, parent-record offsets, relative calls and bitset writes reveals:

1. Compute `word = bone >> 5` and `mask = 1 << (bone & 31)`.
2. If `computed[word] & mask` is nonzero, return immediately without
   overwriting the already calculated bone matrix.
3. Read the local quaternion and position for this bone; call original
   `0x2226D250` to make a **3x4 local matrix**.
4. Resolve the parent through the 216-byte studio bone descriptor table
   (`studiohdr+0xA0` descriptor block; parent ID in descriptor `+4`).
5. If the parent index is `-1`, compose the **supplied entity-root matrix**
   with the bone's local matrix through `0x22269520` and mark this bone
   computed.
6. Otherwise, recursively invoke itself for the parent, then compose the
   parent's world matrix with this bone's local matrix, store the result,
   and mark this bone's bit computed.

The second-loop call uses this helper for targeted bones before calculating
additional track-specific matrices.

### Read-only binary verification

`research/tools/audit_sin_original_pose_bone_matrix.py` matches the owner's
unchanged DLL against **5 SHA-256 code ranges** (including the entire
recursive helper), **8 exact relative CALL targets**, and **15 opcode
anchors**. It separately verifies the second-loop dispatch and backedge.

- Complete recursive helper, `0x22100F30..0x22101026`:
  `70dca7987b0eaba1e1f63be816ee588554bec4e33ce13eedd14401166e533f9f`
- Second-loop entry/sampler branch, `0x22103D70..0x22103E39`:
  `8bc552e59e43d7ad60cf1af4efbec235a7a7d6bafcea4815103af20adcc45b0a`
- Per-track exit/backedge, `0x221048A6..0x221048D4`:
  `23d05658771f0375ca0b892c99330abd06259c7ed6b606a884c201f98fa1a302`

The remaining two SHA witnesses cover the prefixes of `0x2226D250`
and `0x22269520`. Synthetic-only PE32 tests verify rejection of the wrong
hash, call opcodes and out-of-section reads.

## Independently authored portable code

- `original_pose_bone_matrix.hpp/.cpp`: value-owned recursive builder with
  one requested bone, retained computed-bitset, root-world matrix and
  already calculated matrices. It reuses the original quaternion matrix
  and affine matrix multiplication reconstructors from Phase 114.
- `test_original_pose_bone_matrix.cpp`: root and multi-level parent recursion,
  retained bitset, sibling reuse, quaternion rotation, non-topological
  valid parent links, cycles, nonfinite inputs and corrupt snapshots.
- `test_original_pose_bone_matrix_driver_bridge.cpp`: Phase 116 first-pass
  pose assignment feeds world-space bone composition.
- A synthetic-only PE32 audit test and CMake/CTest integration.

The portable implementation protects its owned snapshots from invalid
indexes, cyclic parents, overflow and malformed floats. The 2006 x86
routine assumes trusted native memory. The original function's floating
point rounding and memory aliasing are not reproduced byte-for-byte.

**Not implemented:** the second-loop's subsequent kinematics/effects,
actual animation sequence sampling, the complete native `CStudioHdr`/
MDL-cache bridge, `client.dll` runtime, real in-engine SiN entity execution,
OpenXR/Quest 3 rendering or a playable Android package. All original
binary and commercial game assets remain outside source control.
