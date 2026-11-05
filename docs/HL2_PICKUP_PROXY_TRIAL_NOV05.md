# Stock-HL2 pickup proxy trial (Nov 5, 2026)

This is an **optional, isolated desktop HL2 experiment**, not original SiN Episodes weapon mechanics or a Quest 3 port. It extends the prior successful two-helicopter BSP copy with 5 opt-in classname remaps, preserving all original BSPs and all non-entity lumps.

## Source to HL2 substitutions

| Original SiN classname | Temporary HL2 entity |
|---|---|
| `weapon_magnum` | `weapon_357` |
| `weapon_assault_rifle` | `weapon_ar2` |
| `weapon_scattergun` | `weapon_shotgun` |
| `weapon_grenade` | `weapon_frag` |
| `item_ammo_magnum` | `item_ammo_357` |

The owner's existing finale HL2 logs showed 1, 1, 1, 4 and 3 unknown creation attempts respectively (10 total, not necessarily 10 distinct pickups). Other SiN-specific classes remain unchanged. Valve's Source SDK 2013 contains stock `item_ammo_357` and these standard HL2 weapon entities. A classname replacement **does not** recreate original SiN reload/fire/sound/HUD/magazine behavior, ammo capacity, input/output handling or balancing. It may cause pickup/model behavior not to work.

## Windows trial steps (no Git installation)

From `D:\sin-scene-mvp-updated`, update only the Python testing script to the pinned branch version (if already downloaded, save a backup first):

```powershell
$script = ".\research\tools\sin_bsp_scene_actor_trial.py"
Copy-Item $script "$script.before_pickups.bak" -ErrorAction Stop
Invoke-WebRequest -UseBasicParsing "https://raw.githubusercontent.com/JakX2/sin-episodes-quest3-recomp/1b4a9ed76a7bdce562cc7f6e8e4064a2a94c26b2/research/tools/sin_bsp_scene_actor_trial.py" -OutFile $script
py $script "D:\SiN\maps\se1_finale01.bsp" --generic-target end_hardcorps_heli1 --generic-target end_hardcorps_heli2 --pickup-proxies --report
py $script "D:\SiN\maps\se1_finale01.bsp" --generic-target end_hardcorps_heli1 --generic-target end_hardcorps_heli2 --pickup-proxies --write "D:\SiN_Test_Only\heli_pickups_trial\se1_finale01.bsp"
py $script "D:\SiN\maps\se1_finale01.bsp" --generic-target end_hardcorps_heli1 --generic-target end_hardcorps_heli2 --pickup-proxies --verify "D:\SiN_Test_Only\heli_pickups_trial\se1_finale01.bsp"
```

```powershell
$maps = "C:\Program Files (x86)\Steam\steamapps\common\Half-Life 2\hl2\custom\sin\maps"
$source = "D:\SiN_Test_Only\heli_pickups_trial\se1_finale01.bsp"
$destination = Join-Path $maps "se1_finale01_dual_heli_pickups_trial.bsp"
if (Test-Path $destination) { throw "Refusing to overwrite an existing trial map" }
Copy-Item $source $destination -ErrorAction Stop
```

Load from HL2 console: `map se1_finale01_dual_heli_pickups_trial`. Check one map-load console log for the five original unknown-entity creation errors; then **physically walk to a pickup** and verify HL2 HUD/weapon inventory or ammo count changes and that the weapon can fire. Missing classname errors alone do not prove runtime pickup behavior.

For rollback use `map se1_finale01_dual_heli_trial` (the earlier known working dual-helicopter test). Never overwrite the original SiN BSP or Valve map/VPK. If weapon pickups fail, keep this option disabled. If map references unsupported SiN-specific I/O, avoid assuming stock HL2 matches it.

## Separate blockers

This experiment does not fix the many missing SinTEK NPC classes, `trigger_dynamic_difficulty`, `item_antigen_dispenser`, `logic_choreographed_scene`/missing `Finale_Cinematic.vcd` in `scenes.image`, or Android/standalone VR support. Those require independent tests and/or native code. The user's verified dual-helicopter sequence remains the desktop runtime baseline.

## Owner's desktop HL2 runtime observation (Nov 5, 2026)

The user launched the new pickup-proxy copy of the SiN finale BSP and shared a screenshot plus console output. The screenshot depicts a first-person weapon held by the player and a small pickup box on the floor of a stairwell. The visible console section contains **no `Attempted to create unknown entity type` messages for the five pickup classes**: `weapon_magnum`, `weapon_assault_rifle`, `weapon_scattergun`, `weapon_grenade`, and `item_ammo_magnum`. The already validated first helicopter scripted sequence was still matched to `end_hardcorps_heli1`, reported by the log as:

```text
scripted_sequence 535:"end_hardcorps_heli1_sequence" using NPC 534:"end_hardcorps_heli1"(end_hardcorps_heli1)
```

This is **initial stock-HL2 entity-class acceptance and rendering evidence**, not yet a proof of weapon pickup, ammo increment, fire/reload, or original SiN-specific gameplay mechanics. The user did not provide before/after pickup ammo counters or a firing test. Follow-up: walk over a known SiN pickup, compare HUD ammo/inventory before/after, fire the converted weapon, and note whether it persists after pickup.

Independent blockers persist in the same log: `unhandled input: (Wake) -> (helicopter)`; `unhandled input: (Wake) -> (roddick)`; `Finale_Cinematic.vcd missing from scenes.image`; incompatible `finale_encounter.mp3` sample rate. Do not conflate these with the pickup proxy result. No Android, Quest 3, or true standalone game runtime tested by this screenshot.

## Live desktop HL2 result from owner (Nov 5)

**PASS (pickup behavior):** User independently confirms pickups worked in the isolated `se1_finale01_dual_heli_pickups_trial` map and supplied a screenshot of an equipped HL2 weapon and pickup on the floor. This is beyond a structural-only BSP verification, though it is not a test of original Ritual weapon handling or every possible pickup type.

**Console log cross-check:** the new map-load log records **zero** `Attempted to create unknown entity type` messages for the five remapped pickup classnames. It still reports **65** unknown-entity creation attempts from **13** other SiN-specific classes. The largest groups are `npc_druglab_grunt_assault_rifle` (24), `npc_druglab_grunt_elite` (15), and `npc_druglab_jetpack` (6). Counts describe attempts in this single user capture, not verified population sizes. The model/cubemap filename warnings are expected from loading the renamed copy; `Finale_Cinematic.vcd` still missing from `scenes.image` is a separate postponed task.

**Next gated experiment:** first run `sv_cheats 1`, `npc_create_equipment weapon_ar2`, and `npc_create npc_combine_s` in the live test map, aiming at a flat walkable surface with room around the NPC. Check whether a stock HL2 Combine spawns, moves, attacks, and navigates the SiN layout. Keep this purely transient console test; do not yet globally rewrite 24+15 map grunts, because their Ritual models, spawn flags, targetnames, squad logic and template makers may not map cleanly to Source's Combine class. After a positive smoke result, inspect the original BSP for a **single named** compatible grunt and make a copy-only opt-in targeted rewrite for that one NPC.

## Live combat AI smoke, owner confirmed (Nov 5)

The owner manually used `sv_cheats 1`, `npc_create_equipment weapon_ar2`, `npc_create npc_combine_s` in the SiN finale HL2 test map and positively confirmed **all four** behaviors: stock Combine spawns, walks in the SiN level, notices and attacks the player, and can be damaged/killed. Thus stock HL2 NPC creation, local navigation, combat AI and damage are demonstrated at this tested location.

**Limitations:** this does not prove any `npc_druglab_grunt_assault_rifle` or `npc_druglab_grunt_elite` substitution works, nor their original models/animations/scripts; it is not proof that every campaign encounter, map nav region, or original SinTEK AI behavior works. The next action is a **read-only listing** of the original BSP's grunt `classname`, `targetname`, `model`, and `spawnflags` so a single unambiguous candidate can be patched as an isolated experimental BSP later.
