# Phase 80: SourceVR Android native game-module search paths

**Date:** 2026-11-01. **Evidence:** Read-only, offline analysis of the owner's original PortalVR APK and HL2VR 0.989 APK, including AArch64 instructions and exact library digests. **Not an APK modification, SourceVR SDK, live Android linker test, or working SiN module.**

## Verified source binaries

| ELF | SHA-256 | AArch64 native module-search evidence |
|---|---|---|
| HL2VR 0.989 Vulkan `libfilesystem_stdio.so` | `9641cca0307b152ee2b0d770962c69917a2ff70ff3e107e5281cb5c6fbf4389a` | `0x99f90` candidate search; `0x99da4` wrapper; `0x99e10` `dlopen` PLT call |
| HL2VR 0.989 Turbo `libfilesystem_stdio.so` | **same exact SHA-256 and bytes** | Same loading implementation and offsets |
| PortalVR APK 1.12 `libfilesystem_stdio.so` | `f3602bed1f9b80e9145e12a62cb0cc197d26511fe14bdbf0f8f103f2e6ecc7f8` | Symbol-rich `CBaseFileSystem::LoadModule` `0x4cd34`; `Sys_LoadModule` `0x8aa74`; `Sys_LoadLibrary` `0x8a888` |

Both native implementations import Android `dlopen` from `libdl.so`. HL2VR's filesystem library is stripped of convenient local C++ symbols; the analogous named PortalVR functions provided a cross-check, not an assumption of byte-identical code.

## Exact search-path evidence

The Phase 79 engine's client/server callsites use an `IFileSystem` vtable method at **offset `0xC8`** and supply the path-ID literal `GAMEBIN`. In the corresponding PortalVR filesystem implementation, `CBaseFileSystem::LoadModule` handles a Source-style optional `//pathid/...` override, performs internal path-ID bookkeeping, and passes a **module name** to `Sys_LoadModule`. HL2VR exhibits the same low-level shape; the call from its stripped filesystem front-end to native search is at `0x5a9a8`.

Crucially, the observed low-level `Sys_LoadModule` code does **not** use `GAMEBIN` as a direct pathname. For relative module names it searches:

1. An optional preferred directory from **`APP_MOD_LIB`** (when set/nonempty and an eligible candidate exists).
2. A fallback location from **`APP_LIB_PATH`**.
3. A separate absolute-path handling branch, which checks file existence and uses the same native loader.

The helper examines multiple candidate basename patterns (including `%s/lib%s` and `%s/%s`), adds/normalizes the `.so` extension, checks candidates with `stat`, and ultimately uses `dlopen(path, RTLD_NOW)`. This **does not** mean Android permits native code execution from every folder; `stat` success and `dlopen` success are different checks. A library may still fail due to Android linker namespace, dependencies, SELinux/mount permissions, or incompatible C++ ABI.

In the original PortalVR ELF (which contains local symbols), these are real calls and addresses:

- `CBaseFileSystem::LoadModule` `0x4cd34` calls `Sys_LoadModule` at `0x4ce30`.
- `Sys_LoadModule` begins `0x8aa74`. It reads `APP_LIB_PATH` at `0x8ab30` and `APP_MOD_LIB` at `0x8ab40`. Native search candidates include `%s/lib%s` at `0x8a9a4` and `%s/%s` at `0x8a9d0`.
- A selected candidate calls `Sys_LoadLibrary` at `0x8ab78`, with the `dlopen` call at `0x8a8f4`.

The corresponding verified **HL2VR** callsites are `APP_LIB_PATH` `0x9a04c`, `APP_MOD_LIB` `0x9a05c`, `%s/lib%s` `0x99ec0`, `%s/%s` `0x99eec`, `CBaseFileSystem` front-end to search `0x5a9a8`, a search-to-wrapper call `0x9a094`, and `dlopen` call `0x99e10`.

**Evidence boundary:** The two variables' use as search roots is confirmed inside ELF. The actual *runtime values* assigned by the Java/Android launcher for a running Quest session are **not established** by the ELF inspection alone. The HL2VR Java DEX contains both `APP_LIB_PATH` and `nativeLibraryDir`, which is suggestive but is not enough to prove the exact assignment or native linker namespace.

## A subtle but important distinction: content mods versus compiled game modules

HL2VR 0.989's bundled Java resources describe the `srceng/mods` directory as a place for `config.xml`, VPK files, models, materials and other assets, mounted for a selected campaign. Those resources do **not** describe a compiled native game module interface or promise that an external `libclient.so` or `libserver.so` can be loaded from that directory.

Thus **placing a SiN .so file inside `/sdcard/srceng` or `srceng/mods` is not a demonstrated integration mechanism**. Even though Source's `gameinfo.txt` contains a `gamebin` declaration, the native loader shown above is governed by its own Android search and linker rules. The stock client/server families are compiled and packaged inside the APK.

This narrows the engine integration blocker (#12): the missing piece is **an eligible native module development/runtime path with matching engine interfaces, headers, and Android library namespace access**, not merely a missing filename or `CreateInterface` export.

## Independent reproducibility: SHA-pinned instruction checks

Use the original owner-held library, never an uploaded redistributed .so:

```sh
python3 scripts/verify_sourcevr_module_search.py --profile hl2vr-0989-filesystem --filesystem /private/hl2vr/libfilesystem_stdio.so
python3 scripts/verify_sourcevr_module_search.py --profile portalvr-112-filesystem --filesystem /private/portalvr/libfilesystem_stdio.so
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
```

This verifier reuses Phase 79's independently written ELF section and AArch64 instruction decoders. It enforces whole-ELF SHA-256; validates direct `ADRP`/`ADD` references for both environment names and two path templates; and checks three specific `BL` edges from the filesystem front-end to the search routine, search routine to loader wrapper, and wrapper to `dlopen` PLT. Its JSON explicitly sets `external_sin_game_module_loaded=false`, `android_linker_acceptance_verified=false`, and `quest_runtime_verified=false`.

Both original distinct ELF profiles passed locally. Eight synthetic original-free Python tests passed, covering an encoded miniature ELF, three negative/tampered fixtures, SHA/arch/import/section integrity, and the branch decoder. CI should independently rerun the entire Python suite.

## Still required before a proof-of-loading claim

1. Obtain an authorized, buildable matching SourceVR/game module SDK, or a maintainer-supported custom-module build process, including usable C++ interface headers and licenses.
2. Verify actual Android launcher-provided `APP_LIB_PATH` / `APP_MOD_LIB` values at runtime and whether a developer-built *synthetic* module can be loaded from a permitted app-controlled location.
3. Verify `CreateInterface`/engine callbacks, memory ownership, prediction/save-restore, `VClient017`, `ServerGameDLL010` and actual `ClientVirtualReality001` ABI at runtime.
4. Only then integrate the independent gameplay reconstructions and owner's lawful SiN game content; validate genuine level startup and Quest 3 headset operation.

**Conclusion:** the ELF loader is a genuine candidate for native gamecode integration. We now know its search mechanism; we do **not** yet know of a supported, compatible way to provide a custom SiN module to this app. R1 #12 and campaign runtime #20 remain open. No APK, native library or original game data are committed.
