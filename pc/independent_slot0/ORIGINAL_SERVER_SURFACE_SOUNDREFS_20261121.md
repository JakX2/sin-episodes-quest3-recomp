# Phase 523: Source-authenticated original server surface sound-reference loop

## Evidence, not a fabricated game implementation

The original owner-supplied retail 2006 **server.dll** must have whole-file
SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

The native method at original VA `0x2220D9B0`, **352 bytes**, has complete
function-region SHA256:

`77fbcb512867a57087c25c63f97bf902fcc7149dec58f8d71920569699bc237e`

The exact method body was verified privately. The public repository
stores ONLY this hash, a sparse layout/target signature verifier and
independently written source. Neither complete original bytes nor
proprietary game scripts are committed.

Its original call graph has been verified at the machine instruction level:

| Original action | Original ABI witness | Count |
|---|---|---:|
| Get engine surface database size | virtual byte offset `+8`, SurfacePropCount() | 2 sites, initial and loop-tail |
| Get each surface data struct | virtual byte offset `+0x14`, GetSurfaceData(surfaceIndex) | 1 loop callsite |
| Read 16-bit sound-name index | `surfacedata_t` offsets `+0x2c` through `+0x3e` in 2-byte steps | 10 distinct fields |
| Resolve each **nonzero** sound index | virtual byte offset `+0x18`, GetString(index) | 10 conditional callsites |
| Forward resolved name | wrapper `0x221B6170`, which in turn calls helper `0x221B5DB0` using global `0x22507558` | 10 conditional callsites |

The original method loops over surfaces in increasing index order,
rechecking `SurfacePropCount()` after each iteration. Each of the ten
16-bit name indices is **zero-guarded**, and the source helper skips
virtual `GetString` and the forwarding helper for zero indices.

The original source-era SDK header
`Source-SDK-Base-Legacy-Project/source-sdk-2004/src/public/vphysics_interface.h`
identifies the exact ten names as `stepleft`, `stepright`,
`impactSoft`, `impactHard`, `scrapeSmooth`, `scrapeRough`,
`bulletImpact`, `rolling`, `breakSound`, `strainSound`.
Five physics floats (20 bytes) plus six audio floats (24 bytes) put
the sound-name structure at +44, exactly matching private opcode evidence.

Original server code calls `0x2220D9B0` on paths `0x2220EBCB` and
`0x2220EBE4` before completing a further initialization byte.
The forwarding wrapper's later behavior is **not** reconstructed or
owner-verified yet. Treat it as a candidate sound-reference registration/
precache helper, not an established safe read-only method.

## New original-free implementation

`server_original_surface_soundrefs_core.h` models these ten packed
field indices and ABI offsets with compile-time checks, enumerates
the fields in source order and forwards nonzero indices to an entirely
owned injected callback. It does not call VPhysics engine methods
or interact with runtime sound, graphics, gameplay, or map state.

`test_server_original_surface_soundrefs_core.cpp` uses **94 fake
surfaces** (matching the observed original engine count but not the
game's original surface data), injects six fabricated nonzero IDs,
confirms their exact order, and tests refusals for missing data,
oversized counts and early callback failures. This is a semantic
test, not retail-game equivalence.

`audit_sin_original_server_surface_soundrefs.py --self-test` tests
synthetic x86 signatures and intentional mutations. Its optional
private `--server path` authenticates a full matching original
server image and method-region SHA; no machine code is printed.

## Owner findings and remaining boundaries

The 2026-11-21 original-client Windows ZIP at 02:47:49 established
`SurfacePropCount=94` and original-name indices `0,1,85`
**before** and **after** optional two native ParseSurfaceData calls,
both returning zero. That does not prove what sound-name indices
exist in those 94 engine entries.

**No native server VPhysics GetSurfaceData/GetString call has yet been
observed** through an independently built server.dll. This phase
is a bounded synthetic control-flow reconstruction only. Do not
register sound references, call the original `0x221B6170`, complete
`ServerGameDLL003::DLLInit`, or claim playable map/Android/VR boot
until the server Factory and manifest owner diagnostics are observed
and the helper ABI/side effects are sufficiently understood.

Related work: Phase 520 #520 server physics Factory one-click;
Phase 521 #522 server read-only surface manifest; overlapping
alternative #521 must be reconciled rather than blindly merged.
