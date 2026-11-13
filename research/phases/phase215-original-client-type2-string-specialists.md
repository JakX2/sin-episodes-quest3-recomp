# Phase 215: original SiN client.dll type-2 snapshot string specialist chain

14 November 2026. **Stacked draft on Phase214 PR #268.** Three previously unreviewed native x86 2006 SiN Episodes: Emergence `client.dll` functions were examined against the user-provided unmodified original `bin.zip`. Both full original DLL fingerprints match the existing owner-specific provenance:

- `server.dll`: SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

No original DLL, game content, complete proprietary machine-code body or raw disassembly is committed.

## Three new complete SHA256-verified native methods

| Original `client.dll` VA | Complete native bytes | Full original method SHA256 | Router CALL |
| --- | ---: | --- | --- |
| `0x24198B80` | **83** | `94b49be4773d3a2acc1dee70c21e4551b0f9891843d9c98942da89d4dd57b268` | `0x24199883` |
| `0x24197D00` | **65** | `b4673143787130ff0988b900b3427626a3d9fdcda8d265e5385ae93cb7274986` | `0x24199899` |
| `0x241984A0` | **30** | `ac12b87f0269180714911a5f6c8b0c28f41ba2eb4e6d1f33c2774a68223dfe23` | `0x241998A3` |

**Three new complete function entrypoints, 178 original x86 instruction bytes.** The original machine code is followed by, respectively, **13, 15 and 2** `INT3` padding bytes. All three direct Phase212 router calls resolve to these exact starts. The two diagnostic functions additionally call their verified original formatter targets `0x24197910`, `0x241979C0` and `0x24197FE0` (those targets are NOT newly claimed recovered).

## Original behavior directly observed

1. **`0x24198B80`: string equality/status (original `ret 8`).** If byte at original `this+0x14` is zero, returns 0 without reading strings. Otherwise, if descriptor pointer at `this+0x18` has byte bit `0x04` set at descriptor offset `+0x13`, returns 1 without reading strings. Otherwise compares two NUL-terminated strings **byte for byte**, returning **1 if equal** and **0 if different**. Exact original comparison uses unrolled two-byte increments; case distinctions remain significant. This is a comparison status, not proof of which high-level source class originally owned the method.

2. **`0x24197D00`: conditional string diagnostics (original `ret 12`).** If `this+0x14` is zero, no formatting callbacks fire. Otherwise, if operation/status argument equals zero, first calls original `0x24197910` with `string differs (net %s pred %s)\n`, feeding the source/net pointer and destination/prediction pointer. Then, in all enabled cases, calls `0x241979C0` with `string (%s)\n`, status and destination/prediction pointer. Actual backend and its side effects are outside this phase's complete-function evidence.

3. **`0x241984A0`: conditional preview (original `ret 12`).** Only when original object's 32-bit value at `+0x34` equals its value at `+0x18`, calls `0x24197FE0` with `string (%s)` and the second stack pointer argument. Unlike the diagnostic helper, this branch does not test `this+0x14`.

The original Phase212 type-2 switch case calls four native routines **in order**: equality/status `0x24198B80`, Phase213 NUL-inclusive copy `0x24198B40`, diagnostics `0x24197D00`, and preview `0x241984A0`. Thus we now have original function-level evidence for **all four direct type-2 callees**. The byte-copy helper itself is still separately gated by `this+0x2C` and whether the status is 1; the new reference keeps this gate distinct from the comparison and diagnostic byte at `this+0x14`.

Notably, the two original diagnostic calls happen **after** the copy helper. Any trace reading destination bytes therefore sees whatever the copy has already written. We preserve that order in the portable reference rather than rearranging calls for aesthetic logging.

## Implementation and verification

- `include/sin_compat/original_client_snapshot_type2.hpp`: original-independent C++17 host reference for comparison, ordered trace events and preview, plus **optional direct-four-CALL sequence composition** using Phase213's exact forward-copy model. Explicit bounds/NUL failures are host-only protections; native x86 has unbounded reads.
- `tests/test_original_client_snapshot_type2.cpp`: equality, case sensitivity, early-branch pointer bypass, truncated strings, ordered traces, status handling, preview state gate and post-copy diagnostic sequence.
- `research/tools/audit_sin_original_client_type2_specialists.py`: read-only full dual-DLL SHA256 gate, 3 whole original function hashes, 23 independent sparse x86 witnesses, precise native `RET`/padding boundaries, all 3 original router calls, 3 nested original formatter calls and 3 original `.rdata` format strings.
- `research/tests/test_sin_original_client_type2_specialists.py`: original-free synthetic x86 mutation/regression for every witness, each caller/formatter `CALL`, all return arities, each padding byte, literal and dual-image provenance.
- Root Linux/GCC/Clang and native Win32 MSVC x86 test registrations exercise the clean-room implementation. Neither harness substitutes for the original game engine.

### Reproduce original verification privately

```sh
python3 research/tools/audit_sin_original_client_type2_specialists.py --server /path/to/server.dll --client /path/to/client.dll
python3 -m unittest discover -s research/tests -p 'test_sin_original_client_type2_specialists.py' -v
```

The original verifier only claims authentic native hashes when both exact original 2006 DLL files are supplied. Tests using synthetic data must report **zero original verified methods**.

## Status and next steps

The method atlas advances **243 → 246** (**server 128; client 115 → 118**). This is a function-count inventory, not a percentage of a working port. The next P0 path is the original type-1 float specialist chain `0x24198AA0`, `0x24197CA0`, `0x24198470` with special attention to authentic x87 compare/threshold semantics. The rest of the snapshot record layout, external formatter ownership, source engine initialization, and Windows replacement-module execution are **not** recovered by this phase.

**Priority remains: genuine Windows PC runtime first → Android ARM64 → Quest 3.**
