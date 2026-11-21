# Phase 537: original `CNPC_MobGrunt` virtual `+0x844` combat fallback decompilation

Date: 2026-11-21. **Decompilation-first phase.** This reconstructs the original
function's control flow for inspection and future source recovery. It does not
implement a SourceVR adapter, NPC runtime, physics, surfaces, or model loading.

## Binary and method identity (exact original owner build)

- Original `server.dll` SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll` SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- The primary original Grunt vtable at `0x2249AAA4` stores `0x2235D2C0`
  at byte offset **`+0x844`**. The original branch in the 991-byte main
  schedule selector `0x2235E4E0` calls this fallback if the `+0x848`
  sidestep gate rejects in state `this+0x850 == 3` (Phase536).
- The fallback's complete machine-code extent is **`0x2235D2C0`
  through `0x2235D5A1` inclusive**, length **738 / `0x2E2` bytes**;
  SHA-256 `254800571c72c5a26cf163f745631d1e79c89ea5f707c302d682726be66f42dc`.
  The next routine begins at `0x2235D5B0` after `int3` padding.
- All **26 direct `E8 rel32` CALL instructions** are indexed and target-checked;
  they reach **10 distinct functions**. We separately verify 15 indirect
  virtual-call offsets, 14 literal shared-condition query pushes,
  nine immediate schedule returns, two zero-return sites, two state writes,
  the original x87 distance comparison and four Grunt-local schedule registry
  name/ID associations. A **second complete original helper** reached from
  the final assault path is also verified below, bringing this phase to
  **944 authenticated full-method bytes across two functions**. The full method digest detects any additional
  machine-code mutation not directly covered by those witnesses.

## Verified return IDs and original registry names

| Return ID | Original `mov eax` site | Identity / confidence |
| --- | --- | --- |
| `0x00` | `0x2235D2CE`, `0x2235D59D` (`xor eax,eax`) | No specific schedule / falsey result; meaning of caller fallback remains caller-dependent |
| `0x0D` | `0x2235D439` | Shared schedule; name not yet proven in this build |
| `0x19` | `0x2235D39C` | Shared schedule; name not yet proven |
| `0x2A` | `0x2235D394` | Shared schedule; name not yet proven |
| `0x2D` | `0x2235D4FB` | Shared schedule; name not yet proven |
| `0x30` | `0x2235D3FE` | Shared schedule; name not yet proven |
| **`0x50`** | `0x2235D3CF` | **`SCHED_GRUNT_SUPPRESS`**: original name write `0x22360143`, ID write `0x2236014B`, registration call `0x22360153` |
| **`0x57`** | `0x2235D596` | **`SCHED_GRUNT_ASSAULT`**: name `0x2235FF91`, ID `0x2235FF99`, registration `0x2235FFA1` |
| **`0x58`** | `0x2235D49E` | **`SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE`**: name `0x2235FFCF`, ID `0x2235FFD7`, registration `0x2235FFDF` |
| **`0x59`** | `0x2235D587` | **`SCHED_GRUNT_PRESS_ATTACK`**: name `0x2236000D`, ID `0x22360015`, registration `0x2236001D` |

The four proven local pairs use real names from the original `.rdata` and
original `0x2208C590` schedule-registration CALLs. The shared values are
**not** assigned guessed Source SDK symbolic names. These numbers also
must not be passed directly to a different Source engine's scheduler.

## C-like decompilation of verified branch structure

The pseudocode below preserves original condition IDs, field offsets,
short-circuit order and return values. Symbols named `unknown_*`,
`native_virtual_*` and `original_*` are **editorial descriptions**, not
recovered Ritual source declarations or established C++ method signatures.
It is not compiled and does not simulate the original x86 exception/NaN
behavior. In particular, preserve the original repeated calls and the
unknown helper's side effects when translating later.

```cpp
// Original CNPC_MobGrunt primary vtable +0x844 (0x2235D2C0)
// C-like control-flow reconstruction, NOT an independent implementation.
int Grunt_fallback_844(Grunt* g) {
    if (has_shared_condition(g, 0x1D))
        return 0;

    if (has_shared_condition(g, 0x19)) {
        auto* target = native_virtual_154(g); // name/return contract unresolved
        auto* linked = *(void**)((char*)g + 0x9C4);
        if (linked != nullptr && target != nullptr) {
            bool use_assault_branch = unknown_220AA2D0(linked, g);
            if (!use_assault_branch) {
                if (unknown_220A9210(linked) != nullptr) {
                    // Original calls this getter again before virtual +0x154.
                    auto* related = unknown_220A9210(linked);
                    if (native_virtual_154(related) != target)
                        use_assault_branch = true;
                }
            }
            if (use_assault_branch) {
                unknown_2235A470(g, target); // side effect; signature unproven
                if (has_shared_condition(g, 0x14) && native_virtual_79C(g))
                    return 0x50; // SCHED_GRUNT_SUPPRESS
                return 0x57;     // SCHED_GRUNT_ASSAULT
            }
            auto* related_again = unknown_220A9210(linked);
            if (related_again && native_virtual_584(g, 1))
                native_virtual_588(g, 1, 0);
            if (has_shared_condition(g, 0x0A) && native_virtual_78C(g, 1)
                && unknown_22054D40(g, 0x0C))
                return 0x2A;
            return 0x19;
        }
    }

    // Original queries 3 only when 4 is false, and only queries 0x16
    // when either 4 or 3 is present.
    if (has_shared_condition(g, 4) || has_shared_condition(g, 3)) {
        if (!has_shared_condition(g, 0x16))
            return 0x30;
    }

    if (has_shared_condition(g, 0x10)) {
        // Original global at 0x225721B0, virtual +0x08, arguments 0,0x63.
        int sample = native_global_225721B0_virtual_08(0, 0x63);
        if (sample <= 0x5A && native_virtual_154(g) != nullptr)
            return 0x19;
        return 0x0D;
    }

    if (unknown_2235A560(g))
        return 0;

    if (has_shared_condition(g, 0x0C)) {
        *(unsigned char*)((char*)g + 0xC64) = 1;
        *(unsigned char*)((char*)g + 0xC66) = 0;
        auto* target = native_virtual_154(g);
        if (target && !(target->raw_byte_0xBD & 0x80) &&
            native_virtual_7A0(g))
            return 0x58; // ESTABLISH_LINE_OF_FIRE

        // Original uses virtual +0x214 on target and owner to get two
        // float[3] vectors. This path does NOT have a fresh target-null
        // guard before dereference; its runtime preconditions are unresolved.
        Vector3 delta = native_virtual_214(target) - native_virtual_214(g);
        float squared_dist = delta.x*delta.x + delta.y*delta.y
                           + delta.z*delta.z;
        if (squared_dist > 9000000.0f) // 9e6 constant at 0x22490CF8
            return 0x58; // ESTABLISH_LINE_OF_FIRE
        return 0x2D;     // raw shared ID, meaning unresolved
    }

    if (!has_shared_condition(g, 0x0A) || has_shared_condition(g, 0x14))
        return 0;

    if (has_shared_condition(g, 0x25) && has_shared_condition(g, 0x1E)) {
        // x87: original helper 0x222AA640 returns an object; at +0x500
        // it adds 2 * original_220ADAD0(...). Exact units unknown.
        auto* metrics = unknown_222AA640(g);
        auto* target = native_virtual_154(g);
        metrics->raw_float_0x500 += 2.0f * unknown_220ADAD0(
            unknown_220092D0(target), unknown_220092D0(g));
    }
    if (has_shared_condition(g, 0x25) && native_virtual_79C(g))
        return 0x59; // SCHED_GRUNT_PRESS_ATTACK
    unknown_2235A3A0(g); // side effect preserved, semantics unresolved
    return 0x57;         // SCHED_GRUNT_ASSAULT
}
```

**Important:** the pseudocode uses an explicit `Vector3` expression to
explain the original x87 instructions. The exact original float evaluation
order, `fnstsw` status-bit handling and exceptional values are retained in
machine-code witnesses, not guaranteed by normal C++ floating arithmetic.
Similarly, some displayed `void*` operands may be handles or members of
larger engine objects: the original object types are not recovered here.

## High-confidence branch facts

1. Entry checks shared condition `0x1D`; nonzero returns `0` before any
   linked-entity/target work (`0x2235D2C1..D2D1`).
2. Under condition `0x19`, original field `+0x9C4` and the virtual `+0x154`
   return are required for the first alternate path. The `0x220AA2D0`
   result or another entity's `+0x154` comparison can select suppression
   (`0x50`, additionally gated by condition `0x14` and virtual `+0x79C`)
   or assault (`0x57`). Other paths may return `0x19` or `0x2A`.
3. Three shared conditions `4`, `3`, `0x16` have a specific gate:
   when **either 4 or 3 is present, and `0x16` is absent**, return `0x30`.
   The original checks `3` only if `4` is absent, and checks `0x16` only
   when `4` or `3` is present. No condition-name mapping is inferred.
4. Condition `0x10` invokes original global `0x225721B0` vtable `+0x08`,
   passing `0` and `99` (`0x63`); the signed result threshold is `90`
   (`0x5A`). A result ≤90 **and** non-null `+0x154` returns `0x19`,
   otherwise returns `0x0D`. This is not automatically labelled a percent
   roll until the global interface is identified.
5. Condition `0x0C` writes Grunt bytes `+0xC64 = 1` and `+0xC66 = 0`.
   A target flag at `+0xBD` and native virtual `+0x7A0` may select schedule
   `0x58`. Otherwise x87 compares squared vector distance with exactly
   **9,000,000.0f** at original `.rdata` `0x22490CF8`. The strict ordered
   greater-than branch also selects `0x58`; other cases return `0x2D`.
   For ordinary finite positions, the threshold corresponds to 3,000 units.
6. After rejection by helper `0x2235A560`, the final path requires shared
   condition `0x0A` and absence of `0x14`. A nested `0x25`/`0x1E` condition
   pair can add twice a computed original float to a related object field
   `+0x500`. Another `0x25` check and virtual `+0x79C` can select
   `0x59` (press attack). Otherwise helper `0x2235A3A0` runs and the
   function returns `0x57` (assault).


## Nested original assault-sentence helper `0x2235A3A0` (second full decompilation)

The final assault case calls `0x2235A3A0` (original fallback callsite
`0x2235D590`) before returning `SCHED_GRUNT_ASSAULT`. Its whole function
ends at `0x2235A46D` (206 / `0xCE` bytes), then `int3` padding begins.
Complete SHA-256: `b86979db517e208a3f46a0ca0386a30f5b9066b08050c38de14b14cec6605343`.

Unlike the main fallback, this **does not return a schedule ID**. It is an
optional side-effect action with several gates. The following C-like code
keeps unknown method names and numeric flags intact:

```cpp
// Original 0x2235A3A0; result ignored by its 0x2235D2C0 caller.
void Grunt_assault_side_effect(Grunt* grunt) {
    // Global original 0x225721B0 virtual +0x08, arguments (0,5).
    if (global_225721B0_virtual_08(0, 5) > 1) return;
    auto* candidate = unknown_22031A80(grunt);
    if (!candidate || !grunt->virtual_584(0)) return;

    Vector3 delta = grunt->virtual_214() - candidate->virtual_214();
    if (dot(delta, delta) > 4000000.0f) return;
    if (!candidate->virtual_300(grunt)) return;
    if (!grunt->virtual_1F0(candidate, 0x4081, 0)) return;

    auto value = grunt->virtual_7F0();
    unknown_220A35C0((char*)grunt + 0xCC0, value, 0, 2);
}
```

Verified method details: the random-looking global's numerical range is
bounded here by arguments `0,5`, but its exact interface type is **not**
recovered; the original only tests result `>1`. The squared-distance
constant at `0x223F02BC` is **4,000,000.0f** (2,000 units for ordinary finite
positions). The two direct CALL targets are `0x22031A80` and `0x220A35C0`;
the latter receives the embedded Grunt member at `+0xCC0`, previously
RTTI-identified in Phase535 as `CAI_Sentence<CNPC_GruntBase>`.

It is reasonable to investigate this as a **sentence/announcement trigger**,
but no specific voice line, playback success or fully recovered signature
is claimed. Original x87 NaN comparison and `float` rounding are not
abstractly equivalent to this pseudocode.

## Original direct-call destination inventory

| Destination | Calls | Evidence | Next decompilation question |
| --- | ---: | --- | --- |
| `0x220397C0` | 14 | All calls pass small literal condition ID | Identify original shared-condition namespace safely |
| `0x220A9210` | 3 | All are on the `this+0x9C4` branch | Identify linked-entity retrieval and lifetime |
| `0x220092D0` | 2 | Repeated scalar-helper input conversion | Determine result semantics and ABI |
| `0x220AA2D0` | 1 | Branch selection with field `+0x9C4` and owner | Recover return condition and ownership |
| `0x22054D40` | 1 | Takes ID `0x0C`, guards return `0x2A` | Recover helper's condition role |
| `0x2235A470` | 1 | Called before suppression/assault branch | Identify mutated fields; arguments include target |
| `0x2235A560` | 1 | Nonzero causes immediate no-schedule return | Identify early cancellation gate |
| `0x222AA640` | 1 | Returns object whose `+0x500` float is increased | Identify owner object and unit |
| `0x220ADAD0` | 1 | x87 return doubled and added to `+0x500` | Determine computation inputs |
| `0x2235A3A0` | 1 | Called before final assault `0x57` | Identify side effects and reset logic |

**Runtime grounding:** the owner's `condump001.txt` proves the retail Assault
Rifle Grunt reaches `SCHED_GRUNT_MOBILITY_SIDESTEP`, fails route acquisition,
transitions into `SCHED_GRUNT_COMBAT_FAIL`, and resumes ranged combat.
Phase 536 proves `SCHED_GRUNT_MOBILITY_SIDESTEP == 0x55` by original
registration. The fallback routine's other schedule choices are **not**
asserted to have occurred in that trace merely because their names are in
this method.

## Reproduction

From the repository root, using the *private* original `bin.zip` only on
an authorized machine:

```sh
python3 research/tools/audit_sin_original_grunt_fallback_844.py --self-test
python3 research/tools/audit_sin_original_grunt_fallback_844.py --bin-zip /path/to/bin.zip
```

Output is bounded to addresses, checksums, schedule ID mapping and counts.
The original binary and original disassembly are **not** committed to the
repository. There is no DLL patching, executable launch, or asset processing.

## Decompilation-first next targets

1. Reconstruct `0x2235A560` and the referenced `0x22031A80` /
   `0x220A35C0` helpers to resolve the remaining cancellation and
   sentence/announcement gates. `0x2235A3A0` itself is now verified and
   decompiled in this phase.
2. Confirm original shared-condition ID meanings from the actual game
   registry/engine tables, without assigning speculative Source SDK names.
3. Reconstruct the original `+0x154`, `+0x79C`, `+0x7A0` vtable methods and
   the `+0x9C4` related-entity field contract.
4. Link this *readable* function to its caller's other states and the main
   selector, while separately progressing original `client.dll` lifecycle,
   `server.dll` entity network registration and broader uncovered gameplay.

**What this phase achieves:** two complete authenticated original
methods (738-byte combat fallback plus 206-byte assault side-effect helper),
with reproducible readable logic and a dependency atlas. **What it does not achieve:** complete source-code equivalence for
all helper calls, playable replacement DLLs, or a measurable percentage of
the full `server.dll` / `client.dll` decompiled.