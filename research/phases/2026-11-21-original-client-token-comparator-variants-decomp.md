# Phase 546: source-level reconstruction of both original client token-tree comparators

## Provenance and counting

The only binary input for the original-source claims is the owner-supplied, unchanged **2006 SiN Episodes: Emergence** `client.dll` SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, paired with original `server.dll` SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

Phase 545 previously recovered `0x241D3750` insertion-position selection, `0x241D3E20` existing-token lookup and native indirect comparator calls at `0x241D3794`, `0x241D3E58`, and `0x241D3EBF`. Their `this+0` callback semantics were unresolved. This phase resolves **two code pointers actually installed by the original constructor**, then follows the folded path through its native CRT-style and ASCII fallback helpers.

Six **new, non-overlapping, complete original native methods**, total **709 bytes**, verified by exact full-body SHA256, original PE text membership, original function boundary witness, direct call/jump edges and 29 instruction witnesses. Their symbolic roles below are independent descriptive names, **not recovered original Ritual C++ symbols**.

| Original VA | Bytes | Exact original function SHA256 | Role |
|---|---:|---|---|
| `0x241D3BF0` | 157 | `8285c595abf524eb7a9878b5d83e7712974bda42b8662529e22e44e330dacf76` | Case-sensitive unsigned-byte C-string ordering |
| `0x241D3C90` | 113 | `91e93b8190e900c957b63eae158d119fefb8bbb56ca07ddfb2cb34231c011087` | Case-folded lexicographic comparator wrapper |
| `0x241D4640` | 81 | `c6166577600eeb70647f1a4266e862e922e7e43f7469b36be2430a9df9568200` | Initialize the token comparator function-pointer field when unset |
| `0x2425F81B` | 67 | `0905847aa7a5fa843447c965b7ed17f401401ad60bf6be835038df5637559806` | Dispatch case-folded C-string comparison by runtime locale-state flag |
| `0x24261C60` | 78 | `9208efe7b20ebf707490aa3334dabf576a42bfabcb93356dfd2505c20dd6d658` | ASCII fallback: normalize A..Z and compare case-insensitively |
| `0x2424C373` | 213 | `3fe51247dd4b30901765155d0d9057674a228f00e8384ef010a40040d6fa7838` | Original per-character case folding, including runtime locale-state gate |

**Method-boundary nuance:** `0x2425F81B` is immediately followed by the next function at `0x2425F85E`, `0x24261C60` by a method at `0x24261CAE`, and `0x2424C373` by a method at `0x2424C448`. These three **do not** have INT3 immediately afterward; the verifier checks their genuine next entry instructions. The two comparisons and the constructor *do* have INT3 padding. The `0x2425F824` branch is a native `JMP rel32` to the independently authenticated entry at `0x24261C60`, not a function CALL. No branch-through function bytes are double counted.

## Two-word string descriptors and original C-string resolution

These native comparators do not receive arbitrary `const char*` directly. Each of their first two arguments is a pointer to a 4-byte descriptor containing **two unsigned 16-bit words**:

- When **both** words equal `0xFFFF`, the comparator obtains a string-data pointer from the comparator context at `this+0x30`.
- Otherwise it uses the first word as a page index into the pointer table at context `+0x1C`, the second as an offset into that page, and begins string characters at **page+offset+8**.

The original code implements this resolution **twice** for the two operands before comparing. Page allocation, string lifetime, global locale readiness and invalid-index handling are not established by this phase. The host-neutral C++ code accepts *already resolved* bounded string views; it is not the original 2006 binary ABI.

## `0x241D3BF0`: unsigned-byte, case-sensitive comparator

The original code first loads the left C-string byte (`0x241D3C50`), compares it directly with the right byte (`0x241D3C54`) and, if equal, tests for NUL. It unrolls two bytes per loop iteration with the next byte comparison at `0x241D3C61`, stopping at either mismatch or terminator. On equality, it returns false. On mismatch, the original `SBB` pair at `0x241D3C7D` / `0x241D3C7F` normalizes the carry from the unsigned byte `CMP` to `-1` or `+1`, then uses `SETL` to obtain the Boolean **left less than right**.

Faithful source-level core, after verified descriptor-to-string resolution:

```cpp
bool ordinalLess(const unsigned char* a, const unsigned char* b) {
    while (*a && *a == *b) { ++a; ++b; }
    return *a < *b;
}
```

This is **bytewise ordinal**, not Unicode collation, locale sorting or automatically case-insensitive. `"A" < "a"` is true, and C-string comparison ends at NUL. Original register/memory exact sequencing and two-byte unrolling are optimizations, not separate string semantics.

## `0x241D3C90`: folded comparator

The second comparator resolves the same native descriptors but passes the two resulting C-string pointers to original `0x2425F81B` at `0x241D3CEE`. The return is a signed difference; the wrapper returns Boolean `(difference < 0)` via `TEST`/`SETL` at `0x241D3CF8` / `0x241D3CFA`. In this branch **case folding occurs inside the callee**, not in the wrapper itself.

The original folded comparison at `0x2425F81B` reads the runtime locale readiness/global at `0x243895D0`:

- If zero: **directly jumps** to the original ASCII fallback at `0x24261C60`, with no stack call frame, using byte-range normalization for `'A'..'Z'`.
- Otherwise: reads successive unsigned bytes and twice invokes native per-character folding helper `0x2424C373` (`0x2425F838` and `0x2425F844`), until a folded mismatch or NUL; then it returns the **difference of folded character values**. The helper itself contains locale-dependent branches whose exact active runtime state is unknown. In the helper's locale-unavailable path, it explicitly converts only bytes from `0x41` through `0x5A` to `byte+0x20`.

Faithful *ASCII fallback* core, not a claim about the locale-ready path:

```cpp
uint8_t foldAscii(uint8_t byte) {
    return byte >= 'A' && byte <= 'Z' ? uint8_t(byte + 0x20) : byte;
}
bool asciiFoldedLess(const unsigned char* a, const unsigned char* b) {
    for (;;) {
        auto x=foldAscii(*a++), y=foldAscii(*b++);
        if (x != y) return x < y;
        if (!x) return false;
    }
}
```

For locale-ready execution, **do not silently substitute `std::tolower` or guess the active 2006 engine locale**. The exact helper and its global state need an additional study before full original-runtime equivalence can be claimed.

## Original constructor callback choice, `0x241D4640`

After delegating to original `0x241D3D90`, the constructor initializes the string backing fields, including `this+0x1C`, `+0x20`, `+0x24=8`, `+0x28`, `+0x2C`. Then it checks its caller-supplied **flag byte** and current callback field `this+0x00`:

```cpp
// Bounded source-level decision, not a reconstruction of 2006 ABI:
if (callback_at_this_plus_zero == nullptr) {
    if (argument_flag_nonzero)
        callback_at_this_plus_zero = original_0x241D3C90; // folded
    else
        callback_at_this_plus_zero = original_0x241D3BF0; // bytewise
}
```

The **exact vptr/function-pointer literal stores** at `0x241D4677` and `0x241D4686` are authenticated; the branch is at `0x241D4671`. Critically, when the earlier initializer has already written `this+0`, the constructor does **not** override it. This phase has **not** yet established the live flag's value for the actual `CHudTextureHandle` registry or which of the two callbacks is selected in a specific real-game process. The presence of a casefolded variant alone is not proof that all token lookups are case-insensitive.

## Integration with prior string-token red-black tree

The established tree from Phase545 uses one original context callback at `this+0`, called once per visited node for insertion and up to twice (reverse argument order) for an existing-token lookup. The newly recovered native constructor establishes two exact callback choices. The comparator's key descriptor, byte ordering and casefolded fallback now have documented semantics. This links the tree's **control flow** to actual string comparison behavior, but does not yet recover a native string-table allocator, locale initialization or a full-function equivalent for the tree's 1,387-byte balancing routine.

## Original-free C++17 reference and verification

- `include/sin_compat/original_client_string_token_compare.hpp`: exact native two-word key descriptor width, callback choice, unsigned-byte C-string ordinal comparator and separately bounded ASCII fallback comparator. No original x86 addresses are executed.
- `tests/test_original_client_string_token_compare.cpp`: two compiler builds, equality, empty string, first mismatch, prefix, non-ASCII **unsigned** byte ordering, embedded C NUL, ASCII case folding, callback-overwrite refusal and flag selection tests.
- `research/tools/audit_sin_original_client_token_comparators.py --self-test`: independent synthetic `CALL` and `JMP rel32` five-byte corruption refusals, fake original full-file hash refusal.
- `research/tools/audit_sin_original_client_token_comparators.py --bin-zip /private/bin.zip`: owner-local original pair SHA checks, **six complete original function SHA digests, 709 non-overlapping bytes, four original `CALL rel32` witnesses, one `JMP rel32` witness, and 29 individual instruction witnesses**. Locally passed against the original owner ZIP.

**Scope boundary:** This is an authenticated partial native control-flow decompilation, not access to Ritual's unmodified original C++ source, a complete Windows engine-owned token registry, or any Android/Quest runtime. Original game DLLs and disassembly extracts stay private and are never published in the GitHub report.