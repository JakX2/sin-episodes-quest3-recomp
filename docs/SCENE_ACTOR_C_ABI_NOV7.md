# Scene actor C ABI, November 7 prototype integration step

**Scope:** independently authored *native shared library*, compiled from the JC/Jessica scene-actor bridge in PR #104. This is a portable experimental compatibility component, **not** an original SiN 2006 DLL, Source engine `CreateInterface` game module, HL2 classname registration plugin, headset application or playable APK. Original game assets are not included.

## Why this matters

The original server DLL contains `CNPC_JC` descended from `CAI_BaseActor` and `CNPC_Jessica` descended from `CNPC_PlayerCompanion`/`CAI_PlayerAlly`. In the original HL2 logs, 6 `npc_jc`, 17 `npc_jessica` and 49 `npc_generic_actor` creation attempts fail. The initial C++17 bridge records the minimal scene actions; this phase adds a **real loadable C ABI** that an eventual engine host can call without linking directly against its C++ classes.

| ABI export | Purpose |
|---|---|
| `sin_scene_abi_version_v1` | ABI gate, returns version 1 |
| `sin_scene_create_v1` | Allocate per-actor session from validated callback table and actor type |
| `sin_scene_destroy_v1` | Release the session (refuses nested destruction) |
| `sin_scene_spawn_v1` | Request an engine-host actor with exact original classname and model/targetname defaults |
| `sin_scene_begin_v1` | Forward VCD path and original actor token including `!Target1` |
| `sin_scene_input_v1` | Four Jessica map inputs, preserving type checks |
| `sin_scene_animation_event_v1` | Forward `AE_JESSICA_THROW_ITEM` event with once-per-success behavior |
| `sin_scene_get_actor_v1` | Return the opaque engine actor handle after successful spawn |

The library deliberately does not call a guessed SourceVR method. Its `sin_scene_callbacks_v1` interface requires **real engine-provided** callbacks for entity creation, model/health/targetname setup, choreography, holstering, weapon throwing, and cleanup. The candidate HL2 actor base names (`npc_citizen` and `npc_alyx`) remain **unverified hypotheses**. The host must verify class availability, model/skeleton/activity compatibility, actor name resolution and scene cache/mounting.

## Reproduce from PR #104 (on desktop)

```sh
cmake -S . -B build-scene -DCMAKE_BUILD_TYPE=Release
cmake --build build-scene --target sin_scene_actor_c_abi sin_scene_actor_c_abi_smoke sin_scene_actor_bridge_tests
ctest --test-dir build-scene --output-on-failure -R 'sin_scene_actor_(bridge_tests|c_abi_smoke)'
```

C11 `dlopen` smoke uses **mock callbacks only** but loads the real compiled `.so` and looks up all eight symbols. It checks original model/HP defaults, input type handling, Jessica's delayed throw + ammo values, double-animation-event protection, callback reentry, failed spawn cleanup and generic actors. The test is built with **`-UNDEBUG`** to prevent Release `NDEBUG` silently disabling its checks. `sin_scene_actor_bridge_tests` has the same protection. The version script restricts exports to the eight documented functions.

```sh
nm -D --defined-only build-scene/libsin_scene_actor_c_abi.so
```

This local release build and test passed, with precisely eight function exports and a separate version node. The feature branch additionally configures an Android NDK **ARM64 and x86_64 build** and an x86_64 Bionic emulator `dlopen` smoke test using fake callbacks. Those are meaningful native-loader tests, but neither is a SourceVR game-module load or Quest 3 gameplay execution. GitHub CI must be inspected after committing before claiming success.

## November 7 acceptance gate

1. Verify the real Source-compatible host/game-module architecture and interface availability, rather than guessing an x86 interface against an AArch64 engine.
2. In isolated HL2 custom-game testing, inspect `se1_finale01` and its `logic_choreographed_scene` actor bindings. Make a trial BSP copy using the existing copy-only tool. Never overwrite the user's original BSP.
3. Check JC and Jessica models on actual fallback bases and confirm `!Target1` mapping from the scene controller.
4. Establish `scene.cache`/VCD and speech mounting on the tested engine build, then capture `scene start`, animation and `OnCompletion` evidence.
5. Only when both JC and Jessica **spawn, animate, play one scene and complete outputs** on a real engine can the scripted-scene R1 milestone be claimed.
6. A Quest 3 standalone playable port additionally needs a real SourceVR-compatible entity game module and headset integration, which this shared library alone does not provide.

**Ownership:** the engine host owns spawned entities. Destroying the compatibility session does not implicitly delete a live engine NPC; the host must tear down its level/game entities. Callbacks must remain valid for the session lifetime, must be single-threaded, and must not throw through the C interface. Reentrant calls fail closed. Do not ship original DLLs, BSPs, VCD payloads, or assets with this library.