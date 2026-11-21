# Phase 542: original SiN client `CHudTextureHandle` type-converter registry and 16-bit ordered tree

## Context and exact original evidence

During original client `VClient011::Init`, the VGUI startup helper `0x241EDFB0` conditionally invokes original `0x24009340` with a literal `CHudTextureHandle` and an original pointer. Phase 540 called this *named HUD handle registration*, leaving the underlying registry undefined. Direct static disassembly and instruction-boundary analysis of the owner's unmodified 2006 original DLLs clarify that the call registers a **type converter keyed by the HUD texture-handle type name**, rather than creating or rendering the HUD element.

Source identity: `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` and original `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Original game files remain private. Function names below are **descriptive and not recovered PDB symbols**.

| Full native method | Verified bytes | SHA-256 | Directly observed responsibility |
|---|---:|---|---|
| `0x2400A3C0` | 19 | `b0521eacdb9687665838d9a4df7374f3b2665f842939aea4d2d4c550dc97d413` | Original unsigned 16-bit token strict less-than comparator |
| `0x241D4A80` | 261 | `dc80847fa08da6c33e75659031da2d0bb955f5713e28319e50e1c5659e39c1cc` | String-token lookup and insertion, with 16-bit output |
| `0x2400A4D0` | 230 | `29e7ec79122a4b663661351234688ec42ada95894cad4006fbea930171f69f52` | Search an ordered tree of 24-byte records using token comparator and return matching index or sentinel |
| `0x24009F40` | 128 | `c6a10d23962af7f91b8debedb98e7ae661cf044d54fe55d4dd812a58a945f41c` | Intern token, allocate/find record through helper routines, write token and converter pointer |

**638 distinct original complete method bytes** are authenticated (619 from the three originally targeted methods plus a newly recovered 19-byte comparator). The previously authenticated caller `0x24009340` is checked but not counted again (101 bytes in Phase 540). The 19-byte comparator and the first two other methods end immediately before INT3 padding. The fourth finishes with `ret 4` at `0x24009FBD` and is directly followed by the next function's prologue at `0x24009FC0` **without any INT3 padding**. The auditor verifies that actual boundary rather than assuming all methods are padded.

## Caller `0x24009340`: duplicate registrations explicitly rejected

The original 101-byte function uses a descriptor at original client `0x24322878`. Its first pointer is `0x2400A3C0`, the exact original comparator whose semantics are now recovered.

The original 101-byte caller:

1. Loads its passed type name, which is `CHudTextureHandle` for this caller.
2. Invokes original string-token interning `0x241D4A80` with a short sentinel value initialized to `0xFFFF`.
3. Invokes the original registry lookup `0x2400A4D0` (its specific comparator callback is provided by the caller/descriptor).
4. Compares the returned integer with **`0xFFFFFFFF`**.
5. **If a converter is already present**, emits the diagnostic string `Already have converter for type %s, ignoring...` and returns without registering a second entry.
6. **If not found**, invokes `0x24009F40` to insert a converter entry associated with the type name and a separate original pointer.

The diagnostic is embedded in the exact original `client.dll`, at original string VA `0x242D3708`. Its `%s` placeholder is supplied the type-name argument. This distinguishes the path from normal HUD panel construction. The one-time flag in the VGUI bootstrap limits repeated registration attempts, while this converter registry *independently* refuses duplicate type entries.

## String-token helper `0x241D4A80`

- Receives its string input and a writable short-output parameter; the original uses a native `ret 8` calling convention.
- A null string writes the **16-bit value `0xFFFF`** to the caller's output immediately.
- A non-null string is stored as pointer at native `this+0x30`, then a separate helper `0x241D3E20` attempts an existing-token lookup.
- When that lookup yields a valid 16-bit token, the method returns it without allocating a new string.
- Otherwise it determines the string length, uses `0x241D3D10` to select/create a string-pool location, and on one growth branch allocates at least **`0x800`** bytes of capacity plus its own metadata. It then copies the terminating byte as well as the string, updates native length/capacity, and invokes `0x241D49F0` to finalize the token.
- The output at the native end is explicitly a **16-bit word**. It must not be replaced by a guessed 32-bit enum when mapping the original data structures.

The precise allocator and token-table formats behind `0x241D3E20`, `0x241D3D10`, `0x241D4060`, and `0x241D49F0` are still dependencies. The observed `0x800` is a local pool-growth minimum, not a universal Source SDK string limit.

## Ordered converter tree search `0x2400A4D0` and exact comparator `0x2400A3C0`

The original caller passes the native converter descriptor at **`0x24322878`** into the lookup. Its first 32-bit word is the original callback address **`0x2400A3C0`**, not an arbitrary predicate. The 19-byte complete original comparator reads two **unsigned 16-bit words** from its two pointer arguments, compares them with native `cmp`, and returns **1 precisely when the first token is below the second token** (unsigned), otherwise 0. The `sbb eax,eax; neg eax` instruction pair converts the unsigned carry flag into a Boolean.

The converter registry is therefore an **ordered binary-search tree**, not a linear linked list. The descriptor has a signed root index at `descriptor+0x10`. The root and child-index sentinel is `0xFFFFFFFF`. The original computes `index * 24` and addresses records using `base + 24*index`. Within each record, `+0x00` and `+0x04` are followed as the two tree-child indices, and the 16-bit token compared by the callback is at `+0x10`.

At each node, the original `0x2400A4D0` method performs precisely:

1. Invoke callback as `less_than(query_token, entry_token)`; on true follow the child index at **record +0x00**.
2. Otherwise invoke callback as `less_than(entry_token, query_token)`; on true follow the child index at **record +0x04**.
3. Otherwise both comparisons are false and the tokens are equal, so **return the current node index**.
4. On encountering `0xFFFFFFFF`, return not-found.

The routine contains a lazily initialized fallback/sentinel storage at global `0x2438AFA0` and flag at `0x2438D130`; exact object ownership and reentrancy semantics remain unresolved. The word `left`/`right` is descriptive: the actual comparison direction is proven by the original callback, while any stable engine C++ member names remain unrecovered.

```cpp
// Source-level reconstruction of the verified search core only.
// Descriptor layout and pointer validity are prerequisites of the native caller.
int original_find_converter(const Descriptor& d, uint16_t sought_token) {
    int node_index = d.root_index_at_0x10;
    while (node_index != -1) {
        auto& node = d.nodes[node_index]; // original stride is exactly 24 bytes
        if (sought_token < node.token_at_0x10)
            node_index = node.child_index_at_0x00;
        else if (node.token_at_0x10 < sought_token)
            node_index = node.child_index_at_0x04;
        else
            return node_index;
    }
    return -1;
}
```

The original traversal also includes a fallback/sentinel branch; the sketch compresses that native path. This is an independently authored bounded decompilation, **not** a copy of the original C++ declarations.

## Converter record insertion `0x24009F40`

The original registration method uses the registry object's field `this+0x24` to intern the type name by calling the same `0x241D4A80` helper. It passes the resulting 16-bit token and additional descriptor arguments to native helpers `0x2400ABA0` and `0x2400AB30`, which select and allocate a record index. Then it computes the same **24-byte entry stride** and stores:

- A 16-bit token into entry offset `+0x10` (relative to the array record base).
- The provided converter pointer, observed at entry offset `+0x14`, after dereferencing the pointer supplied through the original stack arguments.

The original instructions include another preceding write to `record+0x14` which is subsequently overwritten before return. Without tracing the helper-owned pointer aliasing and exact caller stack object we should not invent a second permanently stored field. The function returns via native **`ret 4`**. Its records are managed by an original pool/list, not by any `std::unordered_map` interface exposed by this research.

## Bounded readable original control-flow reconstruction

```cpp
// C-like explanation of original 2006 machine code, NOT original Ritual source.
void OriginalRegisterConverter_24009340(const char* type_name,
                                        OriginalConverterPointer converter) {
    uint16_t type_token = OriginalIntern16_241D4A80(type_name);
    int existing = OriginalConverterFind_2400A4D0(type_token, /* verified unsigned-16 tree comparator */);
    if (existing != -1) {
        NativeDiagnostic("Already have converter for type %s, ignoring...", type_name);
        return;
    }
    OriginalConverterInsert_24009F40(type_name, converter);
}

uint16_t OriginalIntern16_241D4A80(const char* name) {
    if (name == nullptr) return 0xFFFF;
    if (auto id = OriginalLookup16_241D3E20(name); id != 0xFFFF)
        return id;
    // Locate or grow native string storage, preserve terminator, then
    // publish through the original 0x241D49F0 method. Full internals pending.
    return OriginalAllocateCopyAndPublish_241D4A80(name);
}

// Original converter records addressed by: base + index * 24 + 0x10.
// Original search uses the verified 16-bit unsigned less-than callback,
// deciding between the two children of each 24-byte tree node.
```

The `if` branch above summarizes the original caller's `-1` test; `OriginalConverterFind` is **not** asserted to implement ordinary string equality. The helper names and C++ argument types are placeholders for native meanings which require further decompilation.

## Verifier, limitations and next work

Run against independent test data: `python3 research/tools/audit_sin_original_client_converter_registry.py --self-test`.

Run against owner-private original pair: `python3 research/tools/audit_sin_original_client_converter_registry.py --bin-zip /private/bin.zip`.

The auditor authenticates all 638 distinct function bytes by complete-method SHA256, the previous caller by separate SHA, **eleven direct CALL edges**, the original comparator function pointer and its complete 19-byte body, original type name and duplicate diagnostic, 16-bit intern token and sentinel, 24-byte record indexing, exact pointer and token stores, and both kinds of function boundary. It rejects whole-DLL SHA mismatch and synthetic mutated call targets.

Next useful native methods to understand converter allocation and owner semantics: `0x2400ABA0` and `0x2400AB30` record-management helpers, `0x241D3E20` string lookup, `0x241D49F0` token publication, and the record allocator/tree insertion helpers that maintain child pointers. These are source-level decompilation dependencies only. No renderer, asset importer, physics/surface/model port or actual replacement Windows client has been added.