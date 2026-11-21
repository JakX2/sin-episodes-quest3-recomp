# SiN Episodes: Emergence | public reverse-engineering research

> **Current decompilation-first research (21 November 2026):** recover
> original 2006 `server.dll` and `client.dll` behavior before implementing
> native Windows, Android or Quest gameplay. Server Phase537 authenticates
> 944 bytes of original Grunt fallback and assault helper logic.
> The client sequence includes:
> Phase538 original 1,220-byte VClient011::Init (interface prefix mapped),
> Phase539 five additional game-system helpers (525 bytes),
> Phase540 four original VGUI/HUD bootstrap helpers (239 bytes),
> Phase541 complete VGUI factory resolver (586 bytes),
> Phase542 four token-converter registry methods (638 bytes),
> Phase544 eight converter red-black insertion/rotation functions (2,528 bytes),
> Phase545 eight distinct 12-byte string-token tree methods (2,678 bytes),
> and Phase546 eight original string-comparator, constructor and casefold helpers (827 bytes), including the HUD converter's static casefolded default. Phase547 adds nine distinct original node/index, array-growth, and string-page methods (615 bytes). Phase548 authenticates another six original global token-table lifecycle and symbol/string resolver methods (553 bytes). Phase549 authenticates five VClient011 level startup/shutdown and owned-global cleanup methods (646 bytes). Phase550 separately authenticates four CVoiceStatus update/bitfield methods (635 bytes). Phase551 adds eight distinct CVoiceStatus 32-byte-key hash and blocking-toggle functions (657 bytes). Phase552 adds 13 more native CVoiceStatus vtable, singleton, callback and timer methods (749 bytes), including its authenticated MSVC RTTI. Phase553 verifies the exact voice_ban.dt V1 binary persistence methods (3 more methods, 322 bytes). Phase554 authenticates four original bit readers (232 bytes) and VoiceMask's 32+32+8 field ordering. Phase555 independently proves the matching original server 9-byte VoiceMask sender, zero-byte RequestState, and eight new native server methods (855 bytes), with a 0.3-second original timer. Phase556 authenticates nine additional CVoiceGameMgr server command/bitfield/compare functions (890 bytes), including original class RTTI and the native vban/VModEnable parsing branches. Phase557 traces the original VoiceMask recipient path to five complete CRecipientFilter/player-resolution helpers (318 bytes), with original IRecipientFilter RTTI. Phase558 pivots to SiN-specific Grunt combat speech: five distinct original server CAI_Sentence dispatch, policy, queue and diagnostics methods (877 authenticated bytes), with an original-free C++17 reference and SHA auditor. Phase559 authenticates another five complete original server gameplay methods (1,828 bytes), including the 1,327-byte Grunt prioritized combat selector, target handle and linked-entity eligibility/group-count rules. Phase560 authenticates nine more complete original methods (1,124 distinct server bytes) and recovers linked-group condition admission, shared bitmap reservations, filtered member iteration, and group identity checks. Phase561 authenticates four more complete bitmap-cache methods (855 native bytes): original strict time deadline at group +0x74, 30.0-second maintenance, record cache +0x5C/+0x60/+0x6C, and bit set/clear/test wrappers. Phase562 adds seven original group-constructor and membership methods (917 native bytes), verifying 120-byte allocation, the 16-member cap, prior-group detachment, owner linking, notification paths and record-index updates. Phase563 adds five complete original group-handle helper methods (402 distinct native bytes), proving forced eviction is handle-array compaction only, append has no intrinsic bounds or duplicate check, and membership search compares serial-resolved entity pointers (including null/null). Phase564 adds ten native Grunt virtual/group methods (376 distinct original bytes), confirming group +0x690 sets owner +0x9C4, virtual +0x154 resolves the +0x8A0 handle into an entity-keyed reservation, and +0x750 chooses real GRUNT_LAST_OF_SQUAD/GRUNT_MAN_DOWN dialogue based on eligible remaining members. Phase565 authenticates nine more native Grunt speech selector/getter methods (432 server bytes): event-based GRUNT_FLANK, GRUNT_DIE vs GRUNT_MUTANTDEATH, and GRUNT_PAIN vs GRUNT_TAUNT with native x87 thresholds and one-second cooldown.
> These method counts are distinct where noted; whole-function hash
> authentication is **not** equivalent to complete source translation.
>
> [Decompilation evidence and priorities](docs/DECOMPILATION_FIRST_PRIORITY_20261121.md)
> | [Original Grunt fallback](research/phases/2026-11-21-original-mobgrunt-combat-fallback-844-decomp.md)
> | [Client Init prefix](research/phases/2026-11-21-original-client-vclient011-init-interface-decomp.md)
> | [Client Init tail](research/phases/2026-11-21-original-client-init-tail-and-game-system-lifecycle.md)
> | [VGUI/HUD bootstrap](research/phases/2026-11-21-original-client-vgui-hud-bootstrap-decomp.md)
> | [VGUI factory resolver](research/phases/2026-11-21-original-client-vgui-factory-array-resolver-decomp.md)
> | [Type-converter registry](research/phases/2026-11-21-original-client-hud-type-converter-registry-decomp.md)
> | [Converter red-black insertion](research/phases/2026-11-21-original-client-converter-red-black-insertion-decomp.md)
> | [String-token tree](research/phases/2026-11-21-original-client-string-token-red-black-tree-decomp.md)
> | [Native string comparator modes](research/phases/2026-11-21-original-client-token-comparator-variants-decomp.md)
> | [Native token storage and string pages](research/phases/2026-11-21-original-client-token-storage-sentinel-capacity-pages-decomp.md)
> | [Global symbol token lifecycle and string ownership](research/phases/2026-11-21-original-client-token-lifecycle-global-table-reset-and-string-ownership.md)
> | [VClient011 level lifecycle and shutdown](research/phases/2026-11-21-original-client-vclient011-level-lifecycle-shutdown-decomp.md)
> | [Original CVoiceStatus player bitfields and server commands](research/phases/2026-11-21-original-client-voice-status-players-and-server-commands-decomp.md)
> | [Original voice blocked-key hash and toggle](research/phases/2026-11-21-original-client-voice-block-identity-hash-and-toggle.md)
> | [Original CVoiceStatus RTTI and lifecycle](research/phases/2026-11-21-original-client-cvoice-status-vtable-construction-messages-and-periodic-tick.md)
> | [Native voice_ban.dt V1 persistence format](research/phases/2026-11-21-original-cvoice-status-voice-ban-dt-persistence.md)
> | [Original VoiceMask 72-bit wire payload](research/phases/2026-11-21-original-client-voicemask-72-bit-message-layout.md)
> | [Original server VoiceMask/RequestState producer](research/phases/2026-11-21-original-server-voicegame-voicemask-requeststate-producer.md)
> | [Original CVoiceGameMgr commands and bitfields](research/phases/2026-11-21-original-server-voicegame-clientcommand-and-bits.md)
> | [VoiceMask recipient filtering and player resolution](research/phases/2026-11-21-original-server-voice-recipient-filter-and-player-resolution.md)
> | [Original Grunt sentence policy and queue chain](research/phases/2026-11-21-original-grunt-sentence-dispatch-and-queue-phase558.md)
> | [Original Grunt combat selector and linked target-group gates](research/phases/2026-11-21-original-grunt-combat-target-group-phase559.md)
> | [Grunt linked-group admissions, bitmap reservation and filtered iteration](research/phases/2026-11-21-original-grunt-group-admission-and-iterator-phase560.md)
> | [Original Grunt 30-second group bitmap refresh and bit access](research/phases/2026-11-21-original-grunt-linked-group-bitmap-refresh-phase561.md)
> | [Original Grunt group constructors and membership lifecycle](research/phases/2026-11-21-original-grunt-group-construction-membership-phase562.md)
> | [Original fixed group handle-array eviction and serial pointer equality](research/phases/2026-11-21-original-grunt-handle-array-eviction-phase563.md)
> | [Original Grunt group pointer virtuals, reservation release and squad speech](research/phases/2026-11-21-original-grunt-group-virtuals-and-squad-voice-phase564.md)
> | [Original Grunt flank, death, pain and taunt speech selection](research/phases/2026-11-21-original-grunt-pain-flank-death-sentence-phase565.md)
>
> These are original-binary-authenticated research artifacts, **not playable
> replacement DLLs**. Historical Phase56 scaffold sections below are retained.


**Status:** Phase 56 starting scaffold. Builds and runs as a **standalone native C++17 test library**. **Not a playable game, Source engine DLL, Android APK, or functional Quest 3 port.**

This repository begins turning the project's earlier assembly-driven research into one accumulative implementation with a host boundary. The original 2006 `server.dll` and `client.dll` are **not** included, nor are Half-Life 2 DLLs, proprietary models, maps, scripts or game assets.

## Preserved reverse-engineering research

The public [research index](research/README.md) includes Phase 44–55 original research reports, original verifier scripts where retained, provenance hashes, and partial earlier enemy/difficulty/vehicle notes. Archived findings are **not** automatically playable functionality.

## Welcome, contributors

This independent, unofficial project is designed to be **forkable and maintainable**.
If the original organizer steps away, start with [the handoff guide](docs/HANDOFF.md)
and the [contributing guide](CONTRIBUTING.md). Source code is provided under
[MIT](LICENSE); **the original games and engine are not**. See
[assets and rights guidance](docs/IP_AND_ASSETS.md).

## Try it

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The current CMake project builds `sin_weapon_runtime` and a `sin_weapon_runtime_tests` executable. The tests use a fake host and **synthetic fixture ammo capacity/model/FOV values**; they demonstrate integration behavior but do not establish live gameplay accuracy.

## What works in the prototype

- Source-style zoom button edge recognition (`0x00080000`) and a guarded ADS toggle/restore reference.
- Shared animation activity selection including assault-rifle unzoomed recoil activity mapping.
- Sequencing of activity-to-sequence lookup, model/offset bind and animation playback; missing sequences do not trigger a bind.
- Explicit no-HMD-FOV-change behavior in VR mode. Flatscreen FOV transitions are available to a host.
- Engine-event-driven ammunition transfer; ammunition is *not* refilled immediately upon beginning reload.
- Conservatively modeled single-round reload **only if externally enabled**. The original runtime enabling path is unproven.
- Holster flow that exits ADS and cleans up reload state before requesting holster animation.

## Not yet implemented

- Real SourceVR or Android/ARM64 engine hookup; no VR runtime bindings, headset position or motion controller interaction.
- Weapon models and animation sequences, real `ZoomFOV` settings, aim poses or reliable gun-specific script values.
- A complete state machine for all original reload timings, owner checks, and attack gates.
- Enemy AI, dynamic difficulty, vehicles, map scripts and campaign integration.
- Original game execution validation.

## Project layout

```text
include/sin_compat/     Public host-agnostic weapon API
src/                    Compatibility prototype implementation
tests/                  Mock-host integration tests
third_party/research/   Unmodified clean-room behavioral reference headers from earlier research
docs/                   Roadmap, status and evidence limitations
.github/workflows/      Linux CMake build/test CI (activates when hosted on GitHub)
```

## Connecting to a game engine

Implement `sin_compat::IWeaponHost` for the selected SourceVR branch. This requires correct engine lifecycle integration for resolving weapon activity sequences, viewmodel binding, animation playback, and *flatscreen-only* FOV changes. The engine is responsible for deciding the authoritative reload transfer event, ensuring a valid owner/weapon, providing per-gun settings, and storing game state.

This prototype deliberately does **not** claim to be a drop-in replacement for Ritual's compiled gameplay modules. See `docs/STATUS.md` for evidence and integration gaps.

## Repository hygiene

Never commit original `server.dll` or `client.dll`, extracted maps, models, assets, game archives, or personal game installation files. The `.gitignore` excludes common formats; review every file before publishing.
