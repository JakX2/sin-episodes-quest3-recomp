# Phase 653: Original SiN 12-byte tree slot growth and free-list resolver

## Scope and private 2006 binary provenance

This phase advances missing *SiN Episodes: Emergence* original symbol/entity
registration, not the already-functioning Half-Life 2-equivalent physics,
materials, surfaces, models, prop handling or rendering. The actual owner
retail PE32 DLLs were verified unchanged:

- `server.dll`: SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

Two complete native methods totaling **199 executable x86 bytes** were directly
authenticated against owner-provided original `server.dll`:

| Retail function | Entire size | Whole original method SHA256 |
|---|---:|---|
| `0x2228E100` free-slot pointer resolver | 84B | `108da89a3abdf321d8eb9e8c542c01db13ea09a046d54a7affe15ce82ed908b1` |
| `0x2228E200` 12B-node array grower | 115B | `bf973beee94fe5f7c2bd5e649e41eb973f99bb49dee68e28b9861757796adb2b` |

A private auditor checks complete DLL hashes, entire method hashes, original
PE32 `.text` ownership, **both original INT3 method-end boundaries**, **four
authentic direct rel32 calls** (two allocations and two Phase652 callers),
**10 short branch targets**, and **66 address-pinned original x86 opcode
witnesses**. Only independent derived C++ source, research and tests are
committed. Neither original DLL nor proprietary executable bytes are committed.

## Original 84-byte `0x2228E100` behavior

Original `__thiscall ECX=map` takes one DWORD stack argument and considers its
low WORD slot index; `RET 4`. It lazily initializes the *shared nested-tree*
static sentinel **even on a valid slot lookup**, before resolving the index:

1. Test bit0 of `BYTE[0x22590134]`. If unset, OR bit0 into the **full DWORD**
   at that address, preserving all other bits.
2. Set the three WORD fields of sentinel `0x2259012C` at offsets `+0,+2,+4`
   to `FFFF` in order, then set WORD at `+6` to `1` (black).
3. If requested WORD index is `FFFF`, return the **sentinel's own VA**
   `0x2259012C`, with no slot-array pointer dereference.
4. Otherwise return `DWORD[map+4] + uint16(index)*12`, with arithmetic
   checked defensively in the portable model.

The native method does not check guest memory validity. The portable
reference explicitly refuses zero/bogus addresses, reentry and callback
errors; those are portability protections and not behaviors attributed to
the original x86 code. Partial writes committed before a host error remain
visible, notably the one-time flag set before sentinel field writes.

## Original 115-byte `0x2228E200` behavior

Native `__thiscall ECX=slot_owner` with one DWORD increment argument and
`RET 4`. The actual slot owner is at **map+4**, laid out as:

- `DWORD[owner+0]`: pointer to 12-byte node array;
- `DWORD[owner+4]`: capacity in 12-byte nodes;
- `DWORD[owner+8]`: growth step (FFFFFFFF means disabled).

For disabled step, return untouched. Otherwise compute signed comparison of
existing capacity with **(capacity + increment) modulo 2^32**. If current
capacity is signed-below required, update capacity in-place in a loop:

- If zero, initialize to **3** slots (not 8; 8 belongs to the separate
  Phase651 block-pointer array grower).
- If nonzero and growth step zero, **double** capacity modulo 2^32.
- Otherwise add the configured growth step modulo 2^32.
- Repeat until signed32(capacity) is at least signed32(required).

Even when no capacity growth is necessary, the original **still reallocates
or allocates** after the guard if the growth step isn't disabled. The byte
size uses original `LEA` and `ADD` sequence to compute `capacity*12` modulo
2^32. If array pointer is nonnull, call recovered original wrapper
`0x2226E400(old_pointer, bytes)`; if null, `0x2226E390(bytes)`. Store the
returned DWORD directly back to owner+0. The actual native engine
memory-manager virtual method remains a mandatory typed host callback, not a
fabricated implementation of Source or Windows heap allocation.

Original signed overflow, null allocator return and committed capacity writes
are retained where defined. A conservative loop bound prevents an infinite
loop on malformed synthetic state; original x86 has no such guard.

## C++ source-reference tests

Added `ResolveSlot`, `GrowSlots`, a typed guest-memory/allocator host and
stage-specific outcomes in `original_nested_slot_growth_phase653.hpp/.cpp`.

Standalone strict GCC C++17 `-Wall -Wextra -Wpedantic -Werror` regression
**PASSED locally**. Independent Clang ASan+UBSan compiled regression
**PASSED locally**. Tests cover sentinel return and lazy initialization,
full DWORD flag high-bit preservation, real slot pointer arithmetic, null
array refusal, original initial three-slot capacity, grow 3→6, additive
step growth, signed overflow, capacity-size wrapping, original unneeded but
still required realloc, disabled no-op, null allocation return, partial
writes, injected allocator failures/exceptions, and reentrancy.

The new **actual compiled Phase652→653** test routes original Phase652
`AcquireSlot` into the Phase653 grower and free-list resolver, and also uses
Phase652 `FindParent` and `LinkNode`. In a synthetic guest-memory table it
constructs a three-node tree, grows to **six 12B slots** on the fourth
acquisition, relocates the underlying array, checks all three previous node
values and parent search after relocation, then reuses a slot through
Phase653's recovered free-pointer resolver and original Phase652 free-head
logic. The synthetic rebalancer only blacks the singleton root for a
three-node no-rotation configuration and does **not** represent the full
original `0x2228EDF0` routine.

A Python public `--self-test` uses **invented bytes** for full-body hash
mutation, rel32/branch mutation, and explicit unrelated-DLL rejection. It
never loads or transmits original DLL data.

### Owner-private original retail audit

`python research/tools/audit_original_nested_slot_growth_phase653.py --server ORIGINAL_SERVER_DLL --client ORIGINAL_CLIENT_DLL`

### Public invented-byte regression

`python research/tools/audit_original_nested_slot_growth_phase653.py --self-test`

## Limitations and next priority

`0x2228EDF0` original 1367-byte rebalance and related native rotations are
**not** reconstructed in Phase653, nor is the actual underlying memory
manager or in-game SiN runtime. Source-level CTests, synthetic Win32 PE32
compatibility DLLs, Android NDK compilation, and real Android Bionic `dlopen`
smoke do not show playable Windows retail-game execution, Android levels or
Quest 3 VR. Follow the original rebalance CFG and confirm rotations before
implementing, keeping the user-designated gameplay/AI scope.
