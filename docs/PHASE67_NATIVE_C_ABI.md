# Phase 67: Native C ABI, shared-library loading and engine-access audit

**Status:** host-neutral dynamic module compiled and tested on Linux. **Not** a SourceVR game module, Android build, Quest APK, or in-engine gameplay test. No new original-binary disassembly or claims are made in this phase.

## What is actually new

Our Phase 66 `WeaponSession` exposed a virtual C++ host contract, but external programs could only link against our C++ library. This phase adds `sin_compat_host_abi` as a native shared library exposing **versioned `extern "C"` symbols** and a v1 C callback table in `include/sin_compat/host_abi_v1.h`. The C ABI delegates to the *real* `WeaponSession`, including the original-derived mock-tested weapon runtime, viewmodel lifecycle, `HideWeapon` effects, zoom and guarded reload events.

- `sin_session_create_v1` rejects null required callback pointers, unknown ABI versions and structure sizes, invalid weapon kinds or missing model names. It copies the callback table and model name; host callback context and entity lifetimes remain owned by the caller.
- `sin_session_usercmd_v1` returns ADS transition bit flags, and never asks the host to change flatscreen FOV in VR mode.
- `sin_session_reload_event_v1` only moves ammunition after a caller-provided validated event and at most once per reload cycle. Configuration includes capacity, **not** original per-weapon SiN script defaults.
- `sin_session_ensure_viewmodel_v1`, `sin_session_hide_weapon_v1`, and `sin_session_cleanup_v1` delegate to the generation-checked native host contract.
- All v1 functions have unmangled public entrypoints. Opaque session memory and C++ objects never cross this boundary.
- A **separately compiled C11** executable loads the actual `.so` with `dlopen` / `dlsym`, supplies a synthetic callback host, exercises both positive and negative lifecycle paths and unloads the module. This is a real dynamic-library call boundary, not an emulated `dlopen` or a SourceVR runtime.

The host must use the correct ABI on the same architecture, provide thread-safe/nonreentrant callbacks as appropriate, avoid throwing C++ exceptions across callback boundaries, and keep callbacks, userdata and live entities valid until session destruction. The v1 module is not a universal Windows-x86-to-Android loader. No public project should redistribute original SiN or Half-Life 2 DLLs.

## Build and tests

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Linux expected outputs: `libsin_compat_host_abi.so`, `sin_host_abi_dlopen_tests` and the existing five C++ test targets. The executable gets the build artifact path from CMake's `$<TARGET_FILE:sin_compat_host_abi>`, without hard-coded output directories. `nm -D` can inspect exported `sin_*_v1` symbols. Windows/Mac may build shared library, but the `dlopen` smoke test requires UNIX and is not run on Windows.

This is **R0.5: an engine-facing shared-library boundary**, not R1 as defined in `docs/SOURCEVR_HOST_ADAPTER_CHECKLIST.md` (R1 needs a true engine SDK build and an in-engine smoke test). There is no VR compositor, map rendering, Android toolchain, Source engine entity factory, multiplayer prediction or serialization at this boundary yet.

## Genuine upstream target evidence (31 October 2026)

- [tinsarfal/SourceVR](https://github.com/tinsarfal/SourceVR) publicly documents Quest 3-compatible Android ports, but its Git tree `0cf2ab64458abda88006c689f921503666c09b67` contains release notes/README, not engine build sources. Its README explicitly says **"Mods that include their own `client.dll` or `server.dll` game code are not supported."** The current APK therefore cannot load our custom gameplay module as a mod. This is stronger than simply saying its public headers are missing.
- [Valve Source SDK 2013](https://github.com/ValveSoftware/source-sdk-2013) (`be57e3d01ac7d7aa6a2cacbfef187c0b46616db0`) has genuine public game headers such as `src/game/server/player.h` with `CBasePlayer::CreateViewModel`, `GetViewModel`, and `DestroyViewModels`, but is under the **SOURCE 1 SDK LICENSE**. It is neither confirmed compatible with SiN 2006 nor with the Quest Android engine, and is **not MIT**. Do not copy its game source into this MIT-only repository without respecting separate licensing.
- [nillerusr/source-engine](https://github.com/nillerusr/source-engine) advertises Android/ARM support but states it is based on a **2020 leaked TF2 Source engine codebase**. We are not adopting, importing or redistributing this source in this project. Its existence does not solve the public/legal engine integration requirement.

**Next milestone:** obtain a lawful, buildable, pinned game engine with module source and VR/Android callbacks or permission/source from SourceVR maintainers; then implement these C callbacks using actual engine classes and event dispatch and compile/load in that engine. If no such target exists, independently reconstruct a compatible host or reconsider static recompilation feasibility with a properly isolated and legally evaluated experiment.

**Follow-up (Phase 68):** The v1 ABI now rejects same-session callback reentrancy and checks its public data layout via a freestanding Android ARM64 C compilation. See [Phase 68](PHASE68_ANDROID_ABI_AND_REENTRANCY.md). Neither check constitutes a full Android or SourceVR build.
