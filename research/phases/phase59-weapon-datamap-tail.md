# Phase 59: Reconstruct the ten late-initialized `CBaseCombatWeapon` server datamap descriptors

**Status:** original-DLL static verification completed, 31 October 2026. This is a byte-accurate *initializer investigation*, not live save/restore, entity I/O, network prediction, or Quest 3 gameplay validation.

## Why this phase matters

Phase 57 verified a `CBaseCombatWeapon` server datamap at `0x2251F810` whose descriptor pointer is `0x2251FFEC`, with **42** entries. Only the first **32** entries have fixed descriptors in the PE file; the final ten occupy zero-initialized storage and are populated by `server.dll`'s startup code. Phase 58 interpreted the static descriptors as save-style rather than prediction-style but did not recover the ten dynamic descriptors. **Phase 59 reconstructs these ten registrations from their original x86 initializer stores.** All VA addresses below are preferred image-base VAs of the original x86 Windows binary, *not* Android pointers.

## Original-binary observations

- The initializer ranges from `0x222B0940` to `0x222B0E22`, with the tail's field storage beginning at `0x2252066C` and ending with index 41 at `0x22520840` (stride **52 bytes**).
- For indices **32–37**, named C strings are each passed by a `push` immediate to a helper at `0x22044E50` via a relative `call`. The return in `eax` is stored at descriptor `+4` (the name pointer), so the *final pointer value is generated at runtime*. Each corresponding function pointer is stored into descriptor `+0x1C`. The initializer also assigns size **1**, flag **`0x20`** and zero-valued field type/normal offset.
- Index **38** contains a named input handler, with distinct internal descriptor name `InputHideWeapon` and external input name `HideWeapon`. Its flag is **`0x08`** and its handler pointer is set at `+0x1C`.
- Indices **39–41** contain typed output fields with object offsets, internal and external names, and flag **`0x16`** (`0x02 | 0x04 | 0x10`). The initializer uses type ID **11** for these outputs and size **1**. The exact value of the runtime custom save/restore ops pointer is not reconstructed here.

| Index | Kind | Internal name | External name | Server offset / handler VA | Flags |
|---:|---|---|---|---|---:|
| 32 | function | `DefaultTouch` | n/a | handler `0x221725F0` | `0x20` |
| 33 | function | `FallThink` | n/a | handler `0x222AAE90` | `0x20` |
| 34 | function | `Materialize` | n/a | handler `0x222AC5C0` | `0x20` |
| 35 | function | `AttemptToMaterialize` | n/a | handler `0x222AAC00` | `0x20` |
| 36 | function | `DestroyItem` | n/a | handler `0x222AB830` | `0x20` |
| 37 | function | `SetPickupTouch` | n/a | handler `0x222AC5B0` | `0x20` |
| 38 | input | `InputHideWeapon` | `HideWeapon` | handler `0x222AD800` | `0x08` |
| 39 | output | `m_OnPlayerUse` | `OnPlayerUse` | object `+0x524` | `0x16` |
| 40 | output | `m_OnPlayerPickup` | `OnPlayerPickup` | object `+0x53C` | `0x16` |
| 41 | output | `m_OnNPCPickup` | `OnNPCPickup` | object `+0x554` | `0x16` |

These slots are **not** ten hidden reload fields, and they must not be reconstructed as if they were. They are function, input, and output records participating in the Source-style entity datamap pattern.

## Cross-reference with public Source definitions

The original binary stores numeric flags. Public Source SDK `datamap.h` uses the following names for the same bit values:

- `0x20` `FTYPEDESC_FUNCTIONTABLE` (`DEFINE_FUNCTION`-style member callbacks);
- `0x08` `FTYPEDESC_INPUT` (`DEFINE_INPUTFUNC`-style entity input);
- `0x16` `FTYPEDESC_OUTPUT | FTYPEDESC_SAVE | FTYPEDESC_KEY` (`DEFINE_OUTPUT`-style entity output).

The `FIELD_CUSTOM`-style type ID 11 matches the Source `fieldtype_t` ordering in the cited header, but the original SiN symbols are not reconstructed from a source-level debug database. The published header is corroboration for *interpretation*, not proof that the game's actual engine executes identical code.

SDK reference: https://github.com/lateleite/source-sdk-cso2/blob/master/public/datamap.h

## Reproducibility

With **original, legally obtained**, SHA-256-matched SiN `server.dll` and `client.dll`:

```sh
python3 research/tools/verify_weapon_dynamic_descriptors.py /path/to/server.dll /path/to/client.dll
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
```

The Python verifier first executes all Phase 56–58 checks, including both binary SHA-256 identities, before checking ten metadata slots. It validates original `.rdata` C strings, bounded initializer instructions, six helper-call targets, helper return stores, static function pointers, input and output external-name stores, offsets, field sizes and type/flag register setup. It does **not** execute original Windows code. The synthetic tests intentionally replace instructions to verify rejection of missing or altered entries. No copyrighted DLLs, disassembly dumps or game asset bytes are included.

**Local checks:** both original SiN DLLs passed Phase 59's verifier; 28 Python synthetic tests including dependencies passed; standalone Linux C++17 CMake build and CTest passed. GitHub CI result is reported separately in the PR, not assumed in this report.

## Next investigation (Phase 60)

1. Identify and validate actual *consumers* of the two weapon map getters (`server.dll` virtual slots 0 and 4, and the client counterpart) rather than assuming every field is used during gameplay.
2. Independently search for alias-aware writes to `m_bReloadsSingly` (`server weapon+0x508`), separating constructor/save-restore/pointer-alias writes from normal reload-selection logic; do not conclude single-shell reload is active without such evidence.
3. Begin a host-compatible entity I/O event adapter and test using synthetic Source-like events while retaining these field names, *without* hardcoding Windows x86 member-function pointers in ARM64 code.
