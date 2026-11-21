# Phase 541: 2006 original client VGUI factory-array resolver, complete branch-level decompilation

## Provenance and limits

Source is the owner's original **2006 SiN Episodes: Emergence `client.dll`**, exact SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, with corresponding original `server.dll` SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. The DLLs are private; no original instructions or image bytes are committed. We extracted source-level facts from an Intel x86 PE32 file using the original addresses, compiler-generated branches and original native string table. The verifier fails closed on whole-file mismatch.

Original function **`0x240010D0` to `0x24001319`** is **586 bytes**, full SHA-256 `73c405920189d4c1c0eeefb994849544d8ff41cf6fc20b79d420e19d52fae64b` and followed immediately by INT3 at `0x2400131A`. This is the helper called from the original client HUD bootstrap `0x241EDFB0` at site `0x241EDFBC` (Phase 540). The call passes original literal `CLIENT`, an address of an interface-factory pointer, and integer `1`. Its Boolean return is required by the caller.

The recovered core behavior is the **factory-array scan and mandatory VGUI interface set**, not the complete class or function type names; no original PDB symbols are assumed. The very first CRT/memory diagnostic and four subsequent helper calls, preceding scanning, remain opaque.

## Exact factory scan order

Factory name pointers, indirect call sites and destination globals are direct original PE32 machine-code witnesses. Eight distinct repeated loops scan the provided array of function pointers. A null factory entry is ignored; an interface not found in an earlier factory is retried in later entries; if no factory resolves an interface the global remains NULL. The count is treated as a signed integer, with `count <= 0` resulting in zero pointers.

| Order | Required interface literal | Original PUSH site | Result global | Original global-store site |
|---|---|---|---|---|
| 1 | `VGUI_ivgui006` | `0x24001150` | `0x2438AE04` | `0x2400116B` |
| 2 | `VGUI_Panel009` | `0x2400117B` | `0x2438AE08` | `0x24001196` |
| 3 | `VGUI_Surface028` | `0x240011A9` | `0x2438ADFC` | `0x240011C4` |
| 4 | `VGUI_Scheme010` | `0x240011D9` | `0x2438ADF8` | `0x240011F4` |
| 5 | `VGUI_System010` | `0x24001209` | `0x2438AE00` | `0x24001224` |
| 6 | `VGUI_Input005` | `0x24001239` | `0x2438ADF4` | `0x24001254` |
| 7 | `VGUI_Localize003` | `0x24001269` | `0x2438AE10` | `0x24001284` |
| 8 | `VFileSystem012` | `0x24001299` | `0x2438AE0C` | `0x240012B8` |

Each factory invocation passes a literal NULL as the second argument (`push 0`), and the interface C string as the first argument; the indirect call is `call eax` from the factory array. The original uses eight separate native loops rather than a single generic `for(name : names)` abstraction; the readable reconstruction below compresses those repeated loops without changing the success and failure conditions. The integer in the output global is a pointer in the original PE32, not an ARM64 interface pointer.

## Final native VGUI initialization and return

After storing the eight results, the original loads `g_IVGui` from **`0x2438AE04`**. If it is NULL, it takes the false-return path. Otherwise it dispatches virtual byte offset **`+0x04`**, at **`0x240012C3`**, passing the original factory pointer array and count. The caller does **not test this virtual call's return value**: it subsequently checks the stored globals.

The eight final null checks are short-circuited in this exact order: `VGUI_Scheme010`, `VGUI_Surface028`, `VGUI_System010`, `VGUI_Input005`, `VGUI_ivgui006` (redundant), `VFileSystem012`, `VGUI_Localize003`, `VGUI_Panel009`. All branch to `0x24001314` on NULL; if none fail, the native function sets `al=1` at `0x24001310` and returns, otherwise sets `al=0` at `0x24001316`. **There is no rollback of already stored interface pointers in the observed function**. This should not be reinterpreted as cleanup in future code.

### Readable C-like source reconstruction

```cpp
// Annotated decompilation of the ORIGINAL x86 logic, not exact Ritual source.
// All helper and variable names are descriptive until original RTTI/PDB proves them.
using OriginalFactory = void* (*)(const char* name, int* return_code);

void* resolve_across_factories(OriginalFactory const* factories,
                               int count, const char* name) {
    for (int i = 0; i < count; ++i) {
        if (factories[i]) {
            void* result = factories[i](name, nullptr);
            if (result) return result;
        }
    }
    return nullptr;
}

bool original_240010D0(const char* label,
                       OriginalFactory const* factories, int count) {
    OriginalNativeStartupPrologue_240010D0_to_24001135(label);
    g_2438AE04 = resolve_across_factories(factories, count, "VGUI_ivgui006");
    g_2438AE08 = resolve_across_factories(factories, count, "VGUI_Panel009");
    g_2438ADFC = resolve_across_factories(factories, count, "VGUI_Surface028");
    g_2438ADF8 = resolve_across_factories(factories, count, "VGUI_Scheme010");
    g_2438AE00 = resolve_across_factories(factories, count, "VGUI_System010");
    g_2438ADF4 = resolve_across_factories(factories, count, "VGUI_Input005");
    g_2438AE10 = resolve_across_factories(factories, count, "VGUI_Localize003");
    g_2438AE0C = resolve_across_factories(factories, count, "VFileSystem012");

    if (!g_2438AE04) return false;
    // The original result of this virtual call is ignored.
    g_2438AE04->virtual_byte_04(factories, count);

    return g_2438ADF8 && g_2438ADFC && g_2438AE00 && g_2438ADF4 &&
           g_2438AE04 && g_2438AE0C && g_2438AE10 && g_2438AE08;
}
```

This is **source-level decompilation documentation**, not directly compilable original Source Engine code. The helper `OriginalNativeStartupPrologue...` intentionally stands for several original calls whose full semantics remain unknown. No exception, cleanup, thread-safety, or typed C++ vtable ownership is invented.

## What is actually authenticated

`research/tools/audit_sin_original_client_vgui_factory_resolver.py` confirms:

- Both unchanged whole-DLL SHA-256 digests, original client PE32 base and `.text` location.
- The **full 586-byte original function hash** and precise following INT3 method boundary.
- Exact client caller's `CLIENT` literal, direct native call site, and caller success/failure gate.
- Original factory array and signed count load, first loop's null skip.
- Eight exact interface C strings, paired PUSH C-string VA, `call eax`, and destination global stores.
- Native VGUI virtual `+0x04` call, eight final `cmp [global],0` tests and all eight failure branches.
- Both original return forms, and synthetic original-free mutation tests.

Test without game files: `python3 research/tools/audit_sin_original_client_vgui_factory_resolver.py --self-test`.

Owner-private exact DLL pair: `python3 research/tools/audit_sin_original_client_vgui_factory_resolver.py --bin-zip /path/to/bin.zip`.

**Neither a reconstructed HUD renderer nor original-game startup is claimed.** The `VGUI_Surface028` interface is an existing client ABI dependency, not authorization to repeat physics, surface-property, model or prop work already handled by the owner's HL2 port. The next reverse-engineering track is the named-handle registry: `0x241D4A80`, `0x2400A4D0`, `0x24009F40`, plus their linked calls `0x2400ABA0`, `0x2400AB30`, `0x241D3E20`.