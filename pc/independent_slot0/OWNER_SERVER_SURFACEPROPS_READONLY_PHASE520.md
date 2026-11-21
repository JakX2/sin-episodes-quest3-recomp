# Phase 520: original server-side physics source manifest read, no engine mutation

The **2026-11-21 02:47:49** owner Windows physics result (SHA256
`56c9163dea74bb72ec17534fc66e24d56333fe0db4620f2fd060115dc33ffe37`)
resolves the Phase 519 baseline question. In the SAME original engine
session, BEFORE ParseSurfaceData: SurfacePropCount=94 and
GetSurfaceIndex(default/solidmetal/metalvehicle)=0/1/85. AFTER two
original ParseSurfaceData slot1 calls returning 0/0, the SAME values
remain: 94 and 0/1/85. Thus a pre-populated table already existed
before the opt-in calls. It is wrong to count those 94 entries as
created by our independent code. Neither a byte-for-byte comparison
of existing surface data nor normal game startup was performed.

## Original-server 2006 source evidence

Original SHA256 server.dll
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

- Original helper 0x2220D910 authenticates exact Factory request order:
  VPhysics030, VPhysicsCollision007, VPhysicsSurfaceProps001.
- Original manifest helper 0x22234060 traverses
  `scripts/surfaceproperties_manifest.txt` opened rb/GAME.
- Original per-script helper 0x22233FD0 uses rb/null search scope,
  closes read handles, then calls VPhysicsSurfaceProps001 slot1.

See `audit_sin_original_server_physics_bootstrap.py` for independent
full-function SHA/authenticated opcode witnesses. Public test code
**never packages original DLL bytes or original surface scripts**.

## Native implementation

New `server_original_surfaceprops_readonly.h` is a role-isolated
server resource adapter, using the prior, Windows-owner-authenticated
VFileSystem012 bounded reader and parser. It requires an already
verified **server-specific physics Factory 3/3** witness, refuses
missing or malformed engine filesystem ABI, and accepts only the
two original source scripts with 93 definitions and 71 base keys,
no duplicates. The script contents remain private, only counts
and status are emitted. Public C++ tests cover refusal and callback
wiring with synthetic objects, not real server physics execution.

A *separate* `SIN_INDEPENDENT_SERVER_PHYSICS_RESOURCES_PROBE=1`
opt-in follows successful owner-only server Factory 3/3 and reads
resources via the server DLLInit's third Factory argument.
It deliberately **does not** call original server surfaceprops
ParseSurfaceData, does not claim normal ServerGameDLL003::DLLInit
success, and exits a dedicated sentinel for diagnostics. The owner
must still reach server DLLInit in the real game process, which is
not yet verified on actual owner Windows.

Read-only client physics is now owner-observed; original server physics
and full independent Windows startup remain separately unobserved.
