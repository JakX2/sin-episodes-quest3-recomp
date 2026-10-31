# Phase 68: Android ARM64 ABI contract and callback reentrancy safety

**Evidence date:** 31 October 2026. **Status:** verified on Linux against the native shared-library test harness. This is a **portable R0.5 improvement**, not a new SiN original-binary discovery, a full Android NDK build, SourceVR module or Quest APK.

## Why this step matters

The Phase 67 shared library exposed a C callback table across the host/runtime boundary. The host could accidentally re-enter the same `sin_session_v1` from an entity, animation or effects callback before the first call finished. For example, a `is_live` callback could indirectly deliver `HideWeapon` again, or a `bind_viewmodel` callback could request ammunition mid-frame. The v1 ABI already documents a one-thread callback contract. Nested calls need an explicit, non-recursive failure response.

## Implemented: non-reentrant v1 session boundary

- Every C API operation on a live session now uses an RAII `SessionCall` guard around the real `WeaponSession` operation. A nested operation on that same session fails with `-1` and does not enter the C++ gameplay runtime. The flag is cleared on both normal exit and caught exceptions. Other sessions do not share the flag.
- `sin_session_destroy_v1` declines destruction when invoked during a callback. Because `destroy` returns `void`, it cannot report that decision. The **host must explicitly call destroy again after the outer call returns**. It is unsafe to free host userdata or assume the session was destroyed during that rejected callback.
- The dynamic C11 `dlopen` smoke test now exercises both a recursively attempted `HideWeapon` inside `is_live` and an attempted `get_ammo` inside `bind_viewmodel`, plus an attempted callback-time `destroy`. The nested results must be `-1`, the outer operations succeed exactly once, and the still-live session is destroyed by the owner later.
- These safeguards prevent **same-thread reentrant** session entry only. They do not provide thread synchronization or protect against concurrent destruction from another thread. The v1 host contract still requires serialized calls, stable callback userdata, and no C++ exceptions crossing into a C callback.
- The shape and meaning of all v1 exported C functions and public structures remain unchanged.

## Implemented: static AArch64 Android ABI contract check

- `tests/abi_layout_contract.c` uses C11 compile-time assertions for entity reference generation offset, ammo quartet, viewmodel offsets, and callback-table geometry. The 64-bit callback table is **184 bytes**, and the weapon configuration is **32 bytes** on the tested LP64 layout.
- `scripts/check_android_abi_layout.sh` compiles this source with Clang targeting **`aarch64-linux-android26`** as a freestanding relocatable object and confirms ELF64 / machine AArch64 with `readelf`.
- This can run without the Android NDK because it compiles **only the public C header contract** with Clang's freestanding built-in C headers. It **does not link the C++ runtime, the Android C library or a usable APK**. A compiled object targeting Android is not the same as an Android-ready shared library.
- The same C layout source is also built and **executed** as a host CTest executable on Linux, independent of the C++ library.
- GitHub Actions now runs the freestanding AArch64 check on every push and pull request, in addition to CMake/CTest and the Phase 56–65 synthetic Python verifiers.

## Reproduce

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
bash scripts/check_android_abi_layout.sh
python3 -m unittest discover -s research/tests -p 'test_*.py'
```

The AArch64 check requires `clang` and `readelf`, not an Android NDK. The native C11 `dlopen` test runs on Linux and cannot run on an Android headset through this script.

## Real Android build gate

Once an appropriate installed Android NDK and lawful engine target are available, the native module can be **attempted** with the NDK CMake toolchain:

```sh
cmake -S . -B build-android-arm64 \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-android-arm64 --parallel
```

This command has **not** been executed in this environment; no Android NDK is installed here. Even a successful NDK build would not make the resulting `.so` loadable by current SourceVR, whose public release notes prohibit custom game-code DLL mods. Integration still needs an accessible, authorized, module-capable SourceVR or other engine. See [issue #12](https://github.com/JakX2/sin-episodes-quest3-recomp/issues/12) and [Phase 67](PHASE67_NATIVE_C_ABI.md).

## Scope and rights

This phase modifies only independently authored C/C++ code, tests, scripts, and documentation. No original SiN/HL2 game DLLs, assets, SDK source or disassembly are included. Passing tests documents a staged host contract, **not** a playable port.

## Follow-up: Phase 69 official NDK build job

The optional local Phase 68 freestanding check remains useful but has been supplemented by a distinct full C++ cross-build workflow in [Phase 69](PHASE69_ANDROID_NDK_CROSS_BUILD.md). That job installs the official pinned NDK and verifies a linked Android ARM64 `.so`; the same game-engine/Quest loader limitations still apply.
