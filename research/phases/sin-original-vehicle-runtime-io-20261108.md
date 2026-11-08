# Original SiN CPropSinVehicle input runtime

Date: 2026-11-08

## Evidence provenance

This implementation is an independent C++17 reconstruction based on the preserved Phase 21, Phase 22, and Phase 24 static-analysis reports for the owner-supplied 2006 SiN Episodes bin.zip. Those reports record the original server.dll SHA-256 as 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 and client.dll SHA-256 as 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

The reports describe read-only PE32 analysis. Their virtual addresses and object offsets refer only to that exact Windows x86 build. This runtime does not transplant those offsets into Android or SourceVR objects. No original DLL, ELF, APK, BSP, model, sound, script, or other game asset is included.

No fresh raw-binary verification was run while authoring this change: the Library references and extracted research reports were readable, but raw-byte materialization of the archived ZIP was unavailable in this execution environment. The evidence below is therefore attributed to the existing hash-pinned reports, not to a new disassembly pass.

## Recovered entity contract

The original server datamap registers 25 CPropSinVehicle map inputs:

- Integer: EnterVehicle, ExitVehicle, EnableUse, SetView, LockView, OpenDoor, OpenWindow, CloseDoor, CloseWindow, EnableHints, EnableAmmo, SetRadioStation.
- Void: OpenDoors, CloseDoors, OpenWindows, CloseWindows, HeadlightOn, HeadlightOff, WipersOn, WipersOff, MonitorOn, MonitorOff, RadioOn, RadioOff.
- String: SetCamera.

The typedescription report records all 25 input handler addresses. The runtime preserves the exact external names and field types in original_input_specs().

The original class inherits from CPropVehicleDriveable. This component reconstructs only the SiN-specific extension contract; ordinary Source vehicle physics, passenger handling, collision, suspension, driving controls and engine entity registration must come from a supported host.

## Behavior represented

- WipersOn/WipersOff write the recovered wiper state.
- OpenDoors/CloseDoors and OpenWindows/CloseWindows fan out across the host-owned collections. The host receives every index; a failed item does not abort the remaining collection walk.
- OpenDoor/CloseDoor and OpenWindow/CloseWindow pass an integer collection index through a bounds-checked host boundary. The local bounds check is a portable safety guard, not a claim that the original handler used the same guard.
- EnableUse, LockView, EnableHints and EnableAmmo update the corresponding recovered state fields. EnableAmmo maps to the original m_bInfiniteAmmo field.
- HeadlightOn/HeadlightOff delegate the full light-side effect to the host and publish the confirmed client property m_bHeadlightIsOn. The original handler performs additional work beyond a Boolean write.
- SetView validates the player-like activator and the index against the configured view count. Phase 24 records that the selected index and the view_allow_weapons byte are copied into two player fields. The remaining seat/view record layout is not guessed.
- SetCamera stores the monitor camera name and unlinks the previous camera handle. Resolution is deferred to refresh_monitor_camera(), matching the separation between the input handler and recurring vehicle update.
- MonitorOn/MonitorOff are idempotent. On a transition they send the named entity monitor the open/close input. MonitorOff then clears the camera link.
- Camera refresh requires an active monitor, a nonempty name, a live named entity, and a CPointCamera-compatible host object. Failed lookup/type/link paths clear the saved camera name.
- RadioOn/RadioOff/SetRadioStation resolve the linked radio entity and delegate to it. The original missing-handle fallback is not sufficiently named, so the portable layer reports MissingLinkedEntity rather than inventing a radio effect.
- EnterVehicle/ExitVehicle delegate to the real host passenger/vehicle API; they are not simulated here.

The client-side report independently identifies C_PropSinVehicle and DT_PropSinVehicle, with the m_bHeadlightIsOn receive property. That is the only client network property wired by this component.

## Deliberate gaps

This is not a Source entity class or a SourceVR module. It does not implement the CPropVehicleDriveable base, full vehicle physics, NPC driver, door/window animation, sound, interactive dashboard hitboxes, monitor render texture, the complete 100-byte view records, all 23 saved datamap fields, the DoorClosed output, or a complete SaveRestore implementation.

The damage_modifier field at view-record offset +0x60 is preserved as metadata but not applied: the Phase 24 report identifies the player-type branch and field copy, but the full damage callsite and direction are not sufficiently established for an authoritative effect.

The original monitor state must be supplied from map/save state or a future verified constructor reconstruction. An absent value returns UninitializedState instead of assuming that the original constructor defaulted it to false.

## Local verification

The new component was compiled with C++17, -Wall, -Wextra, -Wpedantic and -Werror. Its CMake/CTest target passed locally. Tests cover the 25 typed inputs, integer/void/string validation, indexed and bulk door/window dispatch, SetView player fields, headlight publication, monitor idempotency and camera linking, stale entity handles, radio resolution, and delegation of vehicle entry/exit.

These are portable reference tests. They are not a SourceVR engine test, Android headset test, or Quest 3 execution.
