# Phase 517: SHA-authenticated 2006 **server** physics bootstrap

## Origin and genuine source evidence

The owner-private original **SiN Episodes: Emergence (2006)** SE1/bin/server.dll
has SHA256 \`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650\`.
The original owner file was inspected read-only, without disclosing its bytes
or private scripts. \`audit_sin_original_server_physics_bootstrap.py\` verifies
three **complete native function region digests**, plus sparse opcode, call,
string and pointer witnesses:

| Original server VA | Length | Full function SHA256 | Role |
|---|---:|---|---|
| \`0x2220D910\` | 127 bytes | \`415f272c8f5fd8406c8a2a25e04ad99064f7476284e91fd9fa834acd58cbe838\` | Server's own VPhysics factory requests, manifest call, and conditional return |
| \`0x22233FD0\` | 136 bytes | \`0afda89d832437af8f04442195f6afe9c334cd67a432ac557e54e301e52d3a02\` | Per-file binary read, handle close, virtual ParseSurfaceData |
| \`0x22234060\` | 247 bytes | \`37fa813203b8ec7aa57dd642dd72ce25fffad1a81241f520fa6d106e276b9745\` | KeyValues surface properties manifest loop |

**510 complete original executable function bytes** are privately SHA-authenticated.
This number is *neither* full decompilation coverage nor proof of executable source
equivalence. Public CI uses synthetic bytes/objects only.

## New original-server factory semantics

The server helper starts at **0x2220D910**, not 0x2220D920 (the latter is
an interior instruction after a stack setup and helper call).

It obtains the physics Factory via original helper \`0x22263C50\` and
requests its interfaces with null status pointers, exiting false immediately
after the first unavailable result:

| Actual order | Exact original factory interface | Original global |
|---|---|---|
| 1 | \`VPhysics030\` | \`0x22575390\` |
| 2 | \`VPhysicsCollision007\` | \`0x22575398\` |
| 3 | \`VPhysicsSurfaceProps001\` | \`0x225753A0\` |

**Original client order is different:** physics, surface-props, collision.
The server calls \`0x22234060(surfaceProps,filesystem)\` after all three are
found, obtaining filesystem from global \`0x225721A4\`. Only after that
void-like manifest call does it set a byte at \`this+0x19\` and return true.
The original helper does not treat individual surface file failure as a
checked boolean Init failure. Don't retrofit a new game-fatal gate there.

The server manifest opens the exact \`scripts/surfaceproperties_manifest.txt\`
with \`GAME\` and dispatches matching \`file\` entries. The file loader
\`0x22233FD0\` opens each referenced file with \`rb\` and null pathID through
its filesystem's virtual byte offset \`+8\`; it obtains Size(handle) at
\`+0x1C\`, reads at \`+0\`, **closes at +0x0C before parsing**, NUL-terminates
a temporary buffer, then invokes surface-props virtual **+4** with
(filename, text). The native original assumes valid length/stack, while
independent safely bounded probe code refuses untrusted file sizes.

### New direct virtual-method evidence

The original **server** at \`0x2220D9BB\` calls the original surface
interface global \`0x225753A0\` at virtual byte offset **+0x08**.
That directly authenticates the otherwise SDK-guided
\`SurfacePropCount()\` method index (slot 2). Original server
\`0x2220D9CF\` calls byte \`+0x14\` (GetSurfaceData).

Separately the original **client** calls its own surface interface
global \`0x243743A8\` at virtual byte **+0x0C** from original
callsites \`0x240B7EC5\`, \`0x24191E3C\`, \`0x241A7389\`,
matching legacy SDK \`GetSurfaceIndex()\` (slot 3).
Original client \`0x24184F62\` calls byte \`+0x14\` (GetSurfaceData).

Together, the historical public Source SDK 2004 headers and **both**
SHA-authenticated SiN 2006 originals establish the surface interface
slots 1 (Parse), 2 (Count), 3 (Index), 5 (Data) much more strongly than
SDK version strings alone. They do **not** prove Phase 516's new
independent owner-engine readback has run, nor that the material table
is initialized before the recreated callbacks.

## New independent code and owner experiment

\`server_original_physics_factory_probe.h\` makes precisely three native
MSVC x86 cdecl factory calls with their original names and null status
arguments, short-circuits at each missing result, and retains no engine
pointer. \`test_server_original_physics_factory_probe.cpp\` verifies exact
order and every early return on a fabricated 32-bit Factory.

A **distinct** \`SIN_INDEPENDENT_SERVER_PHYSICS_FACTORY_PROBE=1\` terminal
diagnostic in \`ServerGameDLL003::DLLInit\` is enabled only by the
authorized owner-only launcher flag \`-EnableServerPhysicsFactoryProbe\`,
in **Server-only** mode with \`-EnableServerFactoryProbe\`. It performs
the three **read-only** factory lookups after the existing 16 other
engine-interface prefix requests, not necessarily at the original
server's late game-system call position. This is a diagnostic ordering
difference, not recovered normal DLLInit.

If all three are found, it returns terminal exit \`0x53494E78\`.
If any lookup fails, \`0x53494E79\`. Neither is playable game boot.
\`Get-OwnerIndependentStartupVerdict.ps1\` requires precisely three
ordered events *after same-session server DLLInit entry* before printing
the result, and the synthetic classifier tests reject duplicates,
forged 2/3 success and early events. The original dll install is never
replaced manually and the SHA-gated reversible launcher refuses
non-disposable paths.

**No retail server triple has been observed yet in the independent
engine process**, and **no real server surface parse method** has been
invoked through the independent DLL.

## Next implementation priorities

1. Run this optional, SHA-gated, role-isolated server physics Factory
   diagnostic on a **disposable original Windows game copy**.
2. If 3/3 is confirmed, reuse the client surface resource *read-only*
   bridge and compare the original server's manifest search scopes.
3. Only after safe read verification, attempt server ParseSurfaceData
   virtual slot1 in a separate explicit mutating experiment.
4. Continue completing four remaining original client AutoGameSystem
   Init bodies and their engine dependencies, plus original server
   callbacks, before allowing either module to return full Init success.

Windows x86 boot remains first target; Android and Quest 3 follow.
