# Phase 218: original client prediction type-7 signed-short snapshot helpers

14 November 2026. **Stacked DRAFT on Phase217 PR #271.** This phase reexamines both owner-supplied original retail 2006 PE32 `server.dll` and `client.dll` in private `bin.zip`. Both full original SHA256 fingerprints match the preexisting research provenance:

- `server.dll` SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `client.dll` SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

No original DLL, complete machine-code body, proprietary disassembly, original game map or assets is committed.

## Four authentic native function bodies individually SHA-reviewed

| Original 2006 client.dll VA | Native bytes | Entire original native SHA256 | Original Phase212 type-7 direct CALL |
| --- | ---: | --- | --- |
| `0x24197D50` | **84** | `451e236f93e76e4bbb98c8403fbfe966f5902fb8d7de339ba19999010d96ca59` | `0x241999F4` |
| `0x241989E0` | **48** | `e2ddec19682c14da96bb43b3ad18312d1b6789e38f25dd3c629f8d412f209bd6` | `0x24199A05` |
| `0x24197B10` | **79** | `a82821988ab380a78ec609508f6f43502daa19b9ff5d63ad211de7f9c0362fbf` | `0x24199A10` |
| `0x24198380` | **33** | `80ed762c28807f23273f2449855fd6063b3b3d37ee4068a007b77e3b2783cb2f` | `0x24199A1F` |

**Four genuinely new unique native client methods, 244 complete original x86 bytes**. Original `RET` stack cleanup is **12/16/16/16** bytes. The first/third/fourth function ends are followed by **12, 1 and 15** `INT3` padding bytes. The word-copy function at `0x241989E0` is an important **zero-padding** exception: its final `RET 0x10` ends exactly at `0x24198A10`, the previously SHA-reviewed Phase213 type-5 DWORD-copy method's entrypoint. The auditor separately witnesses the next method's native entry bytes and never treats the two adjacent methods as a single body.

## Exact behavior supported by x86 instruction evidence

1. **`0x24197D50`, 84 bytes, signed-short array comparison.** If original `BYTE(this+0x14)==0`, returns **0** without inspecting descriptor or buffers. Otherwise `BYTE(descriptor[this+0x18]+0x13) & 0x04` forces result **1** without buffer access. Otherwise, when native 32-bit **signed** element count is `<=0`, returns **1**. For positive count, compares exactly that many 16-bit little-endian WORD values in a forward loop, returning **0** on the first mismatch and **1** if all match. The test is bitwise equality, not a floating-point epsilon, and is independent of signed numeric interpretation.

2. **`0x241989E0`, 48 bytes, short-array forward copy.** Returns without accessing data when `BYTE(this+0x2C)==0` or the first native DWORD argument (comparison status) equals **1**. Otherwise the last original DWORD argument is an **unsigned WORD count**, multiplied by two in original **32-bit x86 arithmetic**, then copied by `REP MOVSD` over complete four-byte chunks and `REP MOVSB` for any remaining tail. Forward overlapping-copy semantics are NOT `memmove`; a complete DWORD source operand is read before its write on each `REP MOVSD` iteration. Portable C++17 model explicitly refuses `2 * count` overflow and invalid host vectors instead of silently reproducing native wrap/faults.

3. **`0x24197B10`, 79 bytes, signed-short diagnostic formatting.** When original `this+0x14` is nonzero, status **0** invokes original `0x24197910` with authentic `short differs (net %i pred %i) diff(%i)\n`, sign-extending the **first** 16-bit source/net and destination/predicted WORDs to 32-bit, deriving **predicted minus net**. In all enabled statuses the original invokes `0x241979C0` using `short (%i)\n` and the first predicted signed short. If disabled, no format callbacks occur. The native formatters are call targets only, not recovered implementation bodies.

4. **`0x24198380`, 33 bytes, signed-short preview.** Original `DWORD(this+0x34)==DWORD(this+0x18)` enables a call to original formatter `0x24197FE0` with `short (%i)` and the first sign-extended predicted WORD. The preview does NOT depend on `this+0x14`. This is pointer-token identity, not a recovered original runtime object or vtable.

**Original Phase212 type-7 serializer direct call order:** compare `0x24197D50` → word-count copy `0x241989E0` → diagnostics `0x24197B10` → preview `0x24198380`. Hence, the diagnostic and preview run **after** the native copy, and can report already-updated destination values, even when the earlier comparison identified a difference. This order is preserved in the separately authored C++17 host reference. No complete original record object, prediction engine, memory ownership, save/replay protocol or game lifecycle is claimed.

## Verification/implementation

- `research/tools/audit_sin_original_client_short_specialists.py`: read-only owner-only archive or DLL pair; complete original dual-DLL fingerprints, four original entire-function SHA256s, **50 sparse x86 instruction anchors**, four exact Phase212 router CALL edges, three nested formatter CALLs, exact three original `.rdata` short format strings, native `RET` arities, all original `INT3` pads, and the adjacent no-padding method boundary.
- `research/tests/test_sin_original_client_short_specialists.py`: synthetic-only mutation groups for all opcode anchors, all seven exact callsites, each RET arity/padding, no-padding neighbor witness, format literals, and original PE32 image base. Synthetic fixtures **do not count** as native original SHA-verified functions.
- `include/sin_compat/original_client_snapshot_short.hpp`: independent portable C++17 exact word comparison, signed short decoding, `REP MOVSD`/tail forward-copy reuse, host-only overflow/buffer refusal and callback-based trace/preview. Original 2006 Source engine types are NOT assumed.
- `tests/test_original_client_snapshot_short.cpp`: 16-bit equality, signed count <=0, descriptor/enable gates, signed -32768/+32767 and difference, early comparison mismatch without reading unused tail, original forward overlapping DWORD copy, no-copy status, post-copy logging, preview gate, and host negative-count refusal.
- Portable root Linux CTest and real native MSVC Win32 x86 CTest for independent reference. Windows builds are **not** running actual reconstructed retail game DLLs.

Reproduce the private original DLL audit without committing game binaries:

```sh
python3 research/tools/audit_sin_original_client_short_specialists.py --archive /owner-local/bin.zip
python3 -m unittest discover -s research/tests -p 'test_sin_original_client_short_specialists.py' -v
```

## Status and next dependency

Original verified-method inventory **253 → 257** (**128 server / 129 client**). Phase214's scoped original 36-target client snapshot call graph now has **17** unreviewed unique destinations (**21 → 17**), not a total-game decompilation percentage. All four direct native helper calls in type-7's original case now have complete per-method SHA review. The next useful original targets are the still-opaque type-3/type-4/type-5/type-6 specialist methods and their real data layouts, then the full Windows Source engine integration boundary. Windows PC playable modules remain the first priority; Android ARM64 and Quest 3 follow.
