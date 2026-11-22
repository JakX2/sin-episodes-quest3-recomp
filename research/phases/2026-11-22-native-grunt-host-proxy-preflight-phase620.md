# Gameplay Phase620: Load the native Grunt host bridge beside the original server

## Objective

Phase619 created the actual standalone **seven-export** `sin_grunt_native_host_c_abi.dll`, and its Windows x86 and C++ CI passed. The game's existing Windows server proxy still only validated the older five-function behavior DLL. Phase620 provides an **independent opt-in loader** for the new native-host DLL inside the original-game factory process. It never fabricates a Source interface, instantiates a Grunt, calls AI, or returns a replacement server object.

The existing `server.dll` proxy still loads the owner's original `sin_original_server.dll` from a fixed adjacent path, calls its original `CreateInterface`, and returns the *identical object pointer and status*. The new optional probe runs after original DLL initialization but before forwarding, never in `DllMain`.

## Independent probe

A separate environment gate `SIN_PC_GRUNT_NATIVE_HOST_PROBE=1`, disabled by default, checks a **fixed absolute sibling path** `sin_grunt_native_host_c_abi.dll` (no supplied path, no search of arbitrary plugin folders). It uses `LoadLibraryExW(..., LOAD_WITH_ALTERED_SEARCH_PATH)`, verifies all seven exact `sin_grunt_native_*_v1` exports using `GetProcAddress`, and invokes only `sin_grunt_native_abi_version_v1()` to verify ABI version 1.

- `grunt-native-host-ready`: the DLL is loaded, all seven exports exist, version is 1; successful module handle is retained.
- `grunt-native-host-load-failed`: module missing or one of its dependent modules cannot load.
- `grunt-native-host-export-missing`: required export not found (detail is the zero-based symbol index).
- `grunt-native-host-version-rejected`: incompatible version; DLL is released.
- `grunt-native-host-path-failed`, `grunt-native-host-path-too-long`, `grunt-native-host-initonce-failed`: nonfatal diagnostics.

Every negative path **still invokes the real original `CreateInterface` with the exact original arguments**. It does not block the game or imply a current NPC session.

## Real Win32 x86 regression matrix

`pc/windows_x86_proxy/tests/test_grunt_native_host_proxy_phase620.c` verifies factory pointer and return-code identity versus a synthetic original PE32 DLL for all five modes, in per-PID isolated temp directories:

1. Disabled, with both valid libraries present: no native host module or trace.
2. Valid, with real independently built `sin_grunt_native_host_c_abi.dll` and its five-export behavior DLL dependency: both modules loaded and all seven exports accepted.
3. Missing native host DLL: fail-open, no host module.
4. Full seven-export fake with ABI version 2: reject and unload, factory unchanged.
5. Fake DLL with only version export: reject incomplete exports, factory unchanged.

The fixture DLLs and original server in CI are synthetic, with no commercial SiN code. Phase619's DLL itself is real independently written Win32 x86 compiled code. Root CMake gameplay code and original DLL byte accounting are unchanged.

## Owner-only disposable Windows test

The existing safe launcher `pc/windows_x86_proxy/Invoke-OwnerSiNStartupTrace.ps1` now additionally accepts `-GruntNativeHostProbe` **only when `-GruntBridgeProbe` is also supplied**, because the host library depends on the Phase613 behavior DLL.

It checks that both 32-bit DLLs are present in the same confirmed disposable game-copy `SE1\\bin` directory, rejects symlinks/reparse points, and checks MZ / I386 PE32 / DLL characteristics before launch. It enables/disables both opt-ins explicitly and restores the caller's previous environment variables on exit. The script still demands two SHA-authenticated original DLL/proxy pairs and a contained executable from the backed-up disposable copy.

After launch it checks **only the fresh `sin-pc-proxy-<launched PID>.log`**. A server `grunt-native-host-ready` line emits `OWNER_GRUNT_NATIVE_HOST: ABI_V1_LOADED_IN_LAUNCHED_PROCESS`; otherwise it emits `NOT_PROVEN`. A trace from another process never qualifies. This is evidence of in-process DLL loading, not actual Grunt entities, schedule execution, or behavior ownership.

After building the Phase620 Windows x86 DLLs, stage the two proxies in a disposable test copy using the existing protected staging procedure and place `sin_grunt_behavior_c_abi.dll` and `sin_grunt_native_host_c_abi.dll` beside `server.dll`. Run from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\pc\windows_x86_proxy\Invoke-OwnerSiNStartupTrace.ps1 -GameBin "D:\SiN-DisposableTest\SE1\bin" -GameExecutable "D:\SiN-DisposableTest\SinEpisodes.exe" -DisposableTestCopyConfirmed -GruntBridgeProbe -GruntNativeHostProbe
```

**A real engine-created Grunt still requires a verified native host** with actual entity factory, spawn/think/despawn callbacks, authoritative handle identity, condition IDs, and NPC behavior transition/schedule hooks. The proxy explicitly does none of that. We are deliberately not duplicating working HL2 physics, collision parameters, model/prop systems, or rendering.

**Authentic 2006 native recovery totals are unchanged: 43 distinct verified functions / 11,233 original bytes.** Phase620 adds original-free integration and tests.
