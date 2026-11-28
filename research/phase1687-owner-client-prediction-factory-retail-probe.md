# Phase 1687: owner-only real retail-engine prediction-factory probe

## Why this follows Phase1686

Phase1686 extends the **actual independent production `client.dll`**, not a separately compiled test fixture: an explicit owner diagnostic session may now pass the `VClient011` 21-query prefix and publish the source-authenticated `VClientPrediction001` 23-slot factory only after `VEngineCvar002` supplies an accessible `cl_predict` parent ConVar numeric value. Default startup and factory registration remain off; unsupported gameplay callbacks always terminate with native status `0x53494E45`. Phase1686's CTest loads the real compiled client.dll with `LoadLibrary` and an invented engine. This **is not evidence that the actual retail game engine has requested the interface**.

The existing project owner-only launcher `pc/independent_slot0/Invoke-OwnerIndependentEngineProbe.ps1` is the established safe way to test an actual engine startup using **SHA-verified originals, a explicitly marked disposable game installation copy and reversible DLL staging**. Until Phase1687, it did not provide the new prediction diagnostic gate. Inheriting arbitrary environment values was not acceptable, because the new gate intentionally permits the otherwise fail-closed `VClient011.Init` to return 1, which makes the next engine calls observable.

## New explicit launcher option

`-EnableClientPredictionFactoryProbe` is now a first-class PowerShell switch. It requires:

- `-ProbeRole Client`. `Both` or `Server` is rejected before any original DLL is moved, copied or game process is launched.
- `-DisposableTestCopyConfirmed` plus the original SHA-pinned/disposable-copy protections already enforced by the launcher.
- **No** simultaneous `-EnableClientSoundProbe`: that branch is a different terminal diagnostic. The rest of the client terminal stages require the sound path, so this also excludes attempting to mix unrelated probes.
- The existing launcher-generated 32-character lowercase hexadecimal `SIN_INDEPENDENT_TRACE_SESSION`, with trace logging enabled only during the specific child game process session.

The launcher sets `SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE=1` only for this explicitly selected child process and overwrites it with `0` for all ordinary launches. It saves any inherited value and restores that exact original value in the `finally` block, even after an engine crash or refused startup. It does not modify the live Steam installation: the original client.dll is staged and restored only under the existing owner's disposable-copy rules.

Example for an owner-maintained **disposable copy** of their game:

```powershell
.\Invoke-OwnerIndependentEngineProbe.ps1 `
  -GameBin 'D:\SiNEpisodesDisposable\SE1\bin' `
  -GameExecutable 'D:\SiNEpisodesDisposable\SinEpisodes.exe' `
  -IndependentBin 'D:\independent-build\Release' `
  -ProbeRole Client -EnableClientPredictionFactoryProbe `
  -DisposableTestCopyConfirmed
```

These example paths are illustrative, **not asserted as actual filesystem paths**. The project already provides the necessary SHA-verified, read-only original source and independent PE32 build checks.

## Classification without false boot claims

`Get-OwnerIndependentStartupVerdict.ps1` now recognizes the production diagnostics `phase=prediction-bound version=cl_predict-parent-value` and `phase=prediction-bound-refused version=cl_predict-unavailable-or-unsafe` in the same process/session trace, and emits `OWNER_CLIENT_PREDICTION_FACTORY_BOUNDARY` for them. The generic `factory-accepted VClientPrediction001` and `unsupported-callback VClientPrediction001.slot3-unsupported` events remain independently visible. **None** yields a success/boot verdict. A `VClient011.Init` return of 1 in an opt-in diagnostic session means only that the engine may proceed to the next Source-interface query; it does not mean the initializers, client entity list, player simulation or actual gameplay work.

Synthetic PowerShell tests now assert unsafe role/sound combinations reject before any file modification, verify the environment variable is explicitly set/revoked, and replay same-session `VClientPrediction001` factory evidence plus an unsupported callback without classifying it as a running game. No original retail game is executed by public CI.

## Next real blocker

If the engine cannot find `cl_predict`, the current Phase1686 path fails closed at the owner-only prediction binding step. The original client.dll normally constructs and registers its own `cl_predict` ConVar during static initialization, as authenticated by Phase1658 (default `1`, flags `0x200`), but the independent production DLL does not yet create/register that original ConVar. A future narrow source-guided owner-only registration path may be required. Beyond that, the remaining `VClient011` lifecycle and full original `VClientPrediction001` runtime methods still need native integration. No physical modeling, material/prop/model rendering, or HL2-equivalent physics features were added.
