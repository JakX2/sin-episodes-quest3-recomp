# SiN Episodes: Emergence (2006): original parented world-transform refresh

**Research phase 108; 2026-11-08.** Direct inspection of the original 2006
Windows x86 `server.dll` (SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`),
plus independently authored host-neutral C++17. The original binary, executable
code, raw disassembly and copyrighted game assets are **not** in this repository.

## Original binary evidence

The original routine `0x2213FA60..0x2213FBFB` is 412 bytes long. It performs
lazy world-space position, angles, and matrix reconstruction, and is reached
by the Phase 107 velocity-refresh functions when transform dirty bit 11 is set.
The SHA-pinned verifier checks eight complete original function digests,
twelve original relative CALL destinations and fourteen instruction witnesses.

| Observed location | Direct evidence | Intended portable meaning |
| --- | --- | --- |
| `0x2213FA66..0x2213FA7A` | Flags `entity+0xB8`, bit `0x800`; clear on entering dirty path | Skip work when clean; compute only after dirty |
| `0x2213FA7F`, `0x2213FA88` | Local origin `+0x360`, local angles `+0x36C` | Build a local 3x4 transform |
| `0x2213FAB6`, `0x2213FACF` | Cached `matrix3x4_t` at `+0x268`, call `0x2226CC20` | Original helper builds matrix from Euler angles and local origin |
| `0x2213FAD4..0x2213FB07` | Parent handle `+0xF0`, index mask `0xFFF`, generation shift `12` | Resolve parent, including serial mismatch / stale handles |
| `0x2213FB09..0x2213FB56` | Parent absent/stale: copy local origin to `+0x2BC`, local angles to `+0x354` | Direct world cache when unparented |
| `0x2213FB68..0x2213FB8D` | Call `0x2213D960`, multiply matrices `0x22269520`, copy result `0x22268EF0`, extract translation column `0x22268FD0` | Parent/attachment transform concatenated with local transform |
| `0x2213FB90..0x2213FBCE` | Compare local angles to shared vector at `0x225885C4`, test attachment selector `+0xEC`, may obtain parent's angles directly | Identity local angles + ordinary parent shortcut; no attachment shortcut |
| `0x2213FBD6` | Call `0x22268A60` (matrix-to-Euler) | Recover world angles when shortcut not used |
| `0x2213FBDE..0x2213FBF4` | Pass argument `4` to `0x2221DFA0`; invoke `0x22221DB0` if it reports true | Engine-dependent post-rebuild notification not inferred as automatic |

The original `0x2213D960` chooses either parent world matrix or an
attachment-based transform depending on the child's `+0xEC` selector, parent
model/virtual query, and attachment-resolution success. On attachment lookup
failure it may fall back to ordinary parent world matrix. The original
attachment lookup's *full* ABI and complete parent matrix getter are not yet
reconstructed. Therefore an engine host supplies the optional attachment
matrix rather than this portable module inventing attachment/bone data.

## Reconstructed normal-path equations

With row-major Source `matrix3x4_t`, degrees-based Euler angles and local
origin/angles `(p, a)`, build `L = AngleMatrix(a, p)`. If no parent,
`entity.worldMatrix = L`, `entity.worldOrigin = p`, and
`entity.worldAngles = a`.

With resolved parent and selected parent matrix `P`:

```
W = P * L
worldOrigin = (W[0][3], W[1][3], W[2][3])
worldAngles = (local angles are all zero and attachment selector is zero)
             ? parent.worldAngles : MatrixAngles(W)
```

The multiplication includes the parent's rotation and translation. This is
**different** from the velocity-vector transforms in Phase 107, which must
ignore matrix translation. Standard normal finite input, orthonormal
orientations and Source-style degrees are the target, not reproducing every
x87 rounding edge case.

## Implementation and tests

- `include/sin_compat/original_world_transform.hpp` plus
  `src/original_world_transform.cpp`: validated 3x4 AngleMatrix,
  affine concatenation, MatrixAngles, parent-aware lazy refresh,
  no-parent copies, attachment override, dirty-cache commit and
  guarded recursion.
- `include/sin_compat/original_world_transform_velocity_bridge.hpp`:
  explicitly connects Phase 107's `IOriginalAbsoluteVelocityHost` world
  refresh callback to this separate host-managed world graph.
- `tests/test_original_world_transform.cpp`: yaw/pitch/roll matrix roundtrips,
  parent translation and rotation, multiple levels, unchanged caches,
  zero-angle shortcut, attachment override/fallback, NaN, missing parents,
  cycles and maximum recursion depth.
- `tests/test_original_world_transform_velocity_bridge.cpp`: proves the
  previous absolute velocity and additive-impulse implementations now use
  a freshly rebuilt parent matrix (rather than a stale orientation).
- `research/tools/audit_sin_original_world_transform.py`: **read-only**
  original SHA/function/call/instruction verifier. Companion
  `research/tests/test_audit_sin_original_world_transform.py` verifies its
  PE32 rejection behavior using invented, copyright-free binary headers.

Check on user's legally obtained original DLL:

```
python3 research/tools/audit_sin_original_world_transform.py \
  --server /private/original/server.dll
```

Build/test the repository:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Remaining boundaries

This C++ is **not** decompiled original game source, an x86 executable
recompiler, a native SourceVR entity, or a complete Qt/Quest port. The real
2006 DLL clears dirty bit *before* doing subsequent work. The portable model
validates the new matrix and defers dirty-bit clearing until host commit;
this difference deliberately prevents corrupted cache state after failed
host operations. Host callback commits are not transactional by themselves.
Parent handle generation tracking, attachment bone query, native float-angle
rounding, and the conditional `0x2221DFA0` / `0x22221DB0` callback side effects
need an actual compatible engine host before faithful in-game operation.

The library does not claim player collision-center, Jessica animation,
ballistic projectile, client HUD or Quest 3 startup have run together in a
native 3D engine. That requires a verified SourceVR environment and its game
modules. No original binary was committed or bundled.
