# Phase 538: original SiN 2006 `VClient011::Init` interface-acquisition decompilation

## Evidence and scope

**Decompilation first; no new client runtime adapter.** Source evidence comes
from the exact owner-original 2006 PE32 `client.dll` (SHA-256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`),
and the independently checked original server companion SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

- Original `VClient011` first virtual method pointer at **`0x24296C6C`**
  points to **`0x240FE0E0`** (client RVA `0xFE0E0`).
- Original complete Init method byte range:
  `0x240FE0E0`..`0x240FE5A3` inclusive, **1,220 (`0x4C4`) bytes**;
  SHA-256 `14d81449866220aaef1f5f906b577179ea107df78a424a364c715261adbb35db`.
  Its immediate successor is `int3` padding, with a distinct next method at
  `0x240FE5B0`.
- Original success block `0x240FE595`: return **1** using `ret 0x0C`.
  Original common failure block `0x240FE59E`: return **0** using
  `ret 0x0C`. This confirms 12 bytes of stack arguments at the ABI
  boundary, but does not independently prove higher-level type signatures.
- The auditor checks **all 21 ordered engine-factory requests**, their
  original C strings and global pointer destinations, and **all 21 direct
  fail-on-null branches** (the first query has different stack cleanup).
  It additionally proves one separate optional-style request and two
  non-null-checked *derived-interface* acquisitions.

The owner previously demonstrated on genuine Windows engine startup that the
first five interface names are available. That runtime test deliberately
returned failure from the independent client implementation; it did not
complete native client Init. This phase uses **direct original PE32 static
code**, not inferred behavior from that diagnostic.

## Exact original factory requests, in order

These are required by the original code's **immediate zero check and jump to
`0x240FE59E`**. Note that obtaining a non-null pointer does not prove its
virtual interface is safe to call in a reconstructed engine.

| # | Original query call block VA | Exact interface name | Original global pointer destination |
|---:|---|---|---|
| 1 | `0x240FE109` | `VEngineClient012` | `0x2433F684` |
| 2 | `0x240FE127` | `VEngineModel012` | `0x2433F688` |
| 3 | `0x240FE140` | `VEngineEffects001` | `0x2433F68C` |
| 4 | `0x240FE159` | `VEngineCvar002` | `0x2433F690` |
| 5 | `0x240FE172` | `EngineTraceClient003` | `0x2433F6D8` |
| 6 | `0x240FE18B` | `VEngineRenderView011` | `0x2433F694` |
| 7 | `0x240FE1A4` | `VDebugOverlay003` | `0x2433F698` |
| 8 | `0x240FE1BD` | `VMaterialSystem076` | `0x2433F69C` |
| 9 | `0x240FE1D6` | `VEngineCache001` | `0x2433F6A8` |
| 10 | `0x240FE1EF` | `VModelInfoClient003` | `0x2433F6AC` |
| 11 | `0x240FE208` | `VEngineVGui001` | `0x2433F6B0` |
| 12 | `0x240FE221` | `VEngineClientStringTable001` | `0x2433F6B4` |
| 13 | `0x240FE23A` | `SpatialPartition001` | `0x2433F6B8` |
| 14 | `0x240FE253` | `VEngineShadowMgr002` | `0x2433F6C0` |
| 15 | `0x240FE26C` | `StaticPropMgrClient004` | `0x2433F6C4` |
| 16 | `0x240FE285` | `IEngineSoundClient003` | `0x2433F6C8` |
| 17 | `0x240FE29E` | `VFileSystem012` | `0x2433F6BC` |
| 18 | `0x240FE2B7` | `VEngineRandom001` | `0x2433F6CC` |
| 19 | `0x240FE2D0` | `VENGINE_GAMEUIFUNCS_VERSION004` | `0x2433F6DC` |
| 20 | `0x240FE2E9` | `GAMEEVENTSMANAGER002` | `0x2433F6E0` |
| 21 | `0x240FE302` | `VSoundEmitter001` | `0x2433F6E4` |

The first request uses `push null`, `push name`, saves an incoming pointer
at global `0x242FA458`, invokes the saved engine factory, restores 40 bytes
of earlier call arguments, tests the returned pointer and stores it at
`0x2433F684`. The subsequent 20 have the same 25-byte original pattern:
`push null`, `push name`, `call esi`, stack cleanup, `test eax`, global
store, `je fail`. All **20 branches** point to the **same exact failure VA**.

## Derived and optional-style requests

These are separate from the ordered 21 checks and should **not** be silently
counted as equivalent mandatory factory calls:

| Original code site | Interface name | Result handling |
|---|---|---|
| `0x240FE347` | `VMaterialSystemStub001` | Queried via the incoming factory; stored at `0x2433F6A0` **without an immediate null-rejection branch** |
| `0x240FE362` | `MaterialSystemHardwareConfig012` | Queried via virtual `+0x20` of the original material-system interface; stored at `0x2433F6A4`, and null branches to failure |
| `0x240FE3E1` | `MatSystemSurface003` | Queried through a distinct original factory-like virtual `+0x08`; stored at `0x2433F6D0`, and null branches to failure |

At `0x240FE31B`, the original client invokes virtual slot 0 of the
`VSoundEmitter001`-resolved object with the factory in the argument list;
its boolean result is checked and can jump to failure. The original also
checks a `-textmode` name through another interface, setting byte
`0x2433F6FC=1` if that query returns nonzero. A missing `-textmode`
result does not directly take the global failure branch.

## Readable prefix reconstruction

This is **partial pseudocode for the interface-acquisition phase**, not an
accurate full-body C++ port. The original final body additionally performs
many service registrations and initialization calls. No Source class type is
assumed for `void*` here.

```cpp
// Original VClient011 first virtual +0x0 at 0x240FE0E0, 12-byte ret.
// Pseudocode: first part only; exact 21 names and global stores above.
int VClient011_Init(FactoryFn engine_factory, void* arg2, void* arg3) {
    unknown_24121810();
    unknown_2416D720(/* original fixed numeric setup arguments */);
    original_global_242FA458 = arg3; // original stack argument #3

    // The original has 21 individually checked requests, not a loop.
    // Listed as a loop here for readability; preserve original ordering.
    for (each item in exact_21_request_manifest_in_order) {
        void* obj = engine_factory(item.name, nullptr);
        original_global_at(item.global_va) = obj;
        if (!obj) return 0; // all 21 branch to 0x240FE59E
    }
    if (!original_sound_emitter_virtual_00(engine_factory)) return 0;

    if (unknown_textmode_query("-textmode"))
        original_global_byte_2433F6FC = 1;

    original_global_2433F6A0 = engine_factory("VMaterialSystemStub001", nullptr);
    // No direct NULL guard on the optional-style request above.

    original_global_2433F6A4 =
        original_material_interface_virtual_20("MaterialSystemHardwareConfig012");
    if (!original_global_2433F6A4) return 0;

    // More global registration and stage initialization (not yet translated).
    original_global_2433F6D0 =
        original_surface_factory_virtual_08("MatSystemSurface003");
    if (!original_global_2433F6D0) return 0;

    // Dozens of remaining original virtual/cross-DLL calls still require
    // source-level decompilation. No invented success shortcut permitted.
    /* ... */
}
```

The original stack argument #1 is called as a factory, and stack argument
#3 is stored at `0x242FA458`. Their exact **higher-level C++ types** and the
second argument's semantics remain unresolved and should be checked from
the original caller before a typed implementation is attempted. The registration suffix
is **deliberately not replaced with `return 1`**, because it includes later
potential failure gates, such as `0x24164120`, `0x2417F680`,
`0x241EDFB0`, `0x24156860` and `0x24182C60`, which each branch to the
common failure return when the original check fails.

## How to reproduce against the exact original game DLLs

```sh
python3 research/tools/audit_sin_original_client_init_interface_manifest.py --self-test
python3 research/tools/audit_sin_original_client_init_interface_manifest.py --bin-zip /private/original/bin.zip
```

The synthetic test corrupts all six bytes of an invented relative JE and
requires refusal, and also refuses nonmatching whole-DLL checksums. The
private original-binary auditor authenticates all 1,220 function bytes,
original vtable entry, interface names and destinations, 21 null failure
branches, two derived interfaces, one optional-style interface, and success
and failure ABI return blocks. The script commits **no original DLL bytes**.

## Next decompilation steps

1. Extract readable method-level logic for `0x240FE315`..`0x240FE595`,
   including the original callback registrar `0x24156840`, resource
   initialization checks and the first true success path.
2. Recover the original `VClient011` Init signature and the meaning of its
   original three arguments from engine call sites, not SDK guesswork.
3. Record original client receive tables, HUD and input lifecycle with the
   same exact function-SHA and control-flow standard. Keep separate from
   the 2006 server gameplay and Grunt schedule phases.
4. Defer Android/VR port shims until both DLLs' game-specific behavior and
   required interfaces have source-level reconstructions.

**Proof boundary:** exactly one 1,220-byte original method authenticated;
its **interface acquisition prefix is translated**, but the full method's
post-acquisition semantics are not yet fully decompiled. No native client
initialization success or game boot is claimed.