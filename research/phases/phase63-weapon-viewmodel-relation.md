# Phase 63: Indexed related-entity lookup in weapon visibility

**Evidence date:** 31 October 2026. **Status:** verified against user-owned, SHA-256-pinned original 2006 SiN Episodes: Emergence Windows x86 `server.dll` and `client.dll`. This is static analysis only. No copyrighted binaries, game assets or original function dumps are published.

## Summary

We have significantly narrowed down the related entity in the Phase 62 `HideWeapon` visibility routine. The original weapon metadata explicitly names the integer field `m_nViewModelIndex` at server weapon offset `+0x498`. `0x222AE960` reads exactly that field, then uses the owner-side entity array helper at `0x2224AF00` to resolve another **generation-checked entity handle**, conditional on a virtual owner predicate at `+0x120`. The same helper and index are independently used by a weapon visibility checker at `0x222AD7CE`, which examines the related entity's effects bit `0x20`.

Taken together, these facts make the related entity **strongly consistent with the player's indexed viewmodel**. The public Source SDK's `CBasePlayer::GetViewModel(index)` uses player-owned `m_hViewModel[index]` handles and the shared weapon system exposes `m_nViewModelIndex`. This is a corroborating analogy, **not** independent proof of the original 2006 SiN concrete object class, vtable member names, maximum valid index, or Android ABI.

## Exact SiN original-binary evidence

| Evidence | Verified 2006 server VA or offset | What it establishes |
|---|---|---|
| `m_nViewModelIndex` string | `0x2244F26C` | Original field name is present |
| Weapon field descriptors | `0x2251FAD4`, `0x225203FC` | Both name the same type-5 integer field at weapon `+0x498` |
| Weapon visibility routine | `0x222AE960` | Owner handle read at weapon `+0x494`; conditional related lookup; weapon and optional related entity receive effect bit `0x20` on zero-argument path |
| Owner conversion | `0x222AE997` | Owner virtual `+0xFC` |
| Owner condition | `0x222AE9A7` | Boolean return from virtual `+0x120` must be nonzero to attempt related lookup |
| Indexed lookup argument | `0x222AE9B1` / `0x222AE9BA` | Reads weapon `+0x498`, passes it into `0x2224AF00` with owner-like receiver |
| Related-entity helper | `0x2224AF00` | Reads `[owner + index * 4 + 0xBDC]` into an entity handle, checks invalid sentinel, masked index and generation before returning pointer/null |
| Related effect application | `0x222AEA56`–`0x222AEA5E` | Non-null related entity receives effect bit `0x20` via `0x2214C840` on hide path |
| Independent visibility check | `0x222AD7CE`–`0x222AD7E8` | Again reads `weapon+0x498`, resolves via `0x2224AF00`, tests the returned entity's effect bit `0x20` |

All VA/offset values describe the original PE32 server executable, **never** native Android structs. The helper does not show a robust array-index range check; an engine adapter must independently check bounds instead of copying that risk.

### Interpretive limits

- The name `m_nViewModelIndex` is original SiN metadata. The precise owner array member name **is not** independently proven from a typed descriptor in these traces; it is a viewmodel interpretation corroborated by the public Source SDK.
- The virtual owner condition at `+0x120` has not yet been tied to a named original symbol. A host may conservatively require a validated player owner before touching its viewmodel.
- There is no proof every weapon uses slot zero. No host should hard-code zero, or assume a numeric array offset or 32-bit handle encoding in ARM64.
- No original game runtime test or working SourceVR engine adapter has been performed. This is evidence-backed compatibility logic, not a complete port.

## Portable implementation

Added `IWeaponViewModelHost` and `process_hide_weapon_input_with_viewmodel()` as an **opt-in** extension to the existing Phase 62 host-neutral `IWeaponEntityIOHost` interface. It:

1. Retains live-weapon, live-owner and exact selected-weapon equality checks from Phase 62.
2. Requests the host's weapon viewmodel index and checks it against the host's actual allowed range.
3. Accepts a related model only with a live index+generation and an explicit host ownership/type confirmation.
4. Applies `set_weapon_nodraw(weapon,true)` and, when a valid related viewmodel exists, `set_viewmodel_nodraw(model,true)`.
5. Still hides the weapon if there is no valid player/viewmodel. Missing or stale models must not accidentally receive effects.

There is no literal `+0xBDC`, `+0x498`, x86 vtable offset, or Source SDK code in the native adapter. Its host must translate these operations using the **actual** chosen engine's interfaces. It is a mock-tested staging interface, not SourceVR-compiled integration.

Files: `include/sin_compat/weapon_viewmodel_io.hpp`, `src/weapon_viewmodel_io.cpp`, `tests/test_weapon_viewmodel_io.cpp`, plus `CMakeLists.txt` update.

## Reproduction and tests

```sh
python3 research/tools/verify_weapon_viewmodel_relation.py /path/to/original/SiN/server.dll /path/to/original/SiN/client.dll
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Local results: both original DLL SHA-256 values matched the established Phase 56 reference. Phase 56–62 validation chain still passes; **26 anchored instruction checks**, two original field descriptors, 2 direct call targets, **14 new synthetic PE mutation/positive tests**, **78 total Python tests**, and **3 CTest executables** passed. Three CTest executables also passed in an AddressSanitizer+UBSan build in this local container (with leak detection disabled). These tests exercise fake hosts and static signatures, **not** a running game or Quest APK.

## Reference for name interpretation only

- <https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/player.cpp> (`CBasePlayer::GetViewModel` implementation)
- <https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/shared/basecombatweapon_shared.cpp> (weapon viewmodel index)
- <https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/const.h> (`EF_NODRAW = 0x20`)

**Next:** verify concrete original owner class and owner-array types via additional RTTI/vtable/datagroup references where possible, then build a real adapter against a selected, legally usable SourceVR engine branch. Review and merge the stacked research PRs #1–#8 in dependency order before claiming everything is integrated into `main`.
