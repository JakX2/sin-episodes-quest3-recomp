# Phase 524: one-click original server physics source-resource experiment

This package provides a new isolated **read-only** original-server
experiment, after the owner Windows client physics results proved the
original physics table exists already, even before ParseSurfaceData.

## Owner action

Extract the SHA-authenticated Windows artifact outside the installed
SiN Episodes folder. Double-click:

`RUN_SERVER_PHYSICS_READONLY.bat`

Select the genuine original SiN Episodes: Emergence installation.
The launcher checks exact hashes of original client.dll/server.dll,
rejects reparse points, copies the complete game to a disposable TEMP
directory, and stages **only** the independently compiled server.dll
in the clone. The client's installed and clone originals are never
modified or replaced by this server-only stage.

The standalone wrapper passes the mutually-exclusive
`-ServerPhysicsReadOnly` flag to `Start-SiNFileSystemTest.ps1`.
Client presence/read/soundscape tests are explicitly skipped in this
mode, because the original server's physics Factory is a distinct
`ServerGameDLL003::DLLInit` argument and original server interface
order is distinct from VClient011.

The server independent DLL requires sixteen engine interface
dependencies, calls the original physics Factory in the authentic
`VPhysics030`, `VPhysicsCollision007`,
`VPhysicsSurfaceProps001` order, and, only after 3/3 succeeds,
reads the original `scripts/surfaceproperties_manifest.txt`
(`rb/GAME`) plus two source scripts (`rb/nullptr`). The
read-only library requires the original 2 scripts / 93 materials /
71 bases / 0 duplicates and emits no original game text.

The expected diagnostic termination is `0x53494E7A` for verified
sources or `0x53494E7B` for refused script access. Earlier failure
may terminate through the 2006 factory status or an unrelated loader
error. Any missing, duplicate, out-of-order, or cross-session
sequence is rejected by the owner-only trace classifier.

Both original binaries must be SHA-verified restored before cleanup.
The owner receives a small ZIP
`SiN_FileSystem_Test_Results_*.zip` with `server.txt`,
`server-run.txt`, the startup trace, restoration evidence, and a
bounded public-safe `SUMMARY.txt` line
`Original server physics source resources (read-only)`.

## Limits

This is not Source SDK 2004's old server physics Init success and
does not call the original native server ParseSurfaceData mutator.
We cannot yet claim normal `ServerGameDLL003::DLLInit`, map
startup, gameplay, Android, or Quest 3 VR. No original client/server
DLL bytes or game content are published. Only the owner may run
this against their separate authenticated copy.
