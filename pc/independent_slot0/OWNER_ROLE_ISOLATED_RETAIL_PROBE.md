# Owner-only role-isolated Windows retail startup probes (Phase 324)

## Motivation

Two owner retail Windows experiments reached the independent client
Init callback but did not reach the server DLLInit callback. Both DLLs
were replaced in those experiments, and the diagnostic client deliberately
returned failure. A zero engine exit code is not successful startup.

The independent server now has an original-guided 16-request DLLInit
diagnostic. The new launcher mode permits keeping the hash-pinned original
client.dll in place while replacing only server.dll. This isolates one
known source of early failure; it does not establish that DLLInit will
be entered. The diagnostic server still does not implement or advertise
ServerGameEnts001, and has only a bounded startup ABI. The retail engine
may reject it, stop even earlier, or crash. Never use the live installation.

## Owner usage

Use a confirmed disposable game copy, the corresponding CI independent
PE32 diagnostic artifacts, and the updated launcher.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File '.\pc\independent_slot0\Invoke-OwnerIndependentEngineProbe.ps1' `
  -GameBin 'D:\SiN-DisposableTest\SE1\bin' `
  -GameExecutable 'D:\SiN-DisposableTest\SinEpisodes.exe' `
  -IndependentBin '.\independent-bin' `
  -ProbeRole Server -GameArguments '-console -game SE1' `
  -TimeoutSeconds 90 -DisposableTestCopyConfirmed 2>&1 |
  Tee-Object -FilePath '.\sin-server-only-probe.txt'
```

Both original DLLs are authenticated prior to any mutation. Only selected
roles require candidate files. Server mode stages server.dll and leaves
original client.dll in place. Client mode is symmetric. Both remains
the backward-compatible default. Existing backup/reparse/marker guards
apply to the entire game copy. Selected backups are verified and restored
independently; retained original hashes are checked again in the finally
block. Do not rerun if restoration is incomplete. Keep any saved originals
untouched until hash-verified restoration is possible.

Full session evidence is now printed up to 1000 lines (previously only
24 tail lines); the classifier reads all matching session logs. Look for
OWNER_INDEPENDENT_PROBE_ROLE, RETAINED_ORIGINAL_UNCHANGED, RESTORED_VERIFIED,
OWNER_INDEPENDENT_ROLES_OBSERVED and OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES.
No trace, or factory-only trace, is not evidence of successful DLLInit.

## Regression coverage

The Windows guardrail test runs a test-only nonlaunching copy of the
launcher with invented distinct original hashes and a deliberately
replaced launch line. It asserts all three role modes stage the requested
synthetic files only, and that partial-launch failure restores both
original files. No retail executable, original DLL or fake exe is run.

Only the owner running the real disposable installation can establish
retail server DLLInit entry. No such result is claimed by this change.
