# Phase 199: compositional red-black insertion fixup for SiN's original cache-node layout

13 November 2026. **Draft stacked on Phase198 PR #251.** Scope: implement and test a *portable reference* red-black insertion fixup using the byte-grounded 2006 SiN server.dll tree structure and recovered rotation helpers, without claiming a complete machine-code-equivalent decompilation of the 1,387-byte native balancing functions.

## Proven original basis versus inferred model

The following are independently supported by already verified owner-original SiN Episodes: Emergence (2006) i386 PE32 SHA256-gated work:

- Phase191: 24-byte cache node stride, WORD left/right children at offsets +0/+2, unsigned 16-bit index and missing value `0xFFFF`, tree root WORD at tree +0x10.
- Phase192: scene/model-sounds cache lookup compares key's first DWORD as an **unsigned** value.
- Phase196: insertion link sets node parent WORD +4, color WORD +6 to zero, left/right to missing; writes root or parent's chosen child, then calls one of the two original balancers.
- Phase197: each original 1,387-byte balancer recognizes the root and black-parent fast paths, ensures root's color becomes 1, handles uncle recoloring and invokes distinct rotation helpers. Both complete native functions and outgoing calls were SHA-pinned, **but a complete step-by-step reconstruction of their red-parent loops has not been proven**.
- Phase198: four original scene/model-sounds 237/240-byte rotation functions now have verified pointer/parent/root behavior, independently tested.

**This phase adds ZERO newly SHA-verified original native functions and ZERO new native evidence bytes.** The catalog remains **195 reviewed functions (103 server / 92 client)**. No new assertion of original x86 equivalence is made.

## New self-contained model

`include/sin_compat/original_server_cache_balance_composition.hpp` combines the previous original-grounded nodes, colors, parent pointers and rotation helpers with the standard red-black insertion-fixup algorithm:

1. New root is blackened. If the inserted node's parent is black, leave existing topology unchanged and blacken root.
2. If parent is red and uncle is red, blacken both and recolor grandparent red, moving the check up to the grandparent.
3. If parent is red but uncle is black/missing, normalize the LR/RL triangle with a child rotation when needed, blacken the promoted parent, recolor grandparent red and rotate grandparent in the opposite direction.
4. Ensure final root color is black; preserve node key and payload values.

The model calls the Phase198 recovered `rotate()` implementation for actual node pointer updates. It deliberately does **not** emulate native singleton sentinel initialization, x86 thiscall ABI, exact helper dispatch counts, or native allocation. It does not presume the source engine's `IFileSystem` method at vtable byte +0xD4 is decoded.

### Host-only safeguards

This is independent portable C++17, not original x86 memory semantics. It uses a working copy and commits the result only after success, so a corrupt or cyclic parent/child structure fails closed without leaving partially rotated state. It checks all traversed indexes and parent/uncle links and uses bounded iteration fuel. Such checks were **not** observed in the original binary.

## Tests and integration

- `tests/test_original_server_cache_balance_composition.cpp`: runs all four LL/RR/LR/RL three-node insertion patterns, a red-uncle recolor case, and **all 720 permutations of six distinct insertion keys for each of the two original repository configurations (1,440 complete trees)**.
- Every insertion validates the red-black root/parent/color, red nodes with black children, equal black-height paths, no duplicate or unreachable active nodes, in-order unsigned BST ordering, and preservation of each already-inserted native node's key and payload.
- Cross-phase integration uses the real Phase196 portable link callback, the Phase198 rotations, the earlier Phase192 unsigned comparator and Phase191 tree lookup. Tests include malformed-node rejection with no mutation.
- Root CTest and native Windows x86 MSVC CTest targets added to exercise the same C++17 model. Hosted CI status must be checked on this exact branch before claiming pass.

## Integration consequences and next real milestones

This provides an end-to-end reference tree insert/link/fixup/search system with synthetic storage. It is a **useful implementation milestone** toward game code reconstruction, **not proof of full original balancer control flow or a retail SiN game session**. Next tasks are (a) verify the native Phase197 red-parent branches against traces/original x86 instruction boundaries and reconcile any behavioral differences, (b) recover real filesystem/cache record keys, and (c) use the existing Win32 original factory-forwarding proxy under an owner-only retail game launch to obtain actual server/client interface startup traces. Windows PC first; Android second; Quest 3 third. No proprietary original DLLs or game assets are committed.
