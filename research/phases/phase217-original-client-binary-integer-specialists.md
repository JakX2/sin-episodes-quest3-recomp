# Phase 217: original client snapshot binary-data and integer helpers

14 November 2026. **Stacked DRAFT on Phase216 PR #270**. Directly inspected both original owner-provided 2006 PE32 SiN Episodes: Emergence DLLs from private `bin.zip`. Entire original SHA256 fingerprints match the previously pinned provenance:

- `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

No original DLL, proprietary complete native function body, full disassembly, models, maps or game data is committed.

## Four complete previously unreviewed native client functions

| Original client VA | Native bytes | Whole-native SHA256 | Original router callsites |
|---|---:|---|---|
| `0x24197A90` | **56** | `0fb74121fd315127c05255395992aae9e6737a74cee4133f2d1c3685947f0276` | `0x2419992E`, `0x24199A2E` |
| `0x24197AD0` | **60** | `fc0c439370b8fca8033459b1433ead82be379a2c135b394fb8e59b8427281441` | `0x24199964` |
| `0x24197B60` | **197** | `37d16baa4f88742042943342958e62964451d66c7eab39b12c539bffa96d49f2` | `0x24199A78` |
| `0x24198360` | **30** | `c654d2ca175b5fad2fd98a18efcbc4709f8feb15190c9ee733e578d63cf6279d` | `0x24199973`, `0x24199A87` |

Four authentic **SHA256-reviewed native entrypoints, 343 complete x86 bytes**. The original `RET` pops 12, 16, 16 and 16 stack bytes respectively, followed by original 8/4/11/2-byte `INT3` padding. All six original Phase212 router `CALL rel32` instructions are verified against the real DLLs. Six separate nested original formatter CALLs are also pinned. These are **not** six more recovered formatter methods.

## Decoded behavior with original evidence boundaries

**Shared binary comparator `0x24197A90`, type 8 and type 9.** Original `this+0x14` byte 0 immediately returns status 0 without reading buffers. Otherwise descriptor `this+0x18` flags byte `+0x13` bit `0x04` forces status 1. Otherwise the method performs forward x86 `REPZ CMPSB` on **the first native unsigned DWORD byte count**, comparing pointers from stack arguments 2 and 3. Returns 1 if all requested bytes match or the requested byte count is zero; 0 on the *first* unequal byte. A host implementation must not pre-scan both buffers and require unused trailing bytes. The CPU direction flag is expected clear.

**Binary diagnostic `0x24197AD0`, type 9.** If `this+0x14` is zero, no formatters run. Otherwise, when the first stack DWORD comparison status equals zero, the original calls `0x24197910` with `binary data differs (%i bytes)\n`; it then always invokes `0x241979C0` with `binary (%i bytes)\n`. The second stack DWORD is the count printed as integer. The name and real engine object behind either formatter remains unverified.

**Integer diagnostic `0x24197B60`, type 8.** Enabled-only logic uses the first argument as status and the next two arguments as *pointers to signed 32-bit integer values*. Status 0 prints `int differs (net %i pred %i) diff(%i)\n`, where the native difference is predicted minus net, wrapping at 32 bits. It then tests original descriptor flags bit `0x08`. If set, a process-global pointer at original `.data` VA `0x2433F6AC` provides a vtable virtual `+0x04` lookup for the predicted integer; a non-null result is dereferenced through virtual `+0x0C`, passed to the actual **`vstdlib.dll!Q_FileBase`** import at client IAT `0x24276254` with a 512-byte local buffer, then formatted as `integer (%i->%s)\n`. If lookup fails or bit 0x08 is absent, the original prints `integer (%i)\n`. The registry's actual class, string lifetime and formatter internals are NOT reconstructed. The host interface explicitly supplies lookup and basename behavior, with additional host-only buffer limits.

**Shared binary preview `0x24198360`, types 8 and 9.** If `DWORD(this+0x34) == DWORD(this+0x18)`, the original forwards the second stack DWORD byte count to `0x24197FE0` with `binary (%i bytes)`. This branch does not check the `this+0x14` comparison/diagnostic flag.

### Original Phase212 caller context

Type 9 multiplies its native record count by **four**, calls `0x24197A90`, optionally forwards copy with original `REP MOVSD` then `REP MOVSB`, calls `0x24197AD0` and `0x24198360`. Type 8 calls the *same* comparator with the original byte count, conditionally forwards copies, reads sign-extended **first bytes of the copied destination and source**, then calls `0x24197B60` and `0x24198360`. Their inline `REP MOVSD/MOVSB` sites were already in Phase212's fully SHA-verified router body. The host model composes only these selected direct branches, not the entire serializer, record allocations or engine internals.

The original copy gate is separate: `BYTE(this+0x2C) != 0` and comparison status `!=1`. The integer diagnostic gets signed first-byte values **after** any copy. This can make an original mismatch trace show two equal bytes after the destination has already been updated. Host-only signed count overflow refusal, pointer bounds and synthetic callback contracts are consciously not equivalent to native unguarded faults.

## Verification and tests

- `research/tools/audit_sin_original_client_binary_integer_specialists.py`: Requires full original retail SHA256s; checks **four whole-function SHA256s**, **51** sparse native instruction witnesses, exact original `RET` and `INT3` boundaries, **six** exact router `CALL` sites, six nested `CALL` edges, all **six** real diagnostic format strings, the original resolver global section and real `Q_FileBase` named IAT.
- `research/tests/test_sin_original_client_binary_integer_specialists.py`: original-free opcode/CALL/RET/padding/string/IAT mutation checks. Synthetic code never counts as original verified functions.
- `include/sin_compat/original_client_snapshot_binary_integer.hpp`: portable C++17 reference for byte comparison, two diagnostic branches, preview, optional resolver callback, and selected type-8/type-9 direct-call composition. No original class or symbol registry is invented.
- `tests/test_original_client_snapshot_binary_integer.cpp`: bytewise early mismatch, count=0, forced/disabled comparison, case-9 DWORD-to-byte count, copy and post-copy log ordering, signed first bytes, 32-bit wrapping difference, successful/missing symbol lookup, and host-only safety refusal.
- Root Linux and MSVC Win32 x86 native CTests added. They are synthetic host tests, not the retail game.

Run original private audit using the owner archive:

```sh
python3 research/tools/audit_sin_original_client_binary_integer_specialists.py --archive /path/to/bin.zip
python3 -m unittest discover -s research/tests -p 'test_sin_original_client_binary_integer_specialists.py' -v
```

## Checkpoint and caveats

Verified-function inventory **249 → 253** (**128 server, 125 client**), not a playable-game completion percentage. Four more unique native destinations in Phase214's scoped 36-destination snapshot router graph have been recovered, reducing its opaque count **25 → 21**. All direct helper destinations in type-8 and type-9 cases now have complete original code SHA evidence, but neither case is a standalone serializer replacement. Original engine lifecycle, native replacement gameplay DLL startup, full dynamic symbol registry and host-ABI integration remain unproven. **Windows PC first → Android ARM64 → Quest 3.**
