# Phase 78: HL2VR Standalone 0.989 engine/module ABI evidence

**Status:** Static inspection of owner-supplied `HL2VR-Standalone-0.989(1).7z` and earlier `PortalVR_1.1(1).apk`; **no engine module was loaded or executed** and R1 remains blocked. Date: 2026-11-01.

## Provenance (SHA-256)

- Owner 7z: `bb24250e37be1240fb3c65da58124913ea781ff61f9b88a51a897566ba07c5d9`
- Extracted `HL2VR-Standalone-0.989.apk` (268,326,668 bytes): `5ce5c4c16fd2aa934db45ed51de32878a561ed3dfe5405ee502ef199238a785c`
- This **exact APK digest** matches GitHub's [0.989 release asset](https://github.com/alferiko/half-life-2-stanalone-quest-releases/releases/tag/0.989) metadata, published September 21, 2026.
- The 7z archive itself differs from GitHub's release 7z hash; packaging may have been changed, and the exact cause is not verified. The **APK binary matches** GitHub's release.
- The included `RELEASE_NOTES_0.989.md` has an inconsistent APK digest (`c76ad09b54c586908e0d016b0037c714ff68e044a4c785a2483049cf87de2ea6`), likely an outdated build note. Do **not** treat this alone as evidence of modification.

## Dual Android engine stack

The APK's package string is `com.valvesoftware.source.vrtest`, versionName `1.17.26-vr0.989`, arm64-v8a only. It contains **74 native .so files**, arranged as two sets of 37:

| APK directory | Backend | Game/runtime modules |
|---|---|---|
| `lib/arm64-v8a/` | Vulkan/Hybrid | engine, launcher, sourcevr, client, server and campaign-specific binaries |
| `assets/hl2quest_turbo/arm64-v8a/` | GLES/Turbo | second renderer/runtime and game binaries |

Both included native provenance manifests identify source revision `093be22b8ebac99ede17266e13d2f8fccc0025ac`. Every single **37/37 per-library SHA-256 entries** in each provenance manifest matched the exact packaged APK bytes (74/74 total). For the 36 shared library names, 23 have identical file hashes and 13 differ. Vulkan alone has `libdxvk_d3d9.so`; Turbo alone has `libtogl.so`.

| Campaign | Client game module | Server game module | Bundled status |
|---|---|---|---|
| HL2 | `libclient.so` | `libserver.so` | User-selectable |
| Episode One/Two | `libclient_episodic.so` | `libserver_episodic.so` | User-selectable |
| Portal 1 | `libclient_portal.so` | `libserver_portal.so` | User-selectable; Hybrid only |
| Portal 2 | `libclient_portal2.so` | `libserver_portal2.so` | Packaged but not offered in launcher per release notes |

The owner archive includes the `BUILD_GAME_CACHE.sh/.bat` and other cache builders. They deliberately exclude desktop executables, DLLs, game saves and original user mods, taking legally held Steam game data and placing it under `/sdcard/srceng`. The Portal `gameinfo.txt` created by the builder uses `gamebin "portal/bin"`. This is a search path, **not proof of an arbitrary Android gamecode loader**. The APK includes its own native gameplay binaries.

## Concrete comparison with PortalVR 1.1

Both packages advertise these **exact interface strings**:

- `VClient017`; `ClientVirtualReality001`
- `ServerGameDLL010`; `ServerGameClients003`, `ServerGameClients004`
- `VEngineClient014`; `VEngineServer023`
- `SourceVirtualReality001`

Both APKs also export `CreateInterface` from actual ARM64 engine/client/server/VR libraries and import `dlopen`, `dlsym`, `dlclose`. This is evidence of a real dynamically loaded Source-style module architecture. **It is not evidence that SiN's 2006 Windows x86 DLLs are ABI-compatible**, that an external mod is permitted, or that a C++ vtable layout is stable between releases.

Additional concrete HL2VR engine diagnostics observed in ELF strings include:

- `Could not get client.dll interface from library client`
- `Client.dll Init() in library client failed.`
- `Can't start game, no valid server.dll loaded`
- `client.dll is not VR-compatible.`

The last diagnostic raises a specific VR interface/lifecycle compatibility question, but its precise gate/branch has **not** been disassembled or executed. The source revision embedded in build manifests identifies a build, **not publicly available C++ engine source**, SDK headers, or permission to redistribute them.

## Consequences for the SiN recompilation

1. Existing packaged HL2/episodic/Portal client and server modules prove the engine has **multiple separately compiled gamecode targets**, which increases confidence that an *authorized host integration* is technically plausible.
2. Determine **actual loader selection and module path** for the supported campaigns; a path in a gameinfo file or an exported factory does not establish permission or ability to load custom binaries.
3. Seek a legally usable, accessible game-module SDK or authorized matching source revision. Independently confirm `CreateInterface` factories, interface vtable structure, entity ownership, save/restore and VR client callbacks against **real headers**.
4. Only after a real engine host exists, compile a synthetic test game module, load it in Android, check callbacks and then attach Phase 74/77 entity I/O and Phase 76 weapon configuration.
5. Preserve all actual APKs, VPKs, .so files and game content **outside** this public repository; this document contains just independently collected metadata.

**Outstanding:** R1 engine integration issue #12 and live campaign scripting issue #20. No claimed Quest 3 gameplay, private engine source access, SiN client/server rebuild or HMD test.
