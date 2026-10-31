# Phase 65: Original `viewmodel` factory, lazy construction and owner-handle teardown

**Evidence date:** 31 October 2026. **Status:** static verification passed against the exact original 2006 SiN Episodes: Emergence x86 `server.dll` and `client.dll`, identified by SHA-256. No original DLL bytes or game assets are included in the repository. Native C++ integration tests use synthetic entities only.

## Discovery

The Phase 64 owner-side `m_hViewModel[2]` field has identifiable creation, registration and teardown code in the original server DLL. The registration of the name **`viewmodel`** is tied to an original **`CEntityFactory<CBaseViewModel>`** object by RTTI, not solely by name similarity. A player-side routine at `0x2224BEA0` reuses a live indexed viewmodel or requests a new `viewmodel` entity, configures it and installs its EHANDLE in the owner's array. Another routine at `0x2224BF60` processes both model slots in reverse order and calls a removal helper and a handle-update helper. The player constructor at `0x2225397C` and `0x22253982` initializes both handles to the invalid sentinel.

### Original 2006 server evidence

| Item | Server VA | Observed behavior |
|---|---|---|
| Original entity classname | `0x2244F5B0` | Literal `viewmodel` |
| Original runtime RTTI | `0x224EFB88` | `.?AVCBaseViewModel@@` |
| Original factory RTTI name | `0x225183B4` | `.?AV?$CEntityFactory@VCBaseViewModel@@@@` |
| Registered factory object | `0x22518098` | Its vptr is `0x2244F290`; vptr[-1] identifies RTTI locator `0x224C7D94`, whose type descriptor points to the factory RTTI above |
| Entity factory registration | `0x223C1000` | Pushes original classname and factory object for a virtual registration call |
| Player indexed create-or-reuse helper | `0x2224BEA0` | Reads `[owner + index*4 + 0xBDC]`, checks invalid/stale EHANDLE and existing entity, otherwise requests named entity via `0x22266B00` |
| Post-creation configuration | `0x2224BF00`–`0x2224BF35` | Passes owner, slot and parameters to several setup routines; these calls' complete gameplay meaning is **not yet verified** |
| Owner `m_hViewModel` handle assignment | `0x2224BF3A`–`0x2224BF4A` | Reads a handle from the created entity, addresses the owner's array and calls the common indexed handle setter `0x2224B4B0` |
| Viewmodel cleanup routine | `0x2224BF60` | Starts at slot 1 (`+0xBE0`), checks EHANDLE generation before removal helper `0x22286BF0`, then passes invalid sentinel to common handle setter; decrements slot to 0 |
| Player constructor handle initialization | `0x2225397C`, `0x22253982` | Both four-byte fields at `+0xBDC` and `+0xBE0` assigned invalid sentinel (`EDI = -1`) |

**Original field definition:** Phase 64 established `m_hViewModel` has two four-byte EHANDLEs, at server `+0xBDC` and client `+0x1058`. Phase 63 linked the server array to `HideWeapon`. These original offsets and 32-bit serial encodings are research facts, **not** layout requirements for a native Quest 3 port.

## The supported lifecycle

1. Ask the original owner to obtain model slot `index`.
2. A valid, generation-matched existing handle is reused without creating another entity.
3. If absent or stale, request a named entity `viewmodel`, with the registered factory's RTTI tied to `CBaseViewModel`.
4. On success, perform owner/index/model setup and write the resulting EHANDLE to owner `m_hViewModel[index]`.
5. On the observed teardown path, iterate slots **1 then 0**, check EHANDLE generation and entity pointer, call the entity-removal routine on live entries, and update handles using an invalid sentinel.

The original create helper does not visibly protect its indexed access against arbitrary out-of-range slot indices. The portable implementation must enforce an independent host-side bounds check. The original cleanup behavior for stale handles, exact engine removal timing, whether the created entity is spawned/activated by each setup call, and the precise source-level names of those setup functions remain to be established. This is static evidence, not a runtime replay.

## Native-host staging contract

Added `IWeaponViewModelLifecycleHost` and `ensure_viewmodel_slot()` / `clear_viewmodel_slots()` in `include/sin_compat/weapon_viewmodel_lifecycle.hpp` and `src/weapon_viewmodel_lifecycle.cpp`. The host supplies live-entity generation validation, player type, slot count, factory request, configuration, handle commit, removal and clearing. The implementation never hardcodes the 2006 ABI, EHANDLE encoding or old object offsets.

This deliberately safe, engine-neutral implementation extends the original evidence with additional **port policies**:

- Reject unvalidated players and invalid indices before accessing arrays. Guard against obviously broken host slot counts.
- Do not double-create an existing model.
- Roll back a newly created model if configuration or handle assignment fails. This rollback is a portability safeguard, **not a verified original cleanup path**.
- Skip removal of stale/recycled entities and clear stale handles. The exact handling of stale handles by the 2006 path remains different/uncertain.
- Leave actual engine spawn, activation, networking and render lifecycle to the chosen SourceVR host. No `CBaseViewModel` engine code is vendored.

## Reproduce

```bash
python3 research/tools/verify_viewmodel_entity_lifecycle.py /your/original/server.dll /your/original/client.dll
python3 -m unittest discover -s research/tests -p 'test_*.py' -q
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For this checkpoint, both original SHA-256-pinned SiN DLLs passed the Phase 56–64 verification chain and the new verifier. The new verifier checks **28 instruction anchors**, **9 direct calls**, the original class and factory RTTI strings, and the factory object's **vtable-to-RTTI chain**. **18 new Python synthetic fixture/mutation tests (112 total)** passed, along with **four native CTest executables**. These numbers are static/reference tests, **not** proof that SiN has been ported or that Quest 3 engine rendering works.

## Next milestone

Prefer an actual licensed SourceVR host branch and implement `IWeaponViewModelLifecycleHost` with its own entity manager, VR-safe viewmodel rendering and two-player-slot test fixture. Investigate the setup call chain `0x221438A0`, `0x22241250`, `0x22241260`, `0x22287D80`, `0x2214D3C0` in the original game to identify which steps are spawn, activate, owner and slot setup. Verify model spawn/despawn during switching and weapon ownership changes against a legitimate running copy before claiming full campaign compatibility. Keep original code and assets out of public GitHub.
