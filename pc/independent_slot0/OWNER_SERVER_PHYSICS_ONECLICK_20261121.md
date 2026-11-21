# Phase 520: Independent server physics Factory owner one-click

## Why server, instead of repeating client physics

The owner's SHA-authenticated real Windows CLIENT diagnostic has already
returned original surface table Count=94, indices 0,1,85. Phase519 now
captures the surface table before parsing in the original engine to
distinguish a preloaded engine database from a changed one.

The SERVER has different original 2006 physics Factory query order:
1. `VPhysics030`
2. `VPhysicsCollision007`
3. `VPhysicsSurfaceProps001`

That order was recovered from privately SHA-authenticated original server
DLL method `0x2220D910` and tested synthetically in Phase517. No real
server-side factory result has yet been observed from the owner's engine.

## Safe owner test

Extract the latest successful Windows one-click artifact *outside* the
installed game, then double-click `RUN_SERVER_PHYSICS_FACTORY_TEST.bat`.
No command-line physics switches are needed.

The batch invokes existing `Start-SiNFileSystemTest.ps1` with the separate
`-ServerPhysicsFactory` option. That established owner launcher:

- Authenticates the installed original client/server SHA256 pair before
  copying, refuses symlinks/reparse points and original-mismatched DLLs.
- Creates an isolated entire game copy, then verifies both original DLLs.
- Runs the established original-client filesystem presence and read gates.
- For the new server phase, invokes the existing authenticated owner
  `Invoke-OwnerIndependentEngineProbe.ps1` ONLY with
  `-ProbeRole Server -EnableServerFactoryProbe
  -EnableServerPhysicsFactoryProbe`.
- Keeps the original client DLL intact and stages only the independently
  compiled server DLL in the disposable clone.
- Requires a retail `SE1/maps/*.bsp` basename, automatically picking the
  first plain lexicographic filename or accepting an explicit
  `-ServerMap NAME` whose `NAME.bsp` exists. No untrusted path or shell
  punctuation is allowed. It passes `-console -game SE1 +map NAME` to
  ask the original Source engine to enter server DLLInit.
- Collects bounded session-specific server log and classifier results.
- Verifies actual native engine terminal `0x53494E78` together with
  EXACTLY one server `three-available:3:3:1` observation and the
  server-specific non-boot verdict.
- Restores SHA256-confirmed original DLLs, retains a clone rather than
  deleting it if restoration is not verified, and emits a text-only ZIP
  alongside the one-click files.

Expected summary:
`Original server physics Factory triple (read-only):
original-server-physics-three-available:3:3:1`

If the original engine never loads the independent server, the summary is
`NOT_RECORDED`, not success. A missing physics interface has its own
`original-server-physics-missing-*` status; no gameplay boot is claimed
and the diagnostic may stop before server DLLInit for unrelated reasons.

This remains intentionally **read-only**: it does not invoke the original
server's manifest loader, call virtual ParseSurfaceData or return
`ServerGameDLL003::DLLInit` success. Game map loading is only a means
of reaching that engine ABI callback, not an assertion the map works.

Tested only with public synthetic CTest and actual Windows build in CI
until a fresh authentic owner ZIP is supplied.
