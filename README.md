# SiN Episodes: Emergence | Quest 3 compatibility and reverse-engineering research

**Implementation checkpoint: Phase 98. Documentation refresh: Phase 99.**

This unofficial project is building an **independently authored, portable C++17 compatibility library** toward a standalone **Meta Quest 3 VR port** of *SiN Episodes: Emergence*. The public work is **not a playable game, Android APK, Source/SourceVR game DLL, or original game executable**. Here, **Q3 means the Quest 3 headset**, not the Quake III engine.

Development currently proceeds through stacked pull requests. The [Phase 98 implementation PR (#46)](https://github.com/JakX2/sin-episodes-quest3-recomp/pull/46) contains more recent work than the default-branch scaffold; this documentation refresh is stacked on it. Do not assume unmerged PR work is present on \`main\`.

## What is demonstrably implemented

These are *host-neutral reference behaviors* that compile and run against synthetic test hosts. In several places, original 2006 SiN DLL instructions and/or the original SDK scripts have been checked separately against owner-held copies. **That does not mean the game systems have run in the real engine.**

| Area | Portable implementation and evidence | Still missing for real gameplay |
|---|---|---|
| Weapon handling | ADS button edges, zoom state, activity/sequence selection, viewmodel callbacks, holster and guarded ammo transfers; generation-aware player/weapon/viewmodel lifecycle checks | Real Source weapon objects, complete attack/reload timing, network state and animation/event wiring |
| Original weapon configuration | SDK-derived Magnum / assault rifle / Scattergun model paths, ZoomFOV values **50 / 40 / 60**, and clip sizes **20 / 45 / 6** | Loaded models, actual sequence data, real per-weapon sight poses and engine-side script loading |
| Map and entity I/O | Host-neutral Hammer output routing, difficulty checkpoints, NPC template-maker inputs, child quotas, placement and spawn selection | BSP loading, native entity creation and map target resolution, full scripting and campaign transitions |
| Grunt behavior | Original FGD/DLL-grounded grunt inputs including LookOn/LookOff, patrol toggles, and ThrowGrenades(bool); related state and equipment validation | Original perception, schedules, cover/aim/grenade decisions, squad AI and live combat |
| Combat/stat data | Runtime KeyValues lookup of the owner's combat spread rules; schema for **62 stats**, percentage relationships and display/graph reference audits | Weapon ballistic sampling, actual game-stat recording and gameplay event delivery |
| Dynamic difficulty | Independently authored reconstruction of challenge-value averaging, **14-variable** modifier interpolation, scheduling, publication latch, **six** feedback categories, routing and clamped category-band operations | Verified performance-to-adjustment weights/signs, full original control loop and real NPC/engine effects |
| Portable binary interface | Versioned C host ABI shared module, Linux native/dlopen tests; official Android NDK **ARM64 build** and an Android **x86_64 emulator** Bionic dlopen smoke test | Real Quest runtime loader, SourceVR client/server module ABI, OpenXR headset/controller tracking, stereo rendering and in-headset validation |

The dynamic-difficulty components are **not** proof that complete adaptive difficulty works in-game: some input selection, feedback decisions and downstream effects remain unknown. Vehicle features have research/editor metadata, **not live vehicle gameplay**.

## Build and test the current development stack

Use the **Phase 99 branch** or a descendant to include the full cumulative reference library:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s research/tests -p 'test_*.py'
```

The build includes the \`sin_weapon_runtime\` C++ static library and the \`sin_compat_host_abi\` shared module. Synthetic C++/Python tests do **not** require original game assets. Existing Android scripts build with a pinned official NDK and test module loading on an x86_64 emulator. **The x86_64 emulator is not a Quest 3**, and an ARM64 compilation success is **not** headset execution.

## What is not yet working

- **Engine integration:** no real SourceVR/Source client/server ABI, engine callbacks or campaign-capable engine host. The C ABI is only a testable integration boundary.
- **VR player:** no OpenXR session, tracked HMD and controllers, hand aiming/reloading, stereo view, VR player movement or Quest input binding.
- **Original weapons in a game:** no rendered models, validated VR muzzle/sight offsets, complete reload schedule, real original NPC firing or all weapon attack gates.
- **Campaign and AI:** no launched SiN BSP or scripted encounter, complete SinTEK AI, vehicle operation, map transition or original save/load compatibility.
- **Original execution validation:** no original Windows executable executed by the port, no playable standalone Quest APK and no physical Quest 3 gameplay test.

The priority is now **real Source/SourceVR-compatible host integration**, not treating successful isolated math/research tests as a playable port. [Engine integration issue #12](https://github.com/JakX2/sin-episodes-quest3-recomp/issues/12) and [campaign integration issue #20](https://github.com/JakX2/sin-episodes-quest3-recomp/issues/20) remain the major blockers.

## Development evidence and next steps

- [Current implementation/evidence status](docs/STATUS.md)
- [Engineering roadmap](docs/ROADMAP.md)
- [Maintainer handoff](docs/HANDOFF.md)
- [Phase 76: original SDK weapon/map evidence](docs/PHASE76_SIN_SDK_RESEARCH.md)
- [Phase 89: original grunt input handlers](docs/PHASE89_GRUNT_INPUT_HANDLER_EXECUTION.md)
- [Phase 90: original combat spread rules](docs/PHASE90_ORIGINAL_COMBAT_SPREAD_RULES.md)
- [Phase 91: original stat schema](docs/PHASE91_ORIGINAL_STAT_SCHEMA_AND_LAYOUT.md)
- [Phase 93: gameplay-variable interpolation and rating](docs/PHASE93_GPV_INTERPOLATION_AND_CHALLENGE_AVERAGE.md)
- [Phase 98: category-factor bands](docs/PHASE98_ORIGINAL_CATEGORY_FACTOR_BANDS.md)

Research binaries and original SDK data are used only from separately retained, legally owned copies. Original \`server.dll\`, \`client.dll\`, BSPs, maps, models, game scripts, SDK files and VR APKs **must not be committed**. Public code is separately licensed under [MIT](LICENSE); commercial game/engine content is not.

To contribute, see [CONTRIBUTING.md](CONTRIBUTING.md) and the handoff. Label evidence accurately: **binary-verified**, **SDK/config-verified**, **synthetic-test-verified**, or **not yet validated in the game**. Keep claims tied to the environment actually tested.
