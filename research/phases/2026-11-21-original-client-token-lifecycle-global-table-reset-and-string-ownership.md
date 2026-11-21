# Phase 548: Original client global string-token lifecycle, reset and symbol lookup

## Authentic original 2006 source and limits

This research was performed against the owner's *exact*, unchanged `SiN Episodes: Emergence` PE32 DLL pair, with whole-file SHA-256 checks:

- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

The previous Phase 547 established the 12-byte node-array accessor and growth plus string-page allocation list. Phase 548 follows the original **global token table**, its **cleanup**, the token-to-C-string resolver, and the exact branch ordering in the **already-authenticated** `0x241D4A80` intern routine. It also proves that the singleton default comparator is **case-sensitive**, unlike the distinct HUD converter registry's Phase546 **case-folded** initial comparator.

This is a bounded, independently written decompilation report. Names such as `GlobalSymbolTable` are **descriptive**, not original recovered Ritual source symbols. No game executable bytes, model data, or complete disassembly are published.

## Six newly authenticated full original functions

| Original client VA | Exact bytes | SHA-256 of complete original method | Verified native role |
|---|---:|---|---|
| `0x241D3D50` | 51 | `779f7fdb02b3897d68b0e5656ebf7807e7c0d94a4bc3b38d633e1b84469d66cc` | Resolve 16-bit string token into a page+offset C-string, missing token -> static empty string |
| `0x241D3F30` | 132 | `ec8181c826014919010e06ace20c83b2388f0311a3dbf0ab66507226078ee304` | Convert allocated token nodes back into a free-index chain, reset root/count |
| `0x241D4000` | 83 | `3d425e64b29a356d539f8fdbc88a5aae72deb4893878881a5efbd2e` | Clear/release 4-byte page-pointer list according to original grow mode |
| `0x241D47B0` | 151 | `662c71b540086a90b25970a1881f89261864234a765f2dd04bab34681de34bb6` | Lazily allocate and initialize the **global** token table, bytewise comparator |
| `0x241D4940` | 93 | `2a5f32bd203b6987c8582d1ced523f799368065a79d27bac3a256f802cde6964` | Clear token tree, free each string-page block, release pointer list and owned node array |
| `0x241D4B90` | 43 | `4f487899621b4710f1488b4037d200ca521ebe6a3fe09a615aea768af35384a6` | Wrap singleton token interning for an output token value |

**Six new, non-overlapping complete original methods: 553 instruction bytes.** All have the original first post-method `INT3` byte at their independently verified exclusive end boundary. The original 261-byte intern function `0x241D4A80`, previously SHA-authenticated in Phase542/545, was reverified to establish call linkage but is **not counted a second time**.

## The global token table initializes in a different mode from the HUD converter table

`0x241D47B0` tests the original byte flag `0x24377058`. On the initial path, it requests a **`0x34` (52-byte)** token-table object through original `0x2416FCF0`. If this request succeeds, it sets the fields and allocates a first **`0x180` (384-byte)** node-buffer block through `0x2416F8F0`: that is **32 records × 12 bytes**.

The method then initializes the table's native fields:

- callback at `+0x00`: null initially, then original **`0x241D3BF0`** (bytewise/case-sensitive), unless preexisting;
- node-array pointer at `+0x04`, node capacity at `+0x08` = `32`, growth mode at `+0x0C` = `0`;
- 16-bit root index at `+0x10` = `0xFFFF`, active-count `+0x12` = `0`, free-list head `+0x14` = `0xFFFF`, high-water `+0x16` = `0`;
- cached node pointer at `+0x18` equals the allocated node buffer;
- separate string-page pointer-list at `+0x1C`, capacity at `+0x20`, growth mode at `+0x24` = `8`, active page count at `+0x28` = `0`, additional `+0x2C` = `0`.

The singleton pointer is stored to global **`0x24377044`**, and a separate byte **`0x24377058`** is set to 1. The method also has a failed initial object-allocation path that writes a null singleton pointer but **still marks initialization attempted**. It is therefore incorrect to assume the engine automatically retries on that failure; the original call site's null handling has not been established as safe.

**Important distinction established by two native initializer chains:**

| Original native token table | Address | Proven initial comparator |
|---|---|---|
| Global symbol token table | `0x24377044` pointer, created by `0x241D47B0` | Case-sensitive bytewise `0x241D3BF0` |
| HUD converter-name token table | object `0x2432289C`, created by `0x24262410` | Case-folded `0x241D3C90`, as verified in Phase546 |

This is **not** a contradiction: the tables are distinct global objects with distinct constructors. The original code does not establish that either comparator was never changed later or which locale-folding path actually ran on an owner PC.

## `0x241D4B90`: global string token wrapper

This 43-byte wrapper calls the original singleton initialization at `0x241D47B0`, reads original global `0x24377044`, then calls the already-authenticated 261-byte token-intern method **`0x241D4A80`**. It copies the returned unsigned 16-bit token value into the caller's token wrapper object. It does **not** return or store a pointer to the original string as a token.

This proves the global symbol wrapper uses the **global case-sensitive table**, not the separate HUD converter's folded table. It is binary call-site evidence, not a claim that every client identifier in SiN uses case-sensitive matching.

## Previously verified `0x241D4A80`: now annotated through its storage path

This method is **not newly counted**, but is now understood through its dependencies from Phases 545–548:

1. For null input string, write token **`0xFFFF`** to the output and return. No string allocation.
2. Otherwise set a transient string reference at table `+0x30`, initialize a two-word temporary token descriptor to `(0xFFFF,0xFFFF)`, and search the existing-token red-black tree with original `0x241D3E20`.
3. If the comparator finds an existing token, return that token; no new page allocation.
4. For a new string, compute `strlen(input)+1`, including terminating NUL.
5. Call the original **`0x241D3D10`** to find an existing page with `capacity-used >= requested`. If none, request a new block of **`max(requested, 0x800) + 11` bytes** from original `0x2416F8F0`, store `page+0 = max(requested,0x800)` and `page+4 = 0` and append its pointer using original **`0x241D4060`**.
6. Set the new token descriptor's 16-bit page index and 16-bit byte offset from the page's current `used` value. Copy the original string bytes into `page + 8 + used` using the original bulk/remaining-byte copy loop.
7. Add `strlen+1` to the **full 32-bit** used-byte field at page `+4`.
8. Call the original **`0x241D49F0`** to insert a new 12-byte token-tree node and publish the descriptor; copy the resulting 16-bit token into the caller's output.

Readable bounding pseudocode for the safe, normal-memory path:

```cpp
uint16_t InternString(TokenTable& t, const char* text) {
    if (!text) return 0xFFFF;
    auto existing = FindExistingToken(t, text);
    if (existing != 0xFFFF) return existing;
    uint32_t bytes = strlen(text) + 1;
    int pageIndex = FindPageWithSpace(t.pages, bytes);
    if (pageIndex == -1) {
        auto capacity = max<uint32_t>(bytes, 0x800);
        pageIndex = AppendNativePage(t.pages, capacity + 11);
    }
    auto& page = t.pages[pageIndex];
    NativeTokenDescriptor desc{uint16_t(pageIndex), uint16_t(page.used)};
    memcpy(page.data + 8 + page.used, text, bytes);
    page.used += bytes;
    return PublishTokenNode(t, desc);
}
```

The above is **illustrative control-flow pseudocode**, not memory-safe original behavior or exact Ritual C++. Original null allocation handling, 16-bit offset truncation, and x86 overflow branches are not demonstrated as safe. The original extra `+11` allocation padding is **observed**, but the intended reason is not proven; don't label it as a known alignment guarantee. In particular, the page's used count is 32-bit while its descriptor offset is 16-bit. This is a real possible truncation boundary worth checking against actual original gameplay assets before claiming all strings fit.

## `0x241D3D50`: token -> string address

An original `0xFFFF` token returns the address **`0x242D330E`**, which is a real, static **empty C-string** in original read-only data, not a null pointer. For other tokens, the function dereferences the token-table's 12-byte node at `tokenArray + 12*index` and reads the `uint16_t` page index and `uint16_t` byte offset stored at node `+8` and `+10`. It looks up `pagePointerArray[pageIndex]` and returns `page + 8 + offset`.

The string-page allocation, token descriptor and accessor now agree about the two 16-bit descriptor fields. The original function has no independently proven bounds checks for invalid token/page indices. The independent C++ reference deliberately fails closed on invalid synthetic inputs instead of guessing native crash behavior.

## `0x241D3F30`: reset all allocated nodes into a free chain

This function reads the high-water/ever-allocated 16-bit node count at token table `+0x16`. From the highest used index down to zero, it stores:

- the node's **larger-child word at `+2`** = the previously visited free-node index;
- the node's **smaller-child word at `+0`** = the current node index.

It finishes with original root index `+0x10 = 0xFFFF`, active-token count `+0x12 = 0` and free-head `+0x14 = 0` when at least one node existed, otherwise `0xFFFF`. It does **not** free memory and does **not** reset every node's parent, color or payload. The previously recovered `0x241D3900` allocator can then follow the free list from the stored next index.

Pseudocode for the verified finite-index branch:

```cpp
uint16_t next = 0xFFFF;
for (int i = t.highwater - 1; i >= 0; --i) {
    t.nodes[i].larger = next;
    t.nodes[i].smaller = uint16_t(i);
    next = uint16_t(i);
}
t.root = 0xFFFF;
t.activeCount = 0;
t.freeHead = next;
```

Original method also uses the same shared global missing-node sentinel lazy initializer from Phase547; the global initial-call flag is not proof of thread safety. The independent host reference tests 0, 1 and 3 allocated nodes, preservation of untouched fields, and rejects an out-of-bounds high-water counter.

## `0x241D4940` and `0x241D4000`: real cleanup sequence

The parent cleanup method at `0x241D4940` first calls the just-recovered free-list reset `0x241D3F30`, then iterates the existing string-page pointer table **`this+0x1C`** for `this+0x28` pages and calls original native free `0x2416F940` once for **each page pointer**. It sets page count to zero and calls the distinct 83-byte array cleanup method `0x241D4000` on `this+0x1C`. The latter also clears the list count and releases its **backing pointer array** when the grow-mode is not `-1`, keeping the cached pointer in sync.

Finally the parent method checks token-node array mode at `this+0x0C`. Unless it is `-1`, it conditionally releases original node storage `this+0x04` and clears node-array capacity at `this+0x08`. These are **separate layers of ownership**: each allocated string page, the page pointer array, and the token node array.

The null handling and exact external ownership rules remain unverified. Calling the routine on corrupted container fields or concurrently with other users of the token array has no proven safe outcome.

## Independent reference, original-binary audit, and remaining work

`include/sin_compat/original_client_token_lifecycle.hpp` reconstructs a bounded reset/free-chain algorithm and the token descriptor-to-C-string lookup over **synthetic** pages. It also documents the global singleton object size 52 and its first 32 × 12-byte node allocation. The new test suite validates root/count/free chain, preservation of untouched native fields, the missing token empty string, page/offset resolution, and conservative invalid-index/string rejection. It never links the original DLLs or allocates executable game objects.

`research/tools/audit_sin_original_client_token_lifecycle.py --bin-zip /owner/bin.zip` verifies the owner's original pair and **6 complete original function SHA256s (553 new bytes), 15 native rel32 calls, 36 instruction witnesses, plus the previous original 261-byte intern-body checksum**. The built-in `--self-test` mutates each byte of a synthetic CALL and refuses fake DLL hashes. Both tests passed locally; GCC and Clang C++17 strict builds passed.

Remaining dependencies include original allocation failure behavior, which functions release the **global** singleton object at teardown, whether locale/casefold callbacks change at runtime, and exact high-water/16-bit exhaustion semantics. The complete native client/server modules and Android/Quest ports are outside this phase. The next productive original-binary target is the global symbol table's teardown registration/ownership chain and the larger gameplay-specific systems that use these token IDs.