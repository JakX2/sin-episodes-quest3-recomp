# Phase 74: Portable map-entity input/output execution model

**Status:** Independently authored C++17 reference implementation with native tests. **Not a SourceVR game module, original SiN map-runtime reconstruction, or Quest gameplay test.**

## Source of the work

Phase 73 verified 32 *selected* original SiN Episodes (2006) `server.dll` entity-factory registrations, including `logic_auto`, `logic_relay`, `trigger_once`, `trigger_changelevel` and `func_door`. This only establishes their presence in the original registry. The real engine input/output dispatcher, schedule serialization, time service and entity resolver have **not** been recovered or bound to a Quest engine.

Phase 74 adds `EntityIoRuntime` in `include/sin_compat/entity_io_runtime.hpp` and `src/entity_io_runtime.cpp`, plus `tests/test_entity_io_runtime.cpp`, to define and test a future engine-facing **reference contract** for these behaviors. It is not copied from proprietary Source engine code and makes no claim of bit-accurate Source event queue parity.

## Model capabilities, demonstrated in native synthetic tests

- Register named outputs on a generation-safe `EntityRef`, with one connection per target/input/parameter/delay/max-fire definition.
- Fire a single named output and queue its eligible connections, decrementing positive fire counts **per source output firing**, not per resolved target. `max_fires=-1` repeats; zero disables.
- Deliver pending output events at monotonic host-provided time. Sort by due time and insertion ordinal so same-time inputs are deterministic.
- Defer targetname resolution until delivery. The `IEntityOutputHost` adapter owns `!activator`, `!caller`, duplicate names, dynamic names, wildcard logic, target search order, and game entity-input execution. The synthetic host tests only selected examples.
- Carry the originating activator and caller handles through delayed events. Nested output firings from an input handler are safe; **nested event-loop pumping is rejected**.
- Refuse dispatch to stale, deleted or recycled entity generations; remove pending events and output registrations by source on explicit destruction.
- Bound the count of processed **events per advance** so an immediate zero-delay feedback loop does not hang the host. Remaining pending events can continue on subsequent frames.
- Reject malformed output bindings, NaN/infinite delays and nonmonotonic/invalid timestamps.

**This model has intentionally stricter lifetime semantics** than an unverified original Source event queue: a queued event is dropped if its *source* has been deleted or recycled. The original game may permit queued outputs after caller removal. Only actual engine tests and lawful original research can settle that distinction.

## Reproduce

From a Phase 74 branch checkout on Linux/macOS with CMake and a C++17 compiler:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The new `sin_entity_io_runtime_tests` test demonstrates a **synthetic** `logic_auto -> logic_relay -> func_door` I/O chain: one-shot relay, 0.25-second delayed door action, activator/caller propagation, simultaneous fan-out, lazy targets, stale generations, source cleanup, invalid timer values and a bounded zero-delay cycle. No shipped SiN BSP data or engine object is involved.

The entity I/O code is compiled into the existing portable static library and therefore into the **same Android ARM64 NDK shared-library build**, but no C ABI functions are added: the host's exported surface remains the **13 existing versioned `sin_*_v1` symbols**. Phase 71–72's Android x86_64 emulator smoke exercises **weapon ABI only**, not this new entity I/O scheduler; successful Android loading does not establish in-engine scripting.

## Limitations / next steps

1. The demo does **not** create engine entities or load a `.bsp`. Phase 73's static analyzer remains separate. It does not automatically translate arbitrary map outputs into runtime registrations.
2. Entity resolution, engine input semantics and wildcard/special-name handling belong to the future lawfully licensed SourceVR-capable engine adapter. `IEntityOutputHost` is only a seam.
3. This scheduler does not yet implement serialized event queue save/restore, Source `variant_t` value types, event cancelation by target name, entity spawn flags, replicated state or original AI behaviors.
4. `trigger_changelevel` and `info_landmark` are *not* implemented. Their actual execution and save/restore must be demonstrated separately on a real module-capable Source engine; see P0 [issue #20](https://github.com/JakX2/sin-episodes-quest3-recomp/issues/20).
5. The scheduler uses a vector insertion queue for readability and determinism, not an optimized production priority heap. Porting to large levels requires profiling and scheduling/lifecycle parity tests.
6. The user-owned Quest 3 physical ARM64 smoke remains deferred at the user's request. This milestone does not depend on hardware testing and does not imply SiN is playable.

**Proof level:** Local Linux C++17 unit checks and GitHub native/Android *compile* jobs, not real game scripting behavior. Do not treat this as R1 integration.
