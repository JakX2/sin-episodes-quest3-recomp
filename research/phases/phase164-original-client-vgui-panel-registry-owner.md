# Phase 164: original 2006 client VGUI panel-animation class-owner registry

**10 November 2026.** Direct native-code reconstruction from the owner-supplied SiN Episodes: Emergence (2006) original Windows PE32 client.dll. This completes the immediate control-flow bridge from the seven Phase162 timer properties through the Phase163 24-byte metadata array to the **class-owner object** used by their registration. Native symbol-table hashing, object allocation, singleton member construction, VGUI runtime and Color parsing are not yet rebuilt.

## Original retail DLL identity

- client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

All four code-region SHA256s below were recomputed directly from the genuine client.dll, **not** from prior-source guesses. A public verifier checks them against separately owned binaries and does not output executable bytes.

| Original client VA | Native bytes | SHA256, entire original function | Recovered responsibility |
|---|---:|---|---|
| 0x24011D30 | 129 | 03bb9a680ba41e011f0a4181986be0ccaa1c0bdd2d07c7047ae4b8578c105d97 | Lazy class registry singleton initializer |
| 0x24011DC0 | 17 | 65352515a9960692d154459a17610080cf15935918816e34129b64f387297e6e | Class name -> singleton -> class find-or-create wrapper |
| 0x24011B80 | 101 | 4a16915de0aa14d2133e1125a2dc4804ec1adef39edf20d8895670dfb9f260fd | Existing class-symbol lookup and owner-pointer return |
| 0x24011BF0 | 107 | ee5d099c12c1689f9934a681bdde25f685469454b9ae4960d25a89c6fc66bdcb | Find existing class, or allocate and register 28-byte class owner |

The original auditor additionally verifies **29 deliberately sparse instruction anchors**, **20 actual original CALL rel32 edges**, the literal **"vgui::"** in original .rdata at 0x242D3670, and original diagnostic **"No such Panel Animation class %s"** at 0x242D4318. The original IAT at 0x24276218 is named **Q_strnicmp** by the native import table.

## Four functions: actual native algorithm

### 0x24011D30: lazy singleton

The first original x86 instruction checks bit0 of the flag at **0x2438D9C0**. If set, it returns global registry address **0x2438D938** immediately. Otherwise it ORs bit0 into the flag **before** constructing the singleton's member storage and registering the original shutdown handler. A reentrant call during initialization therefore observes the same global address, not a second initialization.

The native initializer calls the original functions 0x241704C0, 0x241D4640 and 0x24248788, plus explicit global member stores. These engine-owned routines have not been reconstructed here; C++17 uses an initialization callback, called at most once.

### 0x24011DC0: class-owner entrypoint

This small **17-byte** function first calls 0x24011D30, loads the supplied class-name pointer and calls 0x24011BF0. It returns the result, including a null failure, without manipulating native Color text or constructing a live HUD object. The original wrapper's caller argument is on the stack, while the registry pointer is passed in EAX to the downstream routine.

### 0x24011B80: find existing class

The function compares the first six bytes of the provided name to original literal **vgui::** through the Source engine's imported case-insensitive **Q_strnicmp**. If the comparison equals zero, it advances the name pointer by exactly **6 bytes**; otherwise it uses the whole original class name. Crucially, the function does **not** lowercase or otherwise rewrite the suffix. Case handling inside the original symbol table remains unknown and is intentionally delegated to the host callback.

It passes that key through native symbol interning 0x241D4A80 and native tree search 0x24012DE0, using registry symbol map subobject **+0x30**. A found index maps to its original **24-byte** class-registry record in storage at registry **+0x34**; the owner pointer is retrieved from **record+0x14**. Missing class returns null, not an exception. All lookup data structures/callbacks are external in the portable model.

### 0x24011BF0: create missing class-owner

This routine first invokes the existing-class lookup (0x24011B80). **On success, it returns the existing pointer with no allocation or insertion.**

When missing, the original:

1. Initializes support machinery through original 0x240093B0.
2. Allocates a free **28-byte owner object** using native 0x24170580.
3. If successful, initializes **seven original DWORD fields to zero** (offsets +0 through +0x18).
4. Again compares the original class name against **vgui::**, removing its six-byte prefix if present.
5. Inserts normalized key and owner pointer into the registry symbol map by calling original 0x240124B0.
6. Returns the allocated owner pointer.

The original attempts registry insertion **even on null allocation**, in which case the C++ observer receives a zero handle and must never dereference it. The portable reference models this branch as an event; it does not emulate undefined original engine behavior for allocation failure.

A **28-byte class owner object is not the same as a 24-byte property descriptor record**. The former holds the latter's array, so conflating the sizes would break subsequent registration and memory layout.

## Original call graph: 20 verified call sites

Internal/direct links:
- 0x24011DC1 -> 0x24011D30
- 0x24011DCA -> 0x24011BF0
- 0x24011BF7 -> 0x24011B80
- 0x24011C03 -> 0x240093B0
- 0x24011C0A -> 0x24170580
- 0x24011C4F -> 0x240124B0
- 0x24011BB3 -> 0x241D4A80
- 0x24011BC6 -> 0x24012DE0
- 0x24011D4F -> 0x241704C0
- 0x24011D98 -> 0x241D4640
- 0x24011DA2 -> 0x24248788
- 0x24011DE1 -> 0x24011D30 (other wrapper)
- 0x24011DEC -> 0x24011B80 (other wrapper)

The additional **seven** constructor-proven original Phase162 property registrations all call class-owner wrapper **0x24011DC0** at 0x24230DF8, 0x24230E68, 0x24230ED8, 0x24236468, 0x242364D8, 0x24236548 and 0x242365B8.

Together this establishes a native stack of: property registration -> class owner resolver -> lazy singleton -> find or allocate owner -> Phase163 24-byte descriptor insertion. No native engine symbol map is pretended to exist in the host-only reconstruction.

## Code, tests and evidential status

- include/sin_compat/original_client_panel_animation_registry.hpp: portable C++17 branch-order model, case-insensitive six-byte prefix handling, lazy singleton bit0 guard, callback-driven find-or-create and a seven-DWORD zeroed owner image.
- tests/test_original_client_panel_animation_registry.cpp: strict C++17 CTest for mixed-case vgui prefix, unchanged suffix, singleton reentrancy, existing-owner fast path, missing-owner callback order, nullable allocation and **actual seven Phase162 property registrations stored into Phase163-style six-DWORD descriptor arrays** for two distinct owners.
- research/tools/audit_sin_original_client_panel_animation_registry.py: owner-only full original both-DLL SHA chain through Phase163, full SHA pins for four methods, 29 partial instruction witnesses, 20 original call edges and original literal verification.
- research/tests/test_sin_original_client_panel_animation_registry.py: original-free mutation of each byte/call witness and independent prefix and owner-resolution tests.
- Root CMake/CTest integration and reviewed original function ledger: **71 to 75 distinct reviewed method addresses**, including 25 server and 50 client. The tiny 0x24011DC0 wrapper has its complete call/return contract modeled; the three larger methods remain marked *machine_code_verified* with only the external-independent decisions modeled. Neither ledger count nor CI compilation constitutes a full DLL decompilation percentage.

Private owner audit:

    python3 research/tools/audit_sin_original_client_panel_animation_registry.py --client /private/client.dll --server /private/server.dll --output /private/phase164.json

Regression:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_panel_animation_registry.py -v
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --target sin_original_client_panel_animation_registry_tests
    ctest --test-dir build -R sin_original_client_panel_animation_registry_tests --output-on-failure

## Next direct original-DLL targets

Native 0x240124B0 class-symbol map insertion, original 0x24012DE0 lookup-tree traversal, original 0x241D4A80 symbol interning and 0x24170580 owner allocator are candidates for later decompilation. Their engine ABI, actual live Color parsing and Source/VGUI callbacks are still unresolved. The Windows original-game-loadable replacement DLL remains incomplete; Android and Quest game execution are subsequent goals. No proprietary original executable bytes or game assets have been committed.
