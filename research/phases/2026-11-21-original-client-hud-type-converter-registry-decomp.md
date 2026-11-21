# Phase 542: original SiN client `CHudTextureHandle` type-converter registry, three complete methods

## Context and exact original evidence

During original client `VClient011::Init`, the VGUI startup helper `0x241EDFB0` conditionally invokes original `0x24009340` with a literal `CHudTextureHandle` and an original pointer. Phase 540 called this *named HUD handle registration*, leaving the underlying registry undefined. Direct static disassembly and instruction-boundary analysis of the owner's unmodified 2006 original DLLs clarify that the call registers a **type converter keyed by the HUD texture-handle type name**, rather than creating or rendering the HUD element.

Source identity: `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` and original `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Original game files remain private. Function names below are **descriptive and not recovered PDB symbols**.

| Full native method | Verified bytes | SHA-256 | Directly observed responsibility |
|---|---:|---|---|
| `0x241D4A80` | 261 | `dc80847fa08da6c33e75659031da2d0bb955f5713e28319e50e1c5659e39c1cc` | String-token lookup and insertion, with 16-bit output |
| `0x2400A4D0` | 230 | `29e7ec79122a4b663661351234688ec42ada95894cad4006fbea930171f69f52` | Traverse 24-byte records, use caller-provided comparison callback, return existing index or sentinel |
| `0x24009F40` | 128 | `c6a10d23962af7f91b8debedb98e7ae661cf044d54fe55d4dd812a58a945f41c` | Intern token, allocate/find record through helper routines, write token and converter pointer |

**619 distinct original complete method bytes** are authenticated. The previously authenticated caller `0x24009340` is checked but not counted again (101 bytes in Phase 540). The first two methods end immediately before INT3 padding. The third finishes with `ret 4` at `0x24009FBD` and is directly followed by the next function's prologue at `0x24009FC0` **without any INT3 padding**. The auditor verifies that actual boundary rather than assuming all methods are padded.

## Caller `0x24009340`: duplicate registrations explicitly rejected

The original 101-byte function:

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

## Existing converter search `0x2400A4D0`

The search receives a descriptor with an initial signed **index at descriptor+`0x10`**. `0xFFFFFFFF` is its end-of-list sentinel. For each non-sentinel index, original x86 computes `index * 24` using shifts and LEA, then addresses the record at `base + 24*index + 0x10`.

It invokes a caller-supplied function pointer stored at descriptor+`0x00`. Different branches pass the candidate record address and search argument in different orders. The function follows next-index fields until an eligible match or sentinel, and returns an integer index. It uses a separate fallback/sentinel storage at global `0x2438AFA0` when a branch requires it; the original also lazily initializes a bit flag at global `0x2438D130`.

Do **not** reduce this to a simple map lookup with invented equality behavior. The original comparator's name, its exact ordering semantics and the registry's memory-ownership invariants remain unresolved. What is proven is the original 24-byte record stride, linked traversal and integer not-found sentinel.

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
    int existing = OriginalConverterFind_2400A4D0(type_token, /* native comparator */);
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
// List-traversal and comparison callback semantics remain partially unknown.
```

The `if` branch above summarizes the original caller's `-1` test; `OriginalConverterFind` is **not** asserted to implement ordinary string equality. The helper names and C++ argument types are placeholders for native meanings which require further decompilation.

## Verifier, limitations and next work

Run against independent test data: `python3 research/tools/audit_sin_original_client_converter_registry.py --self-test`.

Run against owner-private original pair: `python3 research/tools/audit_sin_original_client_converter_registry.py --bin-zip /private/bin.zip`.

The auditor authenticates all 619 distinct function bytes by complete-method SHA256, the previous caller by separate SHA, **eleven direct CALL edges**, original type name and duplicate diagnostic, 16-bit intern token and sentinel, 24-byte record indexing, exact pointer and token stores, and both kinds of function boundary. It rejects whole-DLL SHA mismatch and synthetic mutated call targets.

Next useful native methods to understand converter semantics: `0x2400ABA0` and `0x2400AB30` record-management helpers, `0x241D3E20` string lookup, `0x241D49F0` token publication, and the original comparator supplied to `0x2400A4D0`. These are source-level decompilation dependencies only. No renderer, asset importer, physics/surface/model port or actual replacement Windows client has been added.