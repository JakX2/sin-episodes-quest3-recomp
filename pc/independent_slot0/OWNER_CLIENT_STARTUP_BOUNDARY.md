# Owner-only independent client startup boundary (Phase 1693)

**Not a game build; not a successful Init/boot claim.** The script delegates to the existing SHA-pinned reversible diagnostic launcher and is intended only for a separately copied SiN Episodes game installation that the owner explicitly marks as disposable.

## What the test does

- Loads a separately built, non-forwarding Win32 x86 replacement `client.dll` into the **original retail engine**, while leaving the source-authenticated original `server.dll` unchanged.
- Requests original-order client Init factory dependencies, including native `VSoundEmitter001::Connect`; the existing owner-only source-authenticated prediction, `VClientPrediction001`, `VClientEntityList003` and cold `VCENTERPRINT002` boundaries remain opt-in.
- Uses `TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32` to observe the 32-bit process from 64-bit PowerShell. Captures which enumerator saw the replacement module and the **independent trace**, when one exists. A loaded module alone is not an Init success.
- Requires an existing root file `SIN_INDEPENDENT_DISPOSABLE_TEST_COPY.txt` containing exactly `SIN_INDEPENDENT_DISPOSABLE_TEST_COPY_V1`, original SHA256-pinned retail DLLs in `SE1/bin`, an independent binary folder distinct from the game, and explicit `-DisposableTestCopyConfirmed`.
- On Windows module locks or a still-running process, **retains the original backup and reports RESTORE_PENDING**; do not retry against that test copy until restoration is confirmed.

## Running (after the bundle's Windows CI is green)

1. Extract the GitHub Actions `sin-owner-client-startup-boundary-not-playable` artifact into a fresh directory, outside the installed game and outside the disposable game copy.
2. Verify you are selecting a **disposable game copy**, not the installed Steam game. The existing marker file must already be present in the copy root, and the original `SE1/bin/client.dll` and `server.dll` must be intact. **The wrapper does not create the marker or bypass SHA checks.**
3. Run this from Windows PowerShell, replacing only the game-root argument:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Run-OwnerClientStartupBoundary.ps1 -GameRoot "D:\SiN-DisposableTest" -DisposableTestCopyConfirmed
```

4. Save the terminal transcript and the session-isolated `sin-independent-*.log` trace paths printed by the launcher. Inspect `OWNER_INDEPENDENT_RESTORATION` before rerunning anything.

The original client/server hash pins are from the owner-provided 2006 PE32 files; the GitHub artifact contains **only independently authored DLLs and scripts**. No proprietary game code, assets or whole binaries are uploaded. Module detection, `CreateInterface`, Init callback execution and playable gameplay are separate milestones.


## Optional next runtime probe: cold `ClientLeafSystem001` (Phase 1695)

The owner's genuine Windows game report confirms the original engine
loaded the independently compiled client DLL, obtained all 21 source
interfaces, invoked `VSoundEmitter001::Connect` successfully, and accepted
`VClientPrediction001`, `VClientEntityList003` and `VCENTERPRINT002`.
The next source engine request `ClientLeafSystem001` was rejected because
the previous bundle intentionally omitted it. The game process exited
with code zero: **not** a normal gameplay/boot success. The original
client/server DLLs were verified unchanged after restoration.

Only a **newly verified Windows CI bundle** from the Phase1695 PR can run
the optional next test. Never add flags to or modify the older manifest
bundle. The existing default remains the Phase1693 baseline. For the new
package only, use:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Run-OwnerClientStartupBoundary.ps1 -GameRoot "D:\SiN-DisposableTest" -DisposableTestCopyConfirmed -EnableClientLeafFactoryProbe
```

This opt-in exposes only the source original factory singleton primary
interface with 22 fatal methods and +4/+8 secondary/auxiliary guards.
All 35 source virtual positions are **terminal unsupported callbacks**,
not guessed return values. This deliberately does **not** implement
CClientLeafSystem's real rendering, visibility, model or prop behavior.
A new `factory-accepted:ClientLeafSystem001` event would prove only that
the engine accepted this interface *pointer*. A subsequent
`unsupported-callback:ClientLeafSystem001.*` event would identify the
first original method the engine attempts. Do not equate either with
a rendered frame. The script still preserves the retail server and
restores the original client with SHA256 verification.

## Test limitations

The early client factories may be reached while further original post-sound lifecycle calls remain unsupported. Any intentional `Init` refusal, fatal unsupported callback, exit code zero or unsupported GUI constructor is diagnostic, not game boot. This experiment does not reopen user-deferred HL2-equivalent physics, models or prop rendering work.
