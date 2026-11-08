# SiN Episodes: Emergence (2006): Original parent-aware velocity cache

Phase 107, November 8, 2026. **Original Windows 32-bit server.dll reverse
engineering + independently authored C++17 reference implementation.**
Game-owned original SHA-256:
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
This repository contains neither the original binary nor disassembly dumps.

## Exact original function witnesses

| Function/behavior | x86 virtual address | Original function bytes |
| --- | --- | ---: |
| Set absolute velocity (functional identification) | `0x22141A00..0x22141B32` | 307 |
| Refresh cached absolute velocity | `0x2213D750..0x2213D85A` | 267 |
| World vector to parent local (transpose) | `0x22268E90..0x22268EE2` | 83 |
| Parent local vector to world (matrix multiply) | `0x22268C80..0x22268CD2` | 83 |
| Local velocity field setter | `0x2213E1A0..0x2213E1FB` | 92 |
| Velocity-dependency invalidation | `0x2214B620..0x2214B724` | 261 |

The SHA-256-pinned verifier checks all six complete function byte ranges,
ten original relative call edges and fifteen instruction witnesses. The
identification of C++-style function roles is inferred from their compiled
behavior, not original Ritual symbol exports.

## Original SetAbsVelocity-style function `0x22141A00`

- Compare three incoming world velocity floats with cached fields
  `entity+0x250..0x258`. If equal, **return without writing**.
- Call dependency invalidator `0x2214B620` with mask **4**.
- Clear bit `0x1000` in entity flag storage at `+0xB8`; store the three
  new absolute velocity floats at `+0x250..0x258`.
- Resolve parent handle from `entity+0xF0`, using the original entity
  handle table. If absent/stale, call local vector setter `0x2213E1A0`
  on `entity+0x2C8` with the world velocity unchanged.
- Otherwise refresh the parent's dirty absolute velocity using
  `0x2213D750` and its dirty world matrix using `0x2213FA60` if needed.
  Subtract the parent's world velocity from the new world velocity.
- Call `0x22268E90` with that delta and the parent's row-major
  `matrix3x4_t` at `+0x268`; this is a **rotation transpose**, not a
  full affine inversion and does not use translation. Pass the resulting
  local velocity to `0x2213E1A0`. That local setter may invoke the
  entity's virtual byte slot `+0x27C` when the vector changes.

Reconstructed equation, for an orthonormal parent basis `R`:

```text
child.localVelocity = transpose(parent.R) *
                      (desiredChild.worldVelocity - parent.worldVelocity)
```

## Original CalcAbsoluteVelocity-style function `0x2213D750`

- Check bit `0x1000` at `+0xB8`. If clean, no recomputation.
- Clear the dirty bit; follow parent `+0xF0`.
- If no valid parent, copy local velocity at `+0x2C8` to cached
  absolute velocity at `+0x250`.
- With a valid parent, refresh its matrix when bit `0x800` is set,
  rotate the local velocity by parent's matrix at `+0x268` with
  `0x22268C80`, refresh parent's absolute velocity if bit `0x1000`
  is set, and add the parent's world velocity. This is translation-free
  vector rotation, **not** a point transform.

Reconstructed equation:

```text
child.worldVelocity = parent.R * child.localVelocity +
                      parent.worldVelocity
```

## New portable implementation and integration

- `include/sin_compat/original_absolute_velocity.hpp`,
  `src/original_absolute_velocity.cpp`: host-owned entity state snapshots,
  optional parents, row-major matrices, pure helper transforms, source-like
  update/refresh and explicit dirty-cache handling.
- `include/sin_compat/original_absolute_impulse_adapter.hpp`: maps the
  existing Phase 105 `IOriginalImpulseHost` to the new
  `IOriginalAbsoluteVelocityHost`, so the previously recovered Jessica
  throw and rifle/grenade-related impulse path can use parent-relative
  absolute-velocity updates without altering its animation contract.
- Tests cover unparented/no-op, rotated parent, moving parent, nested
  parent chain, matrix translation ignored, transpose rather than
  generalized inverse, recursive dirty refresh, invalid/NaN matrices,
  missing parent, self-parent, cyclic refresh, depth limit, host failures,
  additive impulse bridge and distinct physics type-6 dispatch.
- `research/tools/audit_sin_original_absolute_velocity.py`: owner-side
  SHA-pinned private binary verifier. Synthetic PE32 regression tests use
  invented MZ/PE headers only.

Local checks:

```sh
python3 research/tools/audit_sin_original_absolute_velocity.py \
    --server /private/original/server.dll
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Implementation differences and unresolved ABI

The original clears dirty flags **before** recursively refreshing, while
the portable safe-host model checks finite inputs, caps parent depth at 64,
detects cycles and clears a dirty flag only after a valid cached vector
is calculated. Local SetAbsVelocity invalidation/storage sequence follows
the normal original call order. Host callbacks can fail midway, so the
future real native adapter must make writes exception-safe/transactional.
The original may use x87 extended precision, whereas the portable helper
uses double intermediates checked before float conversion.

We **do not** claim identical behavior for malformed entity hierarchies,
missing/failed native pointers or non-orthonormal matrices. Native parent
EHANDLE management, dirty world-transform recomputation, CBaseEntity
virtual `+0x27C`, physics vtable `+0xB0`, and dependency invalidation
must be implemented against verified SourceVR interfaces. The portable
`IOriginalAbsoluteVelocityHost` abstracts these effects; it cannot
execute them on an actual game object by itself.

This does not compile a complete `client.dll` or `server.dll`
replacement, load original maps, or produce a playable Quest 3 APK.
No proprietary original SiN DLLs, engine binaries, maps or models included.
