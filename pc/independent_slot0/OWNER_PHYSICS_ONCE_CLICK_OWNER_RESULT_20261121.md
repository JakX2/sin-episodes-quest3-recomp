# Phase 518: Owner physical-engine probes as explicit one-click choices

## Evidence received 2026-11-21

The owner submitted a text-only Windows archive
`SiN_FileSystem_Test_Results_20261121-021805-06536af8b9.zip`,
SHA256 `92e95539e3f35de9ed346817e7e14ccdd7c0bb320a8246618d9083588a23f41c`.
The private raw diagnostics and installed game bytes are **not** tracked.

The archive's `SUMMARY.txt` reports successful independent filesystem
presence, read, 66-stat binding, 27:28:5 hook verification, and original
soundscape catalog `7:301:242:3`. Its three authentic diagnostic
sessions intentionally exit by their designated terminal codes.
The copied original game pair was verified restored and the temporary
copy removed. **The original soundscape ConVar, physics Factory,
surfaceprops read, ParseSurfaceData and physics readback stages all show
`NOT_REQUESTED`**. This new result is a repeated baseline, not an
engine-side physics registration observation.

## Corrective packaging

A new, separate `RUN_PHYSICS_READONLY_TEST.bat` passes the existing
opt-in chain through `-SurfacePropsRead`. It does not call original
physics methods that mutate surface state.

The separately named `RUN_PHYSICS_READBACK_TEST.bat` requires the
owner to type `REGISTER` and only then passes the full chain including
`-SurfacePropsRegister -SurfacePropsReadback`. The underlying SHA-gated
`Start-SiNFileSystemTest.ps1` still authenticates originals,
creates a disposable copy, and refuses unverified restoration.
Both scripts advertise controlled diagnostics, not real game startup.

These wrappers do **not** change either independently reconstructed
DLL's ABI or assert that original surface registration works. Engine-side
results must be collected from a new owner-side Windows run. No normal
`VClient011::Init`, `ServerGameDLL003::DLLInit`, map boot, Android,
or VR gameplay is claimed.

The names and exact flags are unit-tested in the Windows package
validation job; the manifest covers every packaged wrapper by SHA256.
