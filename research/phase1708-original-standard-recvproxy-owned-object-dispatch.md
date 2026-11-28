# Phase 1708: run 999 source-standard 2006 PE32 RecvProxy identities on owned object bytes

## Strict retail boundary unchanged

The last authentic original Windows engine invocation of the independent
reconstructed `client.dll` stopped deliberately at
`VClient011.slot5` rather than exposing an incomplete native
`ClientClass` network registry. This branch does not install an
independent DLL in the user's game, execute original commercial PE32
functions, decode real Source network packets, or run Android/Quest 3 gameplay.
Physical model/rendering/props/surfaces remain excluded as requested.

The source is the unchanged owner-original `client.dll`
SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## Last missing standard source field: all 12 STRING destination capacities

Original native type3 constructor `0x24126AD0`, 117 executable bytes,
copies the literal string-size argument into original RecvProp `+12`.
The actual twelve source callsites and their recorded table/slot/name
destinations were recovered with precise source evidence. Their size
values are **18, 32, 64, 255, or 256 bytes**, per original property,
not a blanket guess based on a newer HL2 SDK.

Original constructor-order source records (12 tuples of
`(tableOrdinal:uint8,propertySlot:uint8,bufferSize:uint16LE)`),
48-byte SHA256:
`62b1a1a412724fbd51a2556545af0c28b8819ee1427ece1b6f7d1e4d5e5b5292`.

| Original source property | Table ordinal / slot | Source buffer bytes |
| --- | --- | ---: |
| `DT_CPropKeypad.m_sInput` | 40 / 2 | 32 |
| `DTKeypad.m_combination` | 47 / 2 | 255 |
| `DT_AR2Explosion.m_szMaterialName` | 62 / 1 | 255 |
| `DT_Tesla.m_SoundName` | 80 / 1 | 64 |
| `DT_Tesla.m_iszSpriteName` | 80 / 2 | 256 |
| `DT_Team.m_szTeamname` | 81 / 2 | 32 |
| `DT_Env* material variables` | 98 / 1–3 | 255 each |
| `DT_Sprite.m_MaterialName` | 101 / 3 | 255 |
| `DT_EnvScreenOverlay.m_iszOverlayNames[0]` | 112 / 1 | 255 |
| `DT_BasePlayer.m_szLastPlaceName` | 118 / 14 | 18 |

`original_client_source_string_sizes.hpp` retains all 12 exact sizes,
with compile-time checks ensuring they cover every original type3
descriptor, none duplicated and all bound to the proven original
string receiver VA `0x24126CA0`.

`original_client_native32_recvtable_image.hpp` now writes the
12 source-proven `+12` size DWORDs into its owned token image and
rejects mutation of those sizes. It deliberately does not pretend
unproven fields in other native property families are fully initialized.

Original-private and public regression audit:

```sh
python research/tools/audit_sin_original_client_string_sizes.py --self-test
python research/tools/audit_sin_original_client_string_sizes.py --client /private/client.dll
```

The latter checks each original x86 CALL, type3 helper destination,
literal name, source member offset, buffer-size PUSH operand, zero flags
and standard string callback argument. No original executable bytes
are checked in.

## Executable independent host-only property router

`original_client_standard_recvproxy_router.hpp` combines the proven
**1,110 property native type tags**, **1,110 +44 member offsets**,
**1,082 canonical source proxy callback VAs**, all **12 string capacities**,
the Phase251 source-standard five primitive copy receiver semantics
and the Phase1707 source-standard string/datatable behaviors.

The independent method
`ApplySourceStandardProperty(tableOrdinal,slot,input,objectBytes,outTableToken)`
selects strictly by a *source-verified callback identity* and writes only
within an owned byte-vector. It refuses out-of-range field offsets,
short strings, nonexistent source records or sizes. Data-table callbacks
only transfer host-owned 32-bit tokens; they never cast a retail process
address to a runnable pointer. Every original custom callback and every
special original constructor returns a precise
`UnsupportedSourceCallback`, never a silent `memcpy` fallback.

The cross-platform native test verifies:

- Byte, WORD, integer DWORD, float and vector3 decoded values are copied
  into the *source-correct original member offsets*, retaining neighboring bytes.
- Source type3 strings obey the exact original capacity and forced
  NUL termination, with transactional refusal on truncated source.
- Original type5 datatable callbacks assign an owned 32-bit token rather
  than fabricating entity pointers.
- All **999** source-standard static property callback assignments
  apply successfully to sufficiently sized fabricated object storage,
  while the **83 custom** and **28 special** static routes are
  refused and leave storage unchanged.
- Bad class/slot indices and undersized host objects refuse.
  No original callback VA or unsafe native `ClientClass*` becomes runnable.

The test runs as a new CMake CTest on native Win32 and portable builds;
the focused original network schema workflow builds it with strict
C++17 and AddressSanitizer/UndefinedBehaviorSanitizer. These are
**original-free, host-owned decoded-value tests**, not real game loading.

## Next critical runtime boundary

Reconstruct the original 83 **custom** RecvProxy behaviors individually,
including handle ownership, interpolation, simulation clock and
player/weapon-specific callbacks, then the 28 specialized
constructor/overlay paths. Restore executable 2006 `ClientClass`
entity/temp-event creators and native table lifetime rules before
ever reconsidering the deliberately guarded retail `VClient011.slot5`.
The current original object registry is still a simulated 32-bit
arena, not a fully compatible engine ABI or Quest game runtime.
