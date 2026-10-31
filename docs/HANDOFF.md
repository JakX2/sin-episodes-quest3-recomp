# Maintainer handoff / start here

This file exists so the project can be continued even if its original
organizer becomes unavailable. **No private chat history is required to start.**

## Current state

- Portable C++17 `sin_weapon_runtime` static library, with a mock host.
- CMake tests cover zoom toggles/interrupts, zoomed activities, viewmodel
  binding, reload gating, ammo conservation and holster ordering.
- The test fixtures are synthetic; no native Quest game or Source DLL exists.
- Some reference behavior comes from the prior reverse-engineering phase
  reports. See `third_party/research/PROVENANCE.md` and `docs/STATUS.md`.

## Quick start

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Most valuable next contribution

**Integrate the `IWeaponHost` interface with a legally usable Source/SourceVR
SDK branch**. Do not guess engine ABI signatures. Keep the initial goal small:
create a compile-tested adapter against real headers, add one mock-to-engine
contract test, then validate a single weapon in an actual game-level fixture.

If no suitable branch is available, improve host-agnostic integration tests,
add deterministic reload timing fixtures and verify the outstanding weapon
reload-mode switch with a legally obtained local DLL.

## Work that is NOT finished

- No real Quest 3/Android ARM64 build, HMD/controller integration, or stereo
  rendering support.
- No actual SiN model-derived sight poses, game scripts or campaign assets.
- No original enemy AI or vehicles integrated into the current binary.
- No proven runtime setter enabling one-round reload mode for the original
  Magnum, assault rifle or Scattergun.

## Evidence / confidence conventions

Tag every nontrivial claim as one of: **binary-verified**, **experiment-verified**,
**reference-model**, or **hypothesis**. Include offsets and hashes only if
actually reproduced against the specified build. A synthetic C++ test is NOT
verification of the original Windows game.

## How to keep handoffs working

1. Update `docs/STATUS.md` with what was genuinely built and tested.
2. Add a dated entry below to this file for each major milestone.
3. Keep all build scripts and tests in the repo; do not depend on chat-only ZIPs.
4. Publish small, reviewable commits and tagged releases when milestones run.
5. Preserve the repo's independence from original proprietary binaries/assets.

### Handoff log

- **2026-10-31:** Initial public-ready repository scaffold assembled from the
  Phase 56 weapon compatibility prototype. Local test suite and CI configuration
  present; public GitHub publication and game-engine integration still pending.
