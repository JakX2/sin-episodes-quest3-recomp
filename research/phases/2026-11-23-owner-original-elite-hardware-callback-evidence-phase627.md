# Phase627: Observe real original-game Elite NPC callbacks without guessing Source interfaces

## Why this is different from synthetic host tests

Phases622–626 developed independently compiled and tested Elite factory, Spawn, animation-event, and generation-safe native-host C adapters. **None had yet observed an actual original-engine NPC calling those methods.** The next milestone requires evidence from a real owner-run game, not another fabricated vtable or synthetic `IServerGameDLL`.

The owner-provided `SinEpisodesSDK.zip` was inspected: it includes Hammer, tools, binaries, `SinEpisodes.fgd` and `GameInfo.txt`, **not the full original Source 2006 C++ game-module headers or real engine lifecycle callback implementation**. Reinterpreting Source SDK 2013 classes as original 2006 engine interfaces would be unsafe.

Phase178 previously verified the original server `ServerGameDLL003` singleton vtable and 31 executable prefix slots. Phase179 verified the complete original 875-byte `CServerGameDLL::DLLInit` and its 16 mandatory prefix interface queries. The original Windows proxy keeps forwarding `CreateInterface` exactly. Replacing that interface or inventing a live `CBaseNPC` is not justified.

**Phase627 instead adds a standalone, owner-run original-only Win32 debugger.** It launches the *actual owner's disposable SiN Episodes executable* under `DEBUG_ONLY_THIS_PROCESS` and only attempts to arm x86 DR0–DR3 hardware **execution** breakpoints after SHA-256 authenticating the original loaded `server.dll` file. It never writes PE `.text`, changes a vtable, replaces `CreateInterface`, calls a guessed engine interface, or injects a shim.

## Exact original functions observed

The source-address pins are **already authenticated by previous original retail binary audits**, not discovered from synthetic runs.

| Hardware register | Source symbol | Original 2006 VA | Module RVA | Recovery |
|---|---|---|---|---|
| DR0 | Elite factory creator entry | `0x22355C70` | `0x355C70` | Phase622, 23 bytes |
| DR1 | Elite Spawn entry | `0x22355820` | `0x355820` | Phase623, 505 bytes |
| DR2 | Elite animation-event override entry | `0x22355480` | `0x355480` | Phase593, 360 bytes |
| DR3 | Elite factory release delegate entry | `0x223552C0` | `0x3552C0` | Phase622, 16 bytes |

The machine-code probes are **pure execution observation**, not hooking into or invoking the four original methods. Original `server.dll` can load at an address other than `0x22000000`; the debugger sets breakpoints using the authenticated **RVA plus actual loaded module base**. It does not assume the DLL always loads at its preferred base.

Before arming, the debugger resolves the exact `LOAD_DLL_DEBUG_EVENT` file handle for `server.dll` or the protective proxy's renamed `sin_original_server.dll`, reopens the loaded file read-only, verifies exact length **6,049,792 bytes** and original full SHA256:

`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

If the module cannot be independently authenticated, the tool refuses to arm. A similarly named non-retail DLL does not pass.

It applies hardware breakpoints to all existing threads and each subsequently created thread. The execution trap is checked against both the DR6 hit mask and the exact expected `EIP`; the debugger sets x86 Resume Flag to allow the original instruction to execute and retains the observation watch. An ambiguous hit does not count. On timeout the tool tries to clear DR0–DR3 on all threads before detaching so the game can continue without a debugger.

## Data captured and proof limits

Fresh JSONL log events are output to an owner-chosen path. Event records contain exact child process ID, thread ID, phase name and hit count; no original DLL machine code or memory bytes. The original x86 `ECX` object pointer on Spawn/event hits is mapped to a **process-local ordinal**, not written as a raw address or assumed to be a valid generation-safe Source entity ID.

A successful trace can establish that **the authentic loaded 2006 retail module entered the recovered native Elite methods during an actual run**. This is the first factual prerequisite for connecting those signals to the Phase626 native host. It does **not** prove a reconstructed game module boots, that the Phase626 ABI already intercepts original engine callbacks, or that Android/Quest gameplay works. The game still executes original retail code under observation.

Absence of an Elite Spawn event means **not observed within that tested level/time window**, not that the original method is unreachable. `TRACE_TOOL_NONZERO_EXIT`, missing exact loaded original, an unarmed watch, incomplete thread watches, spliced log PIDs, or missing process completion all yield `ORIGINAL_ENGINE_CALLBACK_EVIDENCE_NOT_PROVEN`.

The desktop debugger and runner are **never run against commercial game binaries in public GitHub CI**. Public Windows x86 CTest compiles the debugger executable and runs `--self-test` only (RVA arithmetic, Win32 command quoting and CLI guards). Another CTest validates owner launcher PowerShell syntax and evidence gate anchors. No original content is committed.

## Owner Windows reproduction

1. Back up your original game. Create a **disposable, offline copy**, including `SinEpisodes.exe`, `SE1inserver.dll`, and original game assets. Do not point this tool at your live Steam install. The `server.dll` must be the exact original; if using the protected Phase615 Windows proxy, the renamed `sin_original_server.dll` must be the exact original instead.
2. Build with **Visual Studio Windows x86**:

```powershell
cmake -S .\pc\windows_x86_proxy -B .\build-owner-win32 -A Win32
cmake --build .\build-owner-win32 --config Release --target sin_pc_owner_elite_hwtrace_phase627
```

3. Run the wrapper (paths are examples, not assumed existing files):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\pc\windows_x86_proxy\Invoke-OwnerEliteHardwareTrace.ps1 `
  -GameExecutable "D:\SiN-DisposableTest\SinEpisodes.exe" `
  -GameBin "D:\SiN-DisposableTest\SE1\bin" `
  -TraceExecutable ".\build-owner-win32\bin\Release\sin_pc_owner_elite_hwtrace_phase627.exe" `
  -OutputTrace "D:\SiN-DisposableTest\elite-callbacks-phase627.jsonl" `
  -DisposableTestCopyConfirmed -TimeoutSeconds 180 `
  -GameArguments @('-console')
```

While the game runs, load a level containing `npc_druglab_grunt_elite` and exercise an interaction that causes its animation events, then exit the game. The debugger detaches if the time budget expires, leaving the game running without debugger attachment. The trace script classifies observations only when the original module was authenticated and all watched threads were successfully covered.

### What to send back

Send only the generated `elite-callbacks-phase627.jsonl` and the terminal's `OWNER_ELITE_TRACE` summary. No DLLs or original program bytes are necessary.

A genuine `elite_spawn` method-entry hit would establish the first real original-engine spawn-callback evidence needed to design an independently verified, non-synthetic bridge to `sin_elite_native_host_c_abi.dll`. Until this owner test is run, **actual engine callback integration and playable native SiN gameplay remain unverified**.
