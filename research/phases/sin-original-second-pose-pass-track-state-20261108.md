# SiN Episodes: Emergence (2006) original pose driver second-pass metadata (Phase 118)

## Scope and provenance

This phase is an **independently authored C++17 behavior model** derived from
one unchanged owner-supplied original x86 PE32 `server.dll`, SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
The executable and its original disassembly are private and **not included**.
All addresses are original virtual addresses with image base `0x22000000`.

Phase 116 reconstructed initialization and the command pass of `0x22103A60`;
Phase 117 reconstructed its recursive world-bone-matrix helper `0x22100F30`.
This phase reconstructs precisely the **active-track gate, cap marker and
sequence-history invalidation** of the second loop, with the geometry
calculation still delegated to a host callback.

## Verified execution order

At `0x22103D70`, the original code iterates track structures with a stride of
`0xE4` (228 bytes). It reads `float` track influence at `+0x5C`, compares it
against the `double` constant `0.0` at `0x223D8F98`, and jumps directly to
the next track for zero, negative, or unordered (NaN) influences. Positive
influence proceeds to transform computation.

The transform path calls, in order:

- `0x22100F30`: recursive world-space bone matrix computation (Phase 117);
- `0x2226D250`: quaternion/position-to-matrix transformation;
- `0x22268F10`: an additional matrix transform operation;
- `0x22269520`: matrix concatenation.

Those operations need studio-header-specific bone selection and native pose
buffers. **This phase does not substitute invented results for them.** Its
`IOriginalPoseSecondPassGeometry` callback runs before updating track flags.
The caller must eventually bridge that callback to verified model data and the
Phase 117 matrix evaluator.

The original `0x22103E15..0x22103E39` compares `float` track cap `+0x44`
against `double` constant **1.0** at `0x223D22B8` using `FUCOMPP`, parity
and `test ah,0x44`. It sets byte `+0xA0` to `1` **on equality** (else `0`).
The unordered case sets `0`.

At `0x22103EEA..0x22103F1B`, the original checks sequence index supplied at
frame `+0x70` against track's stored sequence at `+0xA8`.

- If current sequence is negative, it clears byte `+0xA1` and the DWORD
  accumulator `+0xA4`.
- If prior sequence is **less than `current - 1`** or **greater than `current`**,
  it clears those fields as well.
- Otherwise, the history flag and opaque accumulator are preserved.
- It always stores current sequence into `+0xA8` on this active-track path.

The semantic meaning of `+0xA1`/`+0xA4` inside the **later** animation-effect
calculations is not established here. We deliberately preserve the accumulator
as opaque bits instead of claiming it is a known timer or position.

## Verification

Run `research/tools/audit_sin_original_pose_second_pass_state.py --server
/path/to/your/private/server.dll` to verify the owner-only original image.
The read-only audit passes:

| Witness | Count |
| --- | ---: |
| Full SHA-256 code-region hashes | 5 |
| Direct relative call targets | 5 |
| Exact opcode anchors | 21 |
| Double-precision constant witnesses | 2 |

`research/tests/test_audit_sin_original_pose_second_pass_state.py` separately
rejects corrupted fabricated PE32 images and incorrect call opcodes, without
embedding copyrighted executable content.

The C++ tests include positive/zero/negative/NaN influence, exact cap markers,
sequence-history carry across equal or next sequence, history resets on skipped
indexes and negative sequences, track bytes parsing, invalid bounds, and a
3-stage integration of the first pose pass, recursive bone matrix helper and
second-pass metadata callbacks. CTest also runs all inherited phases.

## Current limits

This stage does **not** reconstruct the remainder of the original second-loop
state from `0x22103F1C` to `0x221048A6`, pose-driver effect physics, native
model memory layout, `SetupBones` scheduling, host engine ABI, full playable
`server.dll` / `client.dll`, or Quest 3 standalone rendering. Portable code
adds finite guards and returns structured errors instead of reproducing
undefined behavior from the native game.
