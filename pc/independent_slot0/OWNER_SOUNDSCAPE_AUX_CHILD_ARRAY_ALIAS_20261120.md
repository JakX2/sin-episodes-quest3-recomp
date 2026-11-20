# Original soundscape A0/B4 cleanup alias contract

## Source origin

The original 2006 SiN Episodes client.dll was independently
SHA-256 authenticated from the user's bin.zip as

6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

This stage does not discover new game bytes; it integrates
three source-proven methods reconstructed in the preceding
stacked PRs:

- 120-byte original C_SoundscapeSystem constructor at
  VA 0x240F3BC0 (PR #499).
- 223-byte original auxiliary cleanup at VA 0x240F3D10
  (PR #506).
- 83-byte original child-array cleanup helpers at
  0x240F3A20 (A0) and 0x240F3AD0 (B4) (PR #508).
  Both have the same source behavior as 0x240F3970.

## Critical overlap in the source object

The auxiliary cleanup accesses fields using offsets
relative to the parent soundscape object. The deleting
destructor invokes child-array helpers with the *same
parent-memory fields* but with each child as this:

| Parent offset | Parent auxiliary helper | Child-array method offset |
| --- | --- | --- |
| +0xA0 | Buffer A0 pointer | (this+0xA0)+0 |
| +0xA4 | Buffer A0 count | (this+0xA0)+4 |
| +0xA8 | Buffer A0 sentinel flag | (this+0xA0)+8 |
| +0xAC | Buffer A0 secondary count | (this+0xA0)+12 |
| +0xB0 | Buffer A0 pointer mirror | (this+0xA0)+16 |
| +0xB4 | Buffer B4 pointer | (this+0xB4)+0 |
| +0xB8 | Buffer B4 count | (this+0xB4)+4 |
| +0xBC | Buffer B4 sentinel flag | (this+0xB4)+8 |
| +0xC0 | Buffer B4 secondary count | (this+0xB4)+12 |
| +0xC4 | Buffer B4 pointer mirror | (this+0xB4)+16 |

Original constructor sets flag +0xA8 and +0xBC to ZERO.
It does not initialize them to -1. Both original cleanup
implementations regard non--1 flags as releasable arrays.

A naive independent port of these two methods could create
two free operations for the *same field* if the parent and
child state are mistakenly represented as unrelated buffers.
This is the exact aliasing contract needed before wiring the
actual destructor into a recompiled game object.

## New independently authored mapping

client_soundscape_original_array_overlap_owned.h defines:
- Compile-time original parent/child offsets for each
  of A0 and B4, including pointer, counts, flags and
  mirrors; all offset arithmetic is pure constexpr.
- Project(auxiliary state, region) to a separate
  independent child-array representation.
- ProjectBack(parent state, child state, region)
  to keep source-overlapping state synchronized.

No engine pointer is ever dereferenced, passed to free or
stored in this adapter. Both models use only bool
pointer-presence flags and fixed-width integer counts.
The source register/memory and allocator ownership is
still a separate real-engine integration gate.

test_client_soundscape_original_array_overlap_owned.cpp
links all three earlier owned models into one native
Windows x86 fixture and checks:

1. Source constructor has +A8/+BC=0 and both arrays initially
   zero, as shown by the original constructor write set.
2. Auxiliary-first path: both non-null buffers emit one
   free each; both later child destructors see null and
   emit zero further free operations. Total two.
3. Child-first path: both child destructors emit one
   free each; the auxiliary cleanup sees null pointers
   and emits zero further free operations. Total two.
4. Sentinel -1 path: neither method emits a free,
   and pointer/count+4 preservation plus +C count
   clearing and +10 mirror propagation remains exact.

This is an explicit *synthetic composition test* for
source-observed aliasing and at-most-once buffer release.
It is not a claim that original 2006 retail teardown
actually invokes the methods in both orders, nor is it
an actual memory-allocator double-free test.

## Remaining obstacles

The normal independent 2006 VClient011::Init remains
fail-closed; game-system registration and actual soundscape
object size need further source analysis. Other child
arrays at +0x08 and +0x1C are not part of this overlapping
auxiliary pair. Destruction also calls the original base
game-system cleanup, which can interact with engine global
registries and is not implemented here.

The user-observed Windows retail checkpoint remains
0x53494E74. New 0x53494E76/77 soundscape and optional
read-only CVar diagnostic have not been owner-observed
in a retail process in this session. No playable Windows,
Android or Quest 3 build is claimed.
