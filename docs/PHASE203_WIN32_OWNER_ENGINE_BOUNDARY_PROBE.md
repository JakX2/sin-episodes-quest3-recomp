# Phase 203: owner-only Windows x86 module boundary probe and reversible game-copy staging

13 November 2026. **Stacked draft on Phase202 PR #256**. Windows PC is the integration target before Android and Quest 3. No commercial SiN game DLLs, engine DLLs, original maps or extracted assets are committed or distributed.

## Why this phase exists

The recovery atlas at Phase202 contains **203 individually SHA-reviewed original native functions** (111 server, 92 client), including cache-tree data structures, native server allocator dispatch, and native client initialization investigations. **Those tested C++ models do not yet constitute playable SiN replacement DLLs.** The existing `pc/windows_x86_proxy` Phase147 proxy loads owner-original renamed game DLLs and forwards their exact exports. Previously its opt-in debugger trace emitted only the requested interface name, not a success/failure result or a load diagnostic.

Phase203 improves the **actual Windows engine-loading investigation seam**, while distinguishing three different milestones:

1. Synthetic Win32 CI: test x86 PE32 loading with **fake** original server/client DLLs, exact factory export/name behavior and four C++ grenade forwarders.
2. Owner-only preflight: test a **legal, copied, SHA-matched 2006 installation** with real original DLL files, without involving the original game executable.
3. Still to be proven: the **real 2006 Windows game executable** loads both proxies, requests original interfaces, initializes a map and runs SiN gameplay. The project has **NOT** established this third milestone.

**No new original machine-code functions or native bytes are claimed in Phase203.** The reviewed atlas remains 203 (111 server, 92 client).

## New native Win32 x86 executable

`pc/windows_x86_proxy/tests/owner_module_probe.c` builds a real Windows 32-bit executable named `sin_pc_owner_module_probe.exe`. It only uses Win32 APIs such as `GetModuleFileNameW`, `LoadLibraryExW` and `GetProcAddress`, and derives absolute sibling DLL paths from the executable location. It must be placed in the same directory as the two proxies (`server.dll`, `client.dll`).

By default it **only loads PE32 DLLs and checks exports**, without invoking any game factory. It validates the undecorated original `CreateInterface` export in both modules, plus the four original 2006 server decorated `CGrenade...::Detonate` export forwarders. A forwarded export may trigger a load of the sibling renamed original DLL, as intended. It fails with a nonzero exit and a Win32 error code when loading or any export resolution fails.

Only when the owner explicitly supplies `--query-factories` does the executable call the factories, checking that:

- `server.dll` accepts **`ServerGameDLL003`** (non-null, status 0) and rejects `ServerGameDLL010` (null, status 1).
- `client.dll` accepts **`VClient011`** (non-null, status 0) and rejects `VClient017` (null, status 1).
- A sibling module called `sin_original_server.dll` / `sin_original_client.dll` was loaded.

This is a **factory handshake, not Init/PostInit and not a running engine**. Running `--query-factories` against real game code without its engine is an opt-in experiment; an owner should start with the default exports-only mode and capture any load failures. Do not call virtual methods on these returned interface objects.

## Improved opt-in debug trace

The existing proxy's `SIN_PC_PROXY_TRACE=1` mode now logs to **Windows debugger output** (`OutputDebugStringA`):

- `original-load-failed` with the Win32 last-error code.
- `original-factory-export-missing` if a sibling lacks the requested export.
- `original-load-ok` on successful original module load.
- `request` with the interface name.
- `result-object` or `result-null`, with the exact original factory return status (or `0xffffffff` if none was supplied).
- `original-init-failed` / `original-factory-unavailable` when needed.

This does not change the game interface pointer or its return code, create fake virtual tables, run work inside `DllMain`, write a diagnostic file, or try to replace owner gameplay. Traces are disabled by default.

## Build and CI, Windows Visual Studio 2022 x86

Use the existing synthetic-only build:

```powershell
cmake -S pc/windows_x86_proxy -B build-pc-win32 -G "Visual Studio 17 2022" -A Win32
cmake --build build-pc-win32 --config Release
ctest --test-dir build-pc-win32 -C Release --output-on-failure
```

The three added probe CTest cases run default exports-only, optional `--query-factories`, and a third invocation with `SIN_PC_PROXY_TRACE=1`, using **only synthetic test DLLs**. A fourth CTest runs a PowerShell script parser and fail-closed check against those synthetic files. It must reject fake original hashes and attempts to Install/Restore without owner confirmation, and prove **all four fake DLL files have unchanged SHA256s**. GitHub Windows CI is necessary to validate compilation, x86 exports, trace behavior and this script. No real SiN binaries are used in CI.

## Owner-only game-copy staging

`pc/windows_x86_proxy/Prepare-SiNWin32ProxyTest.ps1` defaults to a strictly **read-only `Check` action**, verifying both original 2006 whole-DLL SHA256s:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The user must create **an independent, disposable, fully backed-up copy** of their legally obtained installation. Use the directory where the original Windows executable actually loads the game's own `server.dll` and `client.dll`. No original file in the retail installation or the GitHub repository should be modified.

Example commands, replacing paths with the owner's own copied installation:

```powershell
# Non-mutating provenance check:
powershell -NoProfile -ExecutionPolicy Bypass -File .\pc\windows_x86_proxy\Prepare-SiNWin32ProxyTest.ps1 -Action Check -GameBin "D:\SiN-TestCopy\bin"

# Install requires explicit acknowledgement of the disposable copy:
powershell -NoProfile -ExecutionPolicy Bypass -File .\pc\windows_x86_proxy\Prepare-SiNWin32ProxyTest.ps1 -Action Install -DisposableTestCopyConfirmed -GameBin "D:\SiN-TestCopy\bin" -ProxyBin ".\build-pc-win32\bin\Release"

# Put ONLY the independently built preflight executable in the same bin:
Copy-Item ".\build-pc-win32\bin\Release\sin_pc_owner_module_probe.exe" "D:\SiN-TestCopy\bin\"
& "D:\SiN-TestCopy\bin\sin_pc_owner_module_probe.exe"

# OPTIONAL: invoke the original factory object without original engine Init:
& "D:\SiN-TestCopy\bin\sin_pc_owner_module_probe.exe" --query-factories

# Reversible restoration:
powershell -NoProfile -ExecutionPolicy Bypass -File .\pc\windows_x86_proxy\Prepare-SiNWin32ProxyTest.ps1 -Action Restore -DisposableTestCopyConfirmed -GameBin "D:\SiN-TestCopy\bin"
```

Install first checks the SHA256 of the real owner originals, validates the chosen proxies are **Intel x86 PE32** and different from originals, then renames originals to `sin_original_server.dll` and `sin_original_client.dll` in that copy and copies the built proxies into place. If an error occurs it **attempts rollback**. Restore performs a same-volume atomic replacement using Windows `File.Replace`, retains a backup of each previous proxy, and retains renamed originals; it does not delete the user's retail files. This staging script is deliberately not a game launcher, Steam tool, or automatic bypass of an engine. Do not upload the game copy to GitHub.

## Capturing a real original-engine trace

In the owner-owned test copy, start the **original game's Windows executable** under WinDbg/Visual Studio or use DebugView and set `SIN_PC_PROXY_TRACE=1` in the launching environment. Preserve the debugger output and the game's own console log, if available. Determine separately whether the engine actually probes both DLLs; what `CreateInterface` names it requests; the returned statuses; any loader errors; and whether the engine reaches game Init, map loading and gameplay.

Only claim a gameplay milestone if a real game-engine process and a real campaign map are demonstrably running. No such owner-run evidence has been captured yet. The standalone owner preflight does not prove that the game executable will accept the renamed originals.

## Next engineering work

Use the first owner-only PC runtime trace to identify the exact failing interface or initialization boundary. Then implement **independently authored** server/client compatibility there, checking original ABI layouts and engine vtable offsets, while continuing the DLL-specific AI, entity, HUD and weapon reconstruction. SourceVR/screng Android ARM64 remains downstream and cannot directly use 2006 Win32 x86 virtual objects.
