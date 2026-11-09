# Phase 138: original scene and scripted-sequence map-output routing

## Evidence provenance

This implementation is an independently authored C++17 reconstruction based on the preserved read-only static-analysis evidence:

- Original 2006 Windows x86 server.dll SHA-256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- Original 2006 Windows x86 client.dll SHA-256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- Preserved evidence: sin_scene_sequence_io_evidence.json and sin_scene_anim_event_dispatch_evidence.json.

The raw owner-supplied DLL ZIP was not materialized in this execution, so this change does not claim fresh disassembly. The original PE32 addresses below are evidence references only. No Windows x86 offset, function address, or vtable slot is used as an ARM64 ABI.

## Recovered map-output contract

The original server registers:

- logic_choreographed_scene as CSceneEntity, with OnStart, OnCompletion, OnCanceled, and OnTrigger1 through OnTrigger8.
- scripted_sequence as CAI_ScriptedSequence, with OnBeginSequence, OnEndSequence, and OnScriptEvent01 through OnScriptEvent08.

The preserved CSceneEntity trigger dispatcher at 0x221A2629 uses an eight-entry jump table beginning at 0x221A2808. Each numbered case selects its corresponding output member and calls the shared output-firing routine. The original cancellation and completion paths select OnCanceled at 0x221A2230 and OnCompletion at 0x221A72D5. The choreography start path fires OnStart at 0x221ADDC6.

For CAI_ScriptedSequence, the preserved NPC lifecycle callsites reach OnBeginSequence through 0x221ADF70 and OnEndSequence through 0x221AE2C0. The numbered script-output dispatcher at 0x221ADEC0 accepts only event numbers 1 through 8, selects the corresponding output at a 24-byte stride, and calls the shared output-firing routine.

A separate NPC animation-event route uses raw event ID 0x3EB, parses its parameter as an integer, resolves m_hCine, and invokes the numbered dispatcher. The original symbolic name for 0x3EB is not established; the portable constant intentionally preserves only the numeric ID.

## Independent implementation

- Map the eight CSceneEntity trigger numbers to OnTrigger1..8.
- Map the eight CAI_ScriptedSequence event numbers to OnScriptEvent01..08.
- Expose the verified lifecycle outputs without inventing scene playback behavior.
- Preserve a host-owned opaque output-context token so a real engine adapter can carry its own caller, activator, value, and timing data.
- Reject numbered events outside 1..8 before calling the host.
- Route raw event 0x3EB only when the host has resolved the linked sequence and parsed its integer parameter.
- Use generation/validity-aware host ownership for scene entity tokens; the portable library never dereferences Source objects.

## Evidence boundary

The scene/sequence class infrastructure may be inherited from Source rather than unique to SiN. No stock-HL2 binary comparison is claimed. The evidence proves the original SiN registrations and call paths, not that every campaign VCD/model contains every event.

This library routes map outputs only. It does not implement the scene manager, VCD parser, NPC actor selection, movement, animation playback, script event production, or campaign transitions. The actual engine host must provide the correct FireOutput context and invoke these functions at the recovered lifecycle/event callsites.

## Verification

The C++ regression checks all 8 choreography triggers, all 8 scripted-sequence events, the lifecycle outputs, raw event 0x3EB, invalid event numbers, missing sequence handles, stale entities, host rejection, and unchanged context-token forwarding.

CI is expected to run:
- Desktop CTest for the portable library.
- Android NDK arm64-v8a compilation of the library and regression executable.
- Android Bionic x86_64 execution of the same regression with a mock host.

These checks are not SourceVR execution or Quest 3 gameplay. No proprietary DLL, APK, original ELF, BSP, model, VCD, sound, script, or other game asset is committed or redistributed.
