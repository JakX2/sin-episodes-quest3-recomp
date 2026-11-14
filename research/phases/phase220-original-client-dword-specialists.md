# Phase 220: original client type-5 DWORD snapshot specialists

14 November 2026. **Stacked DRAFT on Phase219 PR #273.** Verified the original owner-provided SiN Episodes: Emergence 2006 Windows x86 client.dll and server.dll from private bin.zip. Whole original client SHA256 is `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`; server SHA256 is `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Commercial DLLs, entire original native method bodies, raw original disassembly and assets are not included.

## Two newly SHA-verified complete native functions

| Original client VA | Native bytes | Complete original function SHA256 | Original Phase212 call |
| --- | ---: | --- | --- |
| `0x24197DB0` | **125** | `256748201d4c29fb46076705316ea2345bbeb24a885a978e21d7b6bb9824cdbf` | `0x241999BC` |
| `0x241983B0` | **144** | `88eb84ee0d78cae0b3754d272b8b462e89e662e52f2735a8c547018b48fad874` | `0x241999E5` |

Two entire original native functions, **269 x86 bytes**, including their complete original `RET 12` and `RET 16` instructions, respectively. Original comparison is followed by **3 INT3** padding bytes. The original 144-byte preview ends **exactly at 0x24198440**, the already-recovered Phase219 Boolean preview's entrypoint, with **no INT3 padding** between the functions. Both authentic method boundaries are independently checked rather than relying on pattern matching alone.

## Original native behavior

1. **`0x24197DB0` (125 bytes): DWORD array compare with an embedded FIRST mismatch trace.** Original `BYTE(this+0x14)==0` returns status **0** immediately without reading original descriptor or arrays. The original descriptor at `DWORD(this+0x18)`, flags `BYTE(descriptor+0x13)&0x04`, forces status **1** when nonzero. Native signed element count `<=0` returns **1**. For positive count the original compares 32-bit DWORD values in forward order and returns **1** if equal. On the FIRST nonidentical DWORD it calls the actual original `0x24197910` formatter using `int differs (net %i pred %i) diff(%i)\n`, with the signed 32-bit values and native wrapping `predicted-net` difference, and returns **0**. This call happens **INSIDE** the comparison before the router's copy, unlike a later separate diagnostic helper.

2. **`0x241983B0` (144 bytes): conditional integer preview and symbolic resolution.** If `DWORD(this+0x34)!=DWORD(this+0x18)` the function exits without reading caller buffers. Otherwise the original reads the FIRST DWORD of the predicted value. Descriptor bit `0x08` optionally invokes the original process-global symbol registry at `0x2433F6AC`, virtual slot `+0x04` for lookup, slot `+0x0C` to retrieve a name, and imported `vstdlib.dll!Q_FileBase` (IAT `0x24276254`) with a 512-byte local name buffer. When name is found, the original calls `0x24197FE0` with `integer (%i->%s)`; otherwise the original calls the same formatter with `integer (%i)`. Actual formatter class, symbol registry object, lifetime and native string-buffer fault behavior remain opaque.

## Four original type-5 direct CALLs now individually SHA-reviewed

The original 1036-byte Phase212 serializer's case-5 direct CALL order is:

1. **`0x24197DB0` compare + pre-copy mismatch formatter (new Phase220).**
2. **`0x24198A10` DWORD-count forward `REP MOVSD` conditional copy (previous Phase213).**
3. **`0x24197B60` integer diagnostics, including the optional symbol registry (previous Phase217).**
4. **`0x241983B0` conditional preview with optional symbol registry (new Phase220).**

All four original direct case-5 callees now have complete native SHA256 reviews. The comparison's nested trace observes the *original* predicted DWORD, while Phase217's separate integer diagnostic runs **AFTER** any Phase213 copy, and may therefore observe the newly copied value. The host-only reference preserves this distinction and does not invent missing engine objects.

## Verification and testing

- `research/tools/audit_sin_original_client_dword_specialists.py`: owner-original, full dual-DLL SHA256 gate; two complete native function SHA256s, **31** sparse original x86 opcode witnesses, two original router CALLs plus Phase213/217 callsites, 3 nested formatter CALL sites, actual `.rdata` format strings, native `RET` and precise `INT3` boundaries, original resolver global, exact imported `Q_FileBase` named IAT, and the zero-padding adjacent function entry.
- `research/tests/test_sin_original_client_dword_specialists.py`: synthetic-only opcode/CALL/RET/padding/string/import/adjacency mutation tests. Synthetic code NEVER increments the original reviewed-function count.
- `include/sin_compat/original_client_snapshot_dword.hpp`: clean-room C++17 exact DWORD comparison and first-mismatch sink, optional symbol-aware preview through host callback, and selected four-original-direct-CALL orchestration using the previously reviewed copy and integer diagnostic reference.
- `tests/test_original_client_snapshot_dword.cpp`: first-mismatch-only trace, status gates, 32-bit wrapping arithmetic, signed counts, pointer-token preview, numeric/symbol name fallback, pre-copy versus post-copy log distinction and host-only safety refusal.
- Linux/GCC CTest and native Windows MSVC x86 CTest registrations exercise the reference. Neither means that rebuilt game DLLs execute in the retail engine.

Owner-only original verification:

```sh
python3 research/tools/audit_sin_original_client_dword_specialists.py --archive /private/bin.zip
python3 -m unittest discover -s research/tests -p 'test_sin_original_client_dword_specialists.py' -v
```

## Checkpoint

Original individually reviewed native function ledger **260 → 262** (**server 128, client 134**). Within the strictly scoped 36-distinct-target original Phase212 snapshot router CALL graph, previously opaque targets decrease **14 → 12**. This is not total game decompilation progress or proof of a playable PC module.

Next original targets: type-3/type-4 remaining serializer specialists, then Source engine initialization and actual reconstructed Windows gameplay DLL loading. **Windows PC first → Android ARM64 → Quest 3.**
