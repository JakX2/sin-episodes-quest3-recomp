# Phase 71: Run the real C ABI weapon smoke in an Android emulator

**Evidence date:** 31 October 2026. **Scope:** independently authored SiN compatibility runtime; no Ritual/Valve binaries or game assets.

## Purpose and precise limit

Phase 69 produced a real official-NDK ARM64 `.so`; Phase 70 produced an ARM64 Bionic `dlopen` smoke executable to run on a user-controlled Quest/Android device. Those jobs only *compiled* Android files, so a successful loader operation on Android was not yet verified.

Phase 71 adds an independent **x86_64 Android emulator** execution path in GitHub CI. A pinned official Android NDK r27c builds both a Bionic-linked C++ shared library and a C11 PIE `dlopen` program for x86_64 / API 29. The [ReactiveCircus emulator runner](https://github.com/ReactiveCircus/android-emulator-runner) boots an actual Android emulator with API 29, and the deployment helper uses `adb shell` to run the smoke program. Its exit status determines the CI job's result: a compilation-only run cannot count as a pass.

**Important:** Passing this job would establish *Android Bionic dynamic-loader and native C ABI execution on an x86_64 emulator*, not ARM64 runtime execution, a real Meta Quest 3 device run, a game module inside SourceVR, a SiN level or a Quest APK. The Phase 70 ARM64 artifact and hardware test remain separately required.

## Source and test coverage

- `scripts/build_android_emulator_x86_64.sh` builds `libsin_compat_host_abi.so` and `sin_android_host_abi_smoke` using `ANDROID_ABI=x86_64`, API 29, `c++_static`. It audits ELF64/x86_64 files, Bionic's `/system/bin/linker64` interpreter, SONAME and dynamic imports, and exactly 13 intended versioned C ABI exports. It fails on a missing or extra export.
- `scripts/run_android_host_abi_smoke.sh` is unchanged for normal Quest/ARM64 users. Its default remains `arm64-v8a`. **Only an explicit** `SIN_SMOKE_EXPECTED_ABI=x86_64` allows the new emulator. Both allowed values require Android API 26 or newer. Any unsupported override, mismatched device architecture or missing binary fails **before upload**.
- `tests/test_android_deploy.sh` verifies the two accepted target ABIs and rejects mismatches/unsupported values using a fake adb implementation, without misrepresenting fake output as a real device test.
- The smoke executable uses the same C11 implementation as Phase 70, resolving all 13 exported symbols by `dlopen`/`dlsym` and invoking guarded weapon/player selection, indexed viewmodel create/reuse, `HideWeapon`, ADS with HMD FOV unmodified, late ammo transfer exactly once, callback reentrancy rejection, stale/invalid entity checks and cleanup.

## Reproduce with the SDK and an x86_64 Android emulator

```sh
export ANDROID_NDK_HOME=/path/to/official/android-ndk-r27c
bash scripts/build_android_emulator_x86_64.sh
# Boot an Android x86_64 emulator running API 29+, then:
SIN_SMOKE_EXPECTED_ABI=x86_64 \
  bash scripts/run_android_host_abi_smoke.sh build-android-emulator-x86_64
```

Run actual Quest/ARM64 hardware smoke using **the original Phase 70 instructions**, **without** the x86_64 override:

```sh
bash scripts/run_android_host_abi_smoke.sh build-android-arm64
```

## Acceptance criteria

1. A GitHub Actions `android-emulator-runtime` job shows **successful completion of the emulator boot and shell execution step**, not just CMake.
2. A device-side program prints `C ABI dlopen v1 smoke test passed` and exits zero, propagated through `adb shell`.
3. The exact 13 public C symbols remain the only exported dynamic symbols.
4. Existing seven CTest executables, Python regressions and official ARM64 Android NDK job continue passing.
5. Mark real ARM64 device runtime and SourceVR in-engine integration **unverified** until supported by actual runtime traces.

This is a **staging/validation milestone** (R0.5). It does **not** change R1's requirement for a lawful SourceVR/Source-variant engine repository with real game module loader and headers, discussed in issue #12.

## Independent CI result, verified 31 October 2026

**PASS: Actual Android execution.** Commit `e3f2bb1df223333d0fb2d35054807bc8fc88fa46` passed [push workflow #41219361107](https://github.com/JakX2/sin-episodes-quest3-recomp/actions/runs/41219361107) and [pull-request workflow #41219366441](https://github.com/JakX2/sin-episodes-quest3-recomp/actions/runs/41219366441). Both ran three jobs: Linux/reference test, official Android ARM64 library+loader cross-build, and **real x86_64 Android emulator runtime**.

The independently retrieved GitHub Actions log for runtime job `124590493583` shows:

- Official Android `google_apis` **API29 x86_64** system image installed and emulator boot completed.
- The x86_64 API29 Bionic loader and C++ `.so` passed the NDK build and ELF/export checks.
- `adb` pushed 497,592 bytes of `libsin_compat_host_abi.so` and 35,024 bytes of `sin_android_host_abi_smoke` to the Android guest.
- The real `adb shell` execution printed **`C ABI dlopen v1 smoke test passed`**, followed by the deployment script's successful completion notice; the emulator job exited zero.

This is stronger evidence than Phase 70's compile-only CI. It remains an **x86_64** emulator result, **not** an ARM64 Quest/phone execution; physical-device execution and actual SourceVR engine integration still await verification.
