# Phase 72: ARM64 hardware smoke evidence hardening

**Status:** Tested offline with fake adb; real Android x86_64 emulator CI required. Physical ARM64 Quest 3 execution is **not yet verified**.

## Why

Earlier, a zero-exit `adb shell` with no output could be incorrectly reported as a passing Android loader. The smoke script now requires the exact native `C ABI dlopen v1 smoke test passed` marker, zero loader exit, and SHA-256 equality of both copied binaries. It writes a local key=value evidence record **only after success**, refuses to overwrite prior records, and excludes device serial numbers, fingerprints and usernames.

## Run on your own developer-enabled Quest 3

Extract Phase70+ official ARM64 NDK artifacts into `build-android-arm64/`: `libsin_compat_host_abi.so` and `sin_android_host_abi_smoke`. Install `adb`, authorize USB debugging on the headset, and use:

```sh
bash scripts/run_quest3_arm64_smoke.sh build-android-arm64 quest3-arm64-hardware-evidence.txt
```

If several devices are connected, set `ANDROID_SERIAL` privately before running. Do not publish the serial. The strict wrapper forces `arm64-v8a` and rejects the common emulator indicator `ro.kernel.qemu=1`. This indicator is **not cryptographic hardware attestation** and cannot establish that the device is specifically a Quest 3. The physical operator must independently confirm device identity. The report records architecture, API, hashes and pass markers only.

No SourceVR installation, proprietary game files, source engine or APK is involved in this test.

## CI and reproducibility

`bash tests/test_android_deploy.sh` exercises success and failures with **mock adb**: wrong ABI/API, missing binaries, invalid architecture override, silent/misbehaving loader, wrong remote hash, nonzero loader exit, known emulator in hardware mode, and refusal to overwrite evidence. These tests cannot prove real Android behavior.

The x86_64 Android emulator CI job now runs the same hardened `adb` script and requires `result=PASS`, `remote_sha256_verified=true` and `loader_marker_verified=true`. It publishes a redacted evidence artifact only when an actual emulator runtime pass occurs. The official ARM64 NDK compiler/linker job continues to run separately.

**Evidence boundary:** Phase71 already showed real native execution in an Android x86_64 emulator. Phase72 cannot claim ARM64 hardware/Quest execution until the owner runs the strict wrapper on a physical device and independently verifies it. A passing standalone C ABI smoke is **not** SiN gameplay inside SourceVR. True game module access remains blocked by issue #12.

No game binaries, user data or third-party engine source are distributed.
