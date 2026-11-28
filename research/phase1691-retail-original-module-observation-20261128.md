# Owner Phase1691 retail-original loading baseline, 2026-11-28

## Provenance and exact observation

Owner-supplied diagnostic text: `phase1691-retail-baseline-20261128-072012.txt`. This was a **read-only original-DLL test** on the owner's disposable Windows game copy, not a reconstruction test and not a shipped game binary. No retail bytes are included in this repository note.

| Observed field | Value |
| --- | --- |
| Game executable | `D:\SiN-DisposableTest\SinEpisodes.exe` |
| Command | `-console -game SE1` |
| Executable size / SHA256 | `73728` / `e524115c13b4113511bbb3111e78aadfae0088b07c3485787695048cf4b56f07` |
| `SE1/gameinfo.txt` | present |
| Original client/server SHA256 precheck | both valid |
| Original client/server SHA256 unchanged | both true |
| Steam process at start | true |
| `steam_appid.txt` | absent (not proven a blocker) |
| Launched/observed PID | `16312` / `SinEpisodes.exe` |
| Launched process command line | `SinEpisodes.exe -console -game SE1` |
| Observed game window | `SiN Episodes` |
| Process still alive after 35-second scan | yes; owner probe did not terminate it |
| Additional related processes | 0 |
| Process scan iterations | 51 |
| Reported module-sample successes / errors | 51 / 0 |
| Client / original SE1 client / engine module seen | false / false / false |
| Replacement or game-bin modification | none |

## Interpretation: module sampling must not be used as a no-load proof

This report disproves the hypothesis that `SinEpisodes.exe` simply could not start: the process existed with the specified SE1 arguments and an observed title-bar window. It does **not** establish a loaded map, functioning original client module, or successful gameplay.

The instrument recorded 51 ostensibly successful module-sampling calls but did not report how many modules each call returned or the full module paths. Therefore `MODULE_SAMPLE_SUCCESSES=51` cannot establish that the inspection actually saw a meaningful module inventory.

This limitation was already demonstrated in
`pc/independent_slot0/FIRST_RETAIL_WINDOWS_STARTUP_EVIDENCE.md`: an earlier owner replacement-DLL trace **positively executed** replacement `CreateInterface` and client `VClient011::Init`, while the existing `$child.Modules` polling reported both modules absent. Such a false negative is documented with stronger evidence than this baseline. The current absence of `engine.dll` and `client.dll` is therefore **inconclusive**, not proof of failure to load either DLL.

No diagnosis of an incorrect `-game SE1` setting, Steam dependency, DLL load failure, or new client code regression is supported by this report. `steam_appid.txt` absent cannot alone explain the observed behavior; Steam was already present at start.

## Bounded next owner test

Keep original `client.dll` and `server.dll` unchanged. The next **read-only module census** should:

1. Start the same executable with `-console -game SE1`, or attach to a known still-running PID, without moving or replacing either original DLL.
2. Record PowerShell host bitness, game PID, process/window/exit state and a count of **all** process modules for each sample. Preserve any .NET enumeration error class.
3. Independently enumerate `TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32` with native Windows `CreateToolhelp32Snapshot/Module32FirstW/Module32NextW`; preserve snapshot Win32 error codes (especially 299 / `ERROR_PARTIAL_COPY`) and module counts, not merely success booleans.
4. Save full names and paths, particularly `engine.dll`, `client.dll`, `server.dll`, `tier0.dll`, `vstdlib.dll`, and the selected original client path under `SE1/bin` or `bin`. Run across menu and optionally user-started map loading.
5. Rehash original DLLs before and after the probe; never infer boot or failure from a missing module sample. If **both** enumerators yield empty snapshots, label the result **inconclusive**.
6. Only after a positive retail-original baseline, repeat the opt-in independently built `client.dll` test and capture factory/Init traces. The old independent trace is stronger evidence of module execution than the missing-module flag.

This work concerns Windows process instrumentation and game-module loading. It neither implements nor reopens deferred HL2-equivalent physics, material/surface, model, prop or rendering subsystems.

## Verification boundary

The text report is owner execution evidence only. A new two-enumerator PowerShell test kit has been prepared separately for owner validation, but no Windows run of that new script is claimed here. The original source-engine runtime, the proposed module sampler, and the reconstructed independent client are distinct test subjects. 


## Follow-up owner test-kit packaging defect and fix (2026-11-28)

The first standalone dual-census ZIP failed **before any process launch** when Windows PowerShell evaluated the optional parameter initializer:

```powershell
[string]$OutputDirectory=(Join-Path $PSScriptRoot 'results')
```

The owner saw: `Join-Path : Cannot bind argument to parameter 'Path' because it is an empty string` at line 8. This is a **probe initialization defect** and says nothing about either original retail DLL. The correct fix is to declare `[string]$OutputDirectory=''` in the `param` block, then resolve `Join-Path -Path $PSScriptRoot -ChildPath 'results'` **inside the body** after initial setup, guarding an absent script root before touching the game. A second robustness fix conditions native helper `Add-Type` on the type not already being loaded, making repeated attach runs possible in one PowerShell host. A fresh **V2 standalone ZIP** was generated and passed ZIP integrity and offline static contract checks. It is not a Windows-executed probe and is not included in this repository PR.

Owner's next action is to extract **V2** into a clean folder and rerun the retail-original census. Successful Windows host self-attach regression and retail-original module inventory remain **pending**. A future committed Windows test should invoke the PowerShell probe **without** `-OutputDirectory` to guard the exact first-run failure mode, not merely parse the script or call it with an explicit output directory.


## V2 owner run: the original loader works; managed sampler gave a reproducible false negative

User-supplied report `sin-retail-module-census-20261128-073455-pid6480.txt` from the November 28 Windows test:

- `PROBE: RETAIL_ORIGINAL_DUAL_MODULE_CENSUS_V2`, `MODE: Launch`, game PID **6480**, original game root `D:\SiN-DisposableTest`, Windows PowerShell **5.1.19041.7844** running **64-bit**.
- **62** module sample iterations, **62/62 success** reported by the managed enumeration, with **at most 5 modules** seen. Its observed names were the retail EXE, `ntdll.dll`, and three WOW64 modules. This is insufficient to identify loaded 32-bit game DLLs.
- **62/62 success** from the native Toolhelp32 enumeration, **zero errors**, peak **162 module entries**. It directly observed the retail **`D:\SiN-DisposableTest\SE1\bin\client.dll`**, **`D:\SiN-DisposableTest\SE1\bin\server.dll`**, and **`D:\SiN-DisposableTest\bin\engine.dll`**, plus `tier0.dll`, `vstdlib.dll`, other Source runtime and Steam modules.
- Window `SiN Episodes` observed; tracked game process exit code **0** after sampling. The owner independently reports that the original game launched and ran. The probe did not record map load, player spawn, or gameplay frames; do not silently infer those.
- Original SHA256 both before and after the test: `client.dll` `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`; `server.dll` `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Both **unchanged**. No DLL replacement or process termination by this diagnostic.
- Result `CLIENT_DLL_OBSERVED_BY_MODULE_CENSUS_NO_GAMEPLAY_CLAIM`.

### Engineering decision

**Eliminate the original negative load diagnosis**. The previously used 64-bit PowerShell `$child.Modules` check can falsely miss the original x86 `client.dll` and `server.dll` even with no exceptions. A successful call is not a complete module census. The retail loader itself succeeded on the verified original installation.

The branch now updates `pc/independent_slot0/Invoke-OwnerIndependentEngineProbe.ps1` to union native `TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32` evidence with legacy managed samples. Native samples have separate error/success/max-count diagnostics. This is a tooling fix, **not** a new binary interface recovery or demonstration that the independently reconstructed client loads. The companion PowerShell synthetic guard checks the native reader against the test host process. The repository modifications have not yet been executed in Windows CI at the time of writing; their exact-head status must be verified before an owner replacement attempt.

**Next test**: the existing owner-safe reversible replacement client probe, only on the already-disposable copy, should positively observe the independent client DLL's module path and corroborate that path with an in-process session trace / source-defined factory invocation. Do not equate mere observed module load with `VClient011::Init` success, player creation, or game boot.
