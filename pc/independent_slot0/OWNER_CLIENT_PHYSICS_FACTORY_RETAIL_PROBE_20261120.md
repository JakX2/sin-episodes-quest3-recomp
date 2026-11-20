# Owner-retail 2006 client physics-factory triple

## Previous proven owner checkpoint

A real owner-run 2026-11-20 Windows diagnostic reached client terminal
exit 0x53494E76: original VFileSystem012 loaded seven soundscape
scripts, the independent catalog contained 301 definitions, 242
nested references and three unresolved; both VEngineCvar002
ICvar::FindVar calls succeeded. The sanitized proof is in PR #512
(OWNER_RETAIL_SOUNDSCAPE_AND_CVAR_SUCCESS_20261120.md).
This new stage is not another repeat of that same soundscape proof.

## Exact original method, SHA-verified privately

The user's original 2006 client.dll is PE32, base 0x24000000,
SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Original 97-byte helper CClientDLL physics init at VA 0x24182C60:
complete SHA256 46e079ea76e0cfee721efe3a421d68920cf980079aea4d76d7542f6f9b5281f2.

VClient011::Init calls it at 0x240FE540 and branches to
original shared Init-failure exit 0x240FE59E if it returns false.
The helper uses exactly the *second* Init argument (physicsFactory),
not appSystemFactory, as its cdecl 2-argument Factory callback:

| Sequence | Requested name | Original global written |
| --- | --- | --- |
| 1 | VPhysics030 | 0x24374398 |
| 2 | VPhysicsSurfaceProps001 | 0x243743A8 |
| 3 | VPhysicsCollision007 | 0x243743A0 |

Each query passes a null optional status pointer. The helper
stores each returned interface pointer, then exits false on
the first null. If all three succeed, the original loads its
previous VFileSystem012 pointer and calls separate helper
0x241927A0 with the surface-properties and filesystem pointers.

The exact private original binary region SHA and disassembly
were re-checked from the owner bin.zip for this phase.
Our earlier Phase180 source auditor also authenticates this
entire method and original call edge.

## New actual engine-facing read-only diagnostic

The independent x86 client now has
client_original_physics_factory_probe.h. Its
ReadOnlyQueryThree implementation calls the engine-provided
physicsFactory pointer using the exact MSVC x86 cdecl
Factory(const char*, int*) signature and nullptr status
argument. It sends the original three string names in order
and stops on a missing result.

The returned engine pointers are not stored or dereferenced;
no physics virtual methods, physics coupling callbacks,
global publishing, renderer interactions or game-system
callbacks are invoked. Results are bounded:
- original-physics-three-available:3:3:1
- original-physics-missing-first:1:0:0
- original-physics-missing-second:2:1:0
- original-physics-missing-third:3:2:0
- original-physics-no-factory:0:0:0

The source-original stage can be reached only through an
additional opt-in after the existing real-engine
soundscape and ConVar stages. The terminal exit codes
0x53494E76 and 0x53494E77 are NOT changed by physics
interface availability. This is still not full Init.

Three bounded client trace events record source order:
1. engine-fs-physics-entered with physics-factory:three-original:read-only
2. engine-fs-physics-result with typed status and exact counters
3. engine-fs-physics-terminal-stop with no-physics-coupling-no-game-init

The same-session classifier publishes an independent
OWNER_CLIENT_PHYSICS_FACTORY_RESULTS line only when these
three events follow exactly one completed soundscape/ConVar
chain and precede the soundscape terminal. It refuses
inconsistent counters, duplicated and premature events,
or missing previous CVar evidence.

## Next owner-run command (after CI passes)

Download the new Windows x86 standalone artifact named
SiN-Episodes-Standalone-OneClick-Filesystem-Test-Win32,
extract it to a folder outside the original game install,
open cmd in the extracted folder and run:

    RUN_SOUNDSCAPE_CATALOG_TEST.bat -SoundscapeCvarLookup -PhysicsFactoryLookup

The launcher requires all earlier presence/read/staged/hook,
soundscape and CVar prerequisites. It checks the original
retail pair SHA256, creates a disposable game copy,
restores originals and reports any incomplete restoration.
The process environment physics flag is restored afterward.
No manual DLL replacement in the installed game is needed.

The generated text-only result ZIP SUMMARY.txt gains:
Original physics Factory triple (read-only): [typed status or NOT_RECORDED].

## Public independent tests and limits

Native MSVC x86 CTest uses a fabricated cdecl physics factory to
verify exact original interface names and order, null status
parameters, 3/3 positive and early failure after each missing
result. The original post-physics coupling at 0x241927A0
is explicitly not invoked.

Owner PowerShell fixtures cover:
- default soundscape and ConVar-only modes do not enable physics;
- explicit physics switch requires CVar plus all earlier gates;
- same-session physics event order and strict result strings;
- forged 3/2, duplicate or early events cannot masquerade as proof;
- the existing soundscape exit verdict stays distinct and unchanged.

The original twelve AutoGameSystem callbacks, 0x241927A0
coupling, post-physics UI/HUD calls, source entity lifecycle,
server ServerGameDLL003::DLLInit and actual engine game
map loading remain unfinished. A positive 3/3 physics
interface availability result would be additional real engine
compatibility evidence, NOT playable Windows/Android/VR boot.
