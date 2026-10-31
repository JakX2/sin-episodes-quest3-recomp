# Phase 56 checkpoint: codebase bootstrap

## Evidence boundary

The phase research archives contain byte excerpts and verification scripts for the original SiN DLLs. This repository vendors **reference code**, not binary disassembly as drop-in C++ source. No new reverse-engineering claims are made by this scaffold. The mock host simulates engine calls; its behavior does not prove the original game or Quest 3 has been executed.

### Source evidence included in references

- Phase 46: zoom button rising-edge model and shared ADS transition model.
- Phase 47: normal versus zoomed activity requests; assault-rifle recoil activities.
- Phase 51: valid animation sequence before viewmodel model/offset binding and playback.
- Phase 52: draw/holster contract and ADS-exit ordering.
- Phases 53–55: guarded clip and reserve transfer; reload stage, mode508 default and conditional single-round branch.

The eight reference headers in `third_party/research` retain their original comments and limitations. Their exact origins are in `PROVENANCE.md`.

## New code written for this scaffold

- An `IWeaponHost` integration boundary.
- A portable `WeaponRuntime` class that composes reference state transitions into host calls.
- A fake host and cross-system integration tests.
- CMake build system and an opt-in GitHub Actions CI workflow.

## Tests completed

- Linux CMake / GCC C++17 compiler build.
- `ctest` mock-host integration test suite (zoom, animations, holster, reload and clip accounting).
- Additional compiler/sanitizer checks are recorded in the accompanying release manifest if performed.

## Known integration gaps and decisions

1. The origin offset is a per-weapon general viewmodel setting, not proof of ADS-specific pose alignment.
2. The `+0x508` one-round mode is `false` for inspected firearm constructors; the runtime enabling path is unresolved. Keep it disabled by default.
3. The prototype injects an explicit transfer-at-validated-event call. The host must derive that event from the original reload schedule; the mock host cannot determine it.
4. Enter/exit zoom has original FOV request timings, but a physical VR implementation must not change headset FOV.
5. Current `holster()` considers a resolvable sequence a successful host animation operation, **not** original Source engine holster success semantics.
6. No actual Source engine ABI, vtable layout, save/load, network tables, or engine hooks are implemented.

## Next milestones

1. Identify a suitable, legally usable SourceVR engine and SDK target, and compile the host adapter against *that actual branch*.
2. Add a headless simulated player/weapon lifecycle test at engine-API boundary including reload timing and weapon switching.
3. Extract original per-gun `ZoomFOV`, weapon script and model/sequence data from legally owned assets, as available.
4. Revisit SiN SinTEK AI and vehicle behavior in a separate compatibility component, then integrate one playable level.
5. Investigate static x86-to-ARM64 translation feasibility only with an isolated representative function and a testable ABI, not promises of automatic complete porting.

## Continuity

This is the first **buildable project foundation**. All future scheduled research phases should update this repository and a versioned checkpoint when a persistent remote is connected. In the meantime, keep the released ZIP available for subsequent runs.

## Public research archive checkpoint

Phase 44–55 original reports and selected verifiers, plus carefully labeled early enemy/vehicle/difficulty historical summaries, are indexed at [`../research/README.md`](../research/README.md). This does not add runnable AI, car or campaign gameplay. The original DLL binaries, assets and excerpt dumps are excluded.

## Phase 56: Named reload-field metadata (verified static evidence)

The original SiN `server.dll` and `client.dll` were available during this investigation and both SHA-256 hashes matched `research/binary-provenance.json`. Typed metadata identifies `m_bReloadsSingly` (`server+0x508` / `client+0x7FC`), `m_bInReload` (`+0x4AC` / `+0x7A0`), and clip1/clip2 fields. See [`../research/phases/phase56-weapon-field-metadata.md`](../research/phases/phase56-weapon-field-metadata.md). Both a binary verifier and independently generated synthetic PE-parser unit tests were run; **no Quest, engine runtime or original game execution was performed**. The meaning of two parallel server descriptor groups and runtime enabling of one-round reload are unresolved.
