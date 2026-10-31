# Phase 70: Android ARM64 native loader and on-device smoke-test protocol

**Scope:** Independent, source-authored host bridge for SiN Episodes: Emergence compatibility code. **Not** the original game engine, a Quest APK, or a SourceVR game-code plugin. Published without proprietary game assets, Windows DLLs, copied Source engine code, or leaked engine source.

## What changes

Phase 69 proved that the **whole portable C++ compatibility library** links as Android API 26 / ARM64 `libsin_compat_host_abi.so` using the official Android NDK r27c. Phase 70 also cross-compiles a **separate Android Bionic-linked executable**, `sin_android_host_abi_smoke`, from the same `tests/test_host_abi.c` C11 test which already runs on Linux through a `dlopen`/`dlsym` boundary.

The Android executable is deliberately **not linked against the SiN compatibility `.so`**. It loads the library by path and looks up all 13 public `sin_*_v1` entrypoints dynamically. When executed in a genuine Android shell it exercises callback validation, weapon/viewmodel creation and hiding, ADS without headset-FOV change, event-timed reload with no double transfer, selection loss, reentrant host callbacks, holster and cleanup.

A **successful NDK build only establishes that the executable and library both compile and link** against the Android sysroot. It does **not** establish that either has run on Android. The CI job must label results accordingly. Real execution requires a connected user-controlled ARM64 Android device or a compatible Android ARM64 emulator.

## Reproducible build

The mandatory `android-arm64` CI job installs an exact official NDK version: **27.2.12479018** (r27c). Reproduce with:

```sh
export ANDROID_NDK_HOME=/path/to/android-ndk-r27c
bash scripts/build_android_arm64.sh
```

It uses CMake's actual Android NDK toolchain, `ANDROID_ABI=arm64-v8a`, `ANDROID_PLATFORM=android-26`, `ANDROID_STL=c++_static`, `BUILD_TESTING=OFF`, and `SIN_BUILD_ANDROID_SMOKE=ON`. The build creates three outputs under `build-android-arm64/`:

- `libsin_compat_host_abi.so`: the portable C++ gameplay/session module (not engine-loaded)
- `sin_android_host_abi_smoke`: separate C11 PIE executable using Android `libdl`/`libc`
- `android-smoke-manifest.txt`: platform, NDK revision and both file hashes

The script checks both files are **ELF64/AArch64**, the library is a DYN shared object with the expected SONAME, and the smoke executable is an ELF PIE dynamic executable with `libdl.so` and `libc.so` dependencies. It rejects a smoke executable linked directly to `libsin_compat_host_abi.so`. The library must retain **exactly 13 intended public C ABI exports**, with no static-libc++ symbol leakage. It also compiles the API layout contract with the real NDK compiler.

The GitHub Actions artifact bundles the three files for user-controlled deployment, with a short retention window. Neither file contains game data.

## Actual Android device procedure (not yet performed by CI)

Use **only a developer-enabled Android ARM64 device you control**. No installation as an app or modification of SourceVR is needed. Android platform-tools (`adb`) must be accessible, and the phone/headset must expose an authorized `adb` shell.

```sh
# From a cloned checkout, with the ARM64 CI artifact unzipped into
# build-android-arm64/ as described above:
adb devices
bash scripts/run_android_host_abi_smoke.sh build-android-arm64
```

The helper verifies `ro.product.cpu.abi=arm64-v8a` and Android API level ≥ 26 **before pushing** anything. It places the files in `/data/local/tmp/sin_compat_api26_smoke`, marks the executable runnable, then invokes:

```sh
cd /data/local/tmp/sin_compat_api26_smoke
./sin_android_host_abi_smoke ./libsin_compat_host_abi.so
```

**Acceptance evidence** is the executable's own `C ABI dlopen v1 smoke test passed` output and process exit code zero. The host helper prints a second success message only after `adb shell` succeeds. Collect `adb logcat` and any loader errors for diagnosis. Multiple attached devices should be selected with `ANDROID_SERIAL` or a suitably configured adb environment before running the helper.

The target directory is under the Android shell user's temporary area; no SourceVR files, user game installations or system partitions are modified. There is no automatic teardown of the temporary directory; to clean up, use `adb shell rm -r /data/local/tmp/sin_compat_api26_smoke` only if you own those test files.

## Offline test coverage

A new `tests/test_android_deploy.sh` uses a mock `adb` program for deployment-command assertions, plus wrong-ABI, old-API and missing-artifact rejection. **Mock adb is not an Android device test.** Existing Linux `sin_host_abi_dlopen_tests`, native CTests, and Python research regressions remain prerequisites.

## R1 remains blocked

Even a passing on-device standalone loader is not game-engine integration. The public `tinsarfal/SourceVR` distribution does not support client/server gameplay DLL mods and its public source checkout lacks the engine interfaces needed for this code. The next R1 milestone still requires a lawful Source/Quest engine with module-loader support, integration of the callbacks with real player/weapon/viewmodel objects, and an observed in-engine smoke test. See [issue #12](https://github.com/JakX2/sin-episodes-quest3-recomp/issues/12).
