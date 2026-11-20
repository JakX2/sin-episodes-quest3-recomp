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
global value object per stat ID.

### Original dynamic named object creation independently recovered

The original SHA-authenticated `0x241FFCD0` helper builds a 256-byte
lookup key using **`snprintf("%s%i", dynamicName, statId)`**, then
searches a keyed tree. If a match exists, it returns the wrapped
value object. If the name is missing, it returns null when its
`create` flag is false; with `create` true, it invokes original
`0x24206430` to construct a new type-specific value object and
insert it into the owner's tree.

The original `DynamicStat` callback passes `create=1`. For
stat IDs **55..65**, the factory additionally constructs a separate
**272-byte (0x110) dynamic wrapper** via `0x24206280`, forwarding
the value update through its embedded 16-type factory object.

The new `client_stats_owned_dynamic_values.h` implements an
independent conservative named store with:

- Exact original key composition **name followed by 2-digit stat ID**
  without separators, e.g. `pistol55`, and distinct per-name values;
- Create-on-miss, reuse-on-hit, and optional lookup without creation;
- The same SHA-recovered ID→factoryType map and owned mutator methods;
- Safe refusal of names longer than **253 bytes**, instead of silent
  truncation/collision in the original 256-byte formatting buffer;
- A conservative **4096-record cap**, atomic new-record publication
  and no original allocator or engine-object pointers.

The exact **original tree comparator and memory-allocation failure
semantics remain unverified**. Our owned map currently uses byte-exact,
case-sensitive lookup and does NOT claim perfect matching behavior
for unusual names, case variants, truncated strings or malformed
original memory. Those differences are deliberate boundaries
rather than invented original engine behavior.

The new native synthetic dynamic fixture covers all 11 dynamic IDs,
unaligned message parsing into named records, existing-value reuse,
missing/create-disabled behavior, empty names, 253/254-byte limits,
malformed frames, atomic failure and capacity exhaustion.
A new read-only auditor SHA-pins the exact original formatter,
dynamic factory, wrapper and name lookup call edges.

## ResetHUD original destruction/rebuild sequence

The original SHA-matched callback at **0x241FCA60** does not read
the one-byte message payload. Instead, the original shared message
catalog declares a one-byte ResetHUD envelope, and the callback
immediately invokes these five original methods in this order:

| Original callback action | Function VA | Evidence |
|---|---|---|
| Tear down previously allocated static values | 0x241FBEC0 | Original 55-value owner cleanup |
| Tear down separate secondary stats collection | 0x241FC740 | **Purpose not fully reconstructed** |
| Clear dynamic names / value tree | 0x241FF990 | Clears the tree/container |
| Rebuild static stat value objects | 0x241FF420 | Loops **55** entries |
| Rebuild dynamic-type buckets | 0x241FF8E0 | Loops **11** entries |

Finally, the callback writes `1` to the original owner reset
flag at `0x24378E94`. The related class instance method at
`0x241FC9E0` invokes the identical five functions and writes
the flag at instance offset `+0xAC`. The original per-method
body SHA256 fingerprints and every call edge were authenticated
in a **private read-only audit** of the original DLL.

New `audit_sin_original_stats_reset.py` verifies seven original
method-body fingerprints, all ten callback/instance call edges,
both 55/11 loop constants and both original reset flag writes.
Its public CI self-test is synthetic, with no retail binary.

The independently authored `client_stats_owned_message_runtime.h`
now combines **55 owned static values**, **independently named
dynamic values**, and the original three stat message IDs:
`StaticStat=27`, `DynamicStat=28`, `ResetHUD=5`. It accepts
a bounded logical bit-cursor copy, decodes and applies static or
dynamic packets, and on ResetHUD recreates all 55 static value
objects, drops dynamically named records, increments its own reset
counter and sets its separate independent reset flag. Invalid or
truncated packets leave both state and the caller cursor unchanged.
Importantly, the original callback does NOT interpret the one-byte
envelope, so the port treats it as opaque too.

A new native original-free regression exercises a **realistic owned
sequence**: update static ID2, create/reuse dynamic ID55 by name,
create dynamic ID65, receive ResetHUD, verify all dynamic names are
gone and all 55 static objects restored to factory defaults, then
send valid static/dynamic messages again. It also checks repeated
resets, invalid envelope size, unknown IDs, incomplete values and
overlong name refusal.

**Important evidence limit:** The secondary stats container
destroyed by `0x241FC740` remains unmodeled, as does the original
dynamic tree comparator, the engine's actual user-message
dispatcher, and normal `VClient011::Init`. This is a source-grounded
independent C++ value-state lifecycle **simulation**, not a
real original-engine game packet test or playable Windows boot.

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

Now that the original named lookup key and creation branch are
recovered, the remaining targets are the **original tree comparator**
and exact dynamic-wrapper ownership, copying and reset behavior.
Then connect the independent stat-value state to a separately
verified BFRead ABI bridge and the real engine's networking/HUD
lifecycle, without enabling slot33 prematurely.
