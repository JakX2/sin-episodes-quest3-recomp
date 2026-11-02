# Maintainer handoff | Phase 99 documentation checkpoint

This repository should remain maintainable without private conversations or access to the original organizer's personal files. **Public code and synthetic tests are sufficient to reproduce the compatibility reference**, while original source binaries, maps and SDK content must be separately obtained and kept private.

## Read this first

- [README](../README.md): current implementation summary and build instructions.
- [STATUS](STATUS.md): evidence/provenance levels, detailed Phase98 subsystem status and gaps.
- [ROADMAP](ROADMAP.md): acceptance criteria for R0–R6, particularly **R1 real engine hookup** and **R6 headset runtime**.
- [Contributor guidance](../CONTRIBUTING.md), [asset rights policy](IP_AND_ASSETS.md) and [research archive](../research/README.md).
- Current cumulative branch: \`integration/phase98-category-band-adjustment\`, [Phase98 PR #46](https://github.com/JakX2/sin-episodes-quest3-recomp/pull/46). Documentation refresh branch: \`integration/phase99-project-status-refresh\`. **PR stack may not be merged into main**.

## Current engineering status

**Implemented in a reference library:**

- C++17 \`sin_weapon_runtime\` and independent \`sin_compat_host_abi\` C shared library with an explicit host callback contract; zoom/ADS, viewmodel selection, reload transfer, holster and entity lifetime guard tests.
- Hammer-style entity I/O, NPC maker quota/placement/input models, difficulty checkpoints and original-evidence grunt tactical input state.
- Runtime combat-spread KeyValues configuration lookup, original stats schema, and original-binary-grounded dynamic difficulty interpolation/rating, lifecycle, timers, publication flag, category routing and two-band factor adjustments.
- Source and original-asset evidence reports with SHA-256 pins; **private originals are not in the repository**.
- Linux CTest, original-free Python synthetic verifier tests, official Android NDK **ARM64 compilation**, and Android **x86_64 emulator Bionic dlopen smoke** all passed for the Phase98 stacked PR.

**NOT implemented or validated:**

- Working Source/SourceVR-native engine host, native game DLLs, original SiN campaign BSP in engine, complete SiN NPC perception/combat AI, vehicles, original Save/Restore or full player-performance feedback.
- Real weapon models/animations and validated VR sight poses or fully timed original attacks and reloads.
- Android Quest app, OpenXR/HMD tracking, controller interaction, stereo VR rendering and physical Quest 3 game execution.

The prior handoff's statement that **no Android ARM64 build exists** became outdated; correct wording is **ARM64 compatibility-library compilation exists, but there is no integrated Quest VR application**.

## Quickstart, entirely original-free

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s research/tests -p 'test_*.py'
```

An official NDK and Android SDK/emulator are required for Android-specific CI scripts, **not** for native CMake/CTest. The Android x86_64 emulator only verifies a shared module can be loaded by Bionic; that is not a Meta Quest VR smoke test.

## Best next contribution: a real engine adapter

1. Find/select a legally available Source/SourceVR-compatible **engine SDK** with actual C++ headers and Android build support. Document license and engine version; avoid simply assuming older Source vtable layouts.
2. Implement a minimal real engine adapter for the existing [host interface](../include/sin_compat/weapon_runtime.hpp) and [versioned C ABI](../include/sin_compat/host_abi_v1.h). Do not expand the ABI casually, since size/layout/link-time export tests protect it.
3. Prove a real engine lifecycle: create a player/weapon, resolve model/sequence, dispatch animation events, exercise reload and entity cleanup. Add a real-header compile/test and separately a minimal playable fixture.
4. Use lawfully owned assets to validate original viewmodels/WeaponData and a first SiN BSP, then integrate entity I/O, NPC spawn and AI. Don't publish proprietary game assets in PRs.
5. Build the actual VR/OpenXR/Quest path **after** engine binding exists, distinguishing device build from emulator smoke and physical headset testing.

If a usable engine target is not available, contribute bounded, binary-verifiable original-research functions and synthetic tests, or improve data-driven host contract fixtures without calling them a completed game subsystem.

## Evidence-labeling rules

- **Binary-verified:** original Windows x86 DLL exact hash, instruction/offset/table check. No original game was executed.
- **SDK/config-verified:** original FGD, weapon scripts or map-source metadata, with original held by owner.
- **Synthetic-test-verified:** tests of the independently authored implementation, including Android static library/ABI behavior.
- **Real-engine-validated / real-headset-validated:** only after actual engine and/or hardware gameplay, with a reproducible report. **Neither is currently achieved.**

The original research references in \`third_party/research\` cannot substitute for engine-specific build interfaces. Beware the \`+0x508\` one-shell reload default and unverified runtime enabling path, unknown VR muzzle/sight poses, missing active challenge average list and unverified category adjustment sign/weight inputs.

## Maintaining the project

- Publish a small, stacked PR from the latest verified integration branch; do not silently merge unreviewed research to \`main\`.
- After each material capability change, update README, STATUS, ROADMAP and this handoff when their claims would change. The automated status check introduced in Phase99 guards the most important capability distinctions.
- Keep all public tests original-free, runnable in CI. Private original-DLL verifiers must take local owner paths and fail on hash mismatches.
- Never commit original engine/game DLLs, BSPs, SDK files, models, scripts, sounds, textures or VR APKs.
- Do not claim the SiN campaign works, or assign verified semantics to opaque original callback functions, merely because an Android module compiled or a mock host passed.

**Newest integration artifacts:** Phases 100–101 supply private-only APK and ELF metadata inspection tools; Phase 102 supplies a native host-event relay; Phase 103 exposes it as a separate shared library with six versioned C exports and a synthetic-host dynamic-loading test. Read [Phase 103 native event bridge](PHASE103_NATIVE_EVENT_BRIDGE.md). No real SourceVR game DLL loader or Quest session exists.

### Handoff log

- **2026-10-31:** Phase56 reference scaffold, initial weapon host boundary and tests.
- **2026-11-01:** Cumulative Phase57–98 research/reference work; original SDK weapon values, host ABI and Android builds, entity/NPC/AI-input modules, combat spread/stat metadata and significant adaptive-difficulty math/timer/category/band modules. **No integrated SourceVR or Quest gameplay.**
- **2026-11-01, Phase99 documentation refresh:** Replaced stale Phase56 project status and inaccurate Android/Quest descriptions with explicit reference-versus-runtime tracking; roadmap and contributor handoff aligned.
