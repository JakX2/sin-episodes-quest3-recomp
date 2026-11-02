# Phase 102: Engine-neutral weapon event lifecycle relay

This phase adds an **integration seam**, not a SourceVR game-code plugin.

## What actually changed

\`EngineEventPump\` (\`include/sin_compat/engine_event_pump.hpp\`, \`src/engine_event_pump.cpp\`) owns a single \`sin_session_v1\` from the existing, versioned C shared library. A future engine adapter can translate engine events into the following lifecycle without duplicating the weapon state machine:

| Host event | Pump call | Existing ABI operation |
|---|---|---|
| Weapon equipped/player session created | \`attach()\`, \`on_equip()\` | \`sin_session_create_v1\`, \`sin_session_ensure_viewmodel_v1\` |
| Source-style viewmodel input | \`on_hide_weapon()\` | \`sin_session_hide_weapon_v1\` |
| User command buttons | \`on_usercmd()\` | \`sin_session_usercmd_v1\` |
| Reload activity start/transfer/end | \`on_reload_begin/event/end()\` | Existing ammo/reload contract |
| Holster | \`on_holster()\` | \`sin_session_holster_v1\` |
| Map transition, weapon removal or shutdown | \`on_map_unload()\`, \`on_weapon_removed()\`, \`detach()\` | Cleanup once; session destroyed |

Safety properties are explicitly narrow:

- Before each event, the pump asks the host if **both the original player and weapon generation refs are live**; on loss it detaches and returns \`-1\` instead of silently retargeting a reused entity index.
- \`attach\` replaces an earlier session, and repeated unload/removal is idempotent.
- Failed reload event results cannot modify caller output integers. Ammo values and reload decisions remain the v1 ABI's behavior.
- VR mode preserves HMD FOV: the existing host-neutral weapon system does not issue flat-screen FOV callbacks while VR zoom is active.
- The calling host must **not** destroy its callback context before \`detach()\` or call pump methods re-entrantly from host callbacks. It must issue events on the same thread. No multi-threading/lifetime guarantee is added.

CMake builds \`sin_engine_event_pump\` against \`sin_compat_host_abi\`, and \`tests/test_engine_event_pump.cpp\` exercises creation, selection, model lifecycle, reload transfer, idempotent unload and recycled-entity-generation guards using an entirely fabricated host.

## Reproduce

\`\`\`sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure -R sin_engine_event_pump_tests
\`\`\`

The change **does not** create a SourceVR/Valve interface, load a SiN game DLL or BSP, implement complete firing/reloading, render VR hands or execute on a Quest headset. SourceVR's published modding guidance does not support user-supplied game DLLs: https://github.com/tinsarfal/SourceVR . Without a permitted, matching SDK/runtime, this adapter has only a synthetic host.

## Next concrete R1 acceptance

Find or build a legally usable Android Source runtime with a documented game-module API. Then implement the host callbacks against *its actual C++/C headers* and demonstrate one real player + weapon session lifecycle. Until that happens, **R1 remains blocked**. Keep all original Windows DLLs and third-party APK content out of public Git.
