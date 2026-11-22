# Gameplay Phase616: owner-only real SiN process Grunt bridge preflight

## Purpose

Phase615 successfully passed **89/89 real Win32 x86 synthetic CTests**. The original Windows server factory proxy can opt into loading and verifying the independently authored five-function Grunt gameplay DLL, while preserving the original `ServerGameDLL003` pointer and return codes in all enabled, disabled, missing, and wrong-ABI scenarios.

The synthetic test does not prove the owner's actual Windows game process loads the bridge. Phase616 extends the existing owner-only disposable-copy runtime launcher to obtain exactly that narrower evidence without creating fake NPCs or removing the original code.

## Files

- `pc/windows_x86_proxy/Invoke-OwnerSiNStartupTrace.ps1` now accepts `-GruntBridgeProbe`.
- `pc/windows_x86_proxy/tests/Test-OwnerStartupTraceLauncherSyntax.ps1` additionally verifies the opt-in, PE32, trace and PID-specific evidence witnesses.
- The original server/client proxy code remains identical to Phase615 and still forwards all original factory operations untouched.

## Safeguards before any process launch

The launcher still requires `-DisposableTestCopyConfirmed`, an existing `.exe` **within the same disposable game-copy root**, and the earlier `Prepare-SiNWin32ProxyTest.ps1 -Action Check` confirmation that both original commercial DLLs are present as SHA256-authenticated renamed siblings of the built Win32 proxies. It never stages the proxies or modifies the game folder itself. The launcher also recognizes the conventional `GameRoot\\SE1\\bin` layout and permits `GameRoot\\SinEpisodes.exe`, instead of falsely rejecting the actual executable due to an overly narrow `SE1`-only path check. It does not widen containment for unrelated directory names.

For `-GruntBridgeProbe` it additionally requires the independently built `sin_grunt_behavior_c_abi.dll` to exist in the *same game-copy bin directory* as the server proxy. It rejects symlink/reparse-point bridge files. Its read-only PE audit checks MZ signature, valid PE header position, `IMAGE_FILE_MACHINE_I386 = 0x014C`, `IMAGE_FILE_DLL` characteristic and `IMAGE_NT_OPTIONAL_HDR32_MAGIC = 0x010B`. It does not claim the DLL is ABI v1 from static PE metadata; the original server proxy's actual `GetProcAddress` checks do that during runtime.

If the probe switch is absent, the launch explicitly forces `SIN_PC_GRUNT_BRIDGE_PROBE=0` so an inherited environment setting cannot turn the feature on accidentally. When set, `SIN_PC_GRUNT_BRIDGE_PROBE=1` is inherited by the launched process, and the previous environment value is restored in a `finally` block. Existing file-based proxy tracing is preserved.

## PID-specific outcome

The launcher prints `OWNER_START_PID: <pid>` for its actual `Start-Process -PassThru -Wait` process. After the process exits, it looks specifically for the refreshed `%TEMP%/sin-pc-proxy-<pid>.log` file. If it contains a `role=server phase=grunt-bridge-ready` line, the script reports:

    OWNER_GRUNT_BRIDGE: ABI_V1_LOADED_IN_LAUNCHED_PROCESS

Otherwise it reports `NOT_PROVEN` and directs inspection of the logs, including potential child-process traces. A recent log written by another process is **never treated as proof** for the tracked launcher PID. Success means only that the original engine process loaded the independent Grunt DLL and validated its five C exports and version; **it is not evidence that a Grunt NPC was constructed, had its eligibility evaluated, or ran AI**.

The script does not emit original PE binary bytes, game-engine object pointers, location data or user assets to GitHub. All logs remain on the owner's Windows PC.

## Safe owner-only reproduction

After obtaining **the CI-built Phase615 Windows x86 server/client proxies and Grunt behavior DLL** and staging the proxies into a **fully backed-up disposable game copy** using the already documented reversible staging script, place the actual built `sin_grunt_behavior_c_abi.dll` into the disposable game-copy bin alongside `server.dll`.

From PowerShell:

    powershell -NoProfile -ExecutionPolicy Bypass -File .\pc\windows_x86_proxy\Invoke-OwnerSiNStartupTrace.ps1 -GameBin "D:\SiN-DisposableTest\SE1\bin" -GameExecutable "D:\SiN-DisposableTest\SinEpisodes.exe" -DisposableTestCopyConfirmed -GruntBridgeProbe

The executable path must satisfy the launcher's strict disposable-copy containment policy. When the game DLL directory is exactly the conventional `SE1\\bin` subtree, the launcher recognizes its parent **game-copy root** and permits a top-level `SinEpisodes.exe`. For other layouts the executable must stay beneath the immediate parent of `GameBin`. The path still must exist and be inside the selected disposable copy. If the user cannot meet the staging, SHA, PE32, and executable-boundary guards, the launch fails before modifying or starting the game. The user can report **only the text log** to continue research.

### Technical and privacy boundaries

The Phase616 launcher is tested for PowerShell syntax and source witness presence under native GitHub Win32 CI, plus the existing Phase615 89/89 real Win32 compiled proxy tests. A real retail owner-run process result remains **unverified until the user's local log is provided**.

No original commercial DLL is uploaded or modified; no Windows engine NPC factory/vtable is reimplemented; no Quest 3 gameplay boot is claimed. Source-authenticated method count remains **40 original functions and 11,224 complete native bytes**, unchanged since Phase611. No generic Half-Life 2 physics, models, surfaces or prop processing is revisited.
