# Phase 575: original `0x2206EF20` dual-trace Grunt waypoint-clearance result

**SiN-specific original gameplay call-chain recovery.** This phase follows
Phase574's original mode-0 waypoint method and its alternate wrapper into
**the same complete 2006 native clearance provider**. It recovers the
caller-visible Boolean/output-point branching while deliberately retaining
the underlying `0x2206E730` engine trace as a named opaque callback. This
is not a project to reimplement generic Half-Life 2 physics, surfaces or
models, and it does not make the game executable on Windows/Android/VR.

## Original private DLL identity and whole-method authentication

- Original 2006 `server.dll` SHA256:
  `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original 2006 `client.dll` SHA256:
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original server PE32 image base: `0x22000000`.
- Complete native `0x2206EF20` span: `0x2206EF20–0x2206F144` inclusive,
  **549 distinct original code bytes**.
- Exact whole-method SHA256:
  `5f17c0fb7a03cf109174a5525df36a0efcaada39ad21fdaa4d9d19ecde7ccb43`.
- Original two native `ret 0x14` paths at `0x2206F119` and `0x2206F142`;
  real `INT3` alignment begins at `0x2206F145`.
- Two original internal direct E8 calls, **both to `0x2206E730`**, at
  `0x2206EFE6` and `0x2206F0BF`.
- Two previously authenticated inbound callsites, checked again:
  Phase574 `0x22071ABD -> 0x2206EF20` and `0x2206F97A -> 0x2206EF20`.
- **32** individual original field/opcode witnesses and three real native
  numeric constants verified. No native method bytes are uploaded.

## Native entry context and first trace

The helper works with the calling navigation/owner association at
`this+0x04`, a source position pointer, several scalar/geometry parameters
and a caller-provided output XYZ pointer. Its exact original C++ name and
higher-level argument typedefs are **not** recovered. The original
function reads owner virtuals through `owner+0xFC` and initializes
temporary original-stack vectors. It then calls the trace-like original
`0x2206E730` using six stack arguments.

One first-trace result byte at original local stack offset `+0x8F` gates
whether a **second** call to `0x2206E730` occurs. If the byte is zero,
there is no second trace. When nonzero:

1. Read the first-trace candidate entity pointer from the native local
   record at `+0xA4`. If null, skip candidate classification.
2. Accept the candidate for a further proximity check if its raw signed
   field `entity+0x204` equals **`6`**, or else if its native virtual
   byte `+0xF4` returns true. The function checks the raw field before
   invoking the virtual, with this short-circuit behavior verified.
3. Compute x87 XYZ distance from the source position to owner position
   at **`owner+0x360`**, through the already-authenticated indirect x87
   square-root pointer `0x2251B1C4`.
4. Compare that result to the original exact **double `0.1`** at
   `0x223D8FE8`. Native `fnstsw ax; test ah, 0x05; jp skip` determines
   whether local byte `BL` becomes 1. Because of the original x87
   status-word test, the portable model reads the comparison AH bits
   through an explicit host callback rather than pretending a C++
   `distance < 0.1` expression reproduces every native NaN/FPU case.
5. Call original `0x2206E730` **a second time**. Its temporary destination
   Z adjustment uses the authentic original float **`0.0625f`** at
   `0x224E076C`. This constant is independently SHA-audited.

The candidate eligibility criteria and numeric raw state `6` are
original-machine facts. Calling them a particular HL2 entity category or
a collision-filter designation would be speculation.

## Original two-path final decision and output XYZ

After one or two trace calls, original x87 instructions compare a trace
metric to exact original float **`1.0f`** at `0x223D22F8`. Then the
routine explicitly tests `ah & 0x44` and branches on parity (`jnp`).
It additionally tests native trace-result bytes at `+0x8E` and `+0x8F`
and the near-candidate register byte `BL`.

**Trace-point branch:** if the original parity test permits, trace byte
`+0x8E` is zero, and either candidate-near is false or byte `+0x8F` is
zero, the native routine copies its trace-computed XYZ to the caller's
output pointer, returns `AL=1`, and executes the first native `ret 0x14`.

**Source-point branch:** otherwise the native routine copies the original
input/source XYZ to the caller's output pointer and returns `AL=BL != 0`.
Thus it can return **true with source XYZ** (an eligible near-candidate
case) or **false with source XYZ**, as distinct from **true with the trace
XYZ**. Collapsing all true returns into a single identical result would be
wrong when reimplementing this original behavior.

C-like control skeleton, with all complex native trace work abstract:

```cpp
Trace t = original_2206E730(...);        // first trace always executes
bool near = false;
if (t.raw_byte_8f) {
    auto* hit = t.raw_entity_a4;
    if (hit && (hit->raw_state_204 == 6 || hit->native_virtual_F4())) {
        auto ah = original_x87_distance_compare_ah(
            sourceXYZ, owner->raw_position_360, 0.1);
        near = !x86_parity_even(ah & 0x05);
    }
    t = original_2206E730(..., z_adjustment_0_0625f, ...);
}
const bool use_trace_xyz = x86_parity_even(t.fraction_cmp_ah & 0x44)
    && !t.raw_byte_8e && (!near || !t.raw_byte_8f);
if (use_trace_xyz) {
    outputXYZ = t.traceXYZ;
    return true;
}
outputXYZ = sourceXYZ;
return near;
```

This is **not** a claim that all original `0x2206E730` memory fields and
arguments are known. The x87 AH bytes in the host test are supplied by
the host rather than independently regenerated from the engine trace;
transient pointer lifetime, native stack frame ABI and exceptional values
remain unproven. The skeleton does document the actual ordering and
status-dependent output/return contract at the original caller boundary.

## Independent host reference

`include/sin_compat/original_grunt_clearance_phase575.hpp` implements a
bounded, C++17-only typed `Trace` record, `clearance_select`, and exact
**parity checks over the provided original x87 AH status bytes**. It
models optional second trace, raw entity state-6/virtual-F4 classification,
the 0.0625f parameter, trace-vs-source coordinates and three outcome
labels without any real engine trace calls or commercial assets.

`tests/test_original_grunt_clearance_phase575.cpp` covers:

- First trace only and successful trace XYZ selection.
- No-near rejection with caller/source XYZ preserved.
- State-6 candidate bypassing virtual F4 and invoking second trace.
- Candidate-near true but second trace flag still set, yielding `true`
  with **source XYZ**.
- Identical candidate with second flag clear, yielding **trace XYZ**.
- Other raw entity states admitted by virtual F4, with trace flag rejection.
- Ineligible candidate returning false, and raw AH parity paths.
- Original source distance-comparison parity versus final fraction-
  comparison parity; they use **different native masks** `0x05` and `0x44`.

All native calls are replaced with host fakes, so test execution does not
require the original game DLLs. This host abstraction is not an engine ABI,
not complete x87 status production, and not a generic trace implementation.

## Reproduce and verification

```bash
python3 research/tools/audit_sin_original_grunt_clearance_phase575.py --self-test
python3 research/tools/audit_sin_original_grunt_clearance_phase575.py --bin-zip /private/SE1/bin.zip
c++ -std=c++17 -UNDEBUG -Wall -Wextra -Wpedantic -Werror -Iinclude \
    tests/test_original_grunt_clearance_phase575.cpp -o /tmp/sin_phase575
/tmp/sin_phase575
```

Original owner-private SHA audit **PASS** for one complete 549-byte method,
**two** direct calls to `0x2206E730`, **two** caller edges, **32** machine
witnesses, **three** original numeric constants, and both RET paths.
Synthetic E8 mutation/wrong-owner tests **PASS**. Strict GCC/Clang C++17,
GCC ASan/UBSan and standalone CTest **2/2 PASS**. GitHub hosted CI is
separate and should not be represented as passed without checking.

## Next original-source priorities

1. Investigate the actual **`0x2206E730`** call interface from original
   `server.dll` sufficiently to reconstruct which native result bytes
   `+0x8E/+0x8F` represent, without implementing generalized HL2 physics.
2. Recover original **`0x22071040`** post-clearance helper from Phase574,
   especially its entity identity and waypoint-response updates.
3. Relate the original Grunt combat schedule and flank events to these
   route admissibility results; preserve original conditional voice triggers.
4. Resume wider original `client.dll` code as remaining Grunt-specific
   original paths converge.

No original DLL binary, full disassembly dump, proprietary game asset or
native executable has been committed, and no playable replacement-DLL boot
is claimed.