# Original SiN Episodes 2006 client/server startup factory contract

8 November 2026. Direct read-only inspection of the owner's genuine PE32 x86 game DLLs. Neither game DLL nor commercial assets are committed.

## Real original binary witnesses

Original server.dll SHA256:
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

Original client.dll SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

| Function | Original PE image VA | x86 stack cleanup | Common failure return |
| --- | --- | --- | --- |
| Server CServerGameDLL DLLInit candidate | 0x221E7640 | ret 0x10 (four pointer args) | 0x221E79A1: false |
| Client CHLClient Init candidate | 0x240FE0E0 | ret 0x0C (three pointer args) | 0x240FE59E: false |

The original game object primary vtable index and roles are consistent with the public Source SDK method names; the stripped original code has no recovered C++ function symbols. The factory requests, call register, interface strings, saved globals and explicit failure branches are direct original machine-code evidence.

## Server: 16 required interfaces

In exact original request order:

1. VEngineServer021
2. VoiceServer001
3. VEngineCvar002
4. VEngineServerStringTable001
5. StaticPropMgrServer002
6. VEngineRandom001
7. IEngineSoundServer003
8. SpatialPartition001
9. VModelInfoServer001
10. EngineTraceServer003
11. VFileSystem012
12. GAMEEVENTSMANAGER002
13. VEngineCache001
14. VSoundEmitter001
15. ServerUploadGameStats001
16. ISERVERPLUGINHELPERS001

The VFileSystem012 call at original VA 0x221E7749 invokes a DIFFERENT factory pointer in EDI. Other listed server requests invoke the primary engine factory in ESI. All 16 check their result in EAX, save it to their specific global slot, and jump to the same false-return exit if null.

After additional setup, the server requests VDebugOverlay003 at 0x221E7944. This is NOT a mandatory request at that site: the original branch on a null result goes into optional debug-allocation handling, and both branches ultimately return success.

## Client: 21 direct and 2 additional required interfaces

In original ESI engine factory request order:

1. VEngineClient012
2. VEngineModel012
3. VEngineEffects001
4. VEngineCvar002
5. EngineTraceClient003
6. VEngineRenderView011
7. VDebugOverlay003
8. VMaterialSystem076
9. VEngineCache001
10. VModelInfoClient003
11. VEngineVGui001
12. VEngineClientStringTable001
13. SpatialPartition001
14. VEngineShadowMgr002
15. StaticPropMgrClient004
16. IEngineSoundClient003
17. VFileSystem012
18. VEngineRandom001
19. VENGINE_GAMEUIFUNCS_VERSION004
20. GAMEEVENTSMANAGER002
21. VSoundEmitter001

The VFileSystem012 request on the client uses the engine factory, unlike the server. Client VDebugOverlay003 is REQUIRED, unlike the server optional request later in its initialization.

After other subsystem setup, two additional interfaces must succeed:

- MaterialSystemHardwareConfig012 at original 0x240FE362, obtained by a material-system virtual method at +0x20. Missing result jumps to the shared client failure exit.
- MatSystemSurface003 at original 0x240FE3E1, obtained by a separate surface/material factory virtual method at +0x08. Missing result also jumps to the shared failure exit.

The client also looks up VMaterialSystemStub001 at 0x240FE349, but absence of the result does not directly jump to the shared failure exit at this site. It is an optional probe for this limited interface acquisition audit.

Thus 23 client interface requests are directly proven null-checked, with 1 optional client probe. Other client startup operations and internal returns can also fail. This is NOT a complete reconstruction of Init.

## Native SourceVR implications

Both original game DLLs require the old VSoundEmitter001 interface. Prior direct SourceVR 0.5.5 Android ELF analysis observed VSoundEmitter002 from its native sound-emitter library and demonstrated incompatible original method-slot use. Directly renaming the interface or handing back an arbitrary object would be unsafe: a real bridge must preserve the behavior and C++ method ABI.

In addition, SiN 2006 game objects use 31 server / 48 client slots while SourceVR 0.5.5 native game objects use 47 / 78. A bigger table is not automatically equivalent to the old one and the new slots cannot be guessed.

## Committed implementation

- research/tools/audit_original_sin_dllinit_factories.py: standard-library read-only PE32 and SHA256 verifier of each original x86 PUSH name, CALL through the right engine/filesystem factory, EAX null check, interface global store, common false-exit branch, optional-success path and indirect material/surface calls. Requires owner-private bin.zip, produces metadata only.
- research/tests/test_audit_original_sin_dllinit_factories.py: 12 fabricated instruction-window regressions; CI has no game DLL.
- include/sin_compat/original_dllinit_factory_contract.hpp: independently written C++17 ordered dependency list and missing-interface short-circuit preflight. This invokes supplied host callbacks only, not unverified native engine interfaces.
- tests/test_original_dllinit_factory_contract.cpp: desktop CTest checks correct provider, missing legacy sound-emitter, wrong interface, and derived dependency failure.
- CMake and original decomp GitHub Action integration.

Original-binary audit on a private original bin.zip:

    python3 research/tools/audit_original_sin_dllinit_factories.py --bin-zip /private/path/bin.zip

Synthetic public regression:

    python3 -m unittest discover -s research/tests -p test_audit_original_sin_dllinit_factories.py -v

## Ready-to-ship boundary

This is a meaningful original-DLL subroutine reconstruction, NOT a game build. A release still requires a supported Android ARM64 game module loader/host, real modern game-interface implementations, correct 2006-to-native interface adapters, full SiN entity/AI/weapon/map behavior, game data loading from authorized original assets, OpenXR stereo rendering and tracked input, and successful native Quest 3 campaign testing.

The existing demonstration libserver.so/libclient.so modules are deliberately inert. They cannot launch SiN and must not be presented as a playable APK. Keep all development on draft PRs pending verified game runtime integration.
