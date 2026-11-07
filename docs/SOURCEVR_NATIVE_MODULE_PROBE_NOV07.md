# Native SourceVR game-name module probes: libserver.so and libclient.so

7 November 2026. Draft implementation stacked on PR #146.

## New buildable native modules

This phase produces actual Android-ready shared objects named
**libserver.so** and **libclient.so**, which correspond to the
filename normalization rules found in the owner's SourceVR 0.5.5
ARM64 engine/filesystem research. Both have a public ELF factory
named **CreateInterface**, matching the C factory entrypoint resolved
by the SourceVR engine's actual dlsym call.

The server and client modules are compiled separately from the
independently authored sourcevr_module_probe.c, with module-specific
roles. An ELF version script permits exactly one callable public
symbol: CreateInterface. The modules implement an intentionally small
private **SiNProbeModule001** diagnostic factory name, which returns
only a fixed C struct containing magic, version, module role and
pointer width.

**Every real engine request fails closed.** ServerGameDLL010 through
ServerGameDLL003, VClient017 and VClient011 all return NULL with
the failure result. The code contains no game-object C++ vtables,
SourceVR native engine functions, copied Windows x86 DLL bytes,
NPCs, graphics, assets or engine maps. This is a safe native loading
experiment, not an implementation of IServerGameDLL or IBaseClientDLL.

## Why those exact filenames?

Original private DLL and Android ARM64 analysis from PRs #145/#146
establishes:

- Original SiN 2006 server requests ServerGameDLL003; client uses
  VClient011. Neither original x86 game-module ABI is directly
  compatible with modern ARM64 code.
- Native SourceVR engine requests module name server.so and client
  through GAMEBIN, then uses Android dlsym to resolve CreateInterface.
- The actual ARM64 filesystem loader checks APP_MOD_LIB and
  APP_LIB_PATH, normalizes lib prefix and .so suffix, then invokes
  Android dlopen.
- The native SourceVR engine attempts ServerGameDLL010 through 004,
  while its client requires VClient017.

See the prior SHA256-pinned native loader and factory audit documents
in research/phases/ for exact virtual addresses and original evidence.

## Independent executable tests

The new C11 loader smoke:

1. Sets APP_MOD_LIB to the staged module folder and APP_LIB_PATH to
   an invalid directory, then loads module names server.so and client.
2. Calls dlsym(CreateInterface) in each file, rejects all actual
   engine game-interface names, and queries only the private
   SiNProbeModule001 record to confirm the module's role.
3. Makes APP_MOD_LIB invalid but APP_LIB_PATH valid, then loads
   both modules again by their full libserver.so/libclient.so names.
4. Makes both directories invalid and verifies that neither module
   is loaded.

The harness intentionally simulates filesystem path normalization:
it is NOT the actual SourceVR engine code. A Linux desktop CTest
uses the same source as the Android Bionic API29 emulator smoke.
The Android NDK API26 arm64-v8a build validates ELF architecture,
SONAME, one callable export and the absence of direct DT_NEEDED
dependencies on the game-name modules from the loader executable.

Desktop reproduction:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target sin_sourcevr_module_probe_tests
    ctest --test-dir build -R '^sin_sourcevr_module_probe_tests$' --output-on-failure

Android NDK build:

    bash scripts/build_android_arm64.sh

The actual game-name ELF outputs reside in
build-android-arm64/sourcevr-probe/libserver.so and
build-android-arm64/sourcevr-probe/libclient.so. They are
experimental inert modules, NOT substitutes for the private game's
real client/server libraries or an installer.

## Critical next steps toward gameplay

1. Determine supported native custom-module deployment and profile
   installation in the actual SourceVR Android application.
2. Reconstruct or obtain a legally usable ARM64 game-module C++
   interface contract for ServerGameDLL010..004 and VClient017.
   A matching CreateInterface symbol is only the first boundary.
3. Implement full engine-owned entity factories and lifecycle,
   then connect our separately written SiN assault NPC, map I/O,
   physics, weapons, networking and XR code.
4. Test with an authorized copy of the real campaign on Quest 3.

**Not proven:** the SourceVR engine can load user modules from an
external directory; the factory could provide a valid game object;
SiN maps or levels render; or any version of SiN is playable on
Meta Quest 3. No original commercial engine/game binary or asset is
redistributed by this change.
