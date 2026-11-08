# Source runtime missing SiN game entities: owner-supplied map logs

Reviewed 8 November 2026. This summarizes the FIVE plain-text console logs shared by the owner in the Drive folder:
https://drive.google.com/drive/folders/1ExaddVDbt0UTrvec4jJdg1lXVdn6AAEJ?usp=sharing

Evidence limitation: these are console logs from map-loading attempts in a Source-style runtime. They are NOT original DLL function inventories, true BSP map entity-lump counts, a real SourceVR custom-game module test, or Quest 3 headset execution. Several sessions were loaded repeatedly. Original log bytes are NOT reproduced or committed.

## Five logs inspected, exact error counts

| TXT file | Size (bytes) | Unknown-entity error lines | Different classnames |
| --- | ---: | ---: | ---: |
| sin stuff arena.txt | 4,887 | 10 | 5 |
| sin stuff finale.txt | 20,683 | 110 | 21 |
| sin stuff highrise.txt | 77,471 | 396 | 39 |
| sin stuff u4-2 to pit 2.txt | 51,596 | 442 | 41 |
| sin stuff intro-u4-1.txt | 65,053 | 573 | 37 |
| **Combined** | **219,690** | **1,531** | **49 unique names** |

An occurrence is one console line with the fixed message 'Attempted to create unknown entity type CLASSNAME!'. The follow-up 'Can't init' line is NOT counted twice. Counts are error occurrences across runs, NOT unique map-placed entities.

## Detailed classname inventory

| Prefix | Distinct classes | Occurrences |
| --- | ---: | ---: |
| npc_ | 17 | 656 |
| prop_ | 12 | 369 |
| item_ | 8 | 199 |
| trigger_ | 2 | 132 |
| weapon_ | 5 | 86 |
| env_ | 4 | 63 |
| ai_ | 1 | 26 |
| **Total** | **49** | **1,531** |

**NPCs:** npc_druglab_grunt_pistol (195), npc_druglab_grunt_assault_rifle (103), npc_druglab_grunt_elite (69), npc_druglab_grunt_scattergun (66), npc_generic_actor (49), npc_civilian (32), npc_druglab_jetpack (29), npc_recombinant_major (21), npc_jessica (17), npc_recombinant_minor (17), npc_securitybot (16), npc_druglab_worker (14), npc_druglab_helicopter (9), npc_quadralex (8), npc_jc (6), npc_hologram (3), npc_sintek_grunt (2).

**Props:** prop_exploding_barrel (92), prop_acetylene_tank (69), prop_u4_barrel (62), prop_mutagen_barrel (39), prop_phone (23), prop_antigen_canister (21), prop_antigen_explosive_barrel (19), prop_oxygen_tank_large (16), prop_oxygen_tank_small (9), prop_propane_tank (9), prop_vehicle_sin (5), prop_oxygen_tank (5).

**Items:** item_antigen_dispenser (49), item_ammo_duag (39), item_ammo_flak (38), item_ammo_magnum (23), item_ammo_buckshot (20), item_ammo_ar_grenade (17), item_megahealth (10), item_ammo_ar (3).

**Triggers:** trigger_dynamic_difficulty (88), trigger_secret (44).

**Weapons:** weapon_grenade (41), weapon_scattergun (22), weapon_magnum (14), weapon_assault_rifle (7), weapon_npc_pistol (2).

**Environment:** env_radio_message (52), env_faint (6), env_poison_gas (4), env_radio (1).

**AI:** ai_goal_takecover (26).

## How this relates to our actual original DLL research

The existing original-server verifier in research/tools/verify_gameplay_entity_registry.py proves 32 SELECTED original entity registrations. It is not an exhaustive original registry. Two of those names also appear as UNKNOWN in the owner's map-load logs:

- trigger_dynamic_difficulty: 88 runtime error occurrences
- npc_sintek_grunt: 2 runtime error occurrences

This is particularly important: a class whose factory registration was proven in the ORIGINAL 2006 server.dll can STILL be absent from the engine that attempted to load these maps. Static research and our portable NPC/weapon C++ tests do not install a game entity factory in that runtime. The 47 other missing names being outside the SELECTED 32-name subset is not proof that the original DLL lacks them.

## Additional runtime problems that are NOT unknown-class errors

- Many point_template references fail, including mapstart_template member names, ammo and grenade targets, a blades_assault_rifle target and Highrise locker-room item targets. Some may be secondary to failed entity creation, but the logs do not establish a cause for every missing target.
- The recognized prop_door_rotating class reports that Highrise doors_solid_01.mdl has no door_options for its skin / QC configuration. This is a model options issue, NOT evidence that prop_door_rotating is an unknown entity.
- Four cinematic .vcd scene paths are missing in these runs: scenes/se1_finale/teaser_radek_and_jessica.vcd, scenes/se1_finale/teaser_jc_hospital.vcd, scenes/se1_finale/teaser_elexis_laughing.vcd, and scenes/se1_highrise/jess_wakeupblade.vcd.
- Highrise reception-desk and Pit radio-tower VMT KeyValues parse errors; a glass-water TextureScroll material proxy error; a Highrise phys_hinge missing entity; and Finale music warnings about 32,000 Hz sample rates.
- The intro-U4 run attempts several existing save filenames, with some failed opens; it does not prove saves correctly restored or identify a universal savegame compatibility failure.

## Practical priorities for a real standalone Quest 3 port

1. Recover/implement a LEGITIMATE, module-capable native engine-host and its entity factory registration. Test actually creating trigger_dynamic_difficulty, a weapon_assault_rifle and a real npc_druglab_grunt variant inside the engine. The current SourceVR content-only mod workflow does not automatically permit custom server/client game libraries.
2. Reconcile all 49 runtime missing classnames against the ORIGINAL 2006 DLL factory registry and the new native host. Do not label an original class as unimplemented merely because it was not in the existing selected 32 factory witnesses.
3. Implement valid keyvalues, spawn/think/touch, owner state, AI, I/O, client networking, save/restore, and native entity lifetimes. A dummy factory alias that removes an error line is NOT a functioning entity.
4. Reload one authorized original game map in the actual native runtime and verify classname errors and linked point_template targets disappear for real. Track failures per map and run rather than summing duplicated historical sessions.
5. After native entity creation works, address remaining original cinematic scenes, material/proxy warnings, door options, and script-driven encounters.
6. Verify actual Quest 3 stereo rendering, tracked controllers, performance and campaign progression before release.

Owner-provided logs are valuable PRIORITIZATION EVIDENCE but NOT a measure of DLL decompilation percentage, and they do not show a playable VR game.
