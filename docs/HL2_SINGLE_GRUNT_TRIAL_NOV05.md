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
