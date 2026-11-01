# Engineering roadmap | Phase 99 documentation checkpoint

**Target:** Standalone *SiN Episodes: Emergence* VR port for **Meta Quest 3**. **Implementation through Phase 98 is reference code**, not real campaign or headset execution. The R0–R6 milestones below require the specified real target environment to count as complete.

| Milestone | Acceptance criterion | Current status |
|---|---|---|
| **R0: portable foundation** | Native C++17 reference library with versioned host ABI, unit tests and independent original-binary research | **Reference implementation complete**; Linux native tests, Android ARM64 NDK builds and Android x86_64 Bionic module smoke verified |
| **R1: engine host** | Real Source/SourceVR-compatible engine selected; compile/link host integration against its actual headers and demonstrate lifecycle callbacks | **Blocked / not implemented**; engine target and ABI issue [#12](https://github.com/JakX2/sin-episodes-quest3-recomp/issues/12) |
| **R2: real weapon** | Load owned original weapon model/animations in a real engine scene; verify timing, ADS/VR poses, attacks and reload events | **Not integrated**; SDK weapon values and portable weapon event models exist |
| **R3: first map** | Real SiN BSP/level launches; NPCs, outputs and a level transition execute in the target engine | **Not integrated**; host-neutral entity I/O and spawn-placement prototypes exist; campaign issue [#20](https://github.com/JakX2/sin-episodes-quest3-recomp/issues/20) |
| **R4: enemy AI** | A live SinTEK grunt senses, navigates, engages and reacts to scripted campaign inputs in the engine | **Not integrated**; tactical input and maker prototypes exist, not original decision AI |
| **R5: dynamic campaign systems** | Real player-performance feedback drives live NPC/difficulty changes; vehicle and special encounters work in campaign | **Partial reference implementation only**; substantial original-derived GPV, stats, feedback, routing and factor math through Phase98; no real vehicle or gameplay integration |
| **R6: standalone Quest VR** | Android ARM64 Quest app launches game with stereo rendering, tracked HMD/controllers, VR interactions and acceptable performance | **Not implemented**; native ARM64 module compile and x86_64 emulator \`dlopen\` are not Quest/VR execution |

## Immediate engineering order

1. **R1 first:** select legally accessible Source/SourceVR-compatible engine/SDK and establish a precise native host contract. Avoid guessing Source x86 vtables or calling conventions on ARM64.
2. **Thin vertical slice:** connect an owned original Magnum/assault rifle/Scattergun model and one weapon lifecycle to actual engine callbacks; separately document true VR aim poses and reload events.
3. **First BSP integration:** load one original level, parse real entities into the engine, resolve Hammer outputs and demonstrate one NPC spawn and interaction.
4. **Broaden gameplay:** connect existing enemy/difficulty reference systems to real engine events, then restore missing six-category feedback semantics, navigation, scripted encounters and vehicles.
5. **R6 device runtime:** integrate stereo, tracked headset/controllers and Quest performance/input, then test on real hardware when possible.

Keep two tracks separate: (a) verified reverse-engineered **behavior** and (b) demonstrated **runtime integration**. A large volume of synthetic tests cannot substitute for R1–R6 acceptance criteria.

See [STATUS.md](STATUS.md) for the evidence matrix, [README](../README.md) for build commands, and [HANDOFF.md](HANDOFF.md) for contribution sequencing. Source files, SDK binaries, game DLLs, maps and assets remain outside public GitHub.
