# Original SiN Episodes DLL Code Crosswalk: 49 HL2-Missing Map Entities (Round 17)

**Read-only static analysis** of original 2006 Windows x86 `server.dll` and `client.dll`, matched to five user HL2 console logs. Factory registration is confirmed code evidence, not a decompiled implementation or working Quest 3 entity.

- `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

**Result:** 49 / 49 classnames have exact server factory registrations; 43 underlying server C++ classes; 9 named client RTTI/vtable counterparts. All 561 server datamap records located, with 305 names statically readable and 256 awaiting static-constructor reconstruction.

## Complete log classname -> original server code

| Missing classname | Failed attempts | Original server C++ class | Original registration VA | Client counterpart |
|---|---:|---|---|---|
| `npc_druglab_grunt_pistol` | 195 | `CNPC_MobGrunt` | `0x223C6FE7` | no direct matching RTTI |
| `npc_druglab_grunt_assault_rifle` | 103 | `CNPC_MobGrunt` | `0x223C7027` | no direct matching RTTI |
| `prop_exploding_barrel` | 92 | `CExplodingBarrel` | `0x223C4647` | no direct matching RTTI |
| `trigger_dynamic_difficulty` | 88 | `CTriggerDynamicDifficulty` | `0x223BE4A7` | no direct matching RTTI |
| `npc_druglab_grunt_elite` | 69 | `CNPC_EliteGrunt` | `0x223C6947` | `C_NPC_EliteGrunt` |
| `prop_acetylene_tank` | 69 | `CAcetyleneTank` | `0x223C4627` | no direct matching RTTI |
| `npc_druglab_grunt_scattergun` | 66 | `CNPC_MobGrunt` | `0x223C7007` | no direct matching RTTI |
| `prop_u4_barrel` | 62 | `CMutagenBarrel` | `0x223C4667` | no direct matching RTTI |
| `env_radio_message` | 52 | `CRadioMessage` | `0x223C3A27` | no direct matching RTTI |
| `item_antigen_dispenser` | 49 | `CAntigenDispenser` | `0x223C40A7` | no direct matching RTTI |
| `npc_generic_actor` | 49 | `CNPC_GenericActor` | `0x223C6B97` | no direct matching RTTI |
| `trigger_secret` | 44 | `CTriggerSecret` | `0x223BE4D7` | no direct matching RTTI |
| `weapon_grenade` | 41 | `CWeaponGrenade` | `0x223C50E7` | `C_WeaponGrenade` |
| `item_ammo_duag` | 39 | `CItemAmmoDUAG` | `0x223C4057` | no direct matching RTTI |
| `prop_mutagen_barrel` | 39 | `CMutagenBarrel` | `0x223C4687` | no direct matching RTTI |
| `item_ammo_flak` | 38 | `CItemAmmoFLAK` | `0x223C4037` | no direct matching RTTI |
| `npc_civilian` | 32 | `CNPC_Citizen` | `0x223C6637` | no direct matching RTTI |
| `npc_druglab_jetpack` | 29 | `CNPC_DruglabJetpack` | `0x223C68B7` | no direct matching RTTI |
| `ai_goal_takecover` | 26 | `CAI_TakeCoverGoal` | `0x223C7F87` | no direct matching RTTI |
| `item_ammo_magnum` | 23 | `CItemAmmoMagnum` | `0x223C3F77` | no direct matching RTTI |
| `prop_phone` | 23 | `CPropPhone` | `0x223C47A7` | `C_PropPhone` |
| `weapon_scattergun` | 22 | `CWeaponScatterGun` | `0x223C54C7` | `C_WeaponScatterGun` |
| `npc_recombinant_major` | 21 | `CNPC_RecombinantMajor` | `0x223C74E7` | no direct matching RTTI |
| `prop_antigen_canister` | 21 | `CAntigenCanister` | `0x223C46D7` | no direct matching RTTI |
| `item_ammo_buckshot` | 20 | `CItemAmmoBuckshot` | `0x223C3FB7` | no direct matching RTTI |
| `prop_antigen_explosive_barrel` | 19 | `CAntigenCanister` | `0x223C46B7` | no direct matching RTTI |
| `item_ammo_ar_grenade` | 17 | `CItemAmmoARGrenade` | `0x223C4017` | no direct matching RTTI |
| `npc_jessica` | 17 | `CNPC_Jessica` | `0x223C6E17` | no direct matching RTTI |
| `npc_recombinant_minor` | 17 | `CNPC_RecombinantMinor` | `0x223C7637` | no direct matching RTTI |
| `npc_securitybot` | 16 | `CNPC_SecurityBot` | `0x223C7707` | no direct matching RTTI |
| `prop_oxygen_tank_large` | 16 | `COxygenTankLarge` | `0x223C45A7` | no direct matching RTTI |
| `npc_druglab_worker` | 14 | `CNPC_DruglabWorker` | `0x223C68E7` | no direct matching RTTI |
| `weapon_magnum` | 14 | `CWeaponMagnum` | `0x223C5157` | `C_WeaponMagnum` |
| `item_megahealth` | 10 | `CMegaHealth` | `0x223C42D7` | no direct matching RTTI |
| `npc_druglab_helicopter` | 9 | `CNPC_SintekHelicopter` | `0x223C77E7` | no direct matching RTTI |
| `prop_oxygen_tank_small` | 9 | `COxygenTankSmall` | `0x223C45E7` | no direct matching RTTI |
| `prop_propane_tank` | 9 | `COxygenTankLarge` | `0x223C45C7` | no direct matching RTTI |
| `npc_quadralex` | 8 | `CNPC_Quadralex` | `0x223C7357` | no direct matching RTTI |
| `weapon_assault_rifle` | 7 | `CWeaponAssaultRifle` | `0x223C4F67` | `C_WeaponAssaultRifle` |
| `env_faint` | 6 | `CEnvFaint` | `0x223C3907` | no direct matching RTTI |
| `npc_jc` | 6 | `CNPC_JC` | `0x223C6DF7` | no direct matching RTTI |
| `prop_oxygen_tank` | 5 | `COxygenTankSmall` | `0x223C4607` | no direct matching RTTI |
| `prop_vehicle_sin` | 5 | `CPropSinVehicle` | `0x223C5BE7` | `C_PropSinVehicle` |
| `env_poison_gas` | 4 | `CPoisonGas` | `0x223C4557` | no direct matching RTTI |
| `item_ammo_ar` | 3 | `CItemAmmoAR` | `0x223C3F97` | no direct matching RTTI |
| `npc_hologram` | 3 | `CNPC_Hologram` | `0x223C6D77` | `C_NPC_Hologram` |
| `npc_sintek_grunt` | 2 | `CNPC_SintekGrunt` | `0x223C77A7` | no direct matching RTTI |
| `weapon_npc_pistol` | 2 | `CWeaponNPCPistol` | `0x223C52C7` | `C_WeaponNPCPistol` |
| `env_radio` | 1 | `CRadio` | `0x223C3937` | no direct matching RTTI |

## Verified client.dll virtual code entrypoints

These nine have **client-side MSVC RTTI + primary vtable + executable first-method pointers**. Many other server classes can reuse generic client rendering, so lack of a directly named client class is not proof of missing presentation code.

| Server C++ class | Client C++ class | Client primary vtable VA | First virtual-method VA | Vtable entries |
|---|---|---|---|---:|
| `CNPC_EliteGrunt` | `C_NPC_EliteGrunt` | `0x242B710C` | `0x2421CAF0` | 175 |
| `CNPC_Hologram` | `C_NPC_Hologram` | `0x242B7624` | `0x2421DB60` | 175 |
| `CPropPhone` | `C_PropPhone` | `0x242B8D94` | `0x242208F0` | 170 |
| `CPropSinVehicle` | `C_PropSinVehicle` | `0x242BAC8C` | `0x24227040` | 173 |
| `CWeaponAssaultRifle` | `C_WeaponAssaultRifle` | `0x242C56C4` | `0x2422B590` | 301 |
| `CWeaponGrenade` | `C_WeaponGrenade` | `0x242C3B14` | `0x2422B300` | 301 |
| `CWeaponMagnum` | `C_WeaponMagnum` | `0x242C4654` | `0x2422B1B0` | 301 |
| `CWeaponNPCPistol` | `C_WeaponNPCPistol` | `0x242C40AC` | `0x2422B420` | 301 |
| `CWeaponScatterGun` | `C_WeaponScatterGun` | `0x242C2FF4` | `0x2422B2C0` | 301 |

## Additional client-side systems

Not one-to-one representations of the 49 logged map classes, but relevant for missing SiN mechanics and player presentation.

| Client class | Primary vtable VA | First virtual method VA |
|---|---|---|
| `CHudArena` | `0x242C71E4` | `0x240016A0` |
| `CHudGrenades` | `0x242C84CC` | `0x240016A0` |
| `CHudPoisonTimer` | `0x242C8F94` | `0x240016A0` |
| `CHudPoliceRadio` | `0x242C928C` | `0x240016A0` |
| `C_DynamicDifficulty` | `0x242ADF34` | `0x241FAC90` |
| `C_NPC_GruntBase` | `0x242B6D94` | `0x2421C9E0` |

## Server datamap pointers and named inputs for high-value entities

| Map entity | Server datamap VA | Resolved / declared fields | Named inputs |
|---|---|---|---|
| `trigger_dynamic_difficulty` | `0x2250B374` | 3/3 | none resolved statically |
| `ai_goal_takecover` | `0x2253B68C` | 8/8 | none resolved statically |
| `npc_jessica` | `0x2253593C` | 5/6 | ThrowItemAtPlayer; SetPrimaryAmmoCountToThrow; SetSecondaryAmmoCountToThrow; HolsterWeapon |
| `npc_druglab_grunt_pistol` | `0x22534DA4` | 39/39 | LookOff; LookOn; StartPatrolling; StopPatrolling; Assault; ThrowGrenades |
| `npc_druglab_grunt_elite` | `0x22534674` | 2/2 | none resolved statically |
| `npc_securitybot` | `0x22537AEC` | 2/2 | none resolved statically |
| `prop_phone` | `0x2252A7B4` | 10/11 | DialNumber; Enable; Disable |
| `prop_vehicle_sin` | `0x225312F8` | 49/54 | EnterVehicle; ExitVehicle; EnableUse; SetView; LockView; OpenDoor; OpenWindow; CloseDoor; CloseWindow; OpenDoors; CloseDoors; OpenWindows; CloseWindows; Head… |
| `env_radio_message` | `0x22524938` | 9/10 | SetMessage; SetSpeaker; SetPriority; Dispatch; Remove |
| `item_antigen_dispenser` | `0x225285C4` | 8/14 | none resolved statically |
| `prop_exploding_barrel` | `0x2252A06C` | 8/9 | Kill |
| `trigger_secret` | `0x2250B38C` | 0/1 | none resolved statically |
| `weapon_grenade` | `0x2252DB28` | 3/3 | none resolved statically |

## How to use this for a working port

1. Implement the map entity factories on the **server** side first and honor the original keyvalues, inputs and outputs. Keep the original `C++` class family relationships where behavior depends on them.
2. For player-visible entities, verify the original server-to-client network representation. The nine direct client classes are an initial target list, not an exhaustive nettable map.
3. Supply the original entity scripts, models and scenes through lawful game mounts. The console errors for missing VCDs, material proxies and audio sample rates are **separate** from unknown entity factories.
4. Start with a minimal no-NPC level and one usable object, then add player weapons, spawnable grunts, one AI encounter and checkpoint logic. Avoid inserting fabricated stubs that hide errors but have no functionality.
5. Test the native Source/ARM64 ABI and record errors on the target build. None of the recovered x86 VAs can be called directly from Quest 3.

## Evidence boundaries

* All map classnames came from the supplied five console logs. The server classes and addresses came from exact original PE factory bytes and RTTI. Client classes and pointers were independently found from the original `client.dll` RTTI and vtable pointers.
* 1,531 is a count of **error lines** and can include the same entity more than once.
* A direct client counterpart was not identified for 40 classnames; this is **not** proof that 40 lack client functionality.
* The audit does not decompile complete behavior for 49 entities or make them work in stock HL2.

## Reproduction

Run `python3 sin_original_entity_code_crosswalk_17.py /path/to/private/files /tmp/sin_original_entity_code_crosswalk_17` with `sin_missing_entity_atlas_audit.py` in the same directory. The private folder must contain the original SHA-matching DLLs in `sin_private/` and the five exact original HL2 logs. The output includes `.json`, `.csv`, `.md`.