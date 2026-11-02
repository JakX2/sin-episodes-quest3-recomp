# Native C bridge (R1 groundwork, NOT an engine module)

`sin_weapon_bridge` is a shared-library wrapper around the portable `WeaponRuntime`, with a small exported C ABI and opaque handles. It contains no SiN assets, engine code, SourceVR headers, or third-party binary modules.

## Supported operations

The host supplies callbacks for activity-to-sequence lookup, viewmodel configuration, playback, and flatscreen FOV changes. The bridge accepts per-weapon settings (including settings loaded from privately owned scripts elsewhere), accepts per-frame held button bits, and exposes explicit reload timing events and ammo snapshots. Destroy a handle before its callback user data is destroyed; do not call methods concurrently on a single handle.

Each function returns success/failure without letting C++ exceptions cross the boundary. Callbacks should not throw. There is no general-purpose error string API yet. This is a single-threaded, experimental ABI version 1, not stable for distribution.

**Critical:** This bridge **does not** export Source's `CreateInterface` and does not implement `VClient017`, `ServerGameDLL010`, engine interfaces, prediction/networking, scene lifecycle, or Android OpenXR glue. Therefore it cannot replace SourceVR's `libclient.so` or `libserver.so` and is not a playable Quest port.

## Native desktop smoke test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The C test links against the real C++ shared library and exercises zoom, callbacks, VR FOV protection, guarded reload transfer, ammo counts, holster and invalid parameters.

## ARM64 Android cross-compile (Android NDK)

```sh
cmake -S . -B build-android \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-android --target sin_weapon_bridge --parallel
```

Expected output: `build-android/libsin_weapon_bridge.so` (exact path depends on generator).

Inspect the ELF architecture and exported symbols with `llvm-readelf -h -s` using the NDK binutils. An ARM64 build is only a compile milestone, **not** proof that the library can be loaded by SourceVR or used as a Quest APK.

## Next actual engine milestone

Choose a compatible game-module SDK revision, implement the Source `CreateInterface`/game lifecycle interface with correct vtables, and verify a minimal implementation loading **in an isolated copy** of the target runtime. Keep launcher/package modifications local and retain legitimate game installation assets outside the public repository.
