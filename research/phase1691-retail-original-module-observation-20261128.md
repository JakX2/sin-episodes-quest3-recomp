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
