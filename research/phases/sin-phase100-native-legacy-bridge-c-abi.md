# Phase 100: a real native, loadable legacy VGUI keyboard and sound-token prebridge

**Milestone:** The project now has its first dedicated, independently authored shared-library implementation for two verified pieces of the original SiN 2006 client/server compatibility boundary. It is a **standalone native `libsin_legacy_bridge_abi.so`**, loadable by `dlopen` with a versioned and frozen **11-function C ABI**, not merely C++ research metadata.

## What this genuinely implements

### Keyboard translation for original SiN VGUI_System010 index 24

Phase99 proved the original 2006 `vgui2.dll` builds a 256-entry little-endian DWORD map at runtime (99 nonzero, unique values, 157 zeros), not from bytes embedded as an initialized array in the DLL. Its entire verified table SHA-256 is:

`6c743e453ce5d6b91214fa2db2fa6b935d7b1ca886c34ebda9538f1ab80a10b0`

The new native prebridge checks the **complete 1,024-byte table SHA-256 inside the shared module** before accepting it. A failed or changed table resets the mapper to unavailable. The original negative/out-of-range key behavior returns 0 when the map is installed; missing map returns `SIN100_NOT_READY`.

Only a user-privately reconstructed exact table is accepted; **the repo and release ZIP contain no original 2006 DLL or derived table payload**. The key table may be privately regenerated using Phase99's `audit_legacy_keymap_sound_namespace.py --export-keymap`, then packaged as exactly 1,024 little-endian bytes by the owner. The implementation recognizes a verified digest but does not execute the original x86 DLL or native VGUI virtual slot.

### Explicit original-sound-token to native-token crosswalk

Phase98/99 proved original `VSoundEmitter001` tokens are validated as full signed 32-bit indices while selected SourceVR 0.5.5 `VSoundEmitter002` candidate operations include 16-bit indexing. Directly truncating old IDs is wrong.

New native `sin100_bind_sound` records an explicit legacy signed ID, legacy count, candidate native ID, a `native_validated` assertion from a **trusted future adapter**, and exact name/roundtrip match. It rejects invalid ranges, native IDs above 65534, failures and ambiguous remappings. An old ID like 70000 can map to native 77 without truncation to 4464.

This service manages isolated per-context state behind a mutex. Name access copies NUL-terminated bytes to **caller-owned** output memory with two-pass buffer-length querying; it does not leak temporary C++ string pointers across ABI. Embedded NULs and overlong names are rejected. Calls never touch engine vtables.

The `native_validated` flag is **only a caller assertion, not verified by this library**. This is a deliberately conservative bridge *component*, not proof that an actual SourceVR sound-emitter method is callable. A trusted native adapter will still need to establish real method signatures and verify name/token outputs.

## ABI contract and files

- `include/sin_compat/legacy_bridge_c_api.h`: version `0x00010000`, opaque `sin100_context`, status enum, key install/translation, sound mapping, name copying and reset.
- `include/sin_compat/phase100_sha256.hpp`: dependency-free C++17 SHA-256, checked against standard vectors and the original derived table digest.
- `src/legacy_bridge_c_api.cpp`: the actual shared-library C ABI implementation, owned state and mutex protection.
- `src/legacy_bridge_exports.map`: Linux/Android ELF export allowlist with **exactly 11 `sin100_` functions**. Prior `libsin_compat_host_abi.so` **retains its independent 13-export ABI**.
- `tests/sin100_sha256_tests.cpp`: standard SHA-256 reference vectors.
- `tests/sin100_dlopen_smoke.c`: C11 executable loads module with `dlopen` / `dlsym` and tests state, version, validation, token mapping, buffer size and resets.
- `tests/test_phase100_ctypes.py`: 12 Python `ctypes` integration test cases, including 100 concurrent callers, context isolation, invalid hashes, 32-bit IDs, name copying, duplicate mapping and private key table if available.
- `CMakeLists.txt`: makes `sin_legacy_bridge_abi`, SHA CTest, C11 `dlopen` CTest, Python integration and Android-specific loader target.
- `scripts/build_android_arm64.sh` and `scripts/build_android_emulator_x86_64.sh`: compile new native `.so` and Android Bionic `dlopen` executable with old modules unchanged, verify exactly 11 dynamic exports and architecture/SONAME.
- `.github/workflows/ci.yml`: existing Linux tests and ARM64 builds now also compile the new library; x86_64 Android emulator job executes **both** original host ABI smoke and new Phase100 loader smoke; saves stdout pass marker.

## Verification and integrity

Before GitHub publication, an isolated standalone Phase100 package compiled under `-std=c++17 -Wall -Wextra -Wpedantic -Werror`, exported exactly 11 expected symbols and passed **CTest 3/3** (SHA, C11 `dlopen`, Python `ctypes` suite of **12 cases**). Independently, setting `SIN100_PRIVATE_KEYMAP_BYTES` to the user's exact derived 1,024-byte map exercised successful SHA-checked private loading and original key translations.

The GitHub full repo desktop CI, Android ARM64 compilation and Android Bionic runtime results are tracked independently after PR opening. Do not claim Android passes before jobs actually finish.

## Security, ownership and boundaries

This code does NOT provide `CreateInterface`, a compatible `VSoundEmitter001` virtual implementation, replacement SiN game `libclient.so` or `libserver.so`, an actual in-SourceVR link, VR locomotion/rendering, APK, playable SiN map, or a Quest 3 on-headset test. The Android emulator test runs **on Bionic x86_64**, not on ARM64 hardware.

This milestone is a genuine native, loadable **compatibility component** with tests, while the full engine/game integration remains blocked on verified native sound/VGUI ABI and game reimplementation.

## Next: Phase101

Implement a separate, tested provider interface that can supply verified SourceVR sound-emitter operations and rejects unverified methods at runtime; add a controlled shim surface for original client input behavior. Only after the remaining method signatures, ownership and engine hooks are validated should the live provider call selected SourceVR 0.5.5 AArch64 functions. Continue towards actual ARM64 game-specific `libclient.so`/`libserver.so` in small, testable slices.
