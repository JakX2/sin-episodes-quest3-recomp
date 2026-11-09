# Phase 147: Windows x86 first, verified original SiN factory forwarding

**9 November 2026. Draft, stacked on Phase 146. PC before Android before Quest 3.**

## Actual original 2006 binary evidence, verified privately

The owner-supplied `bin.zip` was read, not published. The two extracted
binaries are **PE32 Intel i386 DLLs**, with exact SHA256 values:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The original server export table contains **five named exports**:
`CreateInterface`, plus these actual decorated x86 C++ exports:

- `?Detonate@CGrenadeHomer@@UAEXXZ`
- `?Detonate@CGrenadePathfollower@@UAEXXZ`
- `?Detonate@CGrenadeSpitMutant@@UAEXXZ`
- `?Detonate@CGrenadeSpitPoison@@UAEXXZ`

The client contains **only** `CreateInterface`. Original factory
registrations are `ServerGameDLL003` and `VClient011`.

## What is built now

`pc/windows_x86_proxy/` builds **Win32 x86** (not x64) libraries
named `server.dll` and `client.dll`. Their `CreateInterface` entrypoints
load owner-supplied *renamed* originals from the same absolute directory:
`sin_original_server.dll` and `sin_original_client.dll`.
They return the **actual original engine interface objects**, including
the original factory return codes. We never invent replacement virtual tables.

The four original server decorated exports use PE export-table
**forwarders** to `sin_original_server.dll`, retaining the original
C++ thiscall signatures without unsafe homebrewed x86 thunks.

Optional `SIN_PC_PROXY_TRACE=1` writes interface requests to the Windows
debug output stream for WinDbg/Visual Studio/DebugView; default is silent.
Missing originals or failed factory lookups fail closed.

### Build and synthetic smoke on Windows with Visual Studio 2022

```powershell
cmake -S pc/windows_x86_proxy -B build-pc-win32 -G "Visual Studio 17 2022" -A Win32
cmake --build build-pc-win32 --config Release
ctest --test-dir build-pc-win32 -C Release --output-on-failure
```

Test-only DLLs in `build-pc-win32/bin/Release` are synthetic stand-ins
under `sin_original_*.dll`. The test calls real Windows `LoadLibraryW` and
`GetProcAddress`, confirms original interface pointer identity, unknown
name rejection, and exact forwarding of four decorated exports.
**Never mistake these fake DLLs for game code.**

### Optional owner-only original PC boot experiment

Do **not** copy the synthetic test originals into a retail installation.
Use a separate copy of the legally obtained installation in a clean,
fully backed-up test directory. Confirm the two real DLL SHA256 hashes
above. Within the directory from which the original Windows engine
actually loads its game DLLs:

1. Keep permanent independent backup copies of the original `server.dll`
   and `client.dll`, outside the test directory.
2. Rename the *real* `server.dll` to `sin_original_server.dll` and the
   *real* `client.dll` to `sin_original_client.dll` inside the test dir.
3. Copy **only** built `server.dll` and `client.dll` proxies into that
   same directory. Do not copy `sin_original_*.dll` from synthetic CI.
4. Start the real Windows game under a debugger with
   `SIN_PC_PROXY_TRACE=1` and capture requested interface names,
   load outcome, process exit and map startup logs.
5. If anything fails, restore the original DLLs from the safe backup.

The original loader's acceptance of this arrangement and the game's
internal assumptions about module filenames are **not yet validated**.
No real-engine run is claimed. A passing proxy test does not mean the
campaign is now running from reconstructed DLL code.

## Why this is an incremental step, not the final port

This is a **real Win32 module loader/ABI handoff experiment**, not a
recompiled SiN game server or client. Gameplay still executes in the
original owner-provided DLLs. Its value is preserving exact original
exports and creating an instrumentable, honest baseline for replacing
the factory implementation one subsystem at a time.

The real next checkpoint is a **Windows engine-run log** proving the
original engine loads both proxies and starts a real SiN campaign map.
Only then should we implement a fully reconstructed
`ServerGameDLL003`/`VClient011` object against proven x86 method ABIs,
connect our portable SiN logic and incrementally replace original paths.
A standalone synthetic Win32 DLL smoke is not a playable game.

Android ARM64/Quest 3 modules remain separate targets; no 32-bit PE
pointer or vtable is ever used as an Android engine interface.

No original proprietary DLL bytes, game/engine assets or original
C++ source are committed to this repository.
