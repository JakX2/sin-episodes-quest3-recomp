# Phase 291: First independent-DLL load attempt in the actual owned Windows game

This diagnostic (NOT playable) is an owner-only attempt to load the two independently built Windows PE32 gameplay DLLs in an actual SiN Episodes: Emergence executable. The initializers deliberately return failure after checking at most the first few SDK2004 engine interface factories. The entire game remains on the owner's computer.

## Obtain independent diagnostic DLLs

Build from the repo with Visual Studio 2022 Win32 (or use the independent PE32 diagnostic artifact on GitHub Actions):

    cmake -S pc/independent_slot0 -B build-slot0 -G "Visual Studio 17 2022" -A Win32
    cmake --build build-slot0 --config Release --parallel
    ctest --test-dir build-slot0 -C Release --output-on-failure

The built pair lives at build-slot0/bin/Release/client.dll and server.dll. They do **not** forward to the original DLLs. They are not normal playable game DLLs and only expose two virtual slots per module.

## Prepare a disposable standalone test copy

Copy the *entire* owned game installation outside the live Steam folder into a new directory (example: D:\SiN-DisposableTest). Include the actual game executable and both original bin/client.dll and bin/server.dll. Keep another backup independent of this test copy. The script checks full pinned SHA-256 values of both originals and refuses unexpected versions.

Place an explicit test-copy marker in the new game root:

    Set-Content -LiteralPath 'D:\SiN-DisposableTest\SIN_INDEPENDENT_DISPOSABLE_TEST_COPY.txt' -Value 'SIN_INDEPENDENT_DISPOSABLE_TEST_COPY_V1'

## Run the one-command engine probe

From your repository checkout:

    .\pc\independent_slot0\Invoke-OwnerIndependentEngineProbe.ps1 `
      -GameBin 'D:\SiN-DisposableTest\bin' `
      -GameExecutable 'D:\SiN-DisposableTest\YOUR_ACTUAL_GAME_EXE.exe' `
      -IndependentBin '.\build-slot0\bin\Release' `
      -GameArguments '-console' `
      -TimeoutSeconds 90 `
      -DisposableTestCopyConfirmed

Use your actual installed game executable name. The script intentionally rejects an executable outside the same disposable game root and a root path in steamapps/common.

The script validates all source and destination files before staging anything, moves SHA-verified originals to sin_independent_saved_client.dll / sin_independent_saved_server.dll, copies independent DLLs, launches the game with its own independent log switch, waits for exit, and restores both SHA-verified originals in a finally block. On timeout it terminates the launched process tree first. If a process cannot be stopped it deliberately leaves the original DLLs in the saved backup files for manual restoration after the game has closed.

No log or original code is uploaded. Each owner attempt generates a fresh lowercase hexadecimal session ID, inherited by the launched game process. The launcher reports `OWNER_INDEPENDENT_SESSION` and collects **only** matching `%TEMP%\sin-independent-SESSION-PID.log` files. This avoids mistaking recent output from another process for the current engine result. Standalone synthetic tests retain the legacy PID-only trace path when no session is set.

## Outcome classification

* No log: the game may not have loaded either independent DLL; launcher behavior / path must be checked.
* Client request VEngineClient012: the retail executable called our independent client Init candidate.
* Server request VEngineServer021: the retail executable called our independent server DLLInit candidate.
* missing-interface: the first SDK2004 factory interface request unavailable from the real 2006 game.
* sdk2004-preflight-only: the bounded first factory names were found, but the code still **returns failure** and is not game startup.

Do not treat a successful first request as a working game. Client/server initialization, remaining vtable layout and SiN-specific gameplay are not implemented here.

## Automated validation

Native Win32 CTest independent_owner_probe_guardrails checks that the owner script parses and refuses cases with no explicit confirmation, no marker, wrong marker, or wrong original DLL SHA256 **without modifying the synthetic test files**. The separate native independent DLL tests probe callbacks against synthetic factories. These tests do not execute the retail game. Source SDK 2004 defines candidate interface types and names, but Valve game source code is not bundled here.

## Session-specific first startup failure (Phase 294)

The owner launcher now prints `OWNER_INDEPENDENT_VERDICT`, the observed DLL
roles, the exact factory requests and the first recorded blocker from the
current session's logs. This is intentionally an **evidence classification**,
not an assertion of working engine initialization:

* `NO_INDEPENDENT_INIT_EVIDENCE`: no matching role-bearing diagnostic lines;
  verify game executable, DLL load paths and imports before changing ABI.
* `ENGINE_ENTERED_INDEPENDENT_INIT_BUT_REJECTED`: at least one observed
  missing interface, missing factory, or invalid argument. Fix the first
  named blocker using owner evidence and provenance-checked layouts.
* `DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF`: a DLL logged other phases, but
  there is no observed blocker. The independent initializers **still return
  failure**, so this is never a successful game boot.

The classifier runs locally on the owner's machine, using only the random
session token assigned by the launcher. No original game code or log is
uploaded. It can also be run independently against local logs:
`Get-OwnerIndependentStartupVerdict.ps1 -Session <32-lowercase-hex-token>
-LogDirectory $env:TEMP`. Public Windows CI exercises it using synthetic
logs, including cross-session isolation. Actual owner-engine evidence
requires the disposable-copy launcher and cannot be produced by CI alone.

## Game-module factory negotiation, before Init (Phase 297)

The opt-in independent PE32 diagnostics now record an actual call to the
module's exported `CreateInterface`, even if the engine requests an unknown
game-module version and never reaches virtual slot zero.

* `phase=factory-accepted version=VClient011` or
  `phase=factory-accepted version=ServerGameDLL003`: the independent DLL
  returned its SDK 2004 candidate object to the requesting caller.
* `phase=factory-rejected version=<requested-name>`: the caller requested a
  different name and this experimental DLL returned null. An individual
  rejection is **not** proof that the engine treats the request as fatal.
* `OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS`: all accepted/rejected requests
  captured within the current session, separate from engine-factory queries
  made later by the candidate `Init` or `DLLInit` implementation.
* `GAME_MODULE_FACTORY_ONLY_NO_INIT_EVIDENCE`: one or more game-module
  factory requests reached our export, but there is no recorded virtual
  callback entry. This improves diagnosis over a generic no-trace verdict;
  it still does **not** establish successful initialization or executable
  startup.

The native x86 loader regression explicitly exercises accepted and rejected
`CreateInterface` calls and verifies both opt-in trace messages. PowerShell
synthetic classifier tests ensure factory-only observations are not mistaken
for `Init` and cannot receive a successful startup verdict. The original
game installation is never used in public CI.
