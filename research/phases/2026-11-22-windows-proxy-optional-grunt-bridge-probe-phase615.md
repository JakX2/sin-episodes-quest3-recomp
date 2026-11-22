# Gameplay Phase615: optional original Windows proxy to Grunt C bridge

## What changed

Phase613 introduced a genuinely loadable 32-bit Windows Grunt behavior shared library with five versioned C exports. Phase614 manages behavior sessions for host-confirmed existing Druglab Grunt NPCs but still does not invent NPC objects.

Before Phase615, the separately built 32-bit Windows `server.dll` proxy preserved only the original `CreateInterface` factory, loading the user's renamed original `sin_original_server.dll`. It had **no non-invasive way to demonstrate that a real SiN engine process could also load the new Grunt behavior bridge**.

Phase615 adds **explicit opt-in discovery and ABI validation** to the existing original-factory-forwarding Win32 proxy. The bridge load occurs only after the original server factory is initialized and outside `DllMain`. No engine vtables, NPC objects, original symbols, rendering or physics are patched. **Even when the optional Grunt probe fails, `CreateInterface` still calls the original factory exactly as before.**

## Fixed-path opt-in

Set `SIN_PC_GRUNT_BRIDGE_PROBE=1` before starting the owner-controlled disposable Windows test. The server proxy resolves its own DLL directory, then attempts to load the fixed sibling:

    sin_grunt_behavior_c_abi.dll

It never accepts an arbitrary DLL path from the environment or silently loads the bridge from a global search directory. It uses the same absolute sibling path and `LoadLibraryExW(..., LOAD_WITH_ALTERED_SEARCH_PATH)` as the already-tested original factory loader. It validates the five actual Phase613 entrypoints:

1. sin_grunt_behavior_abi_version_v1
2. sin_grunt_behavior_create_v1
3. sin_grunt_behavior_destroy_v1
4. sin_grunt_behavior_step_v1
5. sin_grunt_behavior_snapshot_v1

It then calls the version entry and accepts **only ABI v1**. If any step fails, it releases the optional candidate module and continues the original SiN factory path. After successful version validation it retains the bridge module handle for the remaining process lifetime. It **does not call create/step or invoke any gameplay callbacks**: those still require a verified engine-owned Grunt entity.

If `SIN_PC_GRUNT_BRIDGE_PROBE` is not exactly `1`, the bridge load is skipped. Once enabled, the process attempts the probe at most once per server proxy module. The original interface forwarding behavior is unaffected.

## Safe diagnostic record

With `SIN_PC_PROXY_TRACE_FILE=1`, the proxy writes a process-local diagnostic file under the user's Windows TEMP path named `sin-pc-proxy-<PID>.log`. The role remains `server`.

Potential reported phases:

- `grunt-bridge-ready`: all five exported functions present, returned version 1
- `grunt-bridge-load-failed`: fixed sibling DLL cannot be loaded
- `grunt-bridge-export-missing`: one of the five functions is missing
- `grunt-bridge-version-rejected`: version does not equal 1
- `grunt-bridge-path-failed` / `grunt-bridge-path-too-long` / `grunt-bridge-initonce-failed`: safe diagnostic refusal

No original retail binary bytes, game memory addresses, user-supplied paths, or engine pointers are logged by the new probe.

## Four true native Windows x86 test paths

The new C11 Win32 loader test `pc/windows_x86_proxy/tests/test_grunt_proxy_bridge_phase615.c` and its version-2 synthetic bridge validate:

- **Disabled**: original `CreateInterface` works and no Grunt bridge is loaded or probed.
- **Valid**: original factory result and pointer identity unchanged, all five C ABI symbols available from a true loaded `sin_grunt_behavior_c_abi.dll`, version exactly 1, ready trace present.
- **Missing**: a separate disposable test directory with only `server.dll` and synthetic original server but no Grunt library; original factory still succeeds, missing-library trace recorded.
- **Wrong version**: disposable test directory includes a synthetic library with **all five expected export names** but version 2; proxy explicitly rejects/unloads it and still forwards to the original factory.

The tests compare both the returned interface pointer and status against directly invoking the synthetic original's `CreateInterface`. They also verify unrecognized interface requests still return the original failure code. They never load the commercial SiN binary or spawn a gameplay NPC.

The test helper modules are `fake_grunt_bad_version_phase615.c` and `fake_grunt_bad_version_phase615.def`. CMake builds them only in `pc/windows_x86_proxy`, not the root game library, and registers the four new CTests. Prior native x86 factory and Grunt behavior tests still run.

Reproduce the synthetic test suite:

    cmake -S pc/windows_x86_proxy -B build-pc-win32 -G "Visual Studio 17 2022" -A Win32
    cmake --build build-pc-win32 --config Release
    ctest --test-dir build-pc-win32 -C Release -R sin_pc_grunt_proxy_bridge_phase615 --output-on-failure

## What this does NOT demonstrate

A passing synthetic Windows test proves the **server proxy can optionally load and inspect the actual Grunt behavior C DLL without interfering with factory forwarding**. It does **not** prove a retail SiN process has loaded that bridge, that a real engine factory has spawned one of the three Druglab Grunt classes, or that Source's `ServerGameDLL003` NPC virtual ABI has been implemented.

A genuine game process test must use the user's own backed-up original retail installation and the existing owner-only proxy deployment procedure; synthetic `sin_original_server.dll` and other fake test modules must **never** be copied into a game. The optional Grunt bridge has no engine-owned NPC callbacks wired yet.

Original source recovery remains **40 separately authenticated 2006 native method entries and 11,224 native bytes through Phase615**. This phase adds zero original functions, and excludes HL2-equivalent models/props/surfaces/physics entirely.
