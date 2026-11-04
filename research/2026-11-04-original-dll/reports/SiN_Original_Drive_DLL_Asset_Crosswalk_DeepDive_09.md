# SiN Episodes: Emergence | Original DLL + Drive Asset Crosswalk 09

**2026-11-04. Read-only static research.** Original SHA-pinned 2006 x86 `server.dll` and `client.dll`, seven game asset archives in Google Drive, and five HL2 runtime console logs. No game binaries or port repository were changed. All addresses are original virtual addresses (VAs), not file offsets.

## 1. Research result

- **1,531** missing entity creation attempts, **49** unique map classnames across the five logs. All **49** have been paired with the original `server.dll` classname registration and a `CEntityFactory<T>` RTTI specialization. This is factory metadata verification, **not** proof of runtime entity instantiation in a new engine.
- **23** `CPropSinVehicle` map-facing inputs recovered from the compiled datamap, including doors/windows, radio, headlight, wipers, camera, monitor, and view/usage commands.
- `CRadioMessage` has five distinct map inputs (`SetMessage`, `SetSpeaker`, `SetPriority`, `Dispatch`, `Remove`) and `OnDispatch`. The actual `Dispatch` routine uses the Source output-firing helper at call site `0x222E4BFE` (target `0x22156900`), with a single-player path to resolve the local player. Do not confuse this class with `CRadio` or `CHudPoliceRadio`.
- Original `resource/se1_english.txt` supplies the exact localization tokens which were displayed unresolved on the HL2 dock terminal.
- All four VCD filenames that HL2 reported as absent **exist in the provided scenes archive**. Presence in the ZIP does not prove correct mounting, extraction or scene-cache behavior in the test runtime.

## 2. Evidence provenance

| Local/Drive asset | Entries | Files | SHA-256 |
|---|---:|---:|---|
| `scripts.zip` | 123 | 118 | `517d5b72f23cd6e1164e7c43d72eef97d4c80285365734747c8ced9199e6985d` |
| `scenes.zip` | 279 | 261 | `b3bc833210dabb7fb30ffde46be1ca7136f40e09ae97b42713f48b5b22e5739e` |
| `resource.zip` | 109 | 107 | `c94cca259dab6cbf319e381746b2cbb1fd4d8db712d50186acf4c9a03f5294f7` |
| `expressions.zip` | 36 | 27 | `055a8f43033e015456f2feac8c2f60759f78c0ae6fffd61dcef05c3e2b2ce2fa` |
| `cfg.zip` | 16 | 15 | `2b5dcf23deedab89c82af33503a3c982739965abbcc64338cee5ba6edb322bb6` |
| `bin.zip` | 2 | 2 | `9cf9878fdc8e5a041d664b7102f944646bff100b0c25097cffdbe81f4726d5f4` |
| `bin 1.zip` | 34 | 33 | `2a27dc9f7b2d9bf67ae1b803b86d8ea6bbc8c8caf2e272dca6d23cc6dea2b390` |

Original server SHA-256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
Original client SHA-256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The original `bin.zip` holds `client.dll` and `server.dll`; `bin 1.zip` holds an engine/support DLL set. `expressions.zip` holds facial-expression files for Jessica, JC and Elexis plus common expression presets. `cfg.zip` contains game/config, chapters and control-related files. The original `scenes.zip`, `scripts.zip` and `resource.zip` provide the main behavioral content for this round. The SDK/SourceVR/large APK archives are **catalogued but not exhaustively reverse-engineered in this round**; they will be revisited only where their versions or interfaces answer a concrete question.

## 3. Console attempt counts, per supplied log

| Log | Failed creations | Unique classnames |
|---|---:|---:|
| `sin stuff intro-u4-1.txt` | 573 | 37 |
| `sin stuff u4-2 to pit 2.txt` | 442 | 41 |
| `sin stuff highrise.txt` | 396 | 39 |
| `sin stuff finale.txt` | 110 | 21 |
| `sin stuff arena.txt` | 10 | 5 |

These are **console occurrences**, not unique objects or per-map placements: some logs include more than one load of a given map. All 49 registrations were resolved by finding a literal classname, its `push` reference in registration code, the immediately adjacent factory-object pointer, and the factory object’s actual MSVC RTTI.

### Most common missing classnames

| Classname | Attempts | Original C++ class |
|---|---:|---|
| `npc_druglab_grunt_pistol` | 195 | `CNPC_MobGrunt` |
| `npc_druglab_grunt_assault_rifle` | 103 | `CNPC_MobGrunt` |
| `prop_exploding_barrel` | 92 | `CExplodingBarrel` |
| `trigger_dynamic_difficulty` | 88 | `CTriggerDynamicDifficulty` |
| `npc_druglab_grunt_elite` | 69 | `CNPC_EliteGrunt` |
| `prop_acetylene_tank` | 69 | `CAcetyleneTank` |
| `npc_druglab_grunt_scattergun` | 66 | `CNPC_MobGrunt` |
| `prop_u4_barrel` | 62 | `CMutagenBarrel` |
| `env_radio_message` | 52 | `CRadioMessage` |
| `npc_generic_actor` | 49 | `CNPC_GenericActor` |
| `item_antigen_dispenser` | 49 | `CAntigenDispenser` |
| `trigger_secret` | 44 | `CTriggerSecret` |
| `weapon_grenade` | 41 | `CWeaponGrenade` |
| `item_ammo_duag` | 39 | `CItemAmmoDUAG` |
| `prop_mutagen_barrel` | 39 | `CMutagenBarrel` |
| `item_ammo_flak` | 38 | `CItemAmmoFLAK` |
| `npc_civilian` | 32 | `CNPC_Citizen` |
| `npc_druglab_jetpack` | 29 | `CNPC_DruglabJetpack` |
| `ai_goal_takecover` | 26 | `CAI_TakeCoverGoal` |
| `prop_phone` | 23 | `CPropPhone` |
| `item_ammo_magnum` | 23 | `CItemAmmoMagnum` |
| `weapon_scattergun` | 22 | `CWeaponScatterGun` |
| `prop_antigen_canister` | 21 | `CAntigenCanister` |
| `npc_recombinant_major` | 21 | `CNPC_RecombinantMajor` |
| `item_ammo_buckshot` | 20 | `CItemAmmoBuckshot` |

### Confirmed multi-name aliases

- `CNPC_MobGrunt`: `npc_druglab_grunt_assault_rifle`, `npc_druglab_grunt_pistol`, `npc_druglab_grunt_scattergun`
- `CAntigenCanister`: `prop_antigen_canister`, `prop_antigen_explosive_barrel`
- `CMutagenBarrel`: `prop_mutagen_barrel`, `prop_u4_barrel`
- `COxygenTankSmall`: `prop_oxygen_tank`, `prop_oxygen_tank_small`
- `COxygenTankLarge`: `prop_oxygen_tank_large`, `prop_propane_tank`

These are **factory-equivalent class registrations**, not proof of identical initial parameters or gameplay. Classname-specific initialization remains a separate research task.

## 4. `CPropSinVehicle` server datamap: 23 separate map inputs

Each row is an actual compiled `typedescription_t` input, recovered from the original DLL datamap at `0x22531604` with a `0x34`-byte stride. The `handler VA` is the function pointer stored in the datamap. It verifies the handler exists, not complete in-game behavior.

| External input | Handler VA |
|---|---|
| `EnableUse` | `0x2232DD90` |
| `SetView` | `0x2232FA70` |
| `LockView` | `0x2232DDC0` |
| `OpenDoor` | `0x22330800` |
| `OpenWindow` | `0x22330870` |
| `CloseDoor` | `0x223308D0` |
| `CloseWindow` | `0x22330940` |
| `OpenDoors` | `0x223309A0` |
| `CloseDoors` | `0x223309B0` |
| `OpenWindows` | `0x223309C0` |
| `CloseWindows` | `0x223309D0` |
| `HeadlightOn` | `0x223309E0` |
| `HeadlightOff` | `0x22330A20` |
| `WipersOn` | `0x2232DDF0` |
| `WipersOff` | `0x2232DE00` |
| `MonitorOn` | `0x22330A60` |
| `MonitorOff` | `0x22330A90` |
| `SetCamera` | `0x2232FB20` |
| `EnableHints` | `0x2232DE10` |
| `EnableAmmo` | `0x2232DE40` |
| `RadioOn` | `0x2232F500` |
| `RadioOff` | `0x2232F540` |
| `SetRadioStation` | `0x2232F580` |

Additional saved vehicle state in that same datamap includes `m_hRadio` `+0x768`, `m_bRadioOn` `+0x750`, `m_bWipersOn` `+0x74E`, `m_bHeadlightIsOn` `+0x748`, `m_bEnableUse` `+0x74A`, `m_bViewsLocked` `+0x74B`, `m_bInfiniteAmmo` `+0x74C`, and `m_fDamageTolerance` `+0x754`. These field names/offsets were inspected directly from the same DLL data-region metadata. The `RadioOn` method at `0x2232F500` resolves the stored handle at `+0x768` and calls the radio path; `SetRadioStation` at `0x2232F580` similarly forwards the integer input. This is a real interaction between the vehicle and a separately referenced entity.

## 5. Separate radio entities and event plumbing

**`env_radio` / `CRadio`** has the following datamap fields:

| Internal name | Offset | Exposed key |
|---|---|---|
| `m_hSpeaker` | `0x394` | `` |
| `m_fVolume` | `0x398` | `Volume` |
| `m_sScriptFile` | `0x39C` | `ScriptFile` |
| `m_fSoundEnd` | `0x3A0` | `` |
| `m_nStation` | `0x3A4` | `` |

This class references the literal `scripts/radio.txt` at `0x22462B28`. That script defines one `Scanner` station with seven MP3 entries (`*radio/hc_scan01.mp3` through `*radio/hc_scan07.mp3`). This is the music/scanner station data, not the map-logic dispatch below.

**`env_radio_message` / `CRadioMessage`** has:

| Internal field or input | Entity offset | External key/input/output | Handler VA |
|---|---|---|---|
| `m_message` | `0x3C8` | `message` | `` |
| `m_speaker` | `0x3CC` | `speaker` | `` |
| `m_priority` | `0x3C4` | `` | `` |
| `InputSetMessage` | `0x0` | `SetMessage` | `0x222E4510` |
| `InputSetSpeaker` | `0x0` | `SetSpeaker` | `0x222E43A0` |
| `InputSetPriority` | `0x0` | `SetPriority` | `0x222E43D0` |
| `InputDispatch` | `0x0` | `Dispatch` | `0x222E4B70` |
| `InputRemove` | `0x0` | `Remove` | `0x222E4C10` |
| `m_OnDispatch` | `0x394` | `OnDispatch` | `` |

At `0x222E4B70`, the `Dispatch` handler accepts input data, updates `m_message` at `+0x3C8`, handles dispatch state, invokes a helper, then conditionally finds the local single-player character and fires `m_OnDispatch` at `+0x394` via the shared output helper at `0x22156900`. The actual UI/audio delivery downstream of the other helper has not yet been decoded. Importantly, `CRadioMessage` is not a mere named sound file: it is a runtime event object.

**`client.dll`** contains the RTTI class `CHudPoliceRadio`; `scripts/HudAnimations.txt` has animation sequences targeting `HudPoliceRadio`. This independently establishes a dedicated client presentation component. The exact server-to-client message path remains to be traced; **do not assume it is proven just because both ends exist**.

## 6. Scripted scenes: ZIP content vs HL2 scene errors

| Scene logged missing by HL2 | Found inside `scenes.zip` |
|---|---|
| `scenes/se1_highrise/jess_wakeupblade.vcd` | yes |
| `scenes/se1_finale/teaser_radek_and_jessica.vcd` | yes |
| `scenes/se1_finale/teaser_jc_hospital.vcd` | yes |
| `scenes/se1_finale/teaser_elexis_laughing.vcd` | yes |

There are **261** original `.vcd` files and **392** `speak` events inside them. **376** events resolve to an identically named sound block in the supplied sound scripts. **16** events do not, representing **10 unique cue names**, listed below. The lack of a definition within this archive does not establish absence from other mounted Source game resources.

- `finale.jc_655martial`
- `finale.jc_alreadystrt`
- `finale.jc_fullmed`
- `finale.jc_morebadnews`
- `npc_citizen.answer01`
- `npc_citizen.found_player`
- `npc_citizen.hi01`
- `npc_citizen.question01`
- `npc_citizen.watchout`
- `npc_citizen.whoops01`

The highrise scene `jess_wakeupblade.vcd` references `highrise.jess_shakeoff` and `highrise.jess_rodhole`, both found in `scripts/level_sounds_SE1_highrise_vo.txt`. The finale JC hospital scene references `finale.jc_uplink`, found in `scripts/level_sounds_SE1_finale_vo.txt`. Thus these scenes have their specific dialogue definitions available.

The `teaser_radek_and_jessica.vcd` scene uses an actor named `Roddick`, another named `Jessica`, a `Helicopter` actor, and a `firetrigger` event with parameter `1` for its fade-out. This corroborates the original `logic_choreographed_scene` `OnTrigger1`..`8` dispatch analysis from the earlier binary audit. The `teaser_elexis_laughing.vcd` scene contains facial flex-animation keys rather than a speech cue.

The supplied `scenes.zip` does **not** include a `scene.cache` file. That is an archive-level observation, not proof that the user’s real installation lacks one; depending on engine branch and mounting it may be generated or bundled separately. The exact cause of HL2’s “Scene missing” lines still needs a runtime filesystem trace.

## 7. Sound manifest and localization findings

`scripts/game_sounds_manifest.txt` contains 48 `precache_file` entries. The following referenced files are not present **in the provided `scripts.zip`**:

- `scripts/level_sounds_e3_c17.txt`
- `scripts/level_sounds_e3_town.txt`
- `scripts/level_sounds_e3_bugbait.txt`
- `scripts/level_sounds_SE1_finale_radio.txt`

Three are older E3-level sound scripts. The fourth is `scripts/level_sounds_SE1_finale_radio.txt`, which is more directly relevant to the endgame radio messages. These manifest references are **not evidence of a server.dll bug**; investigate whether other game mounts supply them and what the original build expects.

The original `scripts/phonebook.txt` contains 56 number→sound mappings, confirming that the phone number parser already reverse-engineered has genuine shipped content. `resource.zip` contains `resource/KeypadScheme.res`, and the client DLL references `KeypadScheme.res`, confirming the named in-world keypad resource is present in the Drive archive.

The screenshot’s raw dock panel localization tokens are present in UTF-16 `resource/se1_english.txt`:

| Key | Shipped English value |
|---|---|
| `SE1_VGUI_DOCKS_1A_SEWERHEADER` | Sewer Maintenance Operations - Area 74/321 |
| `SE1_VGUI_DOCKS_1A_DOORSCLOSED` | Flood Doors Closed |
| `SE1_VGUI_DOCKS_1A_OVERRIDEOFFLINE` | OVERRIDE OFFLINE |
| `SE1_VGUI_DOCKS_1A_SEALDOORS` | Seal Dam Doors |
| `SE1_VGUI_DOCKS_1A_RAISEWATER` | Raise Water Level |
| `SE1_VGUI_DOCKS_1A_LOWERWATER` | Lower Water Level |

These resolve the **asset-existence** question. The HL2 test still needs to be checked for registration/mounting of the localization file and VGUI screen update logic.

## 8. Recommended next reverse-engineering investigations (no port changes)

1. **Radio path:** trace `CRadioMessage::Dispatch` helper `0x222E47A0` and client HUD event registration to establish the actual network/sound delivery semantics; do not conflate `CRadio`, `CRadioMessage`, and `CHudPoliceRadio`.
2. **Vehicle path:** from the `m_hRadio` calls at `0x2232F500`, trace `CRadio` lifecycle and input handling plus the `CPropSinVehicle` door/window camera handlers. Use the 23 proven datamap entries as a concrete contract.
3. **Dynamic difficulty:** factory for `CTriggerDynamicDifficulty` is present, with 88 missing instances in the five logs. Recover its actual datamap and its calls into `CDynamicDifficulty` rather than treating it as a passive brush trigger.
4. **Dialogue/choreography:** build a verified VCD→sound name→manifest path map; resolve the four missing finale JC cues; compare map VCD paths against the game filesystem search paths and scene cache behavior.
5. **Weapons/NPCs:** continue reconstructing `CNPC_MobGrunt` and variant configuration, followed by its linked AI goal and the SiN-specific client weapons/animations. Generic Source physics/save-load is lower priority unless a verified incompatibility appears.

## Reproduction and limits

Run `python sin_drive_crosswalk_audit.py /path/to/assets /path/to/original/server.dll` where the supplied assets directory includes the five named HL2 logs and seven ZIP archives, and `client.dll` is alongside `server.dll`. The script SHA-checks both game DLLs, proves all 49 factory lookups and verifies relevant input datamaps and call-site instruction anchors. It writes `sin_drive_crosswalk_evidence_09.json`. No binaries, source reconstructions, or proprietary asset contents are exported into the JSON.

Known limits: `client.dll` HUD class existence is not the complete network message chain; scripted actor names cannot be mapped to unique runtime instances from VCD text alone; asset presence in Drive does not prove mounting by HL2; factory registration does not imply successful spawning. Arena and finale log attempts include duplicates; missing scene cache is only observed within the ZIP.