# Phase 66: engine-facing weapon session and R1 dependency audit

**Status:** engine-agnostic C++17 source/test integration. Not a SourceVR-compiled module, Android library or playable Quest 3 build. This phase introduces **no new original-DLL reverse-engineering claims**.

## Goal and scope

Earlier Phases 56–65 provided independently tested weapon state, `HideWeapon` input, indexed viewmodel visibility and viewmodel lifecycle code. Phase 66 composes them into an engine-facing per-weapon `WeaponSession` that can eventually be invoked from a real Source game-module event loop.

The central interface `IWeaponSessionHost` combines existing interfaces for animation playback, guarded entity handle operations, and viewmodel lifecycle. Its `is_live(EntityRef)` must validate entity **index and generation**, never raw 2006 x86 object offsets. No Source headers, game binaries or third-party game assets are copied into this repository.

| Host lifecycle event | New C++ entry point | Engine requirements / caveats |
|---|---|---|
| Player/weapon selected; ensure a model slot | `ensure_viewmodel()` | Confirm selected owner, validate indexed slot, obtain registered `viewmodel` entity and commit owner handle using real engine APIs |
| User command / zoom | `on_usercmd(bits, block)` | Preserve zoom `0x00080000` independently of attacks; engine must map usercmd prediction and tracked VR input |
| Entity input `HideWeapon` | `on_hide_weapon_input()` | Guard owner/selection and apply weapon/viewmodel visibility only for live handles |
| Primary/secondary attack | `on_primary_attack()` / `on_secondary_attack()` | Request animation activities, not complete projectile/fire or combat rules |
| Reload start | `on_reload_begin()` | Must be chosen at a valid host state; no ammo transfer occurs at start |
| Validated reload animation/timing event | `on_reload_transfer_event()` | **Host must validate the actual event**. Once processed, a second event during the same reload cannot transfer again |
| Reload completion | `on_reload_end()` | Clear stage/reload state; host owns actual animation and timing source |
| Holster | `on_holster()` | Clean up zoom/reload, request holster activity; does not prove original engine success semantics |
| Player teardown | `on_player_cleanup()` | Clear both indexed viewmodels in reverse order, not on every weapon switch |

### Integration safety policies, not recovered SiN behaviors

1. Losing weapon selection cancels local pending zoom/reload state without touching a potentially stale viewmodel. VR projection FOV is **never** modified; a synthetic flatscreen host is asked to restore normal FOV if the detached weapon was zoomed. This is a safety policy for porting, not a claim about Ritual's original game.
2. A reload event is consumed once per active reload cycle even if zero ammo transfers. Retrying with new ammo must require another validated reload cycle; this prevents duplicate engine events from duplicating ammunition.
3. Player teardown is distinct from weapon holster and weapon removal. Only explicit player cleanup removes both viewmodel entities.
4. `EntityRef` index and generation are abstract handles. No Android code may assume 32-bit x86 `EHANDLE` layout or the old object's `+0xBDC` / `+0x498` offsets.
5. The fake host's model path, FOV 55 and clip capacity 6 are **synthetic test data**.

## Real-world SourceVR engine target check (31 October 2026)

- Candidate: [tinsarfal/SourceVR](https://github.com/tinsarfal/SourceVR), default `main` public Git tree at `0cf2ab64458abda88006c689f921503666c09b67` when inspected. The README describes standalone Meta Quest/PICO releases tested on Quest 3. The **public Git tree contains only** `.gitignore`, `README.md` and two release-validation documents. It does **not** expose buildable engine/game module sources or headers in this repo snapshot.
- As a separate reference, [Valve's Source SDK 2013](https://github.com/ValveSoftware/source-sdk-2013) publishes game SDK code, but its [SOURCE 1 SDK LICENSE](https://github.com/ValveSoftware/source-sdk-2013/blob/master/LICENSE) is not the MIT license used by our independently authored compatibility code. It is not proven ABI-compatible with SiN 2006 or the target Quest engine. Do not vendor/merge its source into this MIT-only repository as if it were MIT code.

**R1 blocker:** obtain access to a legally usable **specific** Quest-capable SourceVR engine/game-module source revision and build toolchain. Until then, compiling this C++17 library using a fake engine host is **R0 integration**, not R1 acceptance. No in-engine or on-headset execution was tested.

## R1 engine adapter acceptance criteria

1. Identify exact engine source commit, headers, license, Android ABI/toolchain and availability to the maintainers. Document build and asset instructions without distributing proprietary content.
2. Add an engine-specific source file outside the portable library that implements `IWeaponSessionHost` using that branch's real player/weapon/viewmodel interfaces; explicitly validate handles, model count, event callbacks, and engine entity factory registration.
3. Compile and link the engine-specific adapter **against those real headers**. No synthetic stub header should be passed off as the real engine target.
4. Test on a running PC build if possible: create player, select weapon, allocate two viewmodel slots, `HideWeapon` and restore effects, ADS toggle/hold/release, fire, reload event exactly once, holster and teardown.
5. Verify VR head projection remains untouched and explicitly test Quest/ARM64 only after a legitimate engine build is available.
6. Preserve the original game DLLs and game assets outside the public repository.

## Reproduce this phase's local tests

```sh
python3 -m unittest discover -s research/tests -p 'test_*.py'
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Local verification: **112 Python synthetic PE/regression tests** and **five CTest executables** passed. Also validated five CTest executables under AddressSanitizer/UBSan with `ASAN_OPTIONS=detect_leaks=0`. These are mock/prototype tests only; they do not re-verify the original SiN binaries in Phase 66, and do not establish an engine build.

## Next action

A real Quest engine source dependency or SDK integration contract is required before R1 can be declared complete. Then connect a thin adapter to actual `CBasePlayer`, `CBaseCombatWeapon`, `CBaseViewModel`/entity handle and animation callbacks; do not substitute Valve SDK 2013 as an alleged ABI-compatible drop-in.
