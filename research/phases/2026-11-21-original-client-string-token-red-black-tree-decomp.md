# Phase 545: Original 2006 SiN client string-token red-black tree

## Scope and source identity

This decompilation follows Phase 542's original `CHudTextureHandle` converter-name intern function `0x241D4A80` into its previously unresolved string-index lookup and token publication routines. The original data structures are **not** a single registry: the type-converter tree (Phase 544) is a 24-byte node array using 32-bit indexes, whereas the *underlying string-token tree* examined here is a distinct 12-byte node array with 16-bit indexes.

The only source is the owner's original SiN Episodes: Emergence 2006 PE32 client and server pair, authenticated independently by whole-file SHA-256:

- client.dll `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- server.dll `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

The executable's instruction bytes are not stored in GitHub. All method names and C++ struct names here are independently descriptive, not original Ritual source/PDB symbols.

## Authenticated complete original methods

| Original VA | Bytes | Full-method SHA-256 | Native control-flow role |
|---|---:|---|---|
| `0x241D3750` | 261 | `5cc99b17ccadb013b2f921b34d7f237a68c8fdd988bab0e98226797010c12ed5` | Select insertion parent and side for a token key |
| `0x241D3900` | 86 | `9b74307d379f272da366ba0b6d6ce0c8f486dbd23744783f101446acd14cb14b` | Allocate/reuse a 16-bit node index |
| `0x241D39F0` | 235 | `5b96eff8753dab4996f3b011a5e4bbd6d8f6ccbfec341d22c9833d94d3759b0a` | Tree rotation using larger child |
| `0x241D3AE0` | 238 | `70f1138c52c9aa4ca3a4ce23218620b65935fced6026547bf222ce8a1365e572` | Mirrored tree rotation using smaller child |
| `0x241D3E20` | 268 | `f0b5fefed68fcc281f2d2105046372d02fcf58a064745897c6ce376dbcc5fdc8` | Ordered two-comparison lookup, returns 16-bit token or `0xFFFF` |
| `0x241D40D0` | 1,387 | `3a2c9861ab0ec6917322c25a631f1a7641d1606d0fbfe19d6957c644508d88d1` | String-token red-black insertion fixup and root coloring |
| `0x241D4740` | 109 | `9da5ebfd31dde84f40dbb9c53852824e085b2b2ad5958e5b908d1dca83a03a8a` | Initialize a token node, attach to a parent/root, call the fixup |
| `0x241D49F0` | 94 | `7ca27d4cc81fb20c1576b8289ed71e4d7f1bf889baef72018a811357a799f52d` | Publish newly interned token and payload |

**Eight newly authenticated complete original methods, 2,678 non-overlapping bytes**, all with full-method SHA-256 and original `INT3` boundaries verified. The 261-byte original caller `0x241D4A80` from Phase 542 is rechecked to establish the exact linkage but is *not* counted again.

## Verified 12-byte token-node representation

The original addresses records as `base + 12 * uint16_t(index)` and uses **`0xFFFF`** instead of the converter tree's 32-bit missing index.

| Record offset | Native representation | Evidence |
|---|---|---|
| `+0x00` | 16-bit smaller-child index | Search path and link helper |
| `+0x02` | 16-bit larger-child index | Search path and link helper |
| `+0x04` | 16-bit parent index | Link helper and mirrored rotations |
| `+0x06` | 16-bit color, freshly created `0`, root forced to `1` | `0x241D476A`, `0x241D4632` |
| `+0x08` | 32-bit payload pointer, the original string data address | Original `0x241D4A39` publication store |

Additional native container fields in this build:

- `this+0x00`: callback function pointer used for comparator queries. Exact comparison semantics and precise callable signature are not assumed; the callback is invoked *twice with reversed value order* by the lookup method.
- `this+0x04`: backing 12-byte node array pointer.
- `this+0x10`: 16-bit root index.
- `this+0x12`: 16-bit active-token count, incremented by the publishing routine.
- `this+0x14`: 16-bit head of the free/reusable-index chain.
- `this+0x16`: 16-bit next-unallocated-index / high-water count.

The native sentinel storage at `0x2437704C` and init flag at `0x24377054` handle missing-node behavior; their exact thread-safety and lifecycle semantics have not been proven. The 2006 struct and register ABI are not transplanted into modern engine code.

## Two distinct token search operations

### `0x241D3E20` existing-token lookup

The method reads the 16-bit root from `this+0x10`. If the root is `0xFFFF`, it returns that missing token without invoking the comparator. Otherwise it reads each candidate payload at `base + 12*index + 8` and performs the **native callback twice**, only if needed:

1. First `less_than(query, node_payload)`: when true, follow `node+0x00`.
2. Otherwise `less_than(node_payload, query)`: when true, follow `node+0x02`.
3. Otherwise neither is smaller, so return the current 16-bit token as the match.
4. If the index becomes `0xFFFF`, return the missing token.

The original calls are indirect at `0x241D3E58` and `0x241D3EBF`. They are verified as native `CALL [edi]` instructions. This establishes a **strict weak-order comparator-driven binary-tree lookup**, not an ordinary linked-list string scan. The precise behavior of the original comparator on case, locale, and invalid strings is an open dependency.

### `0x241D3750` insertion-position scan

The insertion scan initializes a 16-bit parent output to `0xFFFF` and a byte side output to `0`, then follows nodes from the root using **one comparator call** per node (`0x241D3794`). It returns the last parent and the chosen child side. Equal comparator values take the larger-child branch; this helper itself does not identify duplicate strings. The earlier lookup is responsible for suppressing duplicates when called through the normal intern path.

```cpp
// Independent bounded pseudo-C++. `less` represents the original
// context-bearing comparator, not a guessed strcpy/strcmp ABI.
uint16_t FindExisting(const Tree& t, const Key& query, Less less) {
    uint16_t node = t.root;
    while (node != 0xFFFF) {
        const Key& stored = *t.entries[node].payload;
        if (less(query, stored)) node = t.entries[node].smaller;
        else if (less(stored, query)) node = t.entries[node].larger;
        else return node;
    }
    return 0xFFFF;
}

void FindInsertSlot(const Tree& t, const Key& query, Less less,
                    uint16_t& parent, bool& smaller) {
    parent = 0xFFFF; smaller = false;
    for (uint16_t node = t.root; node != 0xFFFF;) {
        parent = node;
        smaller = less(query, *t.entries[node].payload);
        node = smaller ? t.entries[node].smaller : t.entries[node].larger;
    }
}
```

These are readable control-flow models. The original comparison call passes additional context/arguments, and its real pointer ownership remains native; no reconstructed C++ pointer is asserted ABI-compatible.

## Node allocator, link, fixup and final publication

`0x241D3900` uses the 16-bit free-list head at `+0x14`. If non-empty it obtains the next free link by calling `0x241D33E0`, then returns the removed index. If empty, it compares the high-water index at `+0x16` to a storage limit, grows the backing storage via `0x241D34E0` when required, then returns the old high-water index while advancing the 16-bit counter. No cross-platform allocation behavior is inferred.

`0x241D4740` constructs the link state for a new index: `smaller=larger=0xFFFF`, `parent=provided index`, `color=0`, then sets the new index as either the selected parent's `smaller`/`larger` child or the tree's root. Its **three branches** each call original `0x241D40D0`. Native return convention is `ret 0x0C`.

`0x241D40D0`, authenticated over its complete **1,387-byte** body, handles red-black rebalancing and color changes. Its direct CALLs connect twice to the `0x241D39F0` rotation, twice to the mirrored `0x241D3AE0`, and four times to the native token-index dereference helper `0x241D33E0`. The original final root receives color value **1** at the 16-bit record offset `+0x06`. The routine uses sentinel globals and low-level memory operations not yet normalized into a fully equivalent portable implementation. The directly observed color and rotations support a **red-black tree** interpretation, not just a heuristic matching Source SDK conventions.

`0x241D49F0` calls `0x241D3750` to find the insertion position, `0x241D3900` to obtain a free/next index, and `0x241D4740` to link/rebalance the resulting node. It then increments `this+0x12` as a 16-bit count, stores the original string payload pointer at `base + 12*index +8`, and returns the 16-bit node index. That return feeds `0x241D4A80`'s existing intern-string path.

## Independent original-free C++17 reference

`include/sin_compat/original_client_string_token_tree.hpp` captures the original 12-byte native field widths and offsets, while `tests/test_original_client_string_token_tree.cpp` exercises source-grounded lookup and insertion-slot branch ordering with a **synthetic** less-than callback. GCC and Clang strict C++17 compilations pass. Cycle and out-of-bounds rejection are conservative *port-side safety*, not a claim about the original x86 code.

These functions are intentionally limited to searchable tree branches and the verified 12-byte record layout. They do **not** masquerade as a complete original RB-tree allocator, tree-rotation implementation, string comparison engine or game DLL.

## Reproduction and proof boundary

Run the synthetic parser/code-identity self-test:

`python research/tools/audit_sin_original_client_string_token_tree.py --self-test`

Verify the owner's original DLL pair:

`python research/tools/audit_sin_original_client_string_token_tree.py --bin-zip /private/owner/bin.zip`

The verifier checks **eight complete method SHA-256 digests, 2,678 distinct instruction bytes, 36 original direct-call edges, three indirect comparator-call sites, and 30 detailed opcode/field witnesses**, plus both original DLL full-file hashes and the parent intern routine from Phase542. Local runs passed both paths. It never exports game executable bytes.

This phase is **not** a playable original `client.dll`, map startup, HUD renderer, Android build, or VR game. The project remains decompilation-first, with later host integration explicitly deferred.

## Further native decompilation leads

- `0x241D3BF0` and `0x241D3C90`: comparator callback variants selected during string-token table construction. Their exact case/locale/string semantics must be proved, not guessed.
- `0x241D33E0`, `0x241D34E0` and the native string-page allocator: resolve storage growth, `0xFFFF` token exhaustion, and free-list reclamation.
- Compare original byte-faithful rebalancing against a source-level red-black algorithm with explicit native sentinel behavior. Whole-method hashes alone do not prove semantic equivalence.