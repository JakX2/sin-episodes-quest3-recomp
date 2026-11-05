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
