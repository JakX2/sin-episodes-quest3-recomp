# Phase 69: Official Android NDK ARM64 shared-library cross-build

**Evidence date:** 31 October 2026. **Scope:** Independently authored portable native compatibility library. **This is not a SiN or SourceVR in-game test or Quest APK.**

## Concrete deliverable

Phase 68 verified only a freestanding AArch64 *C header layout* object, not the actual C++ runtime. Phase 69 adds a distinct GitHub Actions `android-arm64` job using the GitHub Ubuntu runner's official Android SDK command-line manager and the **pinned NDK 27.2.12479018**, building **all** portable C++ sources and their exported C boundary as `libsin_compat_host_abi.so` with CMake:

- Target: `arm64-v8a` / `aarch64-linux-android`, minimum Android API 26.
- Android C++ STL: `c++_static`, so the module does not depend on an separately installed `libc++_shared.so` (it does still depend on normal Android system libraries).
- Target: `sin_compat_host_abi`, with `BUILD_TESTING=OFF`: Linux `dlopen` binaries are not executable inside GitHub's x86 host as Android AArch64 binaries.
- The built `.so` and an independently compiled AArch64 C ABI layout object are inspected by `scripts/build_android_arm64.sh`. The script rejects a missing NDK, non-ELF64, non-AArch64, non-shared output, wrong SONAME, unexpected shared-libc++ dependency or a missing/extra C ABI export.
- Exactly thirteen v1 `sin_*` exports are expected; C++ internals remain hidden. This does not validate any SourceVR/Valve engine loader ABI.
- GitHub Actions uploads the resultant `.so` as a **short-retention engineering artifact** from the official cross-build job. It contains only this project's independently authored compatibility code and linker-produced runtime components, **no original SiN or HL2 game assets**.

The job intentionally invokes the runner's SDK manager directly: in a preliminary CI attempt, `android-actions/setup-android@v3` failed while trying to install the obsolete package `tools`, **before** any C++ compilation. This is a CI setup issue, not a failing ABI or cross-compiler result.

The existing Linux host test job still runs the seven native CTest tests, 112 synthetic Python research tests and the Phase 68 freestanding C ABI check.

## Reproduction with a locally installed official NDK

Requires the official Android SDK and NDK package `ndk;27.2.12479018`, `cmake`, `ninja`, `readelf` and Bash (Linux x86_64 is what CI tests).

```sh
sdkmanager --install 'ndk;27.2.12479018'
export ANDROID_NDK_HOME="${ANDROID_HOME}/ndk/27.2.12479018"
bash scripts/build_android_arm64.sh
```

Equivalent manual CMake invocation:

```sh
cmake -S . -B build-android-arm64 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
  -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF
cmake --build build-android-arm64 --target sin_compat_host_abi --parallel 2
```

Build output: `build-android-arm64/libsin_compat_host_abi.so`. Only GitHub Actions with the installed NDK can actually verify the **linked C++** output; a local Clang freestanding compile alone is *not* equivalent. Test output should be recorded with the relevant CI run before claiming the cross-build passed.

## What this does **not** prove

- An Android linker-accepted ELF cannot be assumed to run on a real Quest device without device testing and compatible integration with the host app.
- The current public `tinsarfal/SourceVR` release documents support for content mods but **not** mods containing their own game code `client.dll` / `server.dll`. Shipping our ELF library alone will not cause the existing APK to load or execute it.
- There is no verified callable SourceVR engine API, Android Java bootstrap, `JNI_OnLoad` entrypoint, graphics/VR runtime, input binding or game asset integration in this project yet. `sin_*_v1` is a private compatibility-host C interface, **not** a Source/SourceVR engine plug-in ABI.
- Original SiN 2006 field offsets, x86 ABI, copyrighted DLLs, game maps and textures must never be built into this ARM64 module.
- R1 remains blocked by identifying a legally usable game-module-capable engine source and completing an observed in-engine smoke test. See [issue #12](https://github.com/JakX2/sin-episodes-quest3-recomp/issues/12).

## Reproducibility / security rules

- Keep the official Android NDK version pinned; do not fetch third-party leaked/proprietary engine forks to make the job pass.
- Fail CI if any of the cross-build architecture/export checks fail. Do not mark the Android job optional or use `continue-on-error`.
- Do not copy Valve Source SDK 2013 headers or implementation code directly into this MIT source without separately addressing its SOURCE 1 SDK LICENSE.
- Do not describe the uploaded ARM64 shared object as an APK, mod, or playable port.
