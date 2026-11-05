# One SinTEK grunt -> stock HL2 Combine: isolated runtime experiment

This research experiment is based on the **original \`se1_finale01.bsp\` entity listing supplied by the owner**. The map has **24** \`npc_druglab_grunt_assault_rifle\` and **15** \`npc_druglab_grunt_elite\` entities. At least 12 rooftop NPCs use unique targetnames; many secret grunts share duplicate names. The first experiment targets only **entity #388**, \`roof_grunt_1_seq1\`, using original class \`npc_druglab_grunt_assault_rifle\`, \`squadname=seq1\`, \`spawnflags=516\`, and no explicit \`model\` key.

## Already verified by the owner
* Original SiN finale map loads in desktop HL2.
* Jessica model appears with proxy \`npc_alyx\`.
* Both HardCorps \`generic_actor\` helicopter instances spawn and the first sequence delivers \`Wake\` to the second.
* \`--pickup-proxies\` supports useful in-game pickups in a new copy of the map.
* An HL2 \`npc_combine_s\` created with \`npc_create_equipment weapon_ar2\` walks, attacks, takes damage and dies on the SiN map.

## Purpose of this trial

New opt-in \`--grunt-target roof_grunt_1_seq1\` changes exactly this NPC's classname to \`npc_combine_s\` and inserts \`additionalequipment=weapon_ar2\` only if no equipment is configured. It preserves \`targetname\`, \`squadname\`, \`spawnflags\`, other keys and all original BSP data. Targets are rejected if missing, duplicated, not one of the two listed SinTEK grunt classes, or using incompatible additional equipment. **Stock HL2 Combine models, AI and weapons are expected; the original SinTEK meshes, Ritual-specific behavior, scripted events and animation are NOT restored.** This is a copy-only experiment, not a VR port.

## Commands for Windows (no Git needed)

From \`D:\sin-scene-mvp-updated\` (the owner must have the previous BSP Python support modules):

\`\`\`powershell
cd "D:\sin-scene-mvp-updated"
$script = ".\research\tools\sin_bsp_scene_actor_trial.py"
if (-not (Test-Path $script)) { throw "Missing existing trial script" }
if (-not (Test-Path "$script.before_grunt.bak")) {
  Copy-Item $script "$script.before_grunt.bak" -ErrorAction Stop
}
Invoke-WebRequest -UseBasicParsing "https://raw.githubusercontent.com/JakX2/sin-episodes-quest3-recomp/661f03a76c1aff243d796545f1d1aedae693ff2e/research/tools/sin_bsp_scene_actor_trial.py" -OutFile $script

$original = "D:\SiN\maps\se1_finale01.bsp"
$trial = "D:\SiN_Test_Only\one_grunt_trial\se1_finale01.bsp"
$args = @(
  $original
  "--generic-target", "end_hardcorps_heli1"
  "--generic-target", "end_hardcorps_heli2"
  "--pickup-proxies"
  "--grunt-target", "roof_grunt_1_seq1"
)
py $script @args --report
if ($LASTEXITCODE -ne 0) { throw "Report failed" }
py $script @args --write $trial
if ($LASTEXITCODE -ne 0) { throw "Generation failed" }
py $script @args --verify $trial
if ($LASTEXITCODE -ne 0) { throw "Verification failed" }
$maps = "C:\Program Files (x86)\Steam\steamapps\common\Half-Life 2\hl2\custom\sin\maps"
$dest = Join-Path $maps "se1_finale01_one_grunt_trial.bsp"
if (Test-Path $dest) { throw "Refusing to overwrite existing test map" }
Copy-Item $trial $dest -ErrorAction Stop
\`\`\`

HL2 console:

\`\`\`text
developer 2
sv_cheats 1
map se1_finale01_one_grunt_trial
ent_dump roof_grunt_1_seq1
\`\`\`

If the named entity is alive, \`ent_dump\` should show \`classname: npc_combine_s\` and \`additionalequipment: weapon_ar2\`. It may be at a distant rooftop location or use map-scripted/squad activation. Use \`ent_text roof_grunt_1_seq1\` to find its location. If it spawns, see whether it walks, targets the player, uses its rifle, and takes damage. Report model or animation errors rather than assuming compatibility. Keep the map in its isolated test filename and leave all previously working maps untouched.

**Rollback:** from HL2 console, \`map se1_finale01_dual_heli_pickups_trial\`.

## Important caveats

* \`spawnflags=516\` is preserved verbatim, but bit meanings can differ between Ritual's SiN classes and Valve's Combine. A missing/idle NPC may be due to spawnflags or map triggers. Do not silently change multiple map NPC flags to make them wake.
* A successful \`npc_create npc_combine_s\` proves the stock spawn/combat engine can run on the map, **not** that converted map-defined NPC I/O will work.
* The 24/15 counts are entity entries from the original BSP, not confirmed active fighters.
* Finale VCD scene-image compilation remains an independent deferred blocker.
* No proprietary original SiN map, models or textures are committed.

## Owner-confirmed original MobGrunt + combat AI baseline (Nov 5)

The owner spawned a live `npc_combine_s` in the SiN finale, with `model models/characters/MobGrunt/mobgrunt.mdl` and temporary `additionalequipment weapon_ar2`. After confirming `ent_dump` reported a live `npc_combine_s` with `sequence: 40`, `health: 50`, and AR2 equipment, the owner explicitly confirmed that the **original MobGrunt mesh moves and fires** under that HL2 NPC. The original MobGrunt animations (including `shootAR2s`) had also played as named `prop_dynamic` tests. That proves a useful desktop **visual-combat compatibility baseline**, not original Ritual NPC AI or authentic SiN weapon handling.

The map conversion tool now supports `--grunt-original-model` (opt-in). Combined with `--grunt-target roof_grunt_1_seq1` and the required explicit `--grunt-equipment-override`, it changes **only one uniquely named original assault grunt** in a BSP copy into an HL2 Combine NPC with **the original MobGrunt model** and temporary AR2. It preserves the original `origin=992 -56 257`, targetname, `squadname=seq1`, `spawnflags=516`, `sleepstate=3`, `NumGrenades=5`, and `damagefilter=grunt_unkillable`. These Ritual keyvalues may not have identical HL2 semantics. The original SiN BSP, its authentic weapon classname, and all other 38 grunts remain unchanged.

### New isolated Windows map experiment

Run from PowerShell (using the existing local tool directory and the untouched original BSP):

```powershell
cd "D:\sin-scene-mvp-updated"
$script = ".\research\tools\sin_bsp_scene_actor_trial.py"
if (!(Test-Path $script)) { throw "Expected existing BSP trial tool is missing" }
if (!(Test-Path "$script.before_model_trial.bak")) {
    Copy-Item $script "$script.before_model_trial.bak"
}
Invoke-WebRequest -UseBasicParsing "https://raw.githubusercontent.com/JakX2/sin-episodes-quest3-recomp/feature/sin-hl2-single-grunt-trial-20261105/research/tools/sin_bsp_scene_actor_trial.py" -OutFile $script

$original = "D:\SiN\maps\se1_finale01.bsp"
$output = "D:\SiN_Test_Only\one_original_mobgrunt_trial\se1_finale01.bsp"
$trialArgs = @(
    $original
    "--generic-target", "end_hardcorps_heli1"
    "--generic-target", "end_hardcorps_heli2"
    "--pickup-proxies"
    "--grunt-target", "roof_grunt_1_seq1"
    "--grunt-original-model"
    "--grunt-equipment-override"
)
py $script @trialArgs --write $output
if ($LASTEXITCODE -ne 0) { throw "BSP generation failed" }
py $script @trialArgs --verify $output
if ($LASTEXITCODE -ne 0) { throw "BSP verification failed" }

$maps = "C:\Program Files (x86)\Steam\steamapps\common\Half-Life 2\hl2\custom\sin\maps"
$dest = Join-Path $maps "se1_finale01_mobgrunt_live_trial.bsp"
if (Test-Path $dest) { throw "Refusing to overwrite an existing experimental map" }
Copy-Item $output $dest -ErrorAction Stop
```

After launching HL2:

```text
developer 2
sv_cheats 1
map se1_finale01_mobgrunt_live_trial
ent_dump roof_grunt_1_seq1
ent_text roof_grunt_1_seq1
```

If the original `sleepstate=3` prevents activation, test `ent_fire roof_grunt_1_seq1 Wake` and watch for input acceptance. A map-defined NPC may be on the rooftop far from the initial player spawn. The original `grunt_unkillable` damage filter may affect damage until the scripted trigger disables it. Distinguish these cases from model/animation incompatibility. This is **not yet verified in the user's map**; currently only manually spawned MobGrunt visual-combat behavior is confirmed.

**Rollback:** `map se1_finale01_dual_heli_pickups_trial`.

## Owner live map-defined spawn confirmation (Nov 5)

Owner loaded the isolated `se1_finale01_mobgrunt_live_trial` test map following the one-grunt BSP rewrite and reported **"it spawned grunts"**. This supports that the test BSP loads and produces visible NPC spawns without manual `ent_create` for those observed grunts. It is not yet confirmed from this report whether the uniquely named original rooftop target `roof_grunt_1_seq1` is among those seen, whether its model is the original MobGrunt, or whether that map-defined soldier moves/shoots and responds to scripted wake inputs. The experimental rewrite intentionally targeted only `roof_grunt_1_seq1`; plural grunts may reflect other map entities, scripted spawners, or existing HL2 entities.

Follow-up evidence: `ent_dump roof_grunt_1_seq1` plus observation of whether the originally modeled grunt moves/shoots without `npc_create`. Do not claim full encounter or authentic SiN AI until those results are established.

## Named rooftop grunt currently absent (owner console result)

After the owner reported seeing multiple grunts spawned in the experimental map, the exact follow-up command `ent_dump roof_grunt_1_seq1` returned **`ent_dump: no such entity`**. This establishes only that the *exact targetname* is not present in the live entity list at the time checked. Do **not** count this map-defined original roof grunt as having passed the named-instance/AI test yet. Plural grunt appearances do not prove this specific entity spawned.

A plausible, testable explanation is Source `point_template` mechanics. Valve Source SDK code creates template copies from source BSP entities and can remove template originals; on `ForceSpawn`, the spawned instance's name can be uniquified unless `SF_POINTTEMPLATE_PRESERVE_NAMES` is used. See `TemplateEntities.cpp`, `point_template.cpp` in Source SDK 2013. Do not assume the template hypothesis without inspecting original BSP `point_template` / `env_entity_maker` references.

**Next read-only local diagnostic:** compare the installed new BSP against the generated test output; enumerate the converted `roof_grunt_1_seq1` classname/model/equipment in the installed BSP; and list every BSP entity whose keyvalues reference that name (especially point_template `Template0x` slots). If installed bytes contain the expected single converted entity but no exact live target, investigate template identity/name fixup or map outputs before changing NPC flags or touching scene.cache. Keep the original BSP untouched.

## Correct installed BSP, map relay verified; point_template hypothesis eliminated

Owner's read-only local BSP inspection (on the installed trial map) reported:
- **Generated and installed BSP are SHA256-identical**: `cc2875d74a328c120f7bfbe53c120137b2450e585a5fbda3afa01b71997fd923`.
- Entity #388 in the installed BSP is `classname npc_combine_s`, `targetname roof_grunt_1_seq1`, `origin 992 -56 257`, `model models/characters/MobGrunt/mobgrunt.mdl`, `additionalequipment weapon_ar2`, `squadname seq1`, `spawnflags 516`, `sleepstate 3`, `damagefilter grunt_unkillable`.
- Entity #409 is `logic_relay` named `begin_roof_battle_seq1`, with `OnTrigger` outputs sending `Wake` to rooftop grunts #1-4, and `SetDamageFilter` with empty parameter to rooftop grunts #1-2; also an `Assault` input to grunt #2 after 0.05s and various door/blocker changes.
- Listed `point_template` entities are for bridges and map pickup templates, **none** references `roof_grunt_1_seq1`. Therefore the previously suggested `point_template` explanation has been ruled out for this specific target.
- Yet live `ent_dump roof_grunt_1_seq1` produced `no such entity` during the owner's check. Thus **the BSP rewrite and installation passed, but live targetname presence did not**. Do not describe the targeted rooftop NPC as successfully map-spawned yet.

SDK cross-check: Source 2013 `ai_basenpc.h` declares `AISS_WAITING_FOR_INPUT` for sleepstate 3 and base NPC spawnflag bits 4 (`SF_NPC_FALL_TO_GROUND`) + 512 (`SF_NPC_FADE_CORPSE`) = 516. Neither is the `SF_NPC_TEMPLATE` flag (2048). Sleeping should not in itself make the entity undiscoverable by exact name, although it may affect visibility. The fall-to-ground+fade-corpse combination raises a **hypothesis**, not proof, of death/removal at startup. Another possibility is `DispatchSpawn`/model initialization failure.

**Next zero-edit live console test**, directly after loading the isolated copy, before interacting with level scripts:

```text
developer 2
sv_cheats 1
map se1_finale01_mobgrunt_live_trial
find_ent roof_grunt_1_seq1
find_ent roof_grunt
find_ent npc_combine_s
find_ent begin_roof_battle_seq1
```

If the relay is found, test explicitly in this **throwaway map only**:

```text
ent_fire begin_roof_battle_seq1 Trigger
find_ent roof_grunt_1_seq1
```

`find_ent` is a documented Source 1 console command that searches partial classname and targetname, useful to catch renamed or other live Combine NPCs; unlike `ent_dump`, it gives the matching class/name/index. Capture console error/warning around map loading and trigger. If no exact entity exists both before and after, the relay's `Wake` alone cannot instantiate the missing NPC: next isolate map-spawn flags or conflicting Ritual-specific keyvalues in **one separate copy**, rather than editing the original map or globally replacing entities.

## Owner-confirmed live rooftop NPC and weapon (Nov 5)

The owner repeated the live console diagnostic in `se1_finale01_mobgrunt_live_trial` and supplied:

```text
find_ent roof_grunt
  'npc_combine_s' : 'roof_grunt_1_seq1' (entindex 218)
  'weapon_ar2' : 'roof_grunt_1_seq1_weapon' (entindex 62)
find_ent npc_combine_s
  'npc_combine_s' : 'roof_grunt_1_seq1' (entindex 218)
find_ent begin_roof_battle_seq1
  'logic_relay' : 'begin_roof_battle_seq1' (entindex 0)
ent_fire begin_roof_battle_seq1 Trigger
find_ent roof_grunt
  'npc_combine_s' : 'roof_grunt_1_seq1' (entindex 218)
  'weapon_ar2' : 'roof_grunt_1_seq1_weapon' (entindex 62)
```

**PASS:** The converted **map-defined** named rooftop NPC, its AR2 weapon entity and the original battle relay all exist in the live HL2 engine entity list. NPC and weapon remain present after attempting to trigger the battle relay. This supersedes earlier speculation that `roof_grunt_1_seq1` was absent due to `point_template` or failed BSP conversion: the earlier `ent_dump` only establishes absence at the earlier check's moment/context. Owner's earlier manual-spawn NPC used the same original MobGrunt model and walked/fired.

**Still pending:** whether the *map-defined* NPC actively responds to the original `Wake` input, traverses its rooftop AI encounter, fires automatically, and is appropriately damageable. `find_ent` proves creation and registration, not behavioral success; there is no `developer 2` I/O trace attached after the `ent_fire` in this report.

Next single diagnostic: with `developer 2` active, `ent_fire begin_roof_battle_seq1 Trigger`, inspect I/O and visually observe the actual rooftop NPC; `ent_dump roof_grunt_1_seq1` can now reconfirm its runtime `health`, `sequence`, `additionalequipment`, and `sleepstate`. The original `damagefilter grunt_unkillable` and `sleepstate 3` can still block combat/damage. Avoid changing original BSP or claiming native SiN AI.
