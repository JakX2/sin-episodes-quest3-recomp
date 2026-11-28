# Phase 1703: recover the 13 original native type-4 array descriptor shapes

## Retail boundary and original source

The owner's unchanged 2006 PE32 `client.dll` SHA256 is
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
The original type-4 array constructor at native `0x24126BD0` is **102 bytes**
with exact source SHA256
`dbfd5065ab7af1f331c11bd90a892adac57943b63a8af72b06e29da18400d8cd`.
Its 13 actual `CALL` destinations were inspected in the original binary,
and their literal argument values and names matched the 211-table/1110-property
network schema from the earlier phases.

The original 60-byte descriptor constructor's x86 instructions are critical:
it zeroes native byte `+44` (`0x24126BE1`), writes the **array element stride**
to `+48` after saving ESI/EDI (`0x24126C13`), and the **element count**
to `+52` before saving registers (`0x24126C01`). Another field, `+28`,
holds its original callback argument (`0x24126C0B`). Thus **native type4
`+44` is NOT the count**. This layout must not be conflated with the
type5 generated child RecvProp `+44` offsets from Phases1697/1699.

## 13 authenticated properties

| Original static table | Property | Count | Stride |
| --- | --- | ---: | ---: |
| `DT_PropVehicleDriveable` | `m_hPassengers` | 3 | 4 |
| `DT_TEBeamSpline` | `m_vecPoints` | 16 | 12 |
| `DT_BaseViewModel` | `m_flPoseParameter` | 24 | 4 |
| `DT_Beam` | `m_hAttachEntity` | 10 | 4 |
| `DT_Beam` | `m_nAttachIndex` | 10 | 4 |
| `DT_BasePlayer` | `m_hObserverViewModel` | 2 | 4 |
| `DT_LocalPlayerExclusive` | `m_hViewModel` | 2 | 4 |
| `DT_EnvScreenOverlay` | `m_iszOverlayNames` | 10 | 255 |
| `DT_EnvScreenOverlay` | `m_flOverlayTimes` | 10 | 4 |
| `DT_Team` | `"player_array"` (literal original name including quotes) | 32 | 0 |
| `DT_Ragdoll` | `m_ragAngles` | 24 | 12 |
| `DT_Ragdoll` | `m_ragPos` | 24 | 12 |
| `DT_ScriptIntro` | `m_flFadeColor` | 3 | 4 |

The unusual `DT_Team` stride zero is **original source evidence**, not a
generic byte-copy array. It passes a specialized **original executable
callback at `0x240F76B0`**. That callback has NOT been ported, and the
owned-token image intentionally stores a null, non-executable callback
rather than the unusable retail PE32 address.

## Source and test implementation

`include/sin_compat/original_client_native_type4_array_fields.hpp`
retains all 13 property ordinals, native slot indices, exact original
property names, counts and strides. Its compile-time check requires the
complete original native type4 property inventory to match these 13
distinct routes, with exactly one special callback exception.

`original_client_native32_recvtable_image.hpp` now writes the
source-backed type4 fields at `+0` (owned copy of name),
`+44=0`, `+48=stride`, `+52=count`. The native type4
source callback `+28` stays null in the simulated image. The
root-scoped `AuditVisitedSourceTable` validates all those source
values, not only the numeric type tag. Existing `VClient011.slot5`
remains deliberately blocked and `CanExposeToRetailEngine()==false`.

`tests/test_original_client_native32_recvtable_image.cpp` confirms all
13 exact fields, and mutates valid-but-source-incorrect stride, count,
+44 and network property names. It also checks that a non-executable
original pointer cannot be silently installed at `+28`.
Previously recovered **1,082 canonical member offsets**, **14 type5
generated parent base fields** and the **13 native type4 fields** now
account for `+44` of **1,109 / 1,110** static properties. The one
specialized animation overlay remains unresolved.

The original-free audit is wired into the Phase1700 strict C++17,
ASan/UBSan, MSVC Win32 x86 and Android build/emulator test target:

```sh
python research/tools/audit_sin_original_client_type4_arrays.py --self-test
```

The exact owner-private source check, without uploading DLL bytes to GitHub:

```sh
python research/tools/audit_sin_original_client_type4_arrays.py --client /private/client.dll
```

The retail Windows original engine was not retested, and this work
does not implement original packet processing, array proxy callbacks,
network table lifetime, rendering, physics, or Android/Quest 3 gameplay.
