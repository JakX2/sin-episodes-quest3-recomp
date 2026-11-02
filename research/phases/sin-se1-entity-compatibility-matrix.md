# SiN Episodes 2006 entity compatibility matrix

2026-11-02 | Original game DLL + original SDK FGD evidence. Not an ARM64 game implementation.

## Provenance

- User-owned Windows 2006 server.dll, SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
- The 510 verified server factory registration records from the original 22-byte x86 registration-call scanner in PR #63 (CSV metadata, no DLL bytes).
- Original SinEpisodesSDK.zip: SinEpisodesSDK/bin/SinEpisodes.fgd (139,100 bytes) and SinEpisodesSDK/bin/base.fgd (263,783 bytes).
- Suggested secondary source: https://developer.valvesoftware.com/wiki/List_of_SE1_entities . Access to that page returned **403 Forbidden**, so no entry is claimed to have been read from it. Optional manually verified plaintext list can later be compared via --wiki-plain. Without one, wiki membership remains unknown.

## Actual original SDK/binary comparison

The supplied original SDK contains 384 non-BaseClass FGD declarations, representing 383 unique entity classnames, plus 65 BaseClass definitions. With the 510 distinct original server.dll registration names, the union contains **536 unique names**.

| Evidence classification | Count |
|---|---:|
| binary_and_sin_fgd | 154 |
| binary_and_base_fgd | 203 |
| binary_only_no_sdk_declaration | 153 |
| sin_fgd_no_recognized_binary_registration | 18 |
| base_fgd_no_recognized_binary_registration | 8 |
| TOTAL distinct | 536 |

An FGD declaration documents Hammer/editor configuration, not necessarily compiled executable behavior. No match in the verified 510 factory call-shapes is not proof that a game factory is absent. The original DLL also contains engine helper classes not exposed in Hammer, such as ai_network, env_particlefire, grenade_homer and event_queue_saveload_proxy. The 26 editor-only names by this comparison include weapon_flaregun, weapon_rocket_launcher, weapon_sniper_rifle, npc_spotlight, func_detail, prop_static and env_cubemap.

## Selected SiN-specific FGD contract evidence

The analyzer resolves keyvalue/input/output names recursively through FGD base(...) definitions. Those names are not a substitute for verifying native behavior.

| Exact original registered classname | FGD evidence |
|---|---|
| npc_sintek_grunt | BaseGrunt, NumGrenades, GrenadeHeight, BeginRappel, ThrowGrenades, StartPatrolling, OnRappelTouchdown |
| npc_sintek_jetpack | JetpackBase, KeepBackDistance, MaxFireRange, ActivateJets, DeactivateJets, ChangePathTrack |
| npc_quadralex | BaseNPC, Behavior, SetChargeTarget, SetRunTarget, OnSeeHiddenPlayer |
| trigger_dynamic_difficulty | TriggerOnce, EndOfGame, EstimatedDifficulty, EstimatedTime |
| prop_phone | KeypadName, PhoneBook, FailSound, DialNumber, Enable, Disable |
| item_antigen_dispenser | antigen, disabled, OnCanisterInserted, OnCanisterEjected |
| prop_vehicle_sin | BaseDriveableVehicle, vehiclescript, HeadlightOn, HeadlightOff, SetCamera, WipersOn, EnterVehicle, ExitVehicle |
| weapon_magnum | Weapon, PrimaryAmmoCount, SecondaryAmmoCount, OnPlayerPickup |
| logic_relay | base.fgd, Trigger, CancelPending, OnTrigger |
| func_door | base.fgd, Open, Close, Lock, Unlock, OnFullyOpen, OnFullyClosed |

## Reproduce using private original inputs

Run the standard-library analyzer from the repository working directory, with user-owned SDK and metadata CSV:

    py research/tools/audit_sin_entity_compatibility.py --factory-csv C:\private\SiN_2006_Server_Entity_Factories.csv --sdk-zip C:\private\SinEpisodesSDK.zip --output-json C:\private\sin-entity-matrix.json --output-csv C:\private\sin-entity-matrix.csv

Optional: --wiki-plain C:\private\verified-vdc-se1-one-classname-per-line.txt . Do not infer wiki membership from a 403 response or parse unrelated pages as proof.

## Practical implementation priorities

1. **Base entity creation and map progression:** worldspawn, info_player_start, player, logic_auto, logic_relay, trigger_once, func_door, and related transition/landmark entities. Need actual ARM64 engine-compatible factory and callback integration, not synthetic tests only.
2. **Game entity keyvalue and I/O:** implement targetname, keyvalues, outputs, delayed dispatch, input handlers and save/restore state using actual SourceVR host signatures. FGD documents candidates for each contract; game C++ implementation must still be reconstructed.
3. **SiN campaign interactions:** port prop_phone, item_antigen_dispenser, poison/difficulty-related trigger/logic behaviors and verify against original campaign maps.
4. **Enemies, vehicle and weapons:** npc_sintek_grunt, npc_sintek_jetpack, npc_quadralex, prop_vehicle_sin, weapon_magnum and other recognized SiN entities, with verified original AI and network state.
5. **Campaign audit:** inventory actual user-owned BSP entity lumps and rank missing behaviors by map occurrence, rather than trying to hand-port 536 names blindly.

## Evidence boundaries

- The compiled 2006 x86 server factory and FGD class are different source evidence layers.
- The FGD base.fgd classification does **not** demonstrate that the modern SourceVR engine already implements a corresponding entity.
- Presence on a community wiki does **not** establish compiled support.
- The project's current Android ARM64 client/server diagnostic stubs do **not** implement these 536 game entities.
