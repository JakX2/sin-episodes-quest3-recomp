# Phase 139: original CSceneEntity playback input reconstruction

## Evidence provenance

This phase extends the preserved original 2006 `server.dll` static-analysis records:

- Server SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- Client SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- Evidence files: `sin_scene_sequence_io_evidence.json` and `sin_scene_anim_event_dispatch_evidence.json`
- The original owner ZIP was not materialized for fresh disassembly during this phase. No original code was executed.

Original x86 addresses, object offsets and vtable byte offsets are provenance labels only. They are not used as native ARM64 object offsets or virtual method slots.

## Reconstructed input order

### Start

Original `InputStartPlayback` at `0x221AAC70`:

1. Checks the active-playback state.
2. Checks the actor-wait state.
3. Calls the prestart routine.
4. Delegates to the scene playback virtual.

The prestart path fires `OnStart` at `0x221ADDC6`. The portable adapter preserves the observable order as prestart, `OnStart`, then playback. Its host's `prepare_start` callback therefore represents prestart work excluding the output itself.

The first two guards are short-circuited in their observed order. A playing scene is rejected before the actor-wait query.

### Pause

Original `InputPausePlayback` at `0x221A2130`:

1. Calls the original virtual at byte offset `+0x2AC`.
2. Sets the pause flag at original object offset `+0x3F4`.

The portable adapter preserves that order through `pause_playback` followed by `set_pause_flag(true)`. The original offset is not exposed as a native field offset.

### Resume and Cancel

- `InputResumePlayback` at `0x221A2150` calls the virtual at original byte offset `+0x2B0`.
- `InputCancelPlayback` at `0x221A2160` calls the virtual at original byte offset `+0x2B4`.

The handlers' direct evidence does not establish an additional pause-flag write for Resume. The portable code does not invent one. The cancel virtual owns its internal cancellation effects, including any `OnCanceled` output; the adapter does not fire that output twice.

## Implementation

- `include/sin_compat/original_scene_playback.hpp`
- `src/original_scene_playback.cpp`
- `tests/test_original_scene_playback.cpp`

The runtime is a portable input dispatcher. The engine host supplies live scene state and the actual playback operations. It reuses Phase 138's output router for `OnStart`.

The host methods are explicit call boundaries, not guessed SourceVR ABI methods. This does not instantiate `CSceneEntity`, parse VCD choreography, select actors, play animations, implement `InterjectResponse` or `StopWaitingForActor`, or provide campaign transitions. `OnCompletion` and the full scene-manager lifecycle remain incomplete.

## Verification

The new regression covers:

- Start guard order and short-circuit behavior.
- Prestart → `OnStart` → playback order.
- Continuation when the host reports an output-delivery rejection.
- Pause virtual → pause-flag write order.
- Resume delegation without a fabricated flag write.
- Cancel delegation without duplicate `OnCanceled`.
- Missing handles and invalid input values.

CI builds the portable component for desktop, Android NDK ARM64 and Android Bionic x86_64. The Bionic emulator runs only the mock-host regression. None of these checks is SourceVR execution or Quest 3 gameplay.

No proprietary DLL, APK, original ELF, BSP, model, VCD, sound, script or game asset is included.
