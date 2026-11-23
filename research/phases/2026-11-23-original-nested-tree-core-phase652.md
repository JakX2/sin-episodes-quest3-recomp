# Phase652: Original 2006 SiN nested tree parent search, node acquisition and link

## User priority

Recover missing original SiN Episodes: Emergence gameplay/entity name registration and
NPC-specific functionality. Owner already ported original maps, models and props into
Half-Life 2, so **do not** spend work on generic HL2-equivalent physics, surfaces,
materials, model/prop loading or rendering.

## Private retail binary authentication

Derived directly from owner-provided genuine 2006 retail `bin/server.dll` and `bin/client.dll`.
Both exact full DLL SHA256s were checked:

- server.dll: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client.dll: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

Three **complete original functions, 456 executable bytes**, authenticate by
whole-method SHA256 and immediate original following INT3 byte:

| Original VA | Bytes | SHA256 of entire original method |
|---|---:|---|
| `0x2228E470` parent/branch search | 261 | `52909d48c6bde81a72b0e10e3f871e8d13de62ab03ca3a3aa2ffe6b145f3b871` |
| `0x2228E620` 12B node acquisition | 86 | `9b74307d379f272da366ba0b6d6ce0c8f486dbd23744783f101446acd14cb14b` |
| `0x2228F460` node link and rebalance dispatch | 109 | `9da5ebfd31dde84f40dbb9c53852824e085b2b2ad5958e5b908d1dca83a03a8a` |

The independent owner-only auditor checks **eight original rel32 call edges**,
**fourteen native branches**, **42 address-pinned instruction witnesses**, the
native indirect comparator call `FF 55 00`, executable `.text` ownership and
all three following `INT3` boundaries. Eight rel32 edges include **three
actual Phase651 insertion calls**, two Phase652 acquire/grow/free-slot calls,
and three rebalance dispatches. Retail DLL images are never published.

Public auditor: `python research/tools/audit_original_nested_tree_core_phase652.py --self-test`.
Private owner invocation:
`python research/tools/audit_original_nested_tree_core_phase652.py --server /private/server.dll --client /private/client.dll`.

## Original function semantics reconstructed

### Parent and insertion direction `0x2228E470`

Native `__thiscall ECX=map`, stack `(key_ref*, out_WORD_parent*,
out_BYTE_branch*, comparator_context*)`. Initialize outputs to `FFFF` and zero,
and return immediately for root `WORD[map+0x10]==FFFF`, without initializing the
sentinel. Otherwise, for each visited node index, store index in the parent's
WORD, invoke the original **cdecl** comparator pointer at `[map+0]` with
`(key_ref, node+8, comparator_context)`, and write comparator's AL into the
branch output. AL!=0 chooses child WORD+0 (left); zero chooses WORD+2 (right).
Nodes have **12-byte stride**.

On first nonempty traversal, lazily initialize static sentinel fields
`WORD[0x2259012C]=FFFF`, `WORD[+2]=FFFF`, `WORD[+4]=FFFF`,
`WORD[+6]=1`, while preserving other bits of the DWORD flag
`[0x22590134]` and setting bit0. The next traversal uses the cached initialized
sentinel. Note that this is an insertion-parent search, **not** Phase650's
two-comparison equality-detecting lookup. Equal values descend right here.

### Slot acquisition `0x2228E620`

If free-list head `WORD[map+0x14]` is `FFFF`, compare next unused `WORD[map+0x16]`
with DWORD capacity `[map+8]`. On equality, call native **`0x2228E200`**
with ECX=map+4 and increment=1. Return the next unused WORD index, refresh
DWORD cache `[map+0x18]` from `[map+4]` and increment next-unused WORD by one.

If free-list head is not `FFFF`, call original **`0x2228E100`** with ECX=map
and argument=free-head WORD. Read the returned node pointer's WORD+2 as the next
free head; refresh cache `[map+0x18]`; update head `[map+0x14]`; return recycled
index. Do not increment next-unused index on recycled allocation.

Those two original callees remain **mandatory typed host dependencies**. This
phase reconstructs the original branch decision, cache/field writes and
free-list effect, without claiming to implement the slot grower or resolver.

### Node initialization and linking `0x2228F460`

Native `__thiscall ECX=map`, stack `(new_slot_WORD, parent_WORD, branch_BYTE)`.
On `node = DWORD[map+4] + new_slot*12`, write in original order:

1. `WORD[node+4] = parent`.
2. `WORD[node+2] = FFFF` (right child).
3. `WORD[node+0] = FFFF` (left child).
4. `WORD[node+6] = 0` (new node is red).

If parent `FFFF`, write map root WORD+0x10; otherwise write new index into
parent's child WORD+0 on true/left branch or WORD+2 on false/right branch.
Finally **always call original `0x2228EDF0`** rebalance with ECX=map and
argument=new slot; no direct source reimplementation of its 1,367-byte body
is claimed. Host must supply it.

## Verification

- Standalone GCC C++17 strict warning + `-Werror` compiled regression passes.
- Independent Clang ASan+UBSan compiled regression passes.
- Synthetic guest-memory regression checks empty/nonempty parent search, left
  and right choices, lazy sentinel write, fresh node versus free-list reuse,
  capacity-grow trigger, parent/child backlink fields, red node before callback,
  rebalance dispatch, host failures, exceptions and protective cycle/reentrance
  guard. Host callback only blackens singleton root, **does not implement
  general native red-black rebalancing**.
- Separate combined regression *source* wires actual Phase651 `TreeInsert`
  callbacks into three Phase652 classes. It is compiled and executed under
  repository CI against the real Phase651 library, not described as local
  original in-game code execution.
- Original retail method SHA audit passes against private unmodified PE32 DLLs;
  synthetic mutations do not embed original code.
- Win32 native MSVC, Linux C++17 and Android NDK targets added.

## Boundaries and next targets

- Actual rebalance `0x2228EDF0` (1,367 original x86 bytes), its original
  rotations, the original 12B slot grower `0x2228E200` and slot pointer resolver
  `0x2228E100` remain unreconstructed. No synthetic stub should be represented
  as the original implementation.
- Passing reference tests, Android module `dlopen` smoke and Win32 synthetic
  factory tests are **not** proof of the real retail game's process running,
  in-level NPC behavior, Android gameplay or playable Quest 3 VR.
- Continue missing SiN gameplay/AI and entity registration, not HL2-equivalent
  maps, props, physics, rendering or materials.
