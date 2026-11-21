# Phase 576: original SiN 2006 Grunt post-clearance candidate scanning

**Scope.** Follow the exact original `server.dll` caller `0x22071B78` from
Phase574's mode-0 spatial clearance into native method `0x22071040`, then
recover the two real original callees relevant to its initial two-dimensional
measurement and terminal candidate filtering. **This is read-only evidence
and a bounded independent C++17 behavior model, not complete translation of
its 2,523-byte body, the game engine's spatial queries, or an executable
reconstruction of `server.dll`.** No HL2 physics, surfaces, prop loading,
models, rendering or Windows/Quest startup work.

## Retail identity and exact machine extents

The *owner-private* archived `SE1/bin.zip` contains the authenticated
unmodified retail pair:

- `server.dll` full SHA-256:
  `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` full SHA-256:
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll` is original PE32 x86 with preferred base `0x22000000`.

| Function VA (inclusive begin) | Exact size | Complete method SHA-256 | Source-bounded purpose |
| --- | ---: | --- | --- |
| **`0x22071040`** | **2,523** (`0x9DB`) | `efc230a266706e2b6acb2de1081ae86d2b9abe9e3a43ef7dde011b1fe35e5bee` | Six-argument post-clearance candidate-search method, result record, probe flags and bounded entity iteration |
| `0x22027C30` | 91 (`0x5B`) | `7bd375fadb682ca28763503174a92c69ed47dd02d4551d7ac1c5b3d3eace0f48` | Two-dimensional normalization in place with x87 length returned |
| `0x22139F60` | 19 (`0x13`) | `d5ea9253030765bcd70227e7860e4620758e52644047e564b5756b5d78eaef44` | Candidate entity virtual `+0xF4` filter returning original pointer or null |
| **Total** | **2,633 distinct bytes** | | **3 newly authenticated complete native methods** |

Function end facts: the main method's final `ret 0x18` occurs at
`0x22071A18`, with `INT3` beginning at `0x22071A1B`, and the following
method begins at `0x22071A20`. The shorter functions end in an actual `ret`
followed by `INT3` padding. Complete original function hashes and the PE
section checks ensure a stray `0xCC` inside machine code cannot truncate a
method. Each function is counted **only once**, even if invoked by several
paths.

## The original caller-to-callee connection

Earlier Phase574 authenticates original `0x22071B78 -> 0x22071040` within
`0x22071A20`. The original `ret 0x18` at `0x22071A18` means six 32-bit
stack arguments were consumed. The native method receives coordinate
inputs, a scalar budget, a flag word, and an optional output-record
pointer, plus the original owner in `ECX`. We do not assert the original
class names, parameter types or exact ABI qualifiers.

The original method has **21 authentic `E8 rel32` calls**, including
`0x220710DE -> 0x22027C30` and `0x22071822 -> 0x22139F60`.
Other authenticated targets include `0x2206F500` (three distinct original
calls), `0x2204C980`, `0x2206E870`, `0x2206FC20`, `0x2206FB70`, native
`0x2225C0F0` diagnostics, and record cleanup `0x2226E3E0`. The original
spatial/engine methods remain external, **not** replaced by guessed
Source SDK calls. `research/tools/audit_sin_original_candidate_filter_phase576.py`
checks all 21 E8 targets, one earlier caller, 65 opcode/field witnesses,
seven original numeric constants and the original sqrt-function pointer.

## Native prefix: zeroing, normalization, and early success

1. It allocates a stack frame with 240 local bytes (`sub esp,0xF0`) and
   zeroes a local 56-byte scratch record via `rep stosd` over **14** dwords.
   If the optional caller output pointer is null, it uses that local
   record. The native method initializes selected fields in an external
   output record; it is not proven to zero **every** byte of an external
   record.
2. Original output dword `+0x00` is zeroed; original source coordinate
   float3 is copied to `+0x04..+0x0C`; an engine-global default float3 to
   `+0x10..+0x18`; entity/association at `+0x1C`, delta `+0x24` and
   accumulator `+0x34` are reset. The actual values of the engine-global
   default float3 are not known merely from the on-disk PE (these addresses
   are not ordinary mapped `.rdata` literals).
3. It computes the **XY delta only** from the second point minus the first
   and calls complete native `0x22027C30`. This function uses its original
   x87 square-root indirection at `0x2251B1C4`, whose original on-disk
   pointer is **`0x222680E0`**, already identified as an x87 `fsqrt` method
   in Phase566. It writes the unit XY direction **into the caller's XY
   scratch pair** for positive length and returns length in x87 `ST(0)`.
   The caller stores length as float to output `+0x20`.
4. The original x87 `fucompp; fnstsw ax; test ah,0x44; jp` distinguishes
   the positive/unordered path from zero and other ordered results.
   A finite, zero-length XY delta returns `true` immediately at
   `0x220710FC..0x22071105`, with no entity/geometry probe calls.
   The portable reference only models finite inputs and **does not**
   claim equivalent NaN, infinity, exception, or x87 roundoff behavior.

## Native budget/category preparation after nonzero distance

The fifth stack argument is a bitfield; the fourth is the floating probe
budget. The input bit gates are original, not guessed semantic enums.

| Source condition | Native result |
| --- | --- |
| bit `0x01` set | raw internal selection `0`, working budget `0`; no first `0x2206F500` call |
| otherwise budget **strictly below `0.001`** | same selection `0` and budget `0` |
| otherwise budget greater than native double `99.999` | working budget overwritten with `100.0f` |
| budget admitted, bit `0x02` set | raw internal selection `1`, no first `0x2206F500` call |
| admitted, bit `0x02` clear, `0x2206F500` true | raw internal selection `1` |
| admitted, bit `0x02` clear, `0x2206F500` false | raw internal selection `2` |

Native constants are actual on-disk values at `0x223DE618` (`0.001`,
`double`), `0x223E9000` (`99.999`, `double`) and the immediate `100.0f`
(`0x42C80000`). Float-to-double boundary conversions are not rounded away
in the host reference.

A **separate** bit `0x08` makes a second call to `0x2206F500` and may
invoke debug visualization helper `0x2225C840` if it rejects. That
optional debug path is distinct from the first selection predicate.

## Candidate iteration and output status

Later native code initializes candidate context, invokes the original
`0x2206E870` and `0x2206FC20` routines, and conditionally iterates a
fixed **maximum of 16 passes** (`0x220715F9: cmp eax,0x10`) through
one search path. Within that path it checks original entity `+0x204==6`,
virtual `+0xF4`, waypoint coordinates using x87 equality status, and
an original dot-product/orientation sign test. It can append an eligible
entity to a temporary collection through `0x2206FB70`. This is **not** a
statement that every call makes 16 probes, or that every original
candidate satisfies a specific game class name.

A selected candidate takes a distinguished `false` result path. The
relevant source-bounded status logic is:

- Original `0x22071822` calls `0x22139F60`, which invokes the candidate
  entity's virtual `+0xF4` and returns the **same entity pointer** if the
  virtual returns nonzero, else null (it does not allocate or clone one).
- If that filter returns a nonnull pointer, report `+0x00` is **`-3`**.
- Otherwise the original global virtual `+0x48` determines status:
  **`-1` for a nonzero result**, **`-2` for zero**. These are original
  numeric statuses and are not assigned guessed engine enum names.
- It writes original output `+0x24` as the prior planar distance `+0x20`
  minus the current accumulated source distance and returns **`false`**
  from `0x220718B7..0x220718C0`.
- The **no-selected-candidate** paths are different: they can return
  `true` at `0x22071A0F..0x22071A18`, or conditionally emit an original
  `-2` status at `0x22071992` and then reach the `false` return. Bit
  `0x04` changes the no-candidate post-processing path.

This leaves the actual 16-iteration search, engine-derived candidate
list, entity identity, trace contents and some x87 status interpretation
**explicitly not translated to a runnable independent source model**.
Native code is fully authenticated but executable source equivalence
requires further original helper decompilation.

## Verified original caller post-filter: `0x22071A20` ignores AL

A further cross-method source check resolves an important interpretation
error: after the **actual** original CALL at `0x22071B78`, its owner
method `0x22071A20` does **not** branch on `AL` at all. Instead it loads an
independent candidate entity at `0x22071B7D` and applies these checks:

1. A null independent candidate skips this entire post-filter.
2. Otherwise it calls **candidate virtual `+0x120`**, whether or not a
   direct report-pointer equality check could have succeeded.
3. Only if that succeeds, it calls original `0x22035100`; only on success
   does it call original `0x22035090`, then the returned object's
   virtual `+0x28`, and compare the resulting identity with the report's
   raw pointer at `+0x1C`.
4. **After** that optional linked-identity computation, it separately
   compares the original candidate entity pointer directly with report
   `+0x1C`. Either direct equality or successful linked identity match
   clears the report's `+0x24` delta, `+0x1C` association, restores its
   runtime-global default vector at `+0x10..+0x18`, and resets its
   signed status at `+0x00` to zero.
5. The original reports' source coordinates at `+0x04..+0x0C` are not
   overwritten by this branch. A candidate that fails both identity
   comparisons leaves the report unchanged.

This is a **post-filter of result data, not a test of the selector's
Boolean return**. The original callsites `0x22071B99 -> 0x22035100` and
`0x22071BA4 -> 0x22035090`, plus nine branch/field instruction witnesses,
are now checked by the auditor. This parent routine was already
whole-body authenticated in Phase574; these checks add **zero new native
function bytes** to the Phase576 count.

The host's additional `original_caller_clear_matching_association()`
models the exact optional probe call order and report-field reset for
safe synthetic entity IDs. The real entity types and virtual contracts
remain unresolved. Tests cover null, direct and indirect matches,
short-circuiting, nonmatch, and original callback evaluation ordering.

## Bounded C++17 reference and tests

- `include/sin_compat/original_grunt_candidate_filter_phase576.hpp`
  implements safe, finite-input XY length/normalization; raw byte-
  independent entity virtual filter; the exact prefix budget/flag gates
  with an abstract `0x2206F500` callback; source-output initialization;
  the three selected-candidate status outcomes, and the parent-caller identity reset; and the native 16-pass
  upper-bound witness.
- It reports distinct **`ImmediateTrue`**, **`NeedsNativeCandidateSearch`**
  and **`UnsupportedInput`** results. `NeedsNativeCandidateSearch` is not
  a claim that waypoint admission succeeds or that the native engine
  query was performed.
- `tests/test_original_grunt_candidate_filter_phase576.cpp` covers
  zero-distance early return, 3-4-5 XY normalization, report field reset,
  exactly `0.001`, `99.999`/`100`, flags `1/2/8`, independent debug
  probe effects, rejected selection, 16 upper limit, virtual+0xF4
  pointer/null semantics, all three negative statuses, distance delta,
  and unsupported nonfinite data.
- Authenticated original project zip:

```sh
python3 research/tools/audit_sin_original_candidate_filter_phase576.py --self-test
python3 research/tools/audit_sin_original_candidate_filter_phase576.py --bin-zip /private/SE1/bin.zip

cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Owner-local run: native SHA audit **PASS**, synthetic direct-call-mutation
self-test **PASS**, strict GCC and Clang C++17 **PASS**, GCC AddressSanitizer
and UndefinedBehaviorSanitizer **PASS**, standalone CTest **2/2 PASS**.
The portable code is an intentionally **partial reference**, not an
ABI-compatible reimplementation of original native C++ or a playable build.

## Next precise research targets

1. Decompile `0x2206F500`, the **original probe predicate called three
   times** within `0x22071040`, to remove a major remaining abstract
   branch without building unrelated generic physics.
2. Resolve the candidate-list behavior of original `0x2206E870` and
   `0x2206FC20` to explain the 16-iteration search and entity-selection
   priorities. Keep any engine-specific trace callbacks opaque until
   independently authenticated.
3. Link selected-candidate statuses `-3/-2/-1` and no-candidate branches
   back to the caller `0x22071A20`, the four-way waypoint dispatch
   `0x22071C50`, and the original `CNPC_MobGrunt` combat event.

**No retail DLLs, large raw original disassembly dumps, assets or
proprietary executables are distributed with the research.**