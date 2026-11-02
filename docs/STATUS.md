# Current implementation status: Phase 98 (Phase 99 documentation checkpoint)

**Last reviewed:** 2026-11-01. **Project target:** standalone *SiN Episodes: Emergence* for **Meta Quest 3**. This repository currently builds a **portable C++17 compatibility reference**, not an Android VR game or runnable Source engine replacement.

The cumulative development tree at this checkpoint is \`integration/phase98-category-band-adjustment\` ([PR #46](https://github.com/JakX2/sin-episodes-quest3-recomp/pull/46)), followed by the Phase 99 documentation branch. **Stacked PRs may be unmerged**; default-branch readers should not assume these components are already on \`main\`.

## Four separate meanings of "implemented"

| Evidence level | What it proves | What it does NOT prove |
|---|---|---|
| Original **binary-verified** | A pinned original 2006 Windows x86 DLL has exact matching instructions, registries, constants or tables | Our host C++ behaves identically in the original engine |
| Original **SDK/config-verified** | Legally held FGD/WeaponData/game scripts contain specific names/defaults/values | The game actually loaded/used the value in the same way in every scene |
| **Reference implemented and synthetic-tested** | Independent C++ code reproduces an explicitly modeled behavior under tests | The real Source runtime, game assets, NPCs or Quest are executing it |
| **Engine/HMD validated** | A game module, level and device were actually executed on the named engine/device | Not reached for this project |

Never collapse a Linux test, Android NDK cross-compile or Android x86_64 emulator dlopen into a claim of Quest 3 gameplay validation.

## Working reference components

### Weapons and host boundary, Phases 44–83

- \`WeaponRuntime\`, \`WeaponSession\` and associated entity/viewmodel input helpers model zoom/ADS transitions, activity selection, animation host callbacks, holster, owner/weapon lifetime and guarded clip/reserve transfer.
- The Source-style ADS button edge is \`0x00080000\`. **VR mode explicitly does not modify physical HMD FOV**; flat-screen FOV requests remain host callbacks.
- Real SiN SDK weapon scripts establish Magnum / assault rifle / Scattergun \`ZoomFOV\` **50 / 40 / 60**, clip capacities **20 / 45 / 6**, and original viewmodel paths. \`original_sdk_weapon_config(WeaponKind)\` is opt-in; its \`{0,0,0}\` origin offset is an explicitly **unknown viewmodel/aiming pose**.
- The original per-shell reload flag was false for inspected default firearms. The real enabling path and complete event/deadline schedule remain unknown. An engine must supply validated reload transfer moments.
- Hammer output binding, entity I/O, difficulty checkpoints, maker quotas/inputs, spawn placement and scripted spawn selection are host-neutral models, **not live Source map entities**.
- The versioned \`sin_compat_host_abi\` C shared module tests a weapon-session interface through callbacks; it is **not** the original game DLL ABI or a working SourceVR plugin.

Evidence: [Phase 76 original SDK research](PHASE76_SIN_SDK_RESEARCH.md), [Phase 89 grunt handler execution](PHASE89_GRUNT_INPUT_HANDLER_EXECUTION.md).

### Enemy and script behavior, Phases 84–90

- Original-SDK-based \`npc_template_maker\` runtime models spawn quota, live-child limits, dynamic inputs and candidate placement. Entity references include generation so recycled slots cannot silently refer to the same NPC.
- \`GruntTacticalControls\` models verified original grunt inputs \`LookOn\`, \`LookOff\`, \`StartPatrolling\`, \`StopPatrolling\`, and \`ThrowGrenades(bool)\`; the exact 2006 handler opcodes were independently checked in Phase 89.
- A KeyValues combat-spread rules loader can read owner-held \`rules_combat.txt\` without embedding the original script. It follows attacker/weapon/target mapping and wildcards.
- **Missing:** enemy-sensing decisions, squad reasoning, original combat schedules, hit detection, firing/spread sampling and actual squad/NPC execution.

Evidence: [Phase 88 grunt controls](PHASE88_GRUNT_TACTICAL_INPUT_CONTROLS.md), [Phase 90 combat spread configuration](PHASE90_ORIGINAL_COMBAT_SPREAD_RULES.md).

### Statistics and adaptive difficulty, Phases 91–98

- Phase 91: original stats metadata has **62 named definitions** and **8 percentage-of relationships**; portable schema validates them. This does **not** prove original stat collection or rating inputs.
- Phase 92: three original \`info_challenge\` query/output handlers, including \`(current - cached_entity_baseline) * 100\`; the original global initial challenge and entity baseline are **separate sources**.
- Phase 93: two original numerical routines reconstructed: interpolate **14 gameplay variables** and compute current challenge as mean clamped normalized contribution × 100. The host must supply the actual active GPV list and original config data.
- Phase 94: original guarded restore and direct console-variable initialization call-site behavior, modeled in a separate lifecycle adapter.
- Phase 95: timed feedback update gate with half-interval counter, six category collection callbacks, pending flag and six downstream follow-up stages.
- Phase 96: six category labels **Health, Damage, Accuracy, Headshot, Death, Time**; global-vs-local difficulty selection is reconstructed. Death/Time bypass a particular global-level override.
- Phase 97: independent report deadline raises the one-shot publication flag and a separate setting controls whether the update wrapper is called.
- Phase 98: six-by-two adjustable category factor table, additive and absolute updates clamped to **0.05–2.0**, and direct copy from saved baselines. The previously investigated helper at \`0x222C0260\` is part of **diagnostic logging** via \`dd_info.log\`, not proof of a weighting calculation.

**Still unknown:** exact player performance inputs and weights, selection and accumulation of original difficulty samples, how deltas acquire sign/magnitude, unmodeled callbacks and their impact on live NPCs, the savegame format and the real engine lifecycle. These are significant blockers to claiming a complete adaptive-difficulty system.

Evidence: [stat schema Phase 91](PHASE91_ORIGINAL_STAT_SCHEMA_AND_LAYOUT.md), [challenge output Phase 92](PHASE92_INFO_CHALLENGE_BINARY_VERIFIED_OUTPUTS.md), [formula Phase 93](PHASE93_GPV_INTERPOLATION_AND_CHALLENGE_AVERAGE.md), [timer Phase 95](PHASE95_ORIGINAL_DIFFICULTY_FEEDBACK_TICK.md), [routing Phase 96](PHASE96_SIX_CATEGORY_FEEDBACK_ROUTING.md), [publication Phase 97](PHASE97_ORIGINAL_FEEDBACK_PUBLICATION_DEADLINE.md), [band factors Phase 98](PHASE98_ORIGINAL_CATEGORY_FACTOR_BANDS.md).

### Campaign assets, scripts and vehicles

- Original SDK research inspected **three example VMF sources** with 7,689 entity blocks and 1,209 explicit Hammer output connections combined. It identified NPC makers, difficulty triggers, transitions and an editor \`prop_vehicle_sin\` entity. These are **editor/source observations**, not mounted/executed BSPs or car controls.
- Source-engine BSP loading, entity spawning from real maps, models/materials/audio, map scripts, navigation, vehicles and level-to-level progression **are not integrated**.

## Platform and test status

| Platform / check | Actual verified scope |
|---|---|
| Linux desktop | CMake builds portable C++17 static library and v1 shared module; CTest synthetic host and contract suite |
| Python | Original-free synthetic PE32/FGD/KeyValues/verifier regression tests in \`research/tests\` |
| Android ARM64 NDK | Real pinned official NDK compile/link of compatibility library, shared ABI and dlopen smoke **executable** |
| Android x86_64 emulator | Real Bionic emulator executes shared-module \`dlopen\` smoke; **not Quest HMD** |
| Original 2006 SiN Windows DLL | Read-only static disassembly/signature analysis against private, SHA-256-pinned source; **not game execution** |
| SourceVR-capable engine / real SiN BSP / Quest 3 | **No successful integrated execution demonstrated** |

The Phase 98 stacked PR's GitHub CI passed Linux, NDK ARM64 and x86_64 emulator jobs. No physical headset testing has been done.

## New integration infrastructure, Phases 100–103

- Phase 100 adds a read-only SourceVR APK/TXT/native package inventory for library variants, manifests and architecture identification.
- Phase 101 adds a bounded read-only ELF64 native dependency/export inspector. Neither tool proves compatible SourceVR mod loading.
- Phase 102 implements an engine-neutral event relay on the original 13-symbol C host ABI, with stale-generation protection.
- Phase 103 packages that relay as a separate native shared library with **six C entry points**. Linux `dlopen`/event-dispatch tests and an **Android x86_64 emulator Bionic runtime** test passed with a fabricated host; the new bridge also builds as an Android ARM64 ELF. None of these tests uses a real Source game host or Quest HMD. See [Phase 103](PHASE103_NATIVE_EVENT_BRIDGE.md).
- **Still no real Source engine host, custom SourceVR game-DLL support, SiN map or headset execution.** R1 is blocked.

## Integration blockers and recommended next engineering work

1. **R1: choose and obtain an actual compatible engine/SDK target** with permitted build/runtime integration. Compile a real host adapter against its headers and test the required lifecycle and callbacks, without assuming Source 2006 x86 layout carries to ARM64. [Issue #12](https://github.com/JakX2/sin-episodes-quest3-recomp/issues/12)
2. **R2–R3: exercise one genuine weapon and map** with legally held assets. Verify load, entity spawn, owner/sequence/events, VR pose design, attack and reload timings, then the first real BSP transition. [Issue #20](https://github.com/JakX2/sin-episodes-quest3-recomp/issues/20)
3. **R4–R5: wire NPC and difficulty models to actual engine events**, reconstruct missing AI and player-performance feedback, and validate game-level behavior rather than mock-host outputs.
4. **R6: add real Quest 3/Android VR runtime** (OpenXR or engine-native equivalent), tracked head/controllers, stereo rendering, locomotion, HMD-safe FOV handling, performance and device-specific input; perform physical-device testing only when available.

## Reproduction and hygiene

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s research/tests -p 'test_*.py'
```

Original-game binaries and SDK files are **never** prerequisites for public CI or distributed as part of this repo. Do not commit original DLLs, APKs, BSPs, VMFs, models, scripts, sound, materials or other commercially owned assets. Owner-private original-verification commands are documented in individual phase files.

This is a **reference-implementation checkpoint**, not a finished VR port. Earlier Phase 56 historical evidence remains preserved in [the research index](../research/README.md); it is no longer the project's current implementation status.
