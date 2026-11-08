# Phase 113: SiN Episodes (2006) original skeletal setup dispatch

**Original specimen:** owner-provided private PE32 Windows `server.dll`, SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`,
preferred image base `0x22000000`. Research is based on original machine
instructions, not Source SDK assumptions. No original executable, source dump,
proprietary assets, or model-cache structures are redistributed.

## Verified function boundaries

| Function | Virtual address and end (exclusive) | Original SHA-256 |
| --- | --- | --- |
| CBaseAnimating virtual skeletal setup dispatcher | `0x220EF510..0x220EF7D7` | `50a14a2a1da29a091a6f38fb899914a9111b6881712ef05e35ec3f57c4e44b00` |
| Parent-linked bone construction | `0x220ED290..0x220ED3C2` | `c6e4dd1d5df71287141c1984818e1d1ba99cbda114d6d4f9e306f4804b7d63e6` |
| Ordinary bone construction | `0x220FE0C0..0x220FE20A` | `de50e3c4b85d14b4ad7fc12e07cb2be7169c57ef95859622f381f65144ea4666` |

Auditor `research/tools/audit_sin_original_skeletal_setup.py` verifies these
three entire function hashes, **22 relative CALL targets, 21 instruction
anchors and 8 original inherited RTTI/vtable pointers** against the private
DLL. The synthetic unit tests use fabricated PE32 headers only.

## Actual `0x220EF510` control flow

1. Enter original profiler, acquire model-cache lock through global
   `0x225721C8` virtual `+0x14`.
2. Obtain the studio model through `0x22139A10`, check model resource type `3`.
3. Refresh dirty entity world transform (`+0xB8` bit `0x800`) through
   `0x2213FA60` and compute the input pose origin from entity world origin
   `+0x2BC`, adding the entity float `+0x3D4` to Z; use angles at `+0x354`.
4. **If pointer `+0x3D8` is nonzero:** increment integer `+0x3DC`, refresh a
   newly dirty world transform if necessary, call pose driver helper
   `0x22106840`, **virtual `+0x2AC`**, helper `0x22103A60`, **virtual
   `+0x2B8`**, helper `0x22104D10` in that order. These functions depend on
   native studio model, pose buffers, engine time, bone masks and virtual
   implementations, which are deliberately host callbacks.
5. **If pointer `+0x3D8` is zero:** call virtual **`+0x2AC`** only.
6. Follow parent EHANDLE at entity `+0xF0`, validating `0xFFF` table-index
   bits and the shifted generation. The MSVC RTTI cast requires a compatible
   `CBaseAnimating`-derived parent.
7. If a compatible parent exists, call original parent bone-cache preparation
   `0x220EB8D0`. If the resulting cache is valid, refresh the child's dirty
   world transform if required and call **`0x220ED290`** for parent-linked
   bone-matrix construction. The original `0x220ED290` itself can request
   individual parent cached bone matrices through `0x221004C0`.
8. If no compatible parent or no available parent cache, call **`0x220FE0C0`**
   for ordinary bone construction.
9. Release model-cache lock, exit original profiler, return `ret 8`.

Crucial dependency: the child skeletal setup can re-enter bone-cache
preparation on an animated **parent**. Native host must handle model-cache
lock lifetime and recursion/cycle-safe parent resolution. A direct Windows
x86 virtual call is **not** an ARM64 ABI.

### Independently verified virtual dispatch differences

Both `+0x2B4` (setup dispatcher) and `+0x2B8` (pose hook) remain the same
function across eight verified class vtables, but `+0x2AC` differs:

- `CBaseAnimating`: `0x220F2AC0`
- `CBaseAnimatingOverlay`, `CBaseFlex`, `CBaseCombatCharacter`,
  `CBasePlayer`, `CHL2_Player`, `CAI_BaseNPC`, `CNPC_Jessica`:
  `0x220F5430`

Thus the reconstruction preserves virtual dispatch for `+0x2AC`, instead of
assuming Jessica uses the base-class implementation.

## New independent C++17 implementation

`original_skeletal_setup.hpp/.cpp` is a **dispatch orchestrator**, not a
complete skeletal solver. It sequences the original observed branches through
`IOriginalSkeletalSetupHost`, which owns virtual pose evaluation, model cache
locks, parent bone cache, `SetupBones`-style matrix construction, and actual
poses. The `OriginalSkeletalPreparationAdapter` supplies the resulting
matrices to Phase 112's 
`IOriginalBonePreparationHost::calculate_bones`, allowing the existing
Phase 111 cache remap and Phase 110 attachment resolution to consume the
refreshed matrices. The code does not return original game pointers or
reinterpret original 32-bit vtables.

Added checks not proven in original binary: rejects invalid studio state,
nonfinite transform data/pose output, floating-point overflow, and native
callback failure; always attempts to release host-owned model-cache lock even
when evaluation throws. These **differ intentionally** from original error
or undefined behavior and should not be represented as exact decompilation.

## Tests

- Normal no-driver path always calls virtual `+0x2AC`, constructs ordinary
  matrices and unlocks the cache.
- Driver path increments `+0x3DC`, adds `+0x3D4` to origin Z, performs all
  driver helper callbacks in original observed order.
- Compatible animated parent + cache dispatches parent-bone builder;
  nonanimating/missing parent cache uses ordinary builder.
- Recheck parent **after** virtual pose callbacks; tests cover parent changes.
- Explicitly check callback errors, model-lock failure and cleanup on throws.
- End-to-end host test: dispatcher -> Phase 112 stale cache updater ->
  Phase 111 compact remap -> Phase 110 attachment getter. Confirms later
  attachment positions update after cache expiry and stay unchanged while
  cache is fresh.
- Synthetic PE32 audit mutation test and read-only original DLL SHA auditor.

## Not yet implemented

The actual animation pose interpolation, `0x22106840` / `0x22103A60` /
`0x22104D10` internal algorithms, the full parent-builder
`0x220ED290` and ordinary-builder `0x220FE0C0` math, native `CStudioHdr`,
model cache lock, entity EHANDLE implementation and animation-memory
management remain unimplemented native-host dependencies. These will need
further byte-level investigation before live engine integration.

This is not a working native standalone SiN Quest 3 APK nor complete rebuilt
Windows `client.dll` or `server.dll`. The desktop HL2 copy is an external
experimental host, and does not establish ABI compatibility with 2006 SiN.
