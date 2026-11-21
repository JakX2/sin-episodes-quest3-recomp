# Phase 574: original 2006 Grunt waypoint-kind 0 admission and scaled clearance

**Source-first recovery, not engine physics reimplementation.** This phase follows
Phase573's original `0x22071C50` four-way mode dispatcher into the complete
native **kind-0** method at `0x22071A20`. It traces the two native clearance
call paths into the original Grunt navigation-component scale selector. The
owner-original `server.dll` provides the machine-code evidence; the independent
C++17 reference is a bounded host model, **not** loadable original C++/MSVC ABI.

## Exact original binary provenance

- Owner-private original `server.dll` SHA256
  `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Owner-private original `client.dll` SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- `server.dll` original PE32 image base: `0x22000000`. Four complete verified
  routines in genuine `.text`; native `0xCC` alignment follows each extent.

| Native complete method | Exact extent | Bytes | Original entire extent SHA256 | Source-bounded role |
| --- | --- | ---: | --- | --- |
| `0x22071A20` | `0x22071A20–0x22071BEB` | **460** | `406d61d26d0955d7e350adf2cfdd756a2bdac148a0a630d8843865554c3afd3e` | Kind-0 distance, first-stage clearance and followup |
| `0x2206F940` | `0x2206F940–0x2206F983` | **68** | `15fff7a2c9dd55774a638b1f9090d86b43f4d233e736e860a2d257100fa267d5` | Additional owner-metric-scaled clearance wrapper |
| `0x2204C980` | `0x2204C980–0x2204C98A` | **11** | `603a2e12dd734e40d381885c907dbfbe41b1ef2c6a0a8d8166ed2168523640a1` | NPC `+0x8F8` accessor and tail-jump to scale source |
| `0x22076910` | `0x22076910–0x22076984` | **117** | `9243ed71b618ba5f96a9164e7ead48314ac3e3e8a835d5812921aa6d26afedd4` | Navigation handle-dependent scale selector |
| **Total** | | **656** | | **Four distinct newly authenticated code extents** |

All four whole-method hashes, eight genuine direct original E8 calls, one
prior-phase caller edge, one E9 original tail-jump, three float literals,
and 40 independent machine-instruction/field witnesses are checked by
`research/tools/audit_sin_original_grunt_mode0_phase574.py`. The four
complete original boundaries each end before genuine `INT3` padding.
This report gives original addresses and selected small witnesses, never a
commercial binary or an exported raw disassembly dump.

## Connection to the original Grunt waypoint generator

Phase572 established that the Grunt navigation component's candidate
waypoint builder calls spatial dispatcher `0x22071C50`. Phase573 verified
its exact case-0 native jump-table entry and the direct call
**`0x22071D8D -> 0x22071A20`**. Phase574 resolves that subordinate method
and its scale dependencies:

```text
Grunt nav +0x8F8 -> active/candidate waypoint structures (Phases 567–572)
  candidate generation 0x220774E0
    -> spatial dispatcher 0x22071C50 (Phase573)
       kind=0 -> 0x22071A20 (Phase574)
          -> NPC virtual +0x5F8(-720.0f, delta)
          -> Grunt wrapper 0x2204C980 -> nav component +0x8F8
             -> tail jump 0x22076910
                -> serial-checked nav +0x7C entity handle
                -> entity virtual +0x120 selects native float
          -> 0x2206EF20 (first-stage clearance; body still opaque)
          -> optional 0x2206F940 -> 0x2206EF20
          -> 0x22071040 and identity/status postprocessing
```

Every method/pointer link in this diagram is observed in original x86.
Opaque engine helpers retain numeric native addresses instead of invented
Source SDK names.

## Kind-0 first-stage operation at `0x22071A20`

1. Read two native pointers to XYZ coordinates. Calculate the **XY** delta
   and Euclidean distance using original x87 operations and the original
   indirect square-root method at `0x2251B1C4`; store float distance at
   output `+0x20`. **Z is not included in this initial distance**.
2. Through `this+0x04`, invoke owner virtual **`+0x5F8`**, pushing literal
   original float `0xC4340000` (`-720.0f`) plus a pointer to delta data.
   The return is an x87 float.
3. Call direct `0x2204C980` using the same owner, obtaining a second native
   float from the owner navigation component. Multiply the two x87 values.
   The scaled result is passed to the original `0x2206EF20` together with
   the original spatial pointers.
4. Branch on the Boolean returned by `0x2206EF20`. If false, set output
   `+0x24 = +0x20` (same XY distance), populate other output fields from the
   source vector, lookup native-world metadata and default-vector globals,
   and set **signed output status `+0x00 = -2`** before `ret 0x1C`.
5. When the first-stage Boolean is true, native control flow continues.
   For input flags **bit `0x04` clear**, it calls the auxiliary wrapper
   `0x2206F940`; with bit set, it chooses the raw input coordinate instead.
   It then always calls additional native helper **`0x22071040`** and
   conditionally checks a returned object through virtual `+0x120`,
   helper `0x22035100`, helper `0x22035090` and virtual `+0x28` to compare
   against the current output `+0x1C` association. A matching condition
   clears output status, distance, entity and other data. Other paths
   preserve state and return. This continuation is documented, not
   falsely presented as fully reconstructed engine behavior.

The native function's first-stage false branch is **not** a whole-mode-0
final result of the parent dispatcher until the parent checks the signed
status. The Phase573 parent returns `true` only for status >=0. The
C++17 host enum `RequiresNativeContinuation` is deliberately **not an
acceptance result**.

### Navigation scale selection at `0x22076910`

The native code reads navigation-component encoded handle **`+0x7C`**;
`0xFFFFFFFF` is invalid. It checks the low 12-bit entity slot and the
upper serial/generation against the real original global entity table
at `0x224F5620`, then looks up the entity pointer. The original performs
this handle resolution twice in its success path and has an unresolved
concurrent-invalidation/null-dereference hazard. On a stable handle:

| Original condition | Original literal float location | Return |
| --- | --- | ---: |
| Raw `-1` or failed handle/serial/entity lookup | `0x223D22F8` | **1.0f** |
| Valid resolved entity and its virtual byte `+0x120` returns false | `0x223E94A0` | **2.6f** |
| Valid resolved entity and its virtual byte `+0x120` returns true | `0x223D8F14` | **10.0f** |

All three exact native float bytes are checked. No semantic class label
(e.g. *player*, *monster*, *obstacle*) is assigned to the entity's virtual
`+0x120`; that class meaning has not been proven. The earlier `-720.0f`
parameter is authentic, but likewise not given an invented engine-domain
meaning.

The 11-byte original Grunt wrapper at `0x2204C980` reads **NPC `+0x8F8`**
and performs a direct **E9 tail-jump** into this 117-byte selector, rather
than a regular E8 call. The separate 68-byte `0x2206F940` wrapper invokes
owner virtual `+0x5F8`, multiplies the result by the same navigation scale
and delegates to `0x2206EF20`, returning its Boolean. Its original native
caller in mode-0 is the optional branch at `0x22071B41`.

## Original direct-call graph

| Native caller site | Native callee |
| --- | --- |
| `0x22071AA8` | `0x2204C980` |
| `0x22071ABD` | `0x2206EF20` |
| `0x22071B41` | `0x2206F940` |
| `0x22071B78` | `0x22071040` |
| `0x22071B99` | `0x22035100` |
| `0x22071BA4` | `0x22035090` |
| `0x2206F961` | `0x2204C980` |
| `0x2206F97A` | `0x2206EF20` |

Previously authenticated parent `0x22071D8D` calls `0x22071A20`.
Native direct E9 at `0x2204C986` tail-jumps to `0x22076910`.
The original indirect call for x87 square root is the already-authenticated
pointer at `0x2251B1C4` (Phase566); this phase does not double-count that
function.

## Host-only C++17 reference and reproducible tests

`include/sin_compat/original_grunt_mode0_phase574.hpp` provides
`nav_scalar`, `mode0_first_stage`, and `alternate_scaled_clearance`.
The host's native entity-handle adapter uses a stable lookup snapshot;
production code must later reproduce original generation/slot checks.
The first-stage model tests XY-only distance, original selector constants,
virtual `-720.0` argument, scaling, call order, rejected `-2` status and
fallback report fields. It returns an **explicit continuation sentinel**
when opaque engine postprocessing is required, rather than mislabelling
first-stage success as an accepted waypoint. It refuses nonfinite model
inputs for host safety instead of claiming undefined original x87 parity.

From the repository root with the private original binary zip:

```bash
python3 research/tools/audit_sin_original_grunt_mode0_phase574.py --self-test
python3 research/tools/audit_sin_original_grunt_mode0_phase574.py --bin-zip /private/SE1/bin.zip
c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude \
  tests/test_original_grunt_mode0_phase574.cpp -o /tmp/sin_grunt_mode0
/tmp/sin_grunt_mode0
```

Owner-private SHA audit **PASS**, synthetic mutation/wrong-binary self-test
**PASS**; strict GCC and Clang C++17 **PASS**, GCC ASan/UBSan **PASS**,
standalone CTest **2/2 PASS**. Hosted GitHub CI status is separate and must
be checked after the branch is committed.

## Priority next decompilation targets

1. **`0x2206EF20`:** first-stage clearance provider shared by the kind-0
   method and auxiliary wrapper; recover exact pointer/status result and
   original owner/trace dependencies before replacing host abstraction.
2. **`0x22071040`:** post-clearance native followup and identity filter;
   recover the branch that can clear result fields.
3. **`0x22076910` virtual `+0x120`:** identify the genuine entity type
   contract and class implementations. Do not substitute guessed HL2
   categories.
4. Resume original Grunt combat schedule/conditions and client.dll
   gameplay code once these SiN-specific navigation dependencies are
   source-bounded. No cycles on generic HL2 physics, surfaces, props or
   platform porting during this research priority.

**Limits:** No original commercial DLLs, raw assembly dumps, game assets,
executable patches, native boot claims, or complete C++ ABI implementation
are distributed in this phase.