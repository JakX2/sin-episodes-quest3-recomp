# Phase 62: Corrected primary weapon vtables and HideWeapon visibility effects

**Evidence date:** 31 October 2026. **Status:** original-byte and RTTI-verified static analysis of the SHA-256-pinned SiN Episodes: Emergence 2006 Windows x86 `server.dll`/`client.dll`. The source script checks exact build hashes; the original DLLs are **not** distributed. All virtual addresses below belong to the Windows server image, *not* to Android object layouts.

## Critical erratum for Phase 61

Phase 61 resolved `InputHideWeapon`'s virtual call via `CBaseCombatWeapon` `+0x350`, but erroneously took `0x2245A578` as the **start** of the base vtable. In reality, its MSVC RTTI complete-object-locator is immediately before the start at `0x2245A554`, and the base constructor writes this **exact** pointer to `[esi]` at `0x222AE399`. The old address is `0x24` bytes (nine 32-bit slots) into that primary table. Consequently the old lookup returned `0x222AF450` (the shared reload-related function), not the **actual** slot-212 target `0x222AE960`.

This is a **correction**, not a competing interpretation. The earlier owner-handle, selected-weapon equality, and call-site checks at `InputHideWeapon` remain valid. The incorrect reload-method identification is withdrawn, and the Phase 61 verifier, tests, and report were corrected on PR #6.

## Four independent concrete primary-vtable resolutions

The Phase 62 verifier checks MSVC RTTI type-descriptor strings, each complete-object-locator, its pointer immediately preceding the primary vtable, the actual constructor instruction storing the vptr, and slot byte-offset `0x350`:

| x86 class | Constructor vptr store VA | Primary vtable VA | Slot `+0x350` |
|---|---|---|---|
| `CBaseCombatWeapon` | `0x222AE399` | `0x2245A554` | `0x222AE960` |
| `CWeaponMagnum` | `0x223168CC` | `0x2247875C` | `0x222AE960` |
| `CWeaponAssaultRifle` | `0x22313C18` | `0x224776AC` | `0x222AE960` |
| `CWeaponScatterGun` | `0x2231BACD` | `0x2247A1E4` | `0x222AE960` |

**Consequence:** all three main firearms inherit the same implementation of this particular virtual slot in this exact 2006 server build. They do **not** override `+0x350`. This result is significantly stronger than interpreting a bare vtable address without confirming where the constructor points.

## The virtual target is a visibility/effects path

`0x222AE960` accepts an argument read at `0x222AE9C1`. The `InputHideWeapon` handler pushes **zero** before dispatch, which selects the path that:

- pushes bit `0x20` on the weapon object at `0x222AEA4D`, then calls `0x2214C840` at `0x222AEA51`;
- conditionally repeats this operation for a resolved related entity at `0x222AEA5A` / `0x222AEA5E`;
- the callee `0x2214C840` merges the bit into the 32-bit effects word at original entity offset `+0x78` (OR instruction at `0x2214C84D`).

The opposite, nonzero-argument branch clears bit `0x20` from the weapon's effects word (`0x222AE9D1`) and, when present, the related object's effects word (`0x222AEA0C`), subject to additional state checks.

In Source SDK 2013's public `src/public/const.h`, `EF_NODRAW` equals `0x020`. Its entity APIs use `AddEffects(EF_NODRAW)` and `RemoveEffects(EF_NODRAW)` for draw suppression. That is an external naming cross-reference for the original x86 instruction's numerical bit; it does not prove the full 2006 SiN engine has identical behavior in every path.

Sources for flag meaning (not source code copied into this repository):
- <https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/const.h>
- <https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/game/server/baseentity.h>

## Portable, engine-independent host adapter

A small C++17 prototype implements the **observed input guard** in `sin_compat::process_hide_weapon_input`. It uses host-provided entity handles containing separate **index and generation** fields. It rejects invalid weapon and owner handles, checks that the owner's live selected weapon equals the subject weapon, then invokes `IWeaponEntityIOHost::set_weapon_nodraw(weapon, true)` exactly once. It does **not** presume the original x86 layout, fabricate an engine ABI, change headset FOV, or automatically update an associated viewmodel entity. Those require the real SourceVR host implementation and concrete engine testing.

New native files: `include/sin_compat/weapon_entity_io.hpp`, `src/weapon_entity_io.cpp`, `tests/test_weapon_entity_io.cpp`, with CMake integration. The test host verifies stale-generation rejection, invalid owners, not-selected or stale selection, and successful effect requests.

## Repeat the verification

From a checkout containing PRs #1–#6 plus this phase:

```sh
python3 research/tools/verify_weapon_vtable_visibility.py /path/to/SiN/server.dll /path/to/SiN/client.dll
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

**Local checks:** original SiN DLL SHA-256 hashes matched; all four RTTI/constructor/slot chains and effect branches passed. **64 Python unit tests passed**, including **13 new synthetic tests** for the Phase 62 verifier. The C++17 runtime built and **2 CTest executables passed** (original weapon tests and new entity-I/O tests). GitHub CI must be independently checked after PR publication.

## Still unproven

- The precise C++ symbol for `0x222AE960` is inferred from behavior; no exported original symbol identifies it. No real game execution was performed.
- The related-entity resolved via owner/player checks has not been symbolically typed in all paths. We must not assume it is always a viewmodel.
- Direct replay of `HideWeapon` in a running original SiN level, or SourceVR Android build, remains untested.
- The runtime setter of the **unrelated** `m_bReloadsSingly` flag remains open. The Phase 61 incorrect vtable lookup must not be reused as evidence for that setter.
- SourceVR/Android dependency choices and actual engine effect calls are still to be implemented and compiled against the target engine.

**Next target:** follow related-entity resolution inside `0x222AE960` and start a minimally compilable SourceVR-specific adapter after a legal, actual engine API target has been selected. Keep report corrections visible when merging the stacked PRs in order.
