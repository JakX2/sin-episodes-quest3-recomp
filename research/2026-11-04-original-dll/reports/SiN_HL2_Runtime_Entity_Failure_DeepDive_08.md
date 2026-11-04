# SiN Episodes: Emergence | HL2 Runtime Missing-Entity Analysis 08

**Evidence:** user-provided Half-Life 2 console log (`Pasted text.txt`, 1,245 lines), two in-game screenshots, and the original 2006 x86 SiN `server.dll`. Read-only original-DLL analysis on 2026-11-04. No port code or game binaries modified.

## Executive finding

The log records **573 failed entity creation attempts** covering **37 distinct map classnames**. For **all 37**, the original SHA-pinned server binary contains a literal classname registration site, a paired `CEntityFactory` object pointer, and a matching MSVC RTTI `CEntityFactory<...>` specialization. This is **37/37 registration verification**, not a runtime instantiation test. The replacement HL2 server lacks these registrations. Counts represent log messages and may include repeated loads or repeated instances.

Original `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

### Failures by role

| Role (name-based grouping) | Console attempts |
|---|---:|
| NPCs and AI | 267 |
| Props, vehicles and phones | 140 |
| Weapons and ammunition | 63 |
| Map triggers and progression | 53 |
| Scripted events and environment | 28 |
| Items, health and pickups | 22 |

## 1. Source-DLL factory crosswalk: high-priority examples

| Map entity classname | C++ factory class in original DLL | Attempts |
|---|---|---:|
| `npc_druglab_grunt_pistol` | `CNPC_MobGrunt` | 150 |
| `npc_druglab_grunt_scattergun` | `CNPC_MobGrunt` | 8 |
| `npc_druglab_grunt_assault_rifle` | `CNPC_MobGrunt` | 4 |
| `npc_civilian` | `CNPC_Citizen` | 32 |
| `npc_jessica` | `CNPC_Jessica` | 6 |
| `npc_jc` | `CNPC_JC` | 2 |
| `prop_vehicle_sin` | `CPropSinVehicle` | 3 |
| `prop_phone` | `CPropPhone` | 14 |
| `env_radio_message` | `CRadioMessage` | 23 |
| `trigger_dynamic_difficulty` | `CTriggerDynamicDifficulty` | 37 |
| `ai_goal_takecover` | `CAI_TakeCoverGoal` | 22 |
| `item_antigen_dispenser` | `CAntigenDispenser` | 19 |
| `weapon_scattergun` | `CWeaponScatterGun` | 6 |
| `prop_antigen_canister` | `CAntigenCanister` | 7 |
| `prop_antigen_explosive_barrel` | `CAntigenCanister` | 7 |
| `npc_druglab_helicopter` | `CNPC_SintekHelicopter` | 3 |

**Critical alias groups:**

- `npc_druglab_grunt_pistol`, `npc_druglab_grunt_scattergun`, and `npc_druglab_grunt_assault_rifle` all map to `CNPC_MobGrunt` (different factory objects, same C++ class). Do not design three independent AI systems unless deeper constructor/keyvalue evidence requires it.
- `prop_antigen_canister` and `prop_antigen_explosive_barrel` both map to `CAntigenCanister`. Do not presume the map names imply unrelated C++ classes.
- `prop_oxygen_tank` and `prop_oxygen_tank_small` both map to `COxygenTankSmall`. `prop_propane_tank` maps to `COxygenTankLarge`. Names are not a reliable guide to their internal C++ class identity.
- `npc_civilian` maps to `CNPC_Citizen`; the HL2 map classname `npc_citizen` should not be assumed equivalent to this original SiN registration without examining differences.

## 2. Complete missing-entity list, linked to original C++ factories

| Failed creations | Map classname | Original server factory C++ class | Registration VA |
|---:|---|---|---|
| 150 | `npc_druglab_grunt_pistol` | `CNPC_MobGrunt` | `0x223C6FE7` |
| 61 | `prop_exploding_barrel` | `CExplodingBarrel` | `0x223C4647` |
| 37 | `trigger_dynamic_difficulty` | `CTriggerDynamicDifficulty` | `0x223BE4A7` |
| 32 | `npc_civilian` | `CNPC_Citizen` | `0x223C6637` |
| 24 | `prop_acetylene_tank` | `CAcetyleneTank` | `0x223C4627` |
| 23 | `env_radio_message` | `CRadioMessage` | `0x223C3A27` |
| 23 | `npc_generic_actor` | `CNPC_GenericActor` | `0x223C6B97` |
| 22 | `ai_goal_takecover` | `CAI_TakeCoverGoal` | `0x223C7F87` |
| 19 | `item_antigen_dispenser` | `CAntigenDispenser` | `0x223C40A7` |
| 16 | `trigger_secret` | `CTriggerSecret` | `0x223BE4D7` |
| 14 | `item_ammo_duag` | `CItemAmmoDUAG` | `0x223C4057` |
| 14 | `prop_phone` | `CPropPhone` | `0x223C47A7` |
| 14 | `prop_u4_barrel` | `CMutagenBarrel` | `0x223C4667` |
| 13 | `weapon_grenade` | `CWeaponGrenade` | `0x223C50E7` |
| 11 | `item_ammo_flak` | `CItemAmmoFLAK` | `0x223C4037` |
| 9 | `item_ammo_magnum` | `CItemAmmoMagnum` | `0x223C3F77` |
| 8 | `npc_druglab_grunt_scattergun` | `CNPC_MobGrunt` | `0x223C7007` |
| 7 | `prop_antigen_canister` | `CAntigenCanister` | `0x223C46D7` |
| 7 | `prop_antigen_explosive_barrel` | `CAntigenCanister` | `0x223C46B7` |
| 6 | `npc_druglab_jetpack` | `CNPC_DruglabJetpack` | `0x223C68B7` |
| 6 | `npc_druglab_worker` | `CNPC_DruglabWorker` | `0x223C68E7` |
| 6 | `npc_jessica` | `CNPC_Jessica` | `0x223C6E17` |
| 6 | `prop_propane_tank` | `COxygenTankLarge` | `0x223C45C7` |
| 6 | `weapon_scattergun` | `CWeaponScatterGun` | `0x223C54C7` |
| 5 | `item_ammo_buckshot` | `CItemAmmoBuckshot` | `0x223C3FB7` |
| 5 | `weapon_magnum` | `CWeaponMagnum` | `0x223C5157` |
| 4 | `env_faint` | `CEnvFaint` | `0x223C3907` |
| 4 | `npc_druglab_grunt_assault_rifle` | `CNPC_MobGrunt` | `0x223C7027` |
| 4 | `npc_druglab_grunt_elite` | `CNPC_EliteGrunt` | `0x223C6947` |
| 3 | `item_megahealth` | `CMegaHealth` | `0x223C42D7` |
| 3 | `npc_druglab_helicopter` | `CNPC_SintekHelicopter` | `0x223C77E7` |
| 3 | `prop_vehicle_sin` | `CPropSinVehicle` | `0x223C5BE7` |
| 2 | `npc_jc` | `CNPC_JC` | `0x223C6DF7` |
| 2 | `prop_oxygen_tank` | `COxygenTankSmall` | `0x223C4607` |
| 2 | `prop_oxygen_tank_small` | `COxygenTankSmall` | `0x223C45E7` |
| 1 | `env_poison_gas` | `CPoisonGas` | `0x223C4557` |
| 1 | `npc_securitybot` | `CNPC_SecurityBot` | `0x223C7707` |

## 3. Screenshots: what is visibly working

**Dock terminal screenshot (`panels.png`):** a textured in-world control panel is visible, and multiple SiN-themed screen materials render. The foreground panel, however, displays unresolved literal localization tokens such as `#SE1_VGUI_DOCKS_1A_SEWERHEADER`, `#SE1_VGUI_DOCKS_1A_DOORSCLOSED`, and `#SE1_VGUI_DOCKS_1A_SEALDOORS`. This suggests some VGUI drawing or screen mounting works while language tokens are absent or not loaded. This screenshot alone does **not** establish whether command buttons fire their map inputs or whether a missing custom entity is involved. Research the appropriate localization resources before treating these labels as a DLL implementation failure.

**Turret screenshot (`turret.png`):** the turret model and environment appear in HL2. Per the tester, its behavior resembles a reskinned HL2 Combine turret. Class identity, AI identity, and map entity name cannot be established from the screenshot alone. This is a lower-priority research target unless more specific behavior failures arise.

## 4. Separate primary entity failures from secondary diagnostics

| Log diagnostic | Occurrences | Interpretation / what it does **not** prove |
|---|---:|---|
| `point_template` missing named targets | 16 | Often a cascade when referenced weapon/NPC entities failed to spawn; must inspect BSP targetname links to prove each causal link. |
| Template with no templates | 4 | Follow-on symptom when original template objects are absent, not necessarily a template-system incompatibility. |
| `npc_template_maker` no template NPC | 1 | Plausibly cascades from absent SiN NPC factory. |
| Door model has no `door_options` | 30 | Door metadata/model QC/keyvalue mismatch; needs asset and engine-version check separate from missing factory registrations. |
| 32 kHz MP3 rejected | 3 | Audio format/runtime support issue, not missing server class or proof that sound files are corrupt. |
| Save/load attempts | 9 | Requested save paths seen in log. No cause can be inferred without files and run context. |
| `ERROR: couldn't open` | 5 | Could mean absent save or wrong path; not proof that Source save serialization is incompatible. |
| Water shader `$abovewater` | 12 | Material/shader compatibility warning; unrelated to missing gameplay factories. |
| Static prop has no VPhysics model | 3 | Collision model mismatch, not evidence that SiN physics engine must be rewritten. |
| Empty ambient_generic | 2 | A separate map entity sound-configuration issue. |
| `NULL Ent in UTIL_PrecacheOther` | 1 | Possible downstream effect of absent entity, requires caller context. |

**Log limitations:** The pasted text has save filenames for `se1_intro03`, `se1_docks01` through `se1_docks04`, and `SE1_U4Lab01`/`02`, but it does not provide reliable complete per-map start markers and timestamps. Do not assign each failed entity count to a specific map solely by line order.

## 5. What this changes in the reverse-engineering roadmap

1. **Entity factory registry and aliases first.** The original maps depend on 37 factory names verified in this sample. Some use the same C++ class, so classify constructors, keyvalues and variant selection accurately before writing replacement logic.
2. **`CNPC_MobGrunt` and `CAI_TakeCoverGoal` next.** The most repeated missing NPC classname maps to `CNPC_MobGrunt`, and missing `ai_goal_takecover` may break combat scripting even if soldiers exist.
3. **`CTriggerDynamicDifficulty` and `CRadioMessage` next.** Both are absent often enough to cause substantial gameplay pacing and narrative-event divergence. Recover datamaps, event inputs/outputs and runtime conditions from `server.dll`.
4. **Interactive/mission-critical entities:** `CPropSinVehicle`, `CPropPhone`, `CAntigenDispenser`, plus Jessica and JC. Use previously completed DLL analysis for the phone, dispenser and companion logic, and correlate actual map triggers later.
5. **Weapons and pickup dependency graphs:** `CWeaponScatterGun`, `CWeaponMagnum`, `CWeaponGrenade`, ammunition classes, `CMegaHealth`; check the `point_template` failures against actual BSP entity lumps rather than merely copying stock HL2 classnames.
6. **Localization/material/audio as separate compatibility tracks.** Terminal text keys, 32 kHz MP3 messages, water shader warnings and door options should not be conflated with missing game DLL logic.

## Method and confidence

**High confidence:** counts in the supplied log, exact registered map classnames in original binary, original C++ entity-factory class via RTTI, saved filenames, visible unresolved localization tokens. **Moderate confidence:** screenshots showing functional part of VGUI rendering; error cascades due to missing templates; turret being a standard Source entity as reported by tester. **Unverified:** whether terminals accept actual map commands, source of localization tokens, unique per-map counts, precise save-open failure cause, whether each classified class has truly Ritual-original logic rather than adapting Valve SDK code.

Reproduce: `python sin_hl2_missing_factory_audit.py "Pasted text.txt" server.dll sin_hl2_missing_factory_evidence.json` (requires the original DLL on the local machine; no DLL redistributed).