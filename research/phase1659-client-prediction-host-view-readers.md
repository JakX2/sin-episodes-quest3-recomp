# Phase 1659: authenticated CPrediction host scalar and view-vector reads

## Scope and provenance

This phase is a **read-only original 2006 PE32 audit plus an actual
original-free MSVC x86 C++ runtime regression**. It is stacked on
Phase1658 PR #1667 at SHA `6c1438064cddc3d670e3b04c966cf5ae8f7f1ac3`.
It does NOT enable the replacement client's incomplete
`VClientPrediction001` factory or make a retail Windows engine playable.

Original `client.dll` complete SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
Original `engine.dll` complete SHA256:
`f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.
Both are authenticated by the existing Phase1658 owner-only audit
before Phase1659 original-method inspection. No commercial bytes,
machine-code body, original engine object or DLL image are in Git.

## Exact newly examined virtual methods

The original 23-slot `CPrediction` vtable resides at
`0x242A363C` and is already fully authenticated in Phase1658.
This phase examines **two more** virtual targets with their complete
method SHA256, sparse x86 structural witnesses, relocation evidence
and separate synthetic native behavioral tests.

| Slot | Original function | Size | Complete original SHA256 |
|---:|---|---:|---|
| 8 | `0x24195DD0` | 17 bytes | `a96aab9ac13d897688080ecbf8ae113f179c813ea30a0723237ec3899b93ff26` |
| 13 | `0x24195F10` | 59 bytes | `702860c2c345ff06d4534b1a1f2d820f0d098d994f2662be466b027813c0f40f` |

Both use the same original client-data global pointer at
`0x24338CF0` (a host object, **not** the `cl_predict` ConVar
from Phase1658). Both native instruction streams have HIGHLOW
relocations on their load of this global.

### Slot 8: scalar DWORD getter

Slot 8 first loads the global host pointer, tests it for zero,
then returns:

- **host absent:** return `0`, plain `RET`, zero explicit parameters.
- **host present:** return DWORD at `host+0x1DC`, plain `RET`.

The method does not use prediction `this` beyond the x86 virtual-call
convention. This phase names the offset, **not** an unproven semantic
field name. No executable host class is inferred.

### Slot 13: three-DWORD out-pointer view getter

Slot 13 receives **one 32-bit pointer argument**, to an output
span of exactly three DWORDs, and returns with `RET 4`.
Its original code branches on the same host global pointer:

- **host absent:** writes zero to the output's three DWORDs at
  offsets `0`, `4`, and `8`.
- **host present:** copies three consecutive DWORDs from
  `host+0xF04`, `host+0xF08`, and `host+0xF0C` to output
  offsets `0`, `4`, and `8`.

The native code copies raw 32-bit bit patterns; the synthetic
regression deliberately includes IEEE-754 infinity/NaN bit patterns
and checks that they remain unchanged. The original does not check
for a null output pointer. The independent model **refuses null**
rather than reproducing the retail null-pointer crash.

## Implementation and tests

`pc/independent_slot0/native_client_prediction23_init_probe.h` now
provides a separate invented `HostSnapshot` buffer that statically
places the DWORD at +`0x1DC` and three-DWORD view at +`0xF04`.
The compiler-generated 23-slot diagnostic prediction fixture
retains source-authenticated slot indices and adds two real x86
`__thiscall` entries at slots 8 and 13:

- `GetHostField1DC()` returns the injected host DWORD or zero.
- `CopyHostView(ViewBits3*)` copies three DWORDs to the provided
  output, defaulting to zero when the host is missing, refusing a
  null output and never dereferencing external game state.

`test_original_client_prediction_host_view_slots.cpp` exercises
both methods through **actual MSVC x86 vtable function addresses**,
passing `this` through ECX and verifying the original `RET0` versus
`RET4` stack cleanup with before/after ESP. It checks both source
field offsets, guard DWORD canaries around the output, bit-exact
three-DWORD copying, missing-host zero output, missing-output refusal,
host teardown, no unexpected prediction initialization, and native
fixture lifetime.

`research/tools/audit_sin_original_client_prediction_view_readers.py`
authenticates both original whole-function hashes and sparse
opcode/operand structure, confirms native relocation sites and
non-method padding, and reuses the full original dual-DLL
Phase1658 SHA gates. Public `--self-test` constructs invented
x86 byte windows and mutates every byte; it never includes original
commercial method bodies or needs the retail game files.

Two new CTests are registered, raising the stacked native target
from **265** to **267** tests. The original decomp synthetic
workflow also runs the self-test on the expected paths.

## Explicit non-goals

This adds **two previously unreviewed full original method extents
(76 bytes)**, distinct from Phase1658's original prediction methods.
Other large prediction slots and engine host construction/lifetime
remain unresolved. This separate host snapshot does not constitute
a real Source engine C++ service implementation.

There is no production `VClientPrediction001` CreateInterface
registration, original client engine call, owned Windows gameplay
test, Android/Quest 3 startup or physics/model/prop/surface work.
CI must pass on the final PR SHA before marking this phase verified.
