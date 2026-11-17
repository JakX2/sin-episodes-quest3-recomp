# Owner-only Windows x86 SiN startup factory trace

This is a **concrete engine-startup diagnostic** for a privately owned,
disposable copy of **SiN Episodes: Emergence**. It does not convert
`server.dll` or `client.dll` into independently reconstructed gameplay
modules. The existing proxy forwards all factory calls to the SHA-reviewed
owner originals in local `sin_original_server.dll` and
`sin_original_client.dll`.

The approach is similar to Metamod:Source's engine/game-DLL boundary
instrumentation, **but does not install Metamod, patch a vtable, hook an
unknown virtual method, or require any native game method signatures**.
The immediate purpose is to observe the real engine's requested
`CreateInterface` names and return codes during startup.

## Prepare the Windows build

Visual Studio 2022 with the native Win32 compiler installed:

```powershell
cmake -S pc/windows_x86_proxy -B build-pc-win32 -G "Visual Studio 17 2022" -A Win32
cmake --build build-pc-win32 --config Release
ctest --test-dir build-pc-win32 -C Release --output-on-failure
```

The synthetic CTest `sin_pc_win32_original_factory_startup_trace` proves
that logging defaults OFF, activates only with the exact value `1`,
and separately records client/server request, success, failure and
null-name states. There is no need to run the retail game to execute CI.

## Stage an isolated game copy

**Use a separate, backed-up copy of a game installation you own**, never
the live Steam installation. Close the game before staging DLLs. The
stager independently authenticates both originals by their full
SHA-256 and refuses a non-Win32 proxy or an unexpected original.

```powershell
$gameBin  = "D:\SiN-DisposableTest\bin"
$proxyBin = ".\build-pc-win32\bin\Release"

.\pc\windows_x86_proxy\Prepare-SiNWin32ProxyTest.ps1 -Action Check -GameBin $gameBin
.\pc\windows_x86_proxy\Prepare-SiNWin32ProxyTest.ps1 -Action Install -GameBin $gameBin -ProxyBin $proxyBin -DisposableTestCopyConfirmed
.\pc\windows_x86_proxy\Prepare-SiNWin32ProxyTest.ps1 -Action Check -GameBin $gameBin
```

`Install` renames the original DLLs to `sin_original_*.dll`
and places the built proxy DLLs at their normal engine paths.
Do not re-stage a copy already staged. Keep independent backups.

## Launch the user's actual game executable

The optional guarded launcher performs the existing SHA256 staging `Check`,
requires explicit confirmation that the directory is a disposable test copy,
enables factory-file tracing only for the launched process, waits for it to
exit, restores the original PowerShell environment setting, and lists
recently created trace paths. Supply your actual executable and arguments;
no executable filename is assumed:

```powershell
.\pc\windows_x86_proxy\Invoke-OwnerSiNStartupTrace.ps1 `
  -GameBin "D:\SiN-DisposableTest\bin" `
  -GameExecutable "D:\SiN-DisposableTest\YOUR_GAME_EXECUTABLE.exe" `
  -GameArguments "-console" -DisposableTestCopyConfirmed
```

The launcher **does not stage DLLs** itself; stage them with the preceding
`Install` step first. Running the command without the explicit confirmation
switch or with unstaged originals is refused.

Alternatively, use an existing PowerShell terminal and set the environment variable
*before launching the test-copy executable*. The following is an example
only: supply the actual executable path and normal owner-specific launch
arguments, which depend on the game installation.

```powershell
$env:SIN_PC_PROXY_TRACE_FILE = '1'
# Example only; replace with your test copy's real executable and arguments.
& "D:\SiN-DisposableTest\YOUR_GAME_EXECUTABLE.exe" -console
```

The proxy writes **only when it is loaded and its factory is requested**.
For each process, the log is in the current user's Windows temporary
folder at `$env:TEMP\sin-pc-proxy-<PID>.log`:

```powershell
Get-ChildItem -Path $env:TEMP -Filter 'sin-pc-proxy-*.log' |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 5 FullName, LastWriteTime
```

Sample event format (illustrative, not a result from a retail engine run):

```text
[SiN PC proxy] role=server phase=request name=ServerGameDLL003 detail=0
[SiN PC proxy] role=server phase=result-object name=ServerGameDLL003 detail=0
[SiN PC proxy] role=client phase=request name=VClient011 detail=0
[SiN PC proxy] role=client phase=result-object name=VClient011 detail=0
```

Other phases include `original-load-ok`, `original-load-failed`,
`original-factory-export-missing`, `result-null`,
`null-factory-name`, and `original-init-failed`. The `detail`
field records the original factory status or Win32 error, **not a method
return value**. When a call has a NULL status pointer, the
`result-*` detail value is `4294967295`, meaning *not supplied*,
not a failure code.

The separate `SIN_PC_PROXY_TRACE=1` environment switch still sends
the same messages to the Windows debugger via OutputDebugString.
`SIN_PC_PROXY_TRACE_FILE=1` is a separate opt-in switch. Both are OFF
by default, and neither redirects game assets or captures C pseudocode.

**Restore** the test-copy originals afterward:

```powershell
Remove-Item Env:\SIN_PC_PROXY_TRACE_FILE -ErrorAction SilentlyContinue
.\pc\windows_x86_proxy\Prepare-SiNWin32ProxyTest.ps1 -Action Restore -GameBin $gameBin -DisposableTestCopyConfirmed
```

## Evidence boundaries and next integration target

- A successful factory request confirms only that the *original* returned
  a pointer through our proxy. It does not establish independent gameplay.
- Missing logs may mean the game never loaded these modules, launched
  another process, or did not inherit the switch; it is **not proof**
  of an ABI mismatch.
- We intentionally do not call or detour unknown virtual methods.
  Validating `DLLInit` / `Init` arguments requires verified layouts
  and a separate, explicit owner-only test.
- The logs include exact engine interface names and numeric statuses.
  They do not include game file paths, original binary bytes, object
  addresses, or proprietary code. Treat owner startup logs as private
  diagnostics rather than sharing them automatically.
- Once a first genuine owner engine boot log is available, compare its
  request order with Source SDK 2004 and prioritize the **first missing
  engine dependency or interface** in the independent Windows DLL.

No GitHub synthetic test can claim the original SiN retail game has started
successfully. This trace is built specifically to make that first manual
startup attempt more informative.
