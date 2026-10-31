# Phase 64: Identify the original player `m_hViewModel` array used by HideWeapon

**Evidence:** 31 October 2026. The original user-owned 2006 SiN Episodes: Emergence **server.dll and client.dll**, SHA-256 pinned to the Phase 56 fingerprints. This is statically verified metadata and code, not a running game or SourceVR build.

## Finding: array member name no longer inferred

Phase 63 traced the weapon visibility routine to an indexed owner-side handle resolver reading `[owner + index*4 + 0xBDC]`. We now independently identify that exact owner field by name as **`m_hViewModel`**, including its declared array size. The original client DLL registers a matching field name in its separate client class layout. This resolves the earlier Phase 63 uncertainty about the *name of the array*. The precise dynamic type of an entity pointed to by an individual viewmodel handle is **still not proven** by field-name metadata alone.

| Original binary observation | Exact value | How checked |
|---|---|---|
| Server `m_hViewModel` name string | `0x22450B88` | NUL-terminated original string |
| Server runtime descriptor storage | `0x225195B0` | 52-byte zero-on-disk slot, initialized by code |
| Server field type/name/offset initializers | `0x22251BFC`, `0x22251C06`, `0x22251C10` | Immediate stores write type `13`, name pointer, `+0xBDC` |
| Server array byte width store | `0x22251C44` | Immediate store of `8` bytes to descriptor `+0x24` |
| Server field/array registration | `0x22249C14`–`0x22249C3D` | Registers same `m_hViewModel` string, offset `+0xBDC`, 4-byte elements, count **2**, with checked call targets |
| Owner indexed handle resolver | `0x2224AF04` | Existing Phase 63 instruction reads `[owner + index*4 + 0xBDC]`, masks handle index and checks generation |
| Client `m_hViewModel` name | `0x2428A43C` | NUL-terminated original string |
| Client *static* descriptor | `0x242F9290` | Type `13`, name pointer, offset `+0x1058`, descriptor count **2**, field width **8** |
| Client array registration | `0x240D3AFD`–`0x240D3B10` | Pushes matching field name, 4-byte element size and count **2**, then calls checked registration target |

Both builds explicitly register **two 4-byte entries**, eight bytes total. Their object offsets differ; these are build- and platform-specific 32-bit layouts, **not** ABI offsets for Android ARM64.

The numeric field type `13` aligns with the Source engine's `FIELD_EHANDLE` terminology. It documents a handle slot, **not** that every target's concrete runtime C++ class has already been verified via RTTI.

### A meaningful distinction in evidence strength

- **Verified directly in original DLLs:** the name `m_hViewModel`, server/client offsets, two-element array registrations, the server indexed-handle read at precisely the registered `+0xBDC`, handle-generation checks, and the `HideWeapon` association established in Phases 61–63.
- **High-confidence interpretation:** the two handles are the player's first-person viewmodel entity slots. The name and use are no longer an analogy with Source SDK 2013.
- **Still open:** exact 2006 server dynamic classes of each resolved handle (the binary contains `CBaseViewModel` class metadata, but that alone cannot establish all values stored into the array); all owner class variants and actual game-side lifetime; how the selected SourceVR fork represents viewmodels; live Quest device behavior.

### Native integration consequence

The Phase 63 adapter already asks the host for `viewmodel_index_of()`, `is_valid_viewmodel_index()`, `viewmodel_of()` and `is_viewmodel_owned_by()`. This remains the safest boundary: **do not** write `owner + 0xBDC` or assume a 32-bit handle layout on Android. The mock host now explicitly tests both original array slots `0` and `1`, and rejects `2`, while letting the eventual real host determine its own index bounds and entity validity rules. An implementation must not confuse world-weapon visibility with viewmodel presentation.

## Reproduction

Using locally owned original game DLLs (not included in GitHub):

```sh
python3 research/tools/verify_weapon_owner_viewmodel_field.py /path/to/SiN/server.dll /path/to/SiN/client.dll
python3 -m unittest discover -s research/tests -p 'test_*.py'
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The verifier first executes the Phase 56–63 SHA-256-pinned verification chain, then checks **four** descriptor initialization stores, **eleven** code anchors, **three** direct call targets, and the matching original client descriptor. It supplies standalone **synthetic PE fixture tests** for changed type, names, offsets, widths, slot counts, generation-resolver offset, and registration calls. These tests do **not** execute original functions or require the DLLs on CI.

**Phase 64 local results:** both original DLL fingerprints matched and the verifier passed; 94 Python tests passed (16 added); native C++17 build and 3 CTest executables passed. No copyrighted binaries, models, maps, PE dumps or engine source code are being added.

## Next work

1. Identify which constructors and entity factories store `CBaseViewModel` instances into `m_hViewModel` and trace relevant RTTI/type checks. Avoid claiming class identity from a field name alone.
2. Select a particular licensable SourceVR branch and compile a real `IWeaponViewModelHost` implementation against *its* engine interfaces. This would be a more important milestone than adding more synthetic tests.
3. Integrate selection, hide/show, weapon switching, and associated model visibility in an engine-backed headless lifecycle test before attempting ARM64 APK packaging.

Earlier work: Phase 63 [viewmodel relation](phase63-weapon-viewmodel-relation.md), Phase 62 [vtable visibility](phase62-hideweapon-concrete-vtables-visibility.md).
