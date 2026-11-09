# Phase 124: Original CRadioMessage input and dispatch reconstruction

**Status:** Independently authored C++17 entity behavior component, integrated with the Phase 74 portable entity-output scheduler. **Not** a native Source entity, audio/radio implementation, client network class, or Quest gameplay test.

## Original 2006 evidence

The preserved SHA-pinned `sin_drive_crosswalk_evidence_09.json` audit records:

- Original `server.dll` SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- `env_radio_message` factory maps to `CRadioMessage` at registration VA `0x223C3A27`.
- Datamap fields: `m_priority` at `+0x3C4`, `m_message` at `+0x3C8`, and `m_speaker` at `+0x3CC`.
- Named inputs and handler VAs: `SetMessage` / `0x222E4510`, `SetSpeaker` / `0x222E43A0`, `SetPriority` / `0x222E43D0`, `Dispatch` / `0x222E4B70`, and `Remove` / `0x222E4C10`.
- Output `OnDispatch` is present in the datamap at `+0x394`.
- The preserved verifier checks the `Dispatch` function's `FireOutput` call at `0x222E4BFE` and records local-player resolution before that call.

The evidence establishes the entity layout, named inputs/output and the dispatch gate. It does **not** establish the complete audio path or the exact `FireOutput` argument registers/stack layout.

## Implemented

- Store the three confirmed input-backed values without arbitrary clamps.
- Reject stale entity handles before changing state or dispatching.
- Resolve a live local player before firing `OnDispatch`.
- Connect `OnDispatch` to the existing portable `EntityIoRuntime`.
- Return the number of connections scheduled separately from whether the output was invoked; an output with zero links is still invoked.
- Delegate `Remove` to a host request. Actual entity destruction and output-queue cleanup remain engine-owned.

The portable output adapter follows the conventional Source mapping of local player as activator and the output-owning entity as caller. That mapping is isolated in one call and remains a **compatibility assumption** until the original call-site argument setup is re-read from the raw DLL.

## Deliberately not claimed

- No radio audio playback, sound-script resolution, speaker arbitration, priority queue, HUD, subtitles or network replication.
- No client-side `C_RadioMessage` class has been established by this evidence.
- No exact `Remove` destruction semantics or output-queue lifetime parity.
- No SourceVR factory registration, map parser, VScript binding or engine-owned entity integration.
- No original binary bytes were re-materialized in this run; this implementation uses the preserved hash-pinned audit evidence. The original DLL remains read-only and is not committed.

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

`sin_radio_message_runtime_tests` covers typed field setters, local-player gating, OnDispatch output integration, activator/caller context, host removal requests and stale generation rejection. The test uses a synthetic host; it is not an original map or engine execution.
