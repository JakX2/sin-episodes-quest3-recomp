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
- SetCamera stores the monitor camera name and unlinks the previous camera handle. Resolution is deferred to refresh_monitor_camera(), matching the separation between the input handler and recurring vehicle update. An absent map key remains unknown rather than being treated as an empty name.
- MonitorOn/MonitorOff are idempotent. On a transition they send the named entity monitor the open/close input. MonitorOff then clears the camera link. The initial monitor flag must be supplied; an unknown constructor default returns UninitializedState.
- Camera refresh requires an active monitor, a known nonempty name, a live named entity, and a CPointCamera-compatible host object. Failed lookup/type/link paths clear the saved camera name.
- RadioOn/RadioOff/SetRadioStation resolve the linked radio entity and delegate to it. The original missing-handle fallback is not sufficiently named, so the portable layer reports MissingLinkedEntity rather than inventing a radio effect.
- EnterVehicle/ExitVehicle delegate to the real host passenger/vehicle API; they are not simulated here.

The client-side report independently identifies C_PropSinVehicle and DT_PropSinVehicle, with the m_bHeadlightIsOn receive property. That is the only client network property wired by this component.

## View-specific damage stage

The Phase 24 report identifies a 100-byte view record, with the float view_damage_mod at +0x60. The original constructor writes 1.0f at 0x2233236C; the KeyValues loader can replace it from the view_damage_mod setting at 0x223335DA. The damage routine at 0x2232F990 applies this field only after its earlier damage-type and global scaling stages, and only on the player-type branch. The final multiplication is at 0x2232FA19 after obtaining the selected view through 0x222488E0.

The new apply_view_damage_modifier() method reconstructs that final stage only:

- Non-player entities return the already-scaled value unchanged.
- Player entities with a valid selected view multiply by that view's damage_modifier.
- A default-constructed ViewRecord uses 1.0f, matching the original constructor default.
- Values are not clamped; zero, negative and greater-than-one multipliers are preserved because the recovered path is a direct float multiplication.
- Missing or out-of-range view indices return the pre-view-scaled value. This is an explicit memory-safety fallback: the original routine does not show the same bounds check, so this fallback is not claimed as original behavior.

The host must still supply the damage value after the earlier flag-selected/global stages, the result of the original player-type test, and the selected view index. The implementation deliberately does not name this incoming or outgoing damage, because the original caller/target direction remains unresolved. It is not yet connected to a Source damage callback.

Regression coverage now checks player-only application, selected-view lookup, the 1.0 default, invalid-index fallback, and unclamped zero/negative/>1 multipliers.

## Deliberate gaps

This is not a Source entity class or a SourceVR module. It does not implement the CPropVehicleDriveable base, full vehicle physics, NPC driver, door/window animation, sound, interactive dashboard hitboxes, monitor render texture, the complete 100-byte view records, all 23 saved datamap fields, the DoorClosed output, or a complete SaveRestore implementation.

The prior flag-selected damage factors and global scale in 0x2232F990 are not yet reconstructed. The view-specific final multiply is implemented as a separate stage, without inferring whether the caller represents incoming or outgoing damage.

## Local verification

The new component was compiled with C++17, -Wall, -Wextra, -Wpedantic and -Werror. Its CMake/CTest target passed locally. Tests cover the 25 typed inputs, integer/void/string validation, indexed and bulk door/window dispatch, SetView player fields, headlight publication, monitor idempotency and camera linking, stale entity handles, radio resolution, vehicle entry/exit delegation, and the player-only view-specific damage stage.

These are portable reference tests. They are not a SourceVR engine test, Android headset test, or Quest 3 execution.
