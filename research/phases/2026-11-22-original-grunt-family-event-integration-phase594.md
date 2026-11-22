# Gameplay Phase 594: Original Grunt-family event handoff composition

## Why this phase exists

Phases 590–593 independently authenticated four gameplay-native boundaries against the unchanged 2006 SiN Episodes: Emergence retail `server.dll`. It remained easy for a future replacement to incorrectly treat event `0x54` as an unconditional Grunt flank command. Phase 594 adds **integration regressions across the already reconstructed host models**; it introduces **no newly decompiled retail method and authenticates zero new original DLL bytes**.

The integration executable lives at `tests/test_original_grunt_event_chain_phase594.cpp`. It invokes the *actual existing* independent C++17 models in `original_jetpack_events_phase590.hpp`, `original_flying_event_filter_phase591.hpp`, `original_grunt_event_parent_phase592.hpp` and `original_elite_grunt_event_phase593.hpp`. It does **not** paste an alternate implementation of these four dispatch layers.

## Previously authenticated provenance, not new binary recovery

| Phase | Original method | Complete 2006 method hash / premise |
| --- | --- | --- |
| 590 | `0x22364280` Jetpack +0x444 | 1,146 bytes; `0x54` selects original fallback |
| 591 | `0x2238B3C0` flying attached filter | 560 bytes; `0x54` is outside `0x09..0x3C`, so the filter returns false |
| 592 | `0x2235BFD0` inherited Grunt +0x444 | 3,340 bytes; `0x54` goes through case 24, then original `0x2202D930` gate |
| 592 | `0x2202D930` optional component gate | 72 bytes; caller sets component `+0x08=1`, then virtual `+0x60`; nonzero after the call consumes the event |
| 593 | `0x22355480` Elite +0x444 | 360 bytes; `0x54` forwards unmodified to inherited Grunt; other special IDs can stop here |

For details, source checks and original hashes consult the Phase 590–593 reports and their read-only auditors. The underlying private `client.dll` / `server.dll` are not committed here.

## Newly tested cross-layer behavior

The executable records a deterministic stage trace for three entry classes:

- **Ordinary Grunt:** inherited Grunt prefix update -> Grunt default dispatch -> optional component gate -> shared NPC.
- **Elite Grunt:** Elite default forward -> inherited Grunt path; `0x47` remains Elite-only and `0x43` remains conditionally inherited.
- **Jetpack Grunt:** Jetpack default -> flying-attachment filter -> inherited Grunt path; `0x09` is consumed by the flying filter and `0x9F` / `0xA4` are Jetpack-only.

For **every** class, event `0x54` is tested with a null optional component, a present component retaining marker `1` (consumed), and a present component clearing marker `0` (shared fallback). The test additionally checks shared `0x54`'s finite float parameter truncation and the independently proven ordered, strict `route_length > 240.0f` condition for inner integer `1`. A `1.99f` parameter truncates to `1`; length exactly `240.0f` does not request `GRUNT_FLANK`.

The mocked shared NPC stage intentionally covers **only** the relevant `0x54` event and its original `+0x5A8` flank predicate, not the complete 2006 shared-NPC event dispatcher. Other native branches remain explicit unresolved callbacks. It deliberately does not simulate actual engine navigation, voice playback success, component concrete type, or x87 exceptional inputs.

## Build and readout

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_grunt_event_chain_phase594_tests
ctest --test-dir build -R sin_original_grunt_event_chain_phase594_tests --output-on-failure
```

**Research-only milestone.** No private binaries, original assembly dumps, assets, generic HL2 physics/surfaces, prop rendering, modified game executable, Android build or VR runtime were added.

### Next real decompilation target

Recover the concrete owner `+0xC18` component class (construction, assignment, vtable and virtual `+0x60`), and determine whether its handler clears the marker for `0x54`. Second priority is the Elite-specific helper pair `0x22355300` and `0x22355370` used by event `0x47`. Those require the original retail binary or equivalent trusted decompilation evidence; the Phase 594 host regression does not invent their method bodies.
