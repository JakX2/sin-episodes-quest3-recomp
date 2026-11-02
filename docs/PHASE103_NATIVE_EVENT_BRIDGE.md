# Phase 103: Loadable native event-bridge shared library

The portable \`sin_compat_host_abi\` already exposes 13 versioned C symbols for weapon-session behavior. Phase 102 added a C++ event pump over those functions. Phase 103 now packages **that pump itself** as a separate, dynamically loadable C interface: \`sin_compat_event_bridge\` (ELF \`libsin_compat_event_bridge.so\`).

This is a new real native library target, built by host Linux CMake and Android NDK CMake. It remains **engine-neutral**: it is **not** a SourceVR game module, a Valve \`CreateInterface\` or a native SiN client/server implementation. A future engine will still need to provide the full callback contract, real entity generations, engine events and gameplay.

## Dynamic host contract

Public header: \`include/sin_compat/event_bridge_v1.h\`. Exact six exported symbols:

- \`sin_event_bridge_abi_version_v1()\` returns 1.
- \`sin_event_bridge_create_v1(callbacks, userdata)\` copies the existing 23-callback \`sin_host_callbacks_v1\` table after checking its ABI version and structure size.
- \`sin_event_bridge_attach_v1(bridge, player, weapon, kind, config, vr_mode)\` starts one weapon session; the new attach replaces an earlier one.
- \`sin_event_bridge_dispatch_v1(bridge, event, input, output)\` dispatches \`EQUIP\`, \`HIDE_WEAPON\`, \`USERCMD\`, \`RELOAD_BEGIN\`, \`RELOAD_TRANSFER\`, \`RELOAD_END\`, \`SET_AMMO\`, \`GET_AMMO\`, \`HOLSTER\`, \`MAP_UNLOAD\`, \`WEAPON_REMOVED\`.
- \`sin_event_bridge_is_active_v1(bridge)\` returns 1 or 0, or -1 for invalid/reentrant call.
- \`sin_event_bridge_destroy_v1(bridge)\` performs cleanup and frees the bridge; callbacks may not destroy the bridge reentrantly.

\`MAP_UNLOAD\` and \`WEAPON_REMOVED\` detach idempotently even if a session is already absent. Invalid events and re-entrant calls are rejected with -1, and failed events do not overwrite output buffers. Only one calling thread is supported. Caller-owned callbacks, userdata and entity references must remain valid until bridge detach/destroy. Nested destroy from a host callback is deliberately ignored; retry after the outer call.

The event bridge is linked against \`sin_compat_host_abi\` and requires both shared libraries at runtime. Keep their matching builds together. It does not expand or modify the original 13-symbol host ABI.

## Original-free reproduction

\`\`\`sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure -R 'sin_event_bridge_dlopen_tests|sin_engine_event_pump_tests'
\`\`\`

The \`sin_event_bridge_dlopen_tests\` executable does **not** link directly against the bridge. It calls \`dlopen\`, obtains all six entry points with \`dlsym\`, constructs a fabricated engine host, runs session lifecycle/reload/usercmd/map cleanup, checks re-entrancy and recycled entity generations, and closes the library. All test data is generated within the test. This proves host-process dynamic invocation on the tested platform, not execution inside SourceVR or the Quest headset.

To use on Android, build with the official NDK as in the existing Android CI, deploy \`libsin_compat_host_abi.so\` and \`libsin_compat_event_bridge.so\` for the **same ABI**, and ensure the native host can resolve both. The Phase 103 CI now executes a **Bionic Android x86_64 emulator smoke test**: it pushes both matching `.so` files and a separately built loader, verifies SHA-256 on the device, runs the native `dlopen` lifecycle with fabricated callbacks, and checks a pass marker. This is real Android **synthetic-host** execution, **not** SourceVR engine integration, an ARM64 device run or a playable Quest port. The Android shell fixture sets `LD_LIBRARY_PATH` to the private library directory so the bridge can resolve `libsin_compat_host_abi.so`.

## Remaining engine blocker

SourceVR publicly documents that content-only mods are supported but mods with their own \`client.dll\`/\`server.dll\` game code are not: https://github.com/tinsarfal/SourceVR . A successful \`dlopen\` of our own bridge proves **nothing** about the ability to replace or hook SourceVR game code.

R1 completion still requires a lawful, buildable ARM64 Source-compatible engine host and **actual** callbacks, verified map/entity and weapon object integration. Keep the user's SourceVR APK, original SiN DLLs/SDK and assets private.
