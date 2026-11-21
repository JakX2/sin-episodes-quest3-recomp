# Phase 544: Original 2006 SiN client converter red-black tree insertion

## Scope and provenance

This research continues the native `CHudTextureHandle` **type-converter registration**, not HUD rendering. The only source is the user's owner-supplied original 2006 SiN Episodes: Emergence DLL pair, identified by immutable full-file SHA-256:

- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

Only function metadata, control-flow facts, independently authored pseudocode, and tests are committed. Original binaries, raw disassembly, in-game files, extracted assets, and source-code guesses are excluded. Every C++/semantic name in this report is descriptive; original Ritual C++ and PDB symbols were **not** recovered.

**Key correction to Phase 542:** the earlier converter lookup correctly identified ordered 24-byte binary-tree records, but it did not know the tree was *red-black* or that insertion runs a rebalancing algorithm. Both facts are now supported by original color writes and mirrored rotations. Lookup remains useful; insertion is not simply a bare linked node.

## Exact original methods authenticated

| Entry VA | Full bytes | Complete-method SHA-256 | Proven responsibility |
|---|---:|---|---|
| `0x2400AB30` | 103 | `ebe740c4da64fa6d47de6a8b28ac58f1db8c8be9d587cc8f2bcce41b45fd6da4` | Allocate/reuse an index for a 24-byte converter record, link it and increment active count |
| `0x2400ABA0` | 230 | `8b2efe3b45eec69c08f1d8f75a3b6c170313d0edde71925108401fb8f9f4be27` | Find parent and child side for insertion using descriptor comparator |
| `0x2400B920` | 106 | `375dff84a7a4a8c27ebdd19eeda1425e080ada46ced58647a00b9bdba8bddd7c` | Grow/reserve 24-byte record storage, including allocator/reallocator calls |
| `0x2400B4B0` | 1,112 | `af5123bc8f0d2f047038c6a32f74d03d878790e701568c1ace27239fb4eb9090` | Shared red-black insertion fixup and recoloring, root forced to color 1 |
| `0x2400BF00` | 150 | `3253db893036184ec1cda9a2b7e95e2effde85390047c069c3563ee5b6a1d2f6` | Test whether an indexed node is its parent's smaller child |
| `0x2400BFA0` | 152 | `33a21a59958be43bfdd5fecd4b78b1e80896e054331d2f7cad62e09891f61bd3` | Mirrored test whether an indexed node is its parent's larger child |
| `0x2400C120` | 337 | `ea91794a6ef125d786b4cc71ffe94b683f974be4ee3740c1d782b75298aac37e` | Rewire parent/child links for left rotation |
| `0x2400C280` | 338 | `eee663603b4c43da589506e883ee9303863e87b75357e166156e28ebf877645e` | Mirrored right rotation |

**Eight complete new methods, 2,528 non-overlapping original instruction bytes.** Every method ends at a checked original `INT3` boundary and has a separately checked whole-method hash. The previously authenticated `0x24009F40` 128-byte converter insertion caller is checked again but **not** double-counted.

**Important distinct region:** `0x2400B230` contributes **89 authenticated bytes** in the `allocate/reuse` path. It initializes and links the record, but all three branches execute `JMP 0x2400B4B0`. This is *not* a self-contained function ending in `ret`; its 89 bytes are reported as a **separate shared-tail entry block** and excluded from the 2,528-byte full-method total. The original branch labels and linked byte range are independently checked. Counting this as an independent full function would overstate completeness.

## Original layout, now substantially resolved

The earlier 24-byte record layout becomes:

| Record offset | Authenticated meaning | Evidence |
|---|---|---|
| `+0x00` | Smaller-child node index | Comparator path; insertion-link updates; right rotation |
| `+0x04` | Larger-child node index | Comparator path; insertion-link updates; left rotation |
| `+0x08` | Parent node index | `0x2400B23D`; reparent during rotations |
| `+0x0C` | Color bit/dword: freshly linked nodes `0`, root `1` | `0x2400B24D` and `0x2400B900` |
| `+0x10` | Unsigned 16-bit token; initialized `0xFFFF` before publication | Phase 542 insertion caller; `0x2400AB72` |
| `+0x14` | 32-bit converter reference | Earlier Phase 542 caller `0x24009FB5` |

Each array record is `base + 24 * index`; absent root/parent/children use signed index `-1`. The static native fallback record at `0x2438AFA0`, initialized via flag `0x2438D130`, is *not* another normal array record. A 2006 PE32 pointer must not be used as a native Android or x64 address.

The registry/container fields observed in its allocator are:

| Native offset | Observed role |
|---|---|
| `descriptor+0x04` | Pointer to native record array |
| `descriptor+0x08` | Capacity-related field consulted by allocation |
| `descriptor+0x10` | Tree root index |
| `descriptor+0x14` | Active record count; incremented after insertion |
| `descriptor+0x18` | Head index of free/reusable records, `-1` when none |
| `descriptor+0x1C` | High-water-mark / next unallocated index |
| `descriptor+0x20` | Stored record-array pointer after allocation |

The exact invariant between the two capacity-related fields at `+0x08` and the backing array manager remains an integration question. Do not conflate free count, element count, and capacity merely because a conventional STL type looks similar.

## `0x2400ABA0`: find insertion parent, not exact-match lookup

This routine is passed descriptor, pointer to 16-bit query key, writable parent-index output and writable side flag. It initializes parent to `-1` and side to `0`, then traverses from the root at `descriptor+0x10`. At each valid node, it writes the current node index to the parent output, passes the two token pointers to the comparator at `descriptor+0`, and chooses the child based **only on `query < current`**. A true comparator result sets flag to 1 and follows `node+0x00`; otherwise flag is 0 and it follows `node+0x04`. It stops at `-1`. Native ABI returns via `ret 0x10`.

This differs from earlier exact-match `0x2400A4D0`, which invokes the comparator twice to distinguish `<`, `>`, and `==`. `0x2400ABA0` does **not** independently reject duplicate values. Equality chooses the larger-child path. The original registration caller had already performed a duplicate lookup, so the two functions complement each other.

```cpp
// Bounded semantic reconstruction of the original core.
// The caller must provide a valid native descriptor and comparator.
parent_index = -1;
insert_on_smaller_child = false;
for (int index = tree.root; index != -1;) {
    parent_index = index;
    const auto& n = tree.entries[index];  // exact stride 24
    insert_on_smaller_child = (query_token < n.token);
    index = insert_on_smaller_child ? n.smaller_child : n.larger_child;
}
// Outputs are consumed by 0x2400AB30. This is NOT an exact-token lookup.
```

No finite-iteration or invalid-pointer guard was observed in the native function; host reference guards are deliberately independent safety features.

## `0x2400AB30`, `0x2400B920`, and the shared node-link entry

`0x2400AB30` reads the reusable-node head from `+0x18`.

- If head is `-1`, it compares a high-water mark with an allocation limit. When equal, it invokes `0x2400B920` to ensure another node slot exists. It then uses the old high-water index and advances `+0x1C`.
- If the head is not `-1`, it follows the free-list link through `0x2400AAE0` and stores the updated head back to `+0x18`, reusing the old head index.
- It computes the selected record's token address at `base + 24*index + 0x10`, writes the 16-bit invalid token `0xFFFF`, and updates a cached native array pointer at `+0x20`.
- It calls `0x2400B230` with the new index and the parent/side outputs from `0x2400ABA0`. Then it increments the active record count at `+0x14` and returns the allocated index via `ret 8`.

`0x2400B920` implements growth for a 24-byte array. The native routine handles an initial two-entry capacity, an additive growth parameter or a doubling branch, and calls original memory helpers `0x2416F8F0` (allocate) or `0x2416F960` (resize). The multiplication by 24 is explicit, not inferred from unrelated SDK code. It is dependent on a native capacity-mode field; full cleanup/allocator failure behavior is **not** established.

The **89-byte shared-tail entry** at `0x2400B230` initializes `node+0x08=parent`, `node+0x00=-1`, `node+0x04=-1`, `node+0x0C=0`. When parent is absent it sets the root to the new index. Otherwise it writes the new index into one of the two parent-child fields using the side flag. Every branch jumps to `0x2400B4B0` instead of returning locally.

## `0x2400B4B0`: red-black maintenance and rotations

The `1,112`-byte whole method is a real rebalancing procedure, not a generic post-insertion callback. It accesses `node+0x0C` color values, tests parent/grandparent relationships, traverses from the newly inserted index, recolors nodes, and invokes either mirrored rotation method. The original calls within its complete body are:

- `0x2400BF00` at two call sites: test whether a node is the smaller child.
- `0x2400BFA0` at one call site: mirrored larger-child test.
- `0x2400C120` at two call sites: left rotation.
- `0x2400C280` at two call sites: right rotation.

It forces the final root's color at `root_node+0x0C` to **1**, after handling internal recolor paths where 0 and 1 are exchanged. Those operations match a **red-black insertion-fixup algorithm**, with zero representing the newly inserted red state and one the black root state. The two orientation helpers test the original parent-child pointers, including the native sentinel case. The rotation functions themselves update both child pointers and parent references; the tree root is updated when the rotated node was the root.

```cpp
// Intent-level reconstruction. Exact sentinel/global accesses remain
// represented by native helper callbacks rather than guessed class members.
void InsertLinkedNodeAndFixup(int new_index, int parent_index, bool smaller) {
    node[new_index].parent = parent_index;
    node[new_index].smaller = node[new_index].larger = -1;
    node[new_index].color = 0; // newly linked red
    if (parent_index == -1) root = new_index;
    else if (smaller) node[parent_index].smaller = new_index;
    else node[parent_index].larger = new_index;

    // Original 0x2400B4B0: check parent color, inspect uncle/grandparent,
    // recolor or rotate in mirrored cases. The exact native sentinel
    // and memory helper behavior must be retained by the final host.
    NativeRebalanceAfterInsert_2400B4B0(new_index);
    node[root].color = 1; // black root, original 0x2400B900
}
```

Unlike the pseudocode above, the original assembly includes many inline checks on the lazily initialized static sentinel, with global flag at `0x2438D130`. The simplified source sketch is *not* a claim that all sentinel paths have been translated. The full original **method bytes and call sites** are authenticated, while exact source equivalence of the full rebalancer has not yet been proved through exhaustive behavioral replay. This distinction matters when measuring decompilation progress.

## Independent source reconstruction and tests

The existing `include/sin_compat/original_client_converter_tree.hpp` is updated with proven `parent_index` (+8), `color` (+12), and the new `locate_original_converter_insert_slot()` branch order. This function models the insertion-slot search, **not** the yet-unported red-black fixup or the memory allocator. The old exact-match lookup remains available.

The independent `tests/test_original_client_converter_insertion.cpp` asserts the 24-byte layout, both child choices, empty-root state, duplicate-key goes to larger branch, and malformed/cyclic host-side inputs. The host's index bounds/cycle protection is a modern safeguard and is not attributed to original x86 behavior. GCC and Clang strict C++17 local builds pass. No original DLL is linked into these tests.

`research/tools/audit_sin_original_client_converter_insertion.py` checks both original full-file hashes, eight whole-method SHA-256s, separately hashed 89-byte shared-tail entry block, **24 original direct CALL rel32 edges**, **three** original near-JMP rel32 edges into the fixup, 29 exact instruction witnesses, and exact original function boundaries. Original-bin audit and ten independent synthetic CALL/JMP corruption cases pass. Source review uses original GCC/binutils disassembly locally; original instruction bytes are not committed.

## Open dependencies

1. Fully map the native sentinel initialization and additional color/fixup branches for a byte-faithful source-level rebalance routine, preferably with synthetic caller-state replay.
2. Finish helper ownership and allocation failure behavior for `0x2400B920`, including the meaning of the capacity mode at `+0x08`.
3. Original token-string lookup at `0x241D3E20` and token publication at `0x241D49F0`, which close the remaining `CHudTextureHandle` interning chain.
4. None of this establishes a replacement native `client.dll`, HUD drawing, original engine execution or Quest 3 gameplay. Decompilation-first scope remains in force.