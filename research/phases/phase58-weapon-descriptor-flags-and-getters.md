# Phase 58: SiN weapon descriptor flags and virtual map getters

**Scope:** original 2006 `server.dll` and `client.dll` as identified by the SHA-256 digests in `research/binary-provenance.json`. This is a *static metadata investigation*, not a recovered original C++ implementation, an operating SiN module, or a runtime proof of prediction or savegame behavior. All VAs below are original preferred image-base addresses.

## Findings verified against both original DLLs

The 52-byte `typedescription_t`-style records expose two adjacent 16-bit quantities at record byte offset `+0x10`: a field size and a bitmask of field flags. In this build every inspected **static** weapon descriptor has `fieldSize=1`.

| Registered group | Number of *static* records inspected | Flags (`typedescription_t::flags` interpretation) |
|---|---:|---|
| `server_matched` (`0x2251F864`) | 29 | 15 × `0x0002`, 12 × `0x0100`, 2 × `0x0900` |
| `client_matched` (`0x242F62FC`) | 29 | 15 × `0x0002`, 12 × `0x0100`, 2 × `0x0900` |
| `server_extended` (`0x2251FFEC`) | 32 (of 42 declared) | 30 × `0x0002`, 2 × `0x0006` |

The first two groups are mirrored **by field name, type, ordering, *and flags***, even though their actual object-field offsets differ as Phase 57 documented. Source's public SDK defines the flags as:

- `0x0002`: `FTYPEDESC_SAVE`;
- `0x0004`: `FTYPEDESC_KEY`;
- `0x0100`: `FTYPEDESC_INSENDTABLE`;
- `0x0800`: `FTYPEDESC_MODELINDEX`.

This provides *strong structural evidence* that the matched descriptor group serves a prediction-oriented field description (it mixes sent-table fields with private saved fields), whereas the extended server group is a save/data-map-oriented description (all its static entries have `SAVE`, and two also have `KEY`). The original SiN engine's actual consumers, invoked callbacks and network/save operations **have not been traced**; the Source SDK names and semantics are a separate, later reference, not SiN source code.

The two fields with flags `0x0006` (`SAVE|KEY`) in the extended server list are `m_overridePrimaryAmmoCount` and `m_overrideSecondaryAmmoCount`. Their flags do **not** establish how or when the engine accepts key/value input during normal gameplay.

### Why it matters for reload research

`m_bReloadsSingly` is marked `0x0002` (`SAVE`) in **all three** descriptor groups. Therefore its presence in a mirrored server/client descriptor group does **not** itself prove that it is in a live send/receive table, or that the Magnum, assault rifle or Scattergun enable single-round reloads. In fact the flag means the opposite of `INSENDTABLE` in this descriptor format. The runtime writer of `weapon+0x508` remains unresolved.

A distinctive finding: the mirrored lists each have **two** descriptors named `m_flTimeWeaponIdle` at indices **7 and 13**, referencing the same object offset in their respective DLL. Both copies have flag `0x0100` (`INSENDTABLE`); these are two actual, separate original descriptor entries, not a string-search duplicate. Their purpose is still unknown and we should not silently deduplicate them when reconstructing these tables.

## Virtual getter evidence

The original RTTI-identified `CBaseCombatWeapon` server vtable at `0x2245A578` contains:

- virtual slot **0**: `0x222AC4E0` (a 6-byte `mov eax, 0x2251F7F4; ret` getter for the matched group);
- virtual slot **4**: `0x222AC540` (a 6-byte `mov eax, 0x2251F810; ret` getter for the extended group).

The corresponding client `C_BaseCombatWeapon` vtable at `0x242845DC`, slot **0**, points to `0x24090EA0`, which returns `0x242F6260`, the mirrored client group. The returned records contain class-name pointers to `CBaseCombatWeapon` and are initially unpopulated at the field-array/count offsets on disk, until the registration initialization stores verified in Phase 57 run. These are independent compiled getters, not proof of every subsequent consumer.

## Source SDK reference (external corroboration, not original SiN source)

- Source-style flag definitions: https://github.com/lateleite/source-sdk-cso2/blob/master/public/datamap.h
- Valve's Source SDK `BEGIN_PREDICTION_DATA` / `DEFINE_PRED_FIELD` examples: https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/client/c_baseplayer.cpp
- Source `BEGIN_DATADESC` + `DEFINE_FIELD` examples: https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/hl2/weapon_rpg.cpp

These later SDK implementations support the **interpretation** of flag values and descriptor categories; 2006 SiN binaries remain the source of the numeric observations.

## Reproduce

After merging Phase 56 and Phase 57's tooling, against **your own legitimate copy** of SiN's original Windows DLLs:

```sh
python3 research/tools/verify_weapon_descriptor_semantics.py /path/to/SiN/server.dll /path/to/SiN/client.dll
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
```

The standalone standard-library verifier validates pinned DLL digests, Phase 56 field identities, Phase 57 registration and array structure, packed flags, all 29 mirrored entries including duplicates, two `SAVE|KEY` fields, and three vtable getter identities. It emits no game data. Synthetic Python unit tests require no game files.

**Verification completed 31 October 2026:** real-binary verification passed for original SiN server and client; the new eight Python synthetic tests passed. This does not establish a working engine/VR port.

## Next: Phase 59

Follow the prediction-style and save-style map getters to **actual consumers** (virtual calls and registration functions, rather than merely describing table signatures). Separately track writes to `m_bReloadsSingly` with alias-aware analysis or live instrumentation in an owned copy. For runtime correctness we need to decide which fields are copied, saved, predicted or transmitted, not just which flags are declared.
