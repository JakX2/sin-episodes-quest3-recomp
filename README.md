# SiN Episodes: Emergence | public reverse-engineering research

**Status:** Phase 56 starting scaffold. Builds and runs as a **standalone native C++17 test library**. **Not a playable game, Source engine DLL, Android APK, or functional Quest 3 port.**

This repository begins turning the project's earlier assembly-driven research into one accumulative implementation with a host boundary. The original 2006 `server.dll` and `client.dll` are **not** included, nor are Half-Life 2 DLLs, proprietary models, maps, scripts or game assets.

## Preserved reverse-engineering research

The public [research index](research/README.md) includes Phase 44–55 original research reports, original verifier scripts where retained, provenance hashes, and partial earlier enemy/difficulty/vehicle notes. Archived findings are **not** automatically playable functionality.

## Welcome, contributors

This independent, unofficial project is designed to be **forkable and maintainable**.
If the original organizer steps away, start with [the handoff guide](docs/HANDOFF.md)
and the [contributing guide](CONTRIBUTING.md). Source code is provided under
[MIT](LICENSE); **the original games and engine are not**. See
[assets and rights guidance](docs/IP_AND_ASSETS.md).

## Try it

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The current CMake project builds `sin_weapon_runtime` and a `sin_weapon_runtime_tests` executable. The tests use a fake host and **synthetic fixture ammo capacity/model/FOV values**; they demonstrate integration behavior but do not establish live gameplay accuracy.

## What works in the prototype

- Source-style zoom button edge recognition (`0x00080000`) and a guarded ADS toggle/restore reference.
- Shared animation activity selection including assault-rifle unzoomed recoil activity mapping.
- Sequencing of activity-to-sequence lookup, model/offset bind and animation playback; missing sequences do not trigger a bind.
- Explicit no-HMD-FOV-change behavior in VR mode. Flatscreen FOV transitions are available to a host.
- Engine-event-driven ammunition transfer; ammunition is *not* refilled immediately upon beginning reload.
- Conservatively modeled single-round reload **only if externally enabled**. The original runtime enabling path is unproven.
- Holster flow that exits ADS and cleans up reload state before requesting holster animation.

## Not yet implemented

- Real SourceVR or Android/ARM64 engine hookup; no VR runtime bindings, headset position or motion controller interaction.
- Weapon models and animation sequences, real `ZoomFOV` settings, aim poses or reliable gun-specific script values.
- A complete state machine for all original reload timings, owner checks, and attack gates.
- Enemy AI, dynamic difficulty, vehicles, map scripts and campaign integration.
- Original game execution validation.

## Project layout

```text
include/sin_compat/     Public host-agnostic weapon API
src/                    Compatibility prototype implementation
tests/                  Mock-host integration tests
third_party/research/   Unmodified clean-room behavioral reference headers from earlier research
docs/                   Roadmap, status and evidence limitations
.github/workflows/      Linux CMake build/test CI (activates when hosted on GitHub)
```

## Connecting to a game engine

Implement `sin_compat::IWeaponHost` for the selected SourceVR branch. This requires correct engine lifecycle integration for resolving weapon activity sequences, viewmodel binding, animation playback, and *flatscreen-only* FOV changes. The engine is responsible for deciding the authoritative reload transfer event, ensuring a valid owner/weapon, providing per-gun settings, and storing game state.

This prototype deliberately does **not** claim to be a drop-in replacement for Ritual's compiled gameplay modules. See `docs/STATUS.md` for evidence and integration gaps.

## Repository hygiene

Never commit original `server.dll` or `client.dll`, extracted maps, models, assets, game archives, or personal game installation files. The `.gitignore` excludes common formats; review every file before publishing.
