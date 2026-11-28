# Phase 1680: original 121-byte PostNetwork preflight and alias-target resolution

## Whole-original PE32 evidence

This phase examines a COMPLETE, previously opaque, native 2006 PE32 helper: **`0x24197F50`**, exactly **121 executable bytes**, original whole-method SHA256 `27a9fc1f34bf44aa1bda7844dc4276085117fd2b5c2c6239a93f6ef59d9b84e7`. It ends in plain **CDECL `RET0`** at `0x24197FC8`, followed by seven `INT3` padding bytes through `0x24197FCF`. The real callee has one 32-bit pointer stack argument, caller cleanup `ADD ESP,4`, and **zero HIGHLOW relocation sites**.

The original 71-byte Phase1670 comparison wrapper calls it at `0x24199BB6` (E8 relative branch), when the input object `+0x10` byte was zero. In the complete 121-byte helper, the real two native direct E8 calls are self-recursion at `0x24197F84` and Phase1679's newly recovered original 121-byte CDECL record-name lookup `0x24197ED0` at `0x24197FA0`. The current whole SHA authenticates every byte in both parent and child methods. Original `client.dll`/ `engine.dll` full SHA matching remains mandatory for private binary checks; no original game DLL, machine-code body or asset is committed.

### Exact observed data flow

- Entry argument is a linked-list node. The original unconditionally stores byte **1 at node+0x10** *before* testing its **signed record count** at node+0x04. Nonpositive counts still mark the node and proceed toward its next link.
- The node's record-array pointer at `+0x00` is traversed in exact **0x34-byte** record strides. Record type `0` skips all other per-record checks.
- Type **10** always recursively calls the same preflight on `record+0x20` and skips the nonrecursive alias-lookup branch, even if its flag bit is set.
- Other nonzero record types test **byte record+0x12 bit 0x80**. If it is set *and* current node+0x0C points to a next node, call `0x24197ED0` to find the original `record+0x04` name in the **next linked node's chain**. When the returned pointer is nonnull, overwrite **record+0x28** with the matching record address. Null return leaves the alias field unchanged.
- After scanning the current node, follow current node+0x0C **only when the next node's existing status byte at +0x10 is zero**. Already-visited next nodes terminate the traversal; entry does not skip an already-visited starting node.

This resolves the source path that populates **record+0x28**, the alias pointer observed in Phase1672. That dispatcher writes the sequence to the aliased record's `+0x2C` before checking the current record's own sequence. A later dispatcher visit to an aliased record with that sequence may be skipped. This is native source-control-flow evidence, not proof of actual game prediction parity.

## Original-free ABI reconstruction

New `native_original_postnetwork_preflight_alias.h` adds a synthetic `NodeBlock` whose first 16 bytes are the earlier source-authenticated list node and whose visited status byte is preserved at exact offset **0x10**. A compiler-generated **one-argument MSVC x86 CDECL `RET0`** function implements the bounded traversal, type10 child recursion, source low-byte `0x80` test, Phase1679 two-argument native CDECL name lookup and conditional alias write. Host-owned grant lists cover every node, record array and name; safety budgets guard unowned pointers and cyclic type10 recursion. The preflight uses exactly the previously reconstructed name finder, temporarily authorizing each known synthetic record's name for lookup and then restoring that authority.

New `test_original_postnetwork_preflight_alias.cpp` uses real Win32 MSVC-generated native calling conventions and tests: visited flags on root/child/next, source type0 skip, type10 recursion (even with alias bit0x80), two successful alias matches from next list, absent-match pointer preservation, signed zero/negative counts that skip forged record arrays but still mark/follow, already-visited next stop, unknown pointer refusal, and bounded recursive cycles. It then invokes the Phase1672 native typed dispatcher on the same invented records: the record+0x28 alias callback writes a sequence at selected next-record+0x2C, and that later typed record is skipped for the same sequence. This test preserves engine-dependent alias-pointer operations as explicit callbacks and **does not execute original game code**.

`research/tools/audit_sin_original_postnetwork_preflight_alias.py` checks both full retail original DLL SHAs, the entire new 121-byte method and its parent/child method SHAs, exact self/lookup/wrapper rel32 CALLs, complete RET and padding, sparse verified source x86 witnesses, and exact zero relocation count. Public CI fabricates non-executable synthetic fragments and rejects mutations, truncation and forged original hashes.

## Verification boundary

The stacked parent Phase1679 adds two tests targeting **304 native Win32 CTests**; Phase1680 adds another **two** original-free tests, targeting **306** only after Phase1679 and Phase1680 exact-head builds actually pass. The original source-engine object ownership, locale-sensitive name comparator, production prediction interface registration, engine restart behavior, retail gameplay boot and Android/Quest 3 execution remain unresolved. No physics, surfaces, props, model or material/rendering work is introduced.
