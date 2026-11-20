# Phase 500: SHA-original 2006 C_SoundscapeSystem auxiliary cleanup

## Original method identity

Private original SiN Episodes: Emergence 2006 client.dll SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

| Original method | VA | Length | Complete SHA256 |
| --- | --- | ---: | --- |
| Soundscape auxiliary cleanup | 0x240F3D10 | 223 bytes | 41ca09e9768fe5a683d57fdbe8ccac0e157ce557efc101e8e103aec3c804cfee |
| Soundscape vtable slot5 direct JMP | 0x240F3E10 | 5 bytes | 9dddea87b38bb2b72eaa40f5738b6c57dbb1ef3fa62d3ca201da507a79fcaeec |

This extends Phase 499's authenticated original 12-entry vtable.
Slot5 contains a direct JMP to helper 0x240F3D10. The helper
is NOT equivalent to Phase 489 soundscape Shutdown (0x240F3EE0).
It separately handles two packed handles and cleans up
two heap-backed arrays associated with the soundscape object.

The private auditor audit_sin_original_soundscape_aux_cleanup.py
--client/--archive checks the original client SHA, vtable slot5,
entire method and thunk SHAs, selected opcode witnesses and
both original direct allocator-free call targets.
The public --self-test fabricates code windows and never loads
any copyrighted original DLL.

## Source-observed control flow

1. If packed handle at object+0x9C is not -1, compute its
   index (handle AND 0xFFF) and generation (handle shifted 12).
   Consult original global handle table 0x242FA734, whose
   entries have 16-byte stride. Only if the generation matches
   and a target object is present, call embedded subobject's
   vtable slot0 through this+0x30 and invalidate +0x9C to -1.
   A stale generation does NOT get invalidated here.
2. If distinct handle +0x94 is non-sentinel, invoke embedded
   subobject vtable slot0 and invalidate +0x94. No equivalent
   generation-table check appears at this callsite.
3. Clear count this+0xAC unconditionally.
4. If vector flag this+0xA8 != -1: free buffer +0xA0 when
   non-null through helper 0x2416F940, then clear pointer
   +0xA0 and count +0xA4. Always copy resulting +0xA0
   into mirror field +0xB0.
5. Clear count this+0xC0 unconditionally.
6. If vector flag this+0xBC != -1: free buffer +0xB4
   when non-null through the same 0x2416F940 helper,
   then clear pointer +0xB4 and count +0xB8.
   Always copy resulting +0xB4 into mirror +0xC4.
   Return via one of two original RET paths.

This ordered conditional cleanup is not an unconditional
memset. Replacing it with blanket zeroing would change
source-owned states when an array flag equals -1.

## Independently owned reconstruction

client_soundscape_original_aux_cleanup_owned.h contains
only source-known fields and caller-supplied *independent*
handle resolution outcome booleans. It has no retail
engine pointers.

OwnedCleanup::Prepare produces a fixed maximum-12-event
ordered plan for both handle branches, two original allocator
call candidates, conditional arrays, unconditional count
clears and pointer mirrors.

OwnedCleanup::SimulateComplete verifies the entire plan,
source revision, handle identities, buffer state and event
counts. It then updates only independently owned state.
It NEVER calls the original allocator at 0x2416F940,
dereferences the original handle table, calls the embedded
Source virtual function or touches game memory.

Native MSVC x86 fixture
test_client_soundscape_original_aux_cleanup_owned.cpp
covers 12 complete source-order events, stale 9C generation
behavior, asymmetric 94 release, both optional array frees,
sentinel-skipped arrays, unconditional count clearing,
mirror ordering, and strict stale/forged plan refusal.

## Remaining integration gates

This does not implement the full destructor: original
vtable slots2/5/11 have additional wrapper/base-class
lifetime behavior, and genuine object size/ownership
requires further verification. Real original sound mixing,
KeyValues ownership, entity triggers, soundscape selection
and the larger update slot10 also remain unfinished.

The last actual user-retail Windows checkpoint remains
0x53494E74. Prepared 0x53494E76/77 soundscape catalog
and Phase 498 optional read-only ConVar lookup have not
yet been observed in the owner's retail game.
No normal VClient011::Init success, playable Windows,
Android or Quest 3 game is claimed by these synthetic
unit tests.
