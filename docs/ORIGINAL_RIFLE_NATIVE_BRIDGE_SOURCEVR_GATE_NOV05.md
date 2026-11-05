# Native original-rifle C bridge and SourceVR ARM64 integration gate

**November 5, 2026. Status: mock-host native bridge, NOT engine weapon registration or playable Quest port.**

## Actual owner's input evidence

- Original SiN SDK and shipped weapon scripts establish classname `weapon_assault_rifle`, 45-round `AR` magazine, `ARNADE` secondary ammo, FOV 40, original models, sounds, and MobGrunt equipment.
- Original Windows x86 `server.dll` contains `CWeaponAssaultRifle` factory RTTI and verified `grenade_ar` projectile-creation path. Those original x86 pointers are **not** callable on AArch64.
- Owner's SourceVR loader subset, `native/hl2/libserver.so`: ELF64 AArch64, SHA-256 `fdc8bb8777e044875609da75850fdf0a58155d5caf84b834a9d72486fe0a2322`, `CreateInterface` is a GLOBAL dynamic export. `native/hl2/libclient.so`: ELF64 AArch64, SHA-256 `0a8d1082e86d857cad812a65ca682695098de2498919931b08e87fb3b38bc362`, also exports `CreateInterface`.
- The inspected SourceVR server `.dynsym` does **not** export `CreateEntityByName`, `CanCreateEntityClass`, or any named standalone entity-factory registration API. This does **not** prove the functions are absent: the stripped native binary contains internal references/diagnostics for `CreateEntityByName` and `CanCreateEntityClass`, and RTTI-like `CEntityFactory` strings.
- The SourceVR native server contains a `ServerGameDLL010` string, whereas the original 2006 SiN server static interface audit identifies `ServerGameDLL003`. A version-string mismatch is evidence of an integration risk, not proof of any exact method ABI or suitability of an adapter.
- The SDK ZIP contains FGD/script/tool binaries but **no C++ `.cpp`/`.h` game code**. The SourceVR loader subset contains native ELF binaries and manifests, not a ready-to-compile SourceVR game server source tree.

## New deliverable

The standalone shared library `libsin_original_rifle_bridge.so` provides three C ABI exports:

1. `sin_original_rifle_abi_version_v1`
2. `sin_original_rifle_get_info_v1` to expose source-verified original classname, models, AR/ARNADE ammo and 45-round configuration
3. `sin_original_rifle_create_session_v1` to create the already tested engine-neutral `sin_session_v1` for the **exact original classname** `weapon_assault_rifle`. It rejects other classes and malformed callback ABI tables, and sets the initial clip to 45.

The library depends on `libsin_compat_host_abi.so`. Its ELF version map restricts the newly exported symbols to the above three. The genuine C-language test uses fake engine callbacks for original model binding, 45-round reload transfer, flatscreen ADS FOV and VR headset FOV protection. These callbacks are **not** copied from SourceVR or original SiN engine DLLs.

### Build/test on a normal development machine

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
readelf --dyn-syms --wide build/libsin_original_rifle_bridge.so | grep sin_original_rifle_
```

The official Android NDK cross-build is performed by CI. Its success indicates a valid AArch64 shared-library build; it does not load into an actual Quest device or SourceVR server.

## Next non-negotiable gate

Obtain and inspect a buildable, legally usable **game server module source tree** compatible with the chosen ARM64 SourceVR loader. Independently implement the native `weapon_assault_rifle` entity class and its factory registration, then connect native entity lifecycle to `sin_original_rifle_create_session_v1`. Implement native model precache, sound, animations, real `AR` ammo consumption, firing and `grenade_ar` secondary projectile. Test that the real engine's `CreateEntityByName("weapon_assault_rifle")` returns a valid live game weapon before patching any original MobGrunt NPC. Do not patch the private binary or claim the sidecar bridge alone registers an entity.

After a successful native gun test, restore `roof_grunt_1_seq1` with its original equipment, `sleepstate=3`, `NumGrenades=5`, spawnflags 516, and `grunt_unkillable` map filter. Temporary HL2 Combine/AR2 fallbacks are not the shipping plan.

No proprietary game DLLs, assets, SDK archives, SourceVR modules or personal device identifiers are included.
