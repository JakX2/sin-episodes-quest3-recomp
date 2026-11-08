# Phase 116: SiN Episodes 2006 pose-driver reset and first command loop

Original specimen: private owner-provided `server.dll` PE32 SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
All addresses use the unchanged original preferred image base `0x22000000`.

## What the original 0x22103A60 function actually contains

The original function runs from **`0x22103A60` to `0x221048D4`**.
It is much larger than one ordinary interpolation loop. The independent
Phase 116 reconstruction intentionally stops before the **second** pose-
processing loop at `0x22103D70`; that later loop performs additional
animation-channel and matrix operations and remains future work.

### Track reset

`this+0x00` holds a pointer to track records; `this+0x0C` contains the
track count. The first loop walks track records at **0xE4 (228) bytes**
per record. In each, the original clears `+0x0C..0x14` (position),
`+0x18..0x24` (quaternion), `+0x4C..0x54` (three values) and `+0x5C`
(influence). It sets `+0x44` and `+0x48` to float 1.0. It **does not**
clear track selector, type, optional payload, or type-3-only trailing
metadata; the portable API accepts a prior track snapshot and preserves
these fields.

### Grouped commands

The original owner has a group array at `this+0x18`, with group count
at `this+0x24`. Each group record advances by **0x14 (20) bytes**, with
command count at group `+0x0C`. Each command record advances by
**0x7C (124) bytes**. Command `+0x04` is the type and command `+0x10`
is the affected track index. The four-entry x86 jump table at
`0x221048D8` routes as follows:

| Type | Original case address | First-pass action |
|---|---|---|
| 3 | `0x22103B22` | Set pose/parameters, update type-3 metadata |
| 4 | `0x22103E87` | Cap track `+0x44`, attenuate influence `+0x5C` |
| 5 | `0x22103B22` | Set pose/parameters, copy optional payload `+0x60` |
| 6 | `0x22103E3A` | Cap track `+0x44` without attenuating influence |

Unknown command types skip the first-pass action. Types 3 and 5 copy
origin from command `+0x20`, a four-component quaternion from `+0x2C`,
selector from `+0x08`, and the three parameter fields in the special
order `+0x14`, `+0x1C`, `+0x18`. Direct pose assignment occurs when
weight `+0x58` equals exactly float 1.0, or when the existing track's
influence is zero. Otherwise, the original calls `0x2226D210` to mix
quaternions, blends positions and the other parameters by the same
weight, and uses the smaller current/new cap. The quaternion operation
is represented by a host callback rather than a guessed native formula.

Types 4/6 use command `+0x5C` as an enable/disable condition for a
track cap. When zero, they clear the cap; otherwise, they clamp it against
`1 - command[+0x54]`. Type 4 additionally multiplies influence by
`1 - command[+0x58] * command[+0x54]`.

## Binary verification

`research/tools/audit_sin_original_pose_driver_first_pass.py` checks
**4 complete SHA-256 code-region witnesses** (including the entire
3,701-byte function and `0x2226D210`), **16 original relative call
targets**, **23 instruction anchors**, **4 switch targets** and the
numeric 0.0/1.0 constants. Verified on the owner's private original
binary. The helper is read-only and refuses other DLL hashes. The
synthetic verifier uses fabricated PE32 test bytes only.

## New native-host-neutral implementation

- `include/sin_compat/original_pose_driver_first_pass.hpp`
- `src/original_pose_driver_first_pass.cpp`
- Tests for reset preservation, 3/4/5/6 dispatch, group ordering,
  direct assignment, mixer callback, parameter blending, malformed
  indices, nonfinite values, overflow guard and the byte-record parser.

The implementation works with immutable input snapshots and returns a
new state, not pointers into x86 engine objects. It bounds track count,
command count and raw record reads and rejects nonfinite float results.
These are intentional portability/safety additions; the original game
may exhibit different behavior for malformed/corrupted memory or NaNs.

**Not complete:** The second pass, precise x87 rounding, actual animation
sequence sampling, real bone/pose allocation, the original client module,
full campaign execution, and Meta Quest 3 rendering/tracking are beyond
this phase. This artifact does not make the game playable.
