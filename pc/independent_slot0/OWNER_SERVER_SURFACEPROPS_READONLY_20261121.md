# Phase 521: Original 2006 server physics manifest read-only bridge

## Proven original-method call graph

The owner's original `server.dll` SHA256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
contains:

- `0x2220D910`, 127 bytes, SHA256
  `415f272c8f5fd8406c8a2a25e04ad99064f7476284e91fd9fa834acd58cbe838`.
  Exact server Factory order: VPhysics030, VPhysicsCollision007,
  VPhysicsSurfaceProps001. After three nonnull results, calls 0x22234060
  with the original server filesystem global and surface interface.
- `0x22234060`, 247 bytes, SHA256
  `37fa813203b8ec7aa57dd642dd72ce25fffad1a81241f520fa6d106e276b9745`.
  Parses `scripts/surfaceproperties_manifest.txt` through the original
  `GAME` search path and iterates `file` child entries.
- `0x22233FD0`, 136 bytes, SHA256
  `0afda89d832437af8f04442195f6afe9c334cd67a432ac557e54e301e52d3a02`.
  Reads two script files using `rb` and null path ID, closes the file
  handle before parsing them through physics vtable slot1.

Original function-region bytes are analyzed privately only. The public
repo contains no complete original instructions or proprietary scripts.

## Latest owner Windows evidence before this server stage

The independent CLIENT-side Phase519 test archive
`SiN_FileSystem_Test_Results_20261121-024749-5e189ae384.zip`
SHA256 `56c9163dea74bb72ec17534fc66e24d56333fe0db4620f2fd060115dc33ffe37`
has **the same real original engine** VPhysicsSurfaceProps001 count
`94` and surface indices `default=0,solidmetal=1,metalvehicle=85`
**before** the two ParseSurfaceData calls and **after** the two
calls returned `0,0`. Installed original file restoration was verified.

Therefore engine-side surface materials were already populated before
this optional native parser call, and claiming 94 independent new
registrations would be wrong. A count/index equality does NOT
prove identical full material fields, nor normal game Init. This is
the reason to shift new diagnostics toward the independently rebuilt
server's Factory and manifest path. The SERVER part is NOT yet
observed on retail Windows.

## New independent code

`server_original_surfaceprops_readonly.h` combines an exact
original-order server physics Factory callback (including short-circuit
behavior) with the already independent, bounded, owner-client-tested
`VFileSystem012` surface resource bridge. It requires the known original
manifest shape: two source scripts, 93 unique material definitions,
71 base references and no duplicate names, with no physics mutation.

New native `test_server_original_surfaceprops_readonly.cpp` tests the
same code using only invented synthetic files and fake interfaces,
including original factory ordering, missing second interface, invalid
filesystem, traversal/refused manifest and complete recovery. No actual
commercial data or original game binaries are used in CI.

## Optional actual-Windows owner experiment

Once CI passes, extract a fresh one-click Windows package outside the
installed game and launch `RUN_SERVER_SURFACEPROPS_READONLY_TEST.bat`.
This is a separate switch chain:

`-ServerPhysicsFactory -ServerSurfacePropsRead`.

The existing SHA-authenticated disposable-copy launcher leaves the
installed Steam DLLs untouched, keeps the original client.dll in the
copy, stages the independent server.dll only, and picks a real retail
BSP to induce `ServerGameDLL003::DLLInit`.

If original server physics Factory 3/3 is reached, it reads the original
manifest and two resources through the server's own VFileSystem012,
with session-bound `server-surfaceprops-entered/result/terminal-stop`
trace records. Read-only resource success terminates at `0x53494E7A`,
resource failure at `0x53494E7B`. Original physics Factory refusal
still uses `0x53494E79` before resource stage.

The classifier prints `OWNER_SERVER_ORIGINAL_SURFACEPROPS_RESULTS` only
for three ordered resource events AFTER a same-session three-event
original server physics 3/3 prefix. The one-click SUMMARY includes
`Original server surfaceprops manifest and scripts (read-only)`.
No unclassified or cross-session raw log can prove resource success.

**Strict limitation:** This does not call the actual server vtable slot1
ParseSurfaceData, create a physics object, initialize any map, complete
ServerGameDLL003::DLLInit or advance Android/VR portability directly.
Actual server runtime evidence requires a NEW owner-side Windows ZIP.
