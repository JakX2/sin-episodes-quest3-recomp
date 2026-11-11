# Phase 178: Original Windows game interface singletons, RTTI and vtables

11 November 2026. Stacked on draft Phase177 PR #230. This research inspects the exact owner-supplied original *SiN Episodes: Emergence* (2006) Windows x86 PE32 `server.dll` and `client.dll`; no proprietary binaries, game assets or complete original machine-code bodies are committed.

## Original binary provenance

- `server.dll`: 6,049,792 bytes, SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, image base `0x22000000`.
- `client.dll`: 3,592,192 bytes, SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, image base `0x24000000`.

The Phase178 native auditor **chains Phase177's entire dual-DLL SHA, export, `CreateInterface` and all 65 static interface-registration checks**. It also independently checks nine complete original factory thunks of six bytes apiece (**54 native bytes**), 105 exact original `.text` method-pointer DWORDs (**420 `.rdata` bytes**) across five static vtable prefixes, five original 20-byte MSVC32 RTTI complete-object-locator records (**100 bytes**), static singleton vptrs, native type descriptor names and four *uninitialized* object addresses. These are distinct evidence categories: method pointers do not imply implemented method bodies.

## Nine actual original factory return values

Every listed original factory thunk returns a fixed 32-bit singleton virtual address. It does **not** allocate a new host object. All nine complete native factory function SHA256 digests and each of the 105 original method addresses appear in the SHA-pinned manifest `research/catalog/original_dll_engine_interface_objects.json`.

| DLL | Registered interface | Native factory VA | Singleton VA | Static vtable | Verified contiguous executable slots | RTTI original type |
| --- | --- | --- | --- | --- | ---: | --- |
| server | `ServerGameDLL003` | `0x221E7630` | `0x22510650` | `0x22442EFC` | 31 | `.?AVCServerGameDLL@@` |
| server | `ServerGameEnts001` | `0x221E82C0` | `0x22510654` | `0x22443030` | 7 | `.?AVCServerGameEnts@@` |
| server | `ServerGameClients003` | `0x221E8320` | `0x22510658` | `0x22443054` | 15 | `.?AVCServerGameClients@@` |
| server | `GameMovement001` | `0x222CFFE0` | `0x225934B0` | runtime only | unknown | unknown |
| client | `GameClientExports001` | `0x240FD810` | `0x242FA4B0` | `0x24296B18` | 4 | `.?AVCGameClientExports@@` |
| client | `VClient011` | `0x240FD840` | `0x242FA4B8` | `0x24296C6C` | 48 | `.?AVCHLClient@@` |
| client | `VClientEntityList003` | `0x241056B0` | `0x2434FDDC` | runtime only | unknown | unknown |
| client | `VClientPrediction001` | `0x2422EA30` | `0x2437FA00` | runtime only | unknown | unknown |
| client | `GameMovement001` | `0x2420FA40` | `0x24379920` | runtime only | unknown | unknown |

The three server singleton addresses are adjacent in `.data` and have separately verified RTTI types. Address adjacency alone **does not prove C++ subobject ownership**. The two client static objects also have distinct RTTI types.

## Native object, vtable and RTTI boundary

For each of the five PE-initialized objects, the first object DWORD holds the original vtable address in `.rdata`. The DWORD at `vtable-4` holds the native MSVC32 CompleteObjectLocator address. Its five DWORDs are `signature=0, offset=0, cdOffset=0, typeDescriptorVA, classHierarchyDescriptorVA`; the auditor checks all 20 bytes by original SHA256 and independently verifies the decorated class name at `typeDescriptor+8`.

Every one of the 105 table entries is verified to target the original executable `.text`, with raw 4-byte pointer equality and whole contiguous prefix SHA256. The first DWORD after each prefix is nonexecutable and separately pinned. **These are proved contiguous vtable code-pointer prefixes, not a guarantee that the interface is entirely decompiled or callable.** No original engine callback signature is invented.

The four other returned singleton addresses live in the virtual/uninitialized part of `.data`; the executable file has **no raw bytes for their vptrs**. Their vtable pointers are determined only at runtime by an as-yet-untraced initializer. Phase178 treats them as *unknown* and actively rejects any claim of static original vtables. Recover initialization or inspect live original Windows process memory before naming those tables.

Two particularly important original vtable methods for the next phase are the **server game DLL slot 0 at `0x221E7640`**, whose native prologue calls through an engine factory parameter, and **client CHLClient slot 0 at `0x240FE0E0`**. These are direct original pointers, but their full engine ABI, argument signatures and side effects remain to be reconstructed.

## Source, independent tests, verification

- `include/sin_compat/original_dll_engine_interface_objects.hpp`: host-neutral C++17 address metadata, exact 4-byte x86 object-vptr and 20-byte RTTI COL layouts, and bounds-checked *observation*, not execution, of vtable slots.
- `tests/test_original_dll_engine_interface_objects.cpp`: nine factory identities, client vs server key disambiguation, uninitialized-vptr rejection, RTTI layout and 105 total source-confirmed slots. Locally compiled GCC C++17 `-Werror`.
- `research/catalog/original_dll_engine_interface_objects.json`: native exact address, RTTI and pointer-prefix manifest; SHA256 of complete file (with trailing newline) `cbe177e1e7277179e18e16a6255c3c44d7f0be695ba3e98749b5b04686b73c13`.
- `research/tools/audit_sin_original_engine_interface_objects.py`: checks the entire original DLL pair, original Phase177 interfaces, all nine complete 6-byte factory hashes, five original object vptrs, 105 exact vtable slot addresses and whole prefix SHA256s, all five RTTI COL SHA256s and type descriptors.
- `research/tests/test_sin_original_engine_interface_objects.py`: **11 passing original-free Python tests**: corruption of every factory byte, all 105 vtable pointers, 25 COL DWORD fields, type names, vtable boundary words, object vptrs, registration/factory pairing and the four forbidden fabricated BSS vtables.
- Registered CTest target `sin_original_dll_engine_interface_objects_tests`.
- Original reviewed-function atlas grows from **122 to 131** (+4 server, +5 client) based on nine verified native factory thunks. **74 server, 57 client**. Neither method count nor number of vtable pointers is a whole-game completion percentage.

### Owner-local verification

```sh
python3 research/tools/audit_sin_original_engine_interface_objects.py --server /path/to/original/server.dll --client /path/to/original/client.dll
python3 -m unittest discover -s research/tests -p test_sin_original_engine_interface_objects.py -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_dll_engine_interface_objects_tests
ctest --test-dir build -R sin_original_dll_engine_interface_objects_tests --output-on-failure
```

## Next genuine Windows integration milestone

1. Recover original server `CServerGameDLL` slot 0 `0x221E7640` and client `CHLClient` slot 0 `0x240FE0E0`, including calling conventions, requested engine interface names, global storage, failure paths and shutdown.
2. Recover constructors installing vtables for runtime-only `GameMovement001`, `VClientEntityList003`, and `VClientPrediction001`.
3. Implement real verified MSVC Win32 engine interface objects, invoke initialization/shutdown under the original game's executable, then start a map, player, weapon, network updates and HUD.
4. After working Windows game modules, move to Android and Quest 3.

**Current limits:** No original-game executable integration, actual initialized DLLs, complete engine interface vtables, playable Windows build, Android gameplay or Quest 3 runtime is demonstrated. The Phase177 synthetic Windows loader smoke is a verified separate test, not original-game loading.
