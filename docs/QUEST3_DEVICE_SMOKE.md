# R1 preflight: dynamically load our weapon bridge on Quest 3

This is a deliberately narrow **device-side runtime loading smoke test**. It verifies our own ARM64 shared library can be opened with `dlopen`, that its C ABI exports resolve with `dlsym`, and that a simulated weapon lifecycle works. It **does not** invoke any Valve/SiN/SourceVR library, load Source game interfaces, draw a frame, or prove a playable port.

## What was built

- `libsin_weapon_bridge.so`: our portable weapon runtime in an Android AArch64 shared object.
- `sin_weapon_bridge_loader`: an Android AArch64 shell executable which uses `dlopen` and `dlsym` to call the bridge via its public C ABI. No linker dependency on our library.
- `libc++_shared.so`: the C++ runtime from the pinned Android NDK, needed by our current build.

All three files are packaged as the `sin-weapon-bridge-android-arm64` GitHub Actions artifact for this branch. **No original game content or SourceVR binaries are included.**

## Run in Quest 3 through ADB (Windows PowerShell)

Prerequisites: Quest 3 in Developer Mode, ADB installed, USB debugging authorized and the GitHub Actions artifact extracted to a local folder.

Open PowerShell in the extracted folder:

```powershell
adb devices
adb shell mkdir -p /data/local/tmp/sin-weapon-smoke
adb push .\libsin_weapon_bridge.so /data/local/tmp/sin-weapon-smoke/
adb push .\libc++_shared.so /data/local/tmp/sin-weapon-smoke/
adb push .\sin_weapon_bridge_loader /data/local/tmp/sin-weapon-smoke/
adb shell chmod 755 /data/local/tmp/sin-weapon-smoke/sin_weapon_bridge_loader
adb shell "LD_LIBRARY_PATH=/data/local/tmp/sin-weapon-smoke /data/local/tmp/sin-weapon-smoke/sin_weapon_bridge_loader /data/local/tmp/sin-weapon-smoke/libsin_weapon_bridge.so"
```

Expected output:

```text
PASS: dlopen + C ABI + zoom/reload/VR-FOV callbacks
```

If it fails, preserve the full ADB shell output and Android version. A failure could reveal missing system libraries, executable permissions, linker namespaces or a genuine runtime bug. No system files or installed apps are modified.

Cleanup after testing:

```powershell
adb shell rm -rf /data/local/tmp/sin-weapon-smoke
```

## Desktop regression test

The same smoke test runs in Linux CI via CTest with a real `dlopen` of `libsin_weapon_bridge.so`. This catches C ABI/export problems without waiting for headset access.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/sin_weapon_bridge_loader ./build/libsin_weapon_bridge.so
```

## Remaining R1 prerequisites

1. Establish lawful, compatible SourceVR game-module development headers and actual factory registration requirements. Static `VClient017` and `ServerGameDLL010` strings do **not** establish full vtable or lifecycle ABI compatibility.
2. Verify real SourceVR module loading in an isolated local installation. Its public launcher only documents content-only mod support.
3. Bind an engine-owned player/viewmodel and animation callbacks to `IWeaponHost` using actual engine lifecycle events.
4. Run a real level and validate animations, weapons, entities, saving, and HMD transforms. None of these are proven by this smoke test.

Do not install `libsin_weapon_bridge.so` as `libclient.so` or `libserver.so`. It is not a substitute for either game module.
