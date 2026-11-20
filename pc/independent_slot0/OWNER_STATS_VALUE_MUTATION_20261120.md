# Phase 476: original client Stats *value mutation* semantics

## Why another piece is necessary

Phase 475 recovered the SHA-pinned 66 original stat names, 66×12 byte
type/metadata registry, 16-case value factory, nine CStatReader virtual
methods and their raw 32-bit wire formats. Its decoder can read
**correct stat IDs and numeric fields**. It did not yet model how
those decoded values change a stat's owned value object.

This phase inspects the exact original 2006 retail client.dll (SHA256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`)
**read-only**. The private audit finds all 16 type-specific value
object vtables, their **+0x18 primary setter**, **+0x2c secondary setter**
and their **+0x48 dirty/change marker**. Every primary, secondary
and dirty method pointer matched the original. All **nine**
CStatReader virtual methods' dispatch calls were verified too.

## Recovered original value-update behaviors

| Factory type | Original reader behavior | Independently modeled mutation |
|---|---|---|
| 0 | 1 signed 32-bit wire integer, cast via x87 into float then +0x08→+0x18 | Integer setter, float32 rounding and x87 truncation |
| 1 | 1 raw 32-bit float via +0x08→+0x18 | Write float to primary field |
| 2 | 2 32-bit integers, calls +0x18 then +0x2c | Write *both* integer fields |
| 3 | 2 32-bit floats, calls +0x18 then +0x2c | Write *both* float fields |
| 4, 5, 6 | One float via +0x18 | Write primary float, leave other original fields unchanged |
| 7 | One float, *direct* field +0x0c write after dirty mark | Write primary float; ordinary +0x18 is **NO-OP** |
| 8, 9, 10, 11 | One float to +0x18 (the constructor-created field +0x14) | Write auxiliary float, preserve -1.0f sentinel at +0x10 and configured literal at +0x18 |
| 12, 14 | One float through +0x08→+0x18 | Conditional **strict maximum**, initialized at -FLT_MAX |
| 13, 15 | One float through +0x08→+0x18 | Conditional **strict minimum**, initialized at +FLT_MAX |

The original dirty marker simply sets the object's byte +0x08 to one.
The max/min implementations set the marker only if the new float
strictly beats the current field. A tie or NaN does not advance
either value. The direct type-7 path calls the marker before writing
the field.

Original integer setters explicitly set the x87 FPU control word to
**round toward zero** for `FISTP`. They first receive a **float32**
value from the CStatReader's signed-integer-to-float conversion. It is
not always equivalent to directly storing the original 32-bit integer:
values above the 24-bit exact float mantissa range may round, and
`0x7fffffff` may become the invalid-integer `0x80000000` sentinel
after rounding. Independent `OriginalIntStoreBits` models that
two-stage conversion, including the x87 integer-indefinite result.

## Independently authored implementation

New `client_stats_owned_value_mutation.h` provides:

- **16 explicit type contracts** identifying original setter VA and
  secondary method, verified in `audit_sin_original_stat_value_mutators.py`;
- `OwnedValue`, storing original field offsets +0x0c, +0x10, +0x14,
  +0x18 and +0x1c as **independent raw32 metadata**, not original
  game classes or ABI pointers;
- `OwnedValue::Apply` guarded by matching factory type, original
  reader field kind and exact field count; state updates are atomic;
- original constructor sentinel initialization, dirty marks, strict
  extrema and type-7 direct-write exception;
- `OwnedStaticValues`, statically allocating and initializing all
  **55 original static** types from the SHA-authenticated compiled
  66-stat registry and applying decoded `StaticStat` frames.

The **11 dynamic IDs 55..65** include a name string and a separate
original name-keyed object hierarchy. They are NOT faked as one
global value object per stat ID. A caller can apply a decoded dynamic
frame to an explicitly supplied independent `OwnedValue`, but the
original collection creation/reuse and lifecycle still require
further analysis.

## Validation and exact boundaries

The owner-original read-only audit locally verified:

- 16/16 primary, secondary and dirty vtable pointers;
- 9/9 reader mutation call routes;
- distinct integer and float setter method SHA-256 fingerprints;
- special direct-write type 7, max/min x87 comparison codes;
- initializers for configured/scaled and extrema value objects.

The new C++ native Windows fixture covers **all 16** value types,
all 55 statically owned records, pair writes, strict max/min,
NaN/ties, dirty bit behavior, x87 narrowing, unknown types, wrong
field kinds/counts and transactional rollback. Public CI runs only
original-free fixtures and never embeds/distributes original DLL
code or data bytes. The private original DLL is never forwarded,
patched, injected or committed.

**Not yet complete:** original dynamic name-key lookup, engine BFRead
24-byte binary ABI bridge, live user-message dispatch, game HUD side
effects, original normal `VClient011::Init`, server gameplay entities,
menu/map load, Android and Quest 3. The highest actual original
owner-Windows terminal milestone is still **0x53494E74**, not a
playable boot or actual message delivery.

## Next target

Reconstruct SHA-authenticated dynamic stat name-keyed record lookup
around **0x241FFCD0** and the mutation lifecycle for its contained
value object. Only then introduce a bounded independent dynamic
collection, without borrowing original engine memory or guessing
whether absent names cause creation, reuse or error.
