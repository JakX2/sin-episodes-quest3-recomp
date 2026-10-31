# SourceVR host adapter: milestone R1 acceptance checklist

**Status:** proposed integration plan, not an implemented engine adapter.

This document turns [the existing roadmap](ROADMAP.md) milestone **R1** into a testable engineering task. It is based on the current C++17 [`IWeaponHost` interface](../include/sin_compat/weapon_runtime.hpp), [`WeaponRuntime`](../src/weapon_runtime.cpp), and [mock-host tests](../tests/test_weapon_runtime.cpp). It does **not** claim those components have already run in SourceVR, on Android, or in a Quest headset.

## 1. Pin the actual engine target

- [ ] Identify the specific **SourceVR branch, commit, platform, supported compiler and SDK/license** that will host the SiN compatibility layer.
- [ ] Document whether that branch supports the needed game DLL/module API, animation activities, prediction/usercmd flags, viewmodel lifecycle, and save/load/network data.
- [ ] Compile the minimal empty game-module/adapter against **real headers from that branch** before porting any more SiN systems.
- [ ] Do not copy SiN 2006 Windows vtable indices, x86 object offsets, or raw disassembly into an ARM64 plugin as though they were a stable engine ABI.

**Acceptance:** a reproducible build target identifying the exact upstream revision and an adapter that can load in the selected engine without touching proprietary game DLLs.

## 2. Connect the existing `IWeaponHost` boundary

The prototype deliberately exposes only four host calls:

| Method | Required engine-side behavior | Test |
|---|---|---|
| `resolve_sequence(activity_id)` | Resolve a real animation activity for the active viewmodel; return `-1` if missing | Unknown activity must not bind/play an invalid sequence |
| `bind_viewmodel(model, origin_offset)` | Configure the selected game's lawful viewmodel and its **general** origin offset | Called only after a valid sequence; don't assume origin offset is ADS pose |
| `play_sequence(sequence_id)` | Request the actual validated viewmodel sequence | Observe the correct sequence in a running engine |
| `set_flatscreen_fov(fov, duration_seconds)` | Apply **flatscreen-only** FOV requests | Must never change the HMD camera FOV in VR mode |

- [ ] Implement these methods in a separate host adapter, keeping `WeaponRuntime` portable.
- [ ] Map activities from the chosen target branch, **not** numeric 2006 vtable offsets or guessed animation names.
- [ ] Exercise missing sequence, missing owner, invalid weapon, weapon removal, and map transition cases.

**Acceptance:** existing mock-host tests continue to pass, plus one real-engine sequence/bind/play smoke test.

## 3. Input and sighting

- [ ] Preserve original independent actions: primary attack, secondary attack, `+zoom` (`0x00080000`), and quick grenade (`0x02000000`).
- [ ] Test button press, hold, release and interruptions with target engine usercmd/prediction behavior. Do not equate the Magnum DUAG secondary shot with ADS.
- [ ] In non-VR mode, use verified requested FOV timing as an initial reference: **0.25 seconds into zoom** and **about 0.15 seconds out**. The real weapon-script `ZoomFOV` values are still needed.
- [ ] In VR mode, keep HMD projection and head tracking under the VR runtime. Sight alignment should be implemented from tracked controller/weapon transforms, **not by narrowing headset FOV**.
- [ ] Resolve the original game's `m_bZooming` versus `m_bZoomScope` state transitions before designing final server-to-client replication.

**Acceptance:** primary, secondary, aiming and grenade input remain distinct; ADS entry/exit and cancellation work during movement, firing, reload and weapon switch.

## 4. Reload, ammo and ownership

The existing `WeaponRuntime` deliberately calls `transfer_bulk_at_validated_event` and `process_one_round_at_validated_event` **only when the host says a valid animation/timing event occurred**.

- [ ] Determine the actual engine animation-event callback that should trigger ammo transfer.
- [ ] Verify primary and secondary clip/reserve accounting against real SiN weapon script values.
- [ ] Keep conservative single-round reload disabled unless the original game and target engine justify enabling it; the enabling path remains unproven.
- [ ] Verify holster, interruption, death, owner loss and save/load restore behavior.
- [ ] Do not treat test fixture capacity `6`, FOV `55`, or model name from `tests/test_weapon_runtime.cpp` as recovered original values.

**Acceptance:** no ammo duplication or early refill, including a reload interrupted before the validated transfer event.

## 5. Integration validation

- [ ] Run existing portable CMake/CTest suites on Linux and, if supported, on the target toolchain.
- [ ] Add an engine-host smoke test with: spawn weapon → enter ADS → fire → reload → interrupt → holster → switch weapon.
- [ ] Keep separate evidence columns for **confirmed DLL finding**, **SourceVR adapter design**, **mock test**, and **in-engine observed result**.
- [ ] Add an issue or failing test for every behavior the engine cannot yet reproduce instead of silently substituting HL2 defaults.
- [ ] After R1 passes, advance to R2: original legally owned weapon scripts, viewmodels, actual animations, and a reproducible single-level PC test.

## Repository/IP hygiene

Never commit `server.dll`, `client.dll`, game archives, maps, textures, models, decrypted/extracted proprietary assets, license keys, or private game-installation paths. Publish independently authored code, small verified data tables, reproducibility scripts and research notes that clearly distinguish evidence from inference.

## R1 completion gate

R1 is **not complete** until the compatibility code compiles against an identified real SourceVR branch and runs at least one observable weapon lifecycle in that engine. Passing mock-host C++ tests alone is R0, **not** a playable SiN port or Quest 3 build.
