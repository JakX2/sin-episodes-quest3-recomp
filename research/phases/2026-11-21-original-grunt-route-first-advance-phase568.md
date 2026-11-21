# Phase 568: authentic 2006 route head replacement, waypoint advancement and path extension

**Scope:** continue the original `server.dll` Grunt navigation dependency recovered in Phases 565–567. Follow the original waypoint-chain constructor into the actual functions that install/advance the route's first node, mutate its state, and invalidate cached path length. This is reverse engineering, not HL2 physics, prop loading, render code, or a claim to possess original Ritual C++ sources.

## Owner-private binary identity and verification

This pass audits the owner's original `SE1/bin.zip` entirely read-only. No original proprietary binary, large assembly dump, or commercial assets are included in the repository or deliverable.

- Original `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original `server.dll` preferred image base: `0x22000000`, x86 PE32, original `.text` code.

| Original VA | Complete method length | Whole-method SHA256 | Source-bounded role |
| --- | ---: | --- | --- |
| `0x220746F0` | 27 B | `3b69acfa2835dd84abec9e5a68302cf56562cf61235dff19bd68d5115c44382f` | unlink a waypoint from both of its neighbors |
| `0x2209D0C0` | 45 B | `8cfdf325e21cda507e7a6ca21bcb8913ea9871c1be43ce79d3a3d2351f8e0ef4` | original route state flag and engine-time transition |
| `0x2209D160` | 80 B | `551b6913d69c63152c9d0c309dfe14910a3bf71892ce8751bd54051d1fc02ebc` | replace first waypoint, invalidate last cached distance, optional endpoint initialization |
| `0x2209D450` | 101 B | `21ed2b0389af568f08d820811302984a9f6e424e71344717e9a4469c0e7c38ed` | total waypoint-chain length plus distance from an external point to first waypoint |
| `0x2209D720` | 128 B | `ec3ec3b9014d2b232569782d9b7400ddd46791c2843b4ea5333a83d227b1daa7` | advance past consumed first waypoint, detach/free node, invalidate last distance cache |
| `0x220B2A10` | 9 B | `aa1d0523d277f6a14d7950c488982e29fa08fe5d88274b15a712c4427302720a` | write first waypoint pointer to route raw `+0x00` |

**Totals:** 6 complete independent native functions, **390 distinct original code bytes**, **8 verified original in-method direct `E8 rel32` CALLs**, **4 original cross-call witnesses**, 43 opcode/field witnesses. Whole-function hashes authenticate other instructions as well. Both main route routines `0x2209D160` and `0x2209D720` have an immediate next valid function after their recorded RET, so the auditor checks the exact successor's machine bytes instead of incorrectly expecting an `INT3` separator. Other method ends have verified authentic `INT3` padding. The **0.0f** native constant at `0x223D2590` is also checked.

### Original caller and callee linkage

The prior original route path provides several caller witnesses:

| Actual original caller CALL site | Destination | Why it matters |
| --- | --- | --- |
| `0x22075522` | `0x2209D160` | route acceptance/installation dispatches into the first-waypoint setter |
| `0x22074280` | `0x2209D720` | navigation code advances the active waypoint |
| `0x22074BB5` | `0x2209D450` | navigation caller asks for an extension-distance measurement |
| `0x22075536` | `0x2209D0C0` | route acceptance/installation writes a route-state transition |

These are verified directed calls, not claims about the full meaning of the still-unreconstructed callers. Within this phase the setter calls the authentic first setter `0x220B2A10` and last-node getter `0x220B2A20` (already investigated in Phase566), and the first-advance method invokes original node-pool release `0x2226EF40`. The latter is opaque and must not be replaced with a guessed `delete` or an allocator from an unrelated Source SDK.

## Original route/waypoint fields used here

The 2006 methods use the existing native linked node ABI, **not** the layout of the C++17 test classes:

| Native object / offset | Observed native role |
| --- | --- |
| route `+0x00` | first waypoint pointer |
| route `+0x14,+0x18,+0x1C` | 3-component base vector used to construct endpoint |
| route `+0x2C` | raw value copied from a removed first waypoint in the flag-4 path |
| route byte `+0x30` | toggled to 0 and 1 when endpoint is initialized |
| route `+0x34,+0x38,+0x3C` | calculated endpoint vector |
| route byte `+0x40` | original state-change marker |
| route dword `+0x44` | previous/current state input and storage |
| route dword `+0x4C` | captured original engine-time field on nonzero previous state |
| waypoint vector `+0x00,+0x04,+0x08` | node point vector |
| waypoint dword `+0x10` | raw value copied into route +0x2C if removed waypoint flag bit 4 is set |
| waypoint float `+0x14` | cached remaining path distance, sentinel **raw `0xBF800000`** (`-1.0f`) |
| waypoint flags `+0x20` | `0x08` terminal/stop, `0x04` special value-copy path |
| waypoint dimensionality `+0x24` | select original XY versus XYZ segment distance helper |
| waypoint `+0x28` | next node pointer |
| waypoint `+0x2C` | previous node pointer |

The symbolic names above are editorial research labels. Original native C++ declarations and complete ownership/lifetime models have not been recovered.

## `0x2209D160`: install route first, optional marked endpoint

1. Store the incoming node pointer directly at route `+0x00` using original `0x220B2A10`.
2. Ask `0x220B2A20` for the final node, walking the route's next links. If there is no final node, exit without mutating a waypoint.
3. Set the last node's cached distance `+0x14` to the **exact bit pattern `0xBF800000`**, invalidating earlier cumulative distances.
4. If the optional Boolean argument is false, return. If it is true, explicitly clear and then set route byte `+0x30`, OR bit `0x08` into final node flags `+0x20`, and write the three endpoint coordinates as the **route base vector + final waypoint position**.

The terminal marker is a waypoint flag, not a proof that a route is semantically complete or can be traversed in the live engine. In particular no link repair is performed by the setter: callers are responsible for providing a valid list.

## `0x2209D720`: advance and retire first waypoint

The original method:

1. Fetches the route's first node. If it exists and has `flags & 8`, returns immediately (cannot advance marked terminal node).
2. If not terminal, obtains the first node's next pointer. On **no next node**, logs an engine message and sets the node's `flags |= 8`, leaving it as first and returning.
3. If there is a next node and the old first had `flags & 4`, copies old node `+0x10` into route `+0x2C`.
4. Repairs successor/predecessor links, invokes **original pool release** `0x2226EF40` on removed first, replaces route `+0x00` with the successor through `0x220B2A10`, then invalidates the new last node's cached distance `+0x14` to raw `0xBF800000`.

**Native precondition/hazard:** the original function does not safely handle an empty route. Its initial null-check only guards the terminal-bit test; the code then dereferences the null first node while trying to access `+0x28`. The host model reports `UnsupportedGraph` instead of reproducing this null dereference. It also deliberately leaves test nodes allocated, rather than pretending to invoke the original 2006 allocator/pool ABI.

## `0x2209D450`: route length plus external segment

The original first checks the route's first-node pointer. Empty route returns engine constant **0.0f**. Otherwise it retrieves the route's last waypoint through native `0x220B2A20`. If the last node's distance cache at `+0x14` is *exactly* raw `0xBF800000`, native cumulative-distance routine `0x2209D370` regenerates the chain's cached lengths. The method retrieves the **first node's** cumulative remaining distance and adds direct segment distance between **the first waypoint position and a caller-supplied external vector**, passing the **first node's own dimensionality flag `+0x24`** to authentic `0x2206D9F0` (Phase566).

This extension-distance method is not the same as Phase566's bare `0x2209D410` path getter. Existing terminal cache values other than `-1.0f` can intentionally skip recalculation, leaving stale first-node values. Original x87 evaluation, NaN cases and invalid external pointers are not claimed equivalent to portable C++.

## `0x2209D0C0`: native previous-state-dependent transition

```cpp
// Source-style explanation only; typed fields here are NOT a native class declaration.
void route_state_transition(OriginalRoute* r, unsigned new_state) {
    if (r->raw_dword_0x44 != 0) {
        r->raw_dword_0x4C = original_engine_time_dword_22572190_plus_0x0C;
        r->raw_byte_0x40 = 1;
    } else {
        r->raw_byte_0x40 = 0;
    }
    r->raw_dword_0x44 = new_state;
}
```

This distinction matters: the decision uses the **previous** route `+0x44` value, not the argument replacing it. When old state is zero, original code does **not** freshly update route `+0x4C`. The original engine-time value is copied as a raw dword (commonly interpreted as `float` in this subsystem), not recomputed through a new host timer.

## `0x220746F0` and `0x220B2A10`: small but relevant invariants

The waypoint unlink routine repairs only the neighbor pointers `forward.backward` and `backward.forward`; it does **not** clear the detached waypoint's forward/backward pointers or change the route object's first pointer. The route-first setter writes exactly the supplied pointer to `route+0x00` and returns. The multiple small native methods should not be conflated with an engine-managed container supporting implicit ownership or guard rails.

## Independent host reference and verification

- `include/sin_compat/original_grunt_route_mutation_phase568.hpp` contains an **independently authored C++17, non-ABI** typed route/waypoint reference and explicit `Result` diagnostics for host-only null/cycle/nonfinite safety.
- `tests/test_original_grunt_route_mutation_phase568.cpp` exercises empty-route length, strict raw invalid-cache representation, waypoint replacement/optional endpoint, XY vs XYZ first-waypoint extension, stale-cache handling, first-node advancement and special +0x2C flag, terminal/no-op, pointer unlink, and previous-state timing.
- Owner-private original byte audit: `PYTHONPATH=research/tools python3 research/tools/audit_sin_original_grunt_route_mutation_phase568.py --bin-zip /private/SE1/bin.zip` **PASS**.
- Original-free mutation self-test: `python3 research/tools/audit_sin_original_grunt_route_mutation_phase568.py --self-test` **PASS**, rejecting wrong-binary identity and deliberate byte changes in each authenticated direct call instruction.
- Strict GCC and Clang C++17 with `-Wall -Wextra -Wpedantic -Werror -UNDEBUG`: **PASS**.
- GCC AddressSanitizer / UndefinedBehaviorSanitizer (`detect_leaks=1`): **PASS**.
- Standalone CMake / CTest: **2/2 PASS**.

**Boundaries:** These tests are not proof of full engine route ownership, native x87 bitwise parity, object validity across real scene transitions, live original NPC combat execution, a complete `server.dll`, or a running PC/Android/Quest port.

## Next authentic source-first targets

1. Decompile original route acceptance caller around `0x22075522`/`0x22075536`, especially its conditions for installing a candidate route, and original navigation advance caller around `0x22074280`.
2. Identify the actual engine event producer that passes **event ID 1** to the Grunt `+0x5A8` speech virtual; the prior `>240.0f` proof does not establish the full event dispatch path.
3. Continue authenticated Grunt combat schedule/condition decompilation and implement only demonstrably recovered gameplay C++ behavior.