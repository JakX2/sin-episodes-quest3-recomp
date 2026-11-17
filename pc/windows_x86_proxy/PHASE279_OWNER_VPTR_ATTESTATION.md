# Phase 279: SHA-authenticated live Win32 original-interface object / vptr probe

## Purpose

The preceding Phase 277/278 development created genuine independent Win32
x86 DLLs with synthetic C++ virtual interfaces, callbacks and startup/shutdown
tests. Those **do not establish original SiN gameplay ABI compatibility**.

Phase 279 adds a separate native 32-bit Windows executable that can inspect
*owner-provided* original SiN Episodes: Emergence 2006 DLL interface pointers
through their already known `CreateInterface` exports. It does not emulate
the Source engine or execute an unknown original virtual function.

## Pinned owner-original evidence

The preexisting reviewed Phase 178/226 singleton/vtable metadata identifies
five **PE-initialized** `CreateInterface` objects. This probe verifies the
returned singleton RVA, the first vtable RVA, the exact first method RVA,
and ownership/executability for the previously reviewed contiguous vtable
prefix. ASLR relocation is handled by comparing RVAs relative to the module
actually mapped by Windows.

| module | interface | previously documented prefix slots |
|---|---|---:|
| server.dll | ServerGameDLL003 | 31 |
| server.dll | ServerGameEnts001 | 7 |
| server.dll | ServerGameClients003 | 15 |
| client.dll | GameClientExports001 | 4 |
| client.dll | VClient011 | 48 |

These are *previously documented* prefix data, NOT newly verified source
method signatures, full virtual tables, method implementations or proof of
engine handshake. The four additional original singleton objects whose
runtime-initialized vptrs are unknown are intentionally excluded.

**Security and provenance:** `--owner` opens the nominated DLL as a file,
checks its exact original size and entire SHA-256 digest against the pinned
owner's authentic 2006 PE32 image, and refuses to load it when either differs.
Only after authentication does it call `LoadLibraryExW` and resolve
undecorated `CreateInterface`. Every returned singleton/vtable/method
address must belong to committed, correctly protected pages of the same
PE image. The probe reads pointers but never invokes original vtable slots.

## Public CI: synthetic-only

The existing `pc/windows_x86_proxy` fake-original test objects now contain
a **real synthetic C++ virtual method** instead of a plain integer token.
The native probe in `--synthetic` mode:

- loads each fake PE32 original, calls its synthetic factory with
  `ServerGameDLL003` or `VClient011`, and receives a synthetic C++ object;
- checks the object address, vptr and one callable virtual-method pointer
  are in the loaded DLL's memory image;
- invokes **only the fake known-signature virtual method**, asserting the
  separate server/client magic token.

A third CTest runs `--owner` against the *fake* server DLL and demands the
precise pre-load original SHA/size mismatch exit status 2. This guards against
accidentally relaxing the provenance requirement. The existing proxy factory
tests continue to use fake originals. Public CI never needs real game files.

## Optional owner-only execution

Build the native Win32 x86 tools (Visual Studio 2022 on Windows):

```powershell
cmake -S pc/windows_x86_proxy -B build-pc-win32 -G "Visual Studio 17 2022" -A Win32
cmake --build build-pc-win32 --config Release
ctest --test-dir build-pc-win32 -C Release --output-on-failure
```

In a **disposable backed-up copy** of the 2006 game, with the owner's matching
original DLLs and dependencies available in the appropriate bin folder:

```powershell
.\build-pc-win32\bin\Release\sin_pc_owner_vptr_probe.exe --owner server "D:\SiN-TestCopy\bin\server.dll"
.\build-pc-win32\bin\Release\sin_pc_owner_vptr_probe.exe --owner client "D:\SiN-TestCopy\bin\client.dll"
```

If the owner has already staged separate original DLLs with the existing
`Prepare-SiNWin32ProxyTest.ps1` tool, pass
`sin_original_server.dll` and `sin_original_client.dll` instead.
The SHA gate accepts the original **bytes**, irrespective of filenames.
The tool does not modify any original DLL, install anything into the game,
call `Init`, invoke a map, launch the game executable or write private data.
Windows may execute the original modules' normal `DllMain`/CRT startup during
`LoadLibraryExW`; use an isolated game copy and understand that loading a DLL
executes its initialization code.

If the local original DLLs or their dependencies cannot be loaded outside a
full engine session, record that result as a real limitation. Do not count
an unsuccessful owner probe as a verified object or method. In particular,
there is **no claim that owner mode was executed in hosted CI or on the owner's
machine** as part of this phase.

## Roadmap significance

A successful user-executed, exact-hash owner probe will provide evidence for
the *live loaded* original singleton/vptr address relationships. It still
does not recover virtual method signatures or implement even one retail
engine callback. The next stage needs vtable slot mapping and a meaningful
independent `VClient011` / `ServerGameDLL003` lifecycle handshake under
the real executable. Windows x86 first; Android ARM64 and Quest 3 follow.
