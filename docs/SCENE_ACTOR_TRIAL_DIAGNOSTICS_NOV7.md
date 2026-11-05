# November 7 scene-actor runtime test: reproducible preflight and log comparison

**State: test tooling only.** This phase does not add new entities to SourceVR, guarantee an HL2 fallback works, or show JC/Jessica gameplay. It aims to collect decisive evidence from a real user's **HL2 test-mod** run, on an experimental *copy* of a lawfully owned SiN BSP.

## Confirmed five-log baseline

The bundled machine-readable [baseline](../research/evidence/scene_actor_hl2_baseline_nov05.json) was produced from the user's **five original console logs**, not a guessed map roster. The logs contain **1,531 unknown entity creation attempts and 49 distinct names**. Relevant actor attempts:

| Original log | JC | Jessica | Generic actor | Missing VCD paths reported |
|---|---:|---:|---:|---|
| Intro through U4 Lab 1 | 2 | 6 | 23 | None |
| U4 Lab 2 through Pit 2 | 3 | 4 | 7 | None |
| Highrise | 0 | 3 | 0 | `scenes/se1_highrise/jess_wakeupblade.vcd` |
| Finale | 1 | 4 | 19 | `teaser_radek_and_jessica.vcd`, `teaser_jc_hospital.vcd`, `teaser_elexis_laughing.vcd` |
| Arena | 0 | 0 | 0 | None |
| **Total** | **6** | **17** | **49** | **4 unique** |

Counts may contain repeat map loads and are **not** counts of independent NPC placements. All four VCD paths are present in the original user-provided `scenes.zip`, but their presence does not prove Source scene-cache mounting.

## Tools and what they do

- [`sin_bsp_scene_actor_trial.py`](../research/tools/sin_bsp_scene_actor_trial.py) creates an **optional** modified *copy* of a BSP, substituting `npc_citizen` for `npc_jc` and `npc_alyx` for `npc_jessica` by changing only the text entity-lump classnames. `--include-generic` additionally substitutes `npc_generic_actor`, but keep it off for the initial trial. **The replacements are hypotheses**; they are not faithful 2006 class implementations.
- [`sin_scene_actor_diagnostics.py`](../research/tools/sin_scene_actor_diagnostics.py) reads a BSP entity lump without modification and finds the map's JC/Jessica targetnames, `logic_choreographed_scene` references and Target1..Target8 aliases. It compares those references to original VCD actor tokens when `scenes.zip` is supplied. It can also analyze all baseline logs and compare two console captures. It records only map-facing metadata and diagnostics, not proprietary map body data.
- [`sin_scene_archive_preflight.py`](../research/tools/sin_scene_archive_preflight.py) finds referenced missing VCD paths and confirms archive presence.

**Actor name caveat:** The highrise VCD uses `!Target1`, which refers to a `logic_choreographed_scene` controller's `Target1`, not an entity with a literal `!Target1` targetname. The Finale VCD uses `JC_Hospital`; the two-character teaser uses `Roddick`, `Jessica`, and `Helicopter`. A name match is a candidate only; actual SceneManager actor resolution must be observed in-engine.

## Five steps on Windows (without overwriting original game files)

1. Get the tools from this feature branch under `research/tools/`. Prepare a separate HL2 testing/mod folder with legally owned SiN map and model/scene assets mounted. Never copy edited BSPs over the original SiN installation.
2. Inspect the original Finale BSP entity names and scene references:

   ```powershell
   py research/tools/sin_scene_actor_diagnostics.py preflight --bsp "C:\SiN\maps\se1_finale01.bsp" --scenes "C:\SiN\scenes.zip" --log "C:\Logs\sin stuff finale.txt" --output "C:\Temp\sin_finale_preflight.json"
   ```

   The output tells us whether a scene has a map controller, direct actor targetname candidate, and/or special `!Target1` alias. An unresolved alias is a reason to inspect map entity I/O, **not** a claim that the VCD is defective.
3. Create an experimental BSP **copy** (the output path must not exist):

   ```powershell
   py research/tools/sin_bsp_scene_actor_trial.py "C:\SiN\maps\se1_finale01.bsp" --report
   py research/tools/sin_bsp_scene_actor_trial.py "C:\SiN\maps\se1_finale01.bsp" --write "C:\SiN_Test_Only\se1_finale01.bsp"
   ```

   Place the copy under the same map basename within the isolated HL2 test mod's `maps/` folder. The BSP test is not an actual Source engine plugin and **does not restore** Jessica's four original custom inputs. Make sure required models, sounds, scripts and scene cache are mounted separately; missing content is a different failure class.
4. With unmodified `se1_finale01` first, then with the copy under the *same settings*, capture **one map load per log** using HL2's console logging (`con_logfile` if supported by your build, or manually save the console text). Name them `finale-before.txt` and `finale-after.txt`. Do not combine multiple campaign loads into either capture. Repeat for `se1_highrise` when ready.
5. Compare only those two captures:

   ```powershell
   py research/tools/sin_scene_actor_diagnostics.py compare --before "C:\Logs\finale-before.txt" --after "C:\Logs\finale-after.txt" --output "C:\Temp\finale_comparison.json"
   ```

   Seeing `npc_jc` or `npc_jessica` stop appearing as **unknown entity classnames** is a narrow registry check only. We must still verify models, skeleton/animations, scripted actor binding, sound, all VCD event callbacks and map outputs. Other SiN-specific NPCs will continue to fail in this stock-HL2 test environment.

## Acceptance evidence and failure interpretation

| Observation | Meaning | Next action |
|---|---|---|
| Unknown `npc_jc`/`npc_jessica` disappears | Only proves map classname replacement avoided that original failure | Check NPC actually appears with correct model and targetname |
| NPC appears but T-poses | Skeleton, activities, model or animation incompatibility likely | Inspect model animation / sequence mapping; do not claim scene success |
| VCD reported missing even though archive has file | Scene mount/cache incompatibility | Check relative file paths and `scene.cache` for the **tested** runtime |
| VCD loads but gestures/voice never start | Actor binding, response, sound or model event path | Check actor targetname and scene controller `Target1..Target8` |
| `npc_alyx` spawns with HL2's Alyx model | The fallback base overwrote the original model | Avoid claiming Jessica visual compatibility; change host-side entity constructor when supported |
| JC & Jessica spawn, animate, play one VCD each, complete scene I/O | Candidate desktop scripted scene acceptance | Only then assess VR engine integration |

## Remaining blockers

The project does **not** yet have a verified real SourceVR entity factory/game-module adapter, a generated and accepted `scene.cache` for the chosen Source branch, or headset testing of the JC/Jessica actors. This tooling is intentionally limited to safe static inspection and repeatable local measurements.

Do **not** merge the stacked draft branch automatically. All tools are pure Python standard library or host-only C++17; no third-party proprietary game code/assets are committed.
### Round 19 actor alias preflight correction

The BSP scanner now computes `has_scene_alias` from `logic_choreographed_scene` controller `Target1..Target8` values, **not** from arbitrary NPC fields. An alias row additionally records whether the referenced targetname is visible in the *initial BSP entity lump* and the matching classname candidates. An unresolved targetname could still be created later by map I/O, so this is diagnostic evidence rather than a failed runtime test. One synthetic regression test guards the previously false-positive `!Target1` binding case.


## November 5 follow-up: output-directory fix and deterministic BSP verification

The BSP trial writer now creates missing destination parent directories for explicit `--write` requests. Output files are opened in exclusive-create mode, so it will still **refuse to overwrite** an existing trial or original. No input BSP is ever changed. A new read-only `--verify` mode regenerates the expected trial bytes in memory and compares **every byte** of the written copy to that expectation, including all unmodified lumps and map data.

From the repository root in PowerShell, using the owner's previously reported paths:

```powershell
py research/tools/sin_bsp_scene_actor_trial.py "D:\SiN\maps\se1_finale01.bsp" --report
py research/tools/sin_bsp_scene_actor_trial.py "D:\SiN\maps\se1_finale01.bsp" --write "D:\SiN_Test_Only\se1_finale01.bsp"
py research/tools/sin_bsp_scene_actor_trial.py "D:\SiN\maps\se1_finale01.bsp" --verify "D:\SiN_Test_Only\se1_finale01.bsp"

py research/tools/sin_scene_actor_diagnostics.py preflight --bsp "D:\SiN\maps\se1_finale01.bsp"
py research/tools/sin_scene_actor_diagnostics.py preflight --bsp "D:\SiN_Test_Only\se1_finale01.bsp"
```

No need for a separate `New-Item` call now. If the output BSP already exists, `--write` returns an error by design; use `--verify` to check it or select a new test-only filename instead of overwriting it. If `--include-generic` was used to create the copy, also supply `--include-generic` when verifying.

**Meaning of a verification pass:** only that the generated binary exactly matches the intended classname-only BSP rewrite. It is not evidence of HL2 engine acceptance, model/skeleton compatibility, scene cache mounting, actor binding, or successful Quest 3 gameplay. Retain the existing before/after single-map console-log procedure for those runtime questions.


## Direct use of already-extracted SiN scenes (November 5 follow-up)

The preflight tool can now load the original VCD text directly from a directory, without recreating `scenes.zip`. Use the extracted **scenes directory** containing `se1_finale/` and `se1_highrise/`, or its game-content parent folder containing `scenes/`. Files are only read; no original content is copied or rewritten.

```powershell
# First locate your already-extracted copy:
Get-ChildItem -Path "D:\SiN" -Filter "finale_cinematic.vcd" -File -Recurse |
  Select-Object -First 5 -ExpandProperty FullName

# Example ONLY if results point under D:\SiN\scenes\se1_finale\ :
py research/tools/sin_scene_actor_diagnostics.py preflight --bsp "D:\SiN\maps\se1_finale01.bsp" --scenes "D:\SiN\scenes" --output "D:\SiN_Test_Only\finale_preflight.json"
```

The script also accepts `--scenes "D:\SiN"` if that is the parent that directly contains `scenes\`. It counts and tokenizes the VCD files found, and shows actor names and map/controller alias candidates. Omit `--output` if the report filename already exists, because reports are not overwritten. **This verifies file presence and bindings only, not scene.cache acceptance or working choreography in HL2.**


## November 5 follow-up: boss helicopter scripted sequence

The owner confirmed that original `se1_finale01.bsp` contains the following linked entities:

- `boss_death_counter` (`math_counter`) signals `end_hardcorps_heli1` with `Wake`, and `end_hardcorps_heli1_sequence` with `BeginSequence`.
- `end_hardcorps_heli1` is `npc_generic_actor` with original model `models/vehicles/hardcorps_helicopter/helicopter_prop.mdl`.
- `end_hardcorps_heli1_sequence` is `scripted_sequence`, target `end_hardcorps_heli1`, action `highrise_scene`, pre-action idle `idle`, and `OnEndSequence` requests actor deletion.

Stock HL2 currently emits `scripted_sequence ... can't find NPC "end_hardcorps_heli1"`, consistent with the unsupported SiN `npc_generic_actor` factory. An older Valve Source implementation defines `generic_actor` as a `CAI_BaseActor` NPC explicitly intended for scripted sequence work. **Its availability and ability to play the SiN helicopter model's animation have not been verified on the owner's HL2 build.**

A *targeted* new opt-in `--generic-target` flag rewrites only the named SiN generic actor to `generic_actor`, preserving Jessica's `npc_alyx` substitution, the original helicopter model, the actor's targetname, and the `scripted_sequence` I/O. It **does not** change the other four generic actors.

From the repository root (requires the updated feature branch):

```powershell
py research/tools/sin_bsp_scene_actor_trial.py "D:\SiN\maps\se1_finale01.bsp" --generic-target end_hardcorps_heli1 --write "D:\SiN_Test_Only\heli_trial\se1_finale01.bsp"
py research/tools/sin_bsp_scene_actor_trial.py "D:\SiN\maps\se1_finale01.bsp" --generic-target end_hardcorps_heli1 --verify "D:\SiN_Test_Only\heli_trial\se1_finale01.bsp"
py research/tools/sin_scene_actor_diagnostics.py preflight --bsp "D:\SiN_Test_Only\heli_trial\se1_finale01.bsp" --scenes "D:\SiN\scenes"
```

The `--generic-target` selector refuses missing or ambiguous names, non-`npc_generic_actor` source classes, and `--include-generic` conflicts. It still creates only a separate output BSP. To actually load the experimental variant via `map se1_finale01`, the owner must select it within the already-existing HL2 custom test setup, backing up any previous trial map and avoiding multiple copies of the same map name in the search path.

**Evidence required:** absence/presence of `generic_actor` unknown-class errors, whether `end_hardcorps_heli1` exists as an NPC, whether `idle` and `highrise_scene` are available in its model, whether `OnEndSequence` fires and the helicopter is removed. If the engine rejects `generic_actor`, stop and implement a real compatible entity in a Source-based game module rather than substituting a static prop. This is a **desktop engine compatibility hypothesis**, not a complete SiN vehicle/NPC behavior implementation.


## Owner's live HL2 result: helicopter 1 scripted sequence completed (November 5)

**Evidence: owner-supplied HL2 console capture and screenshot**, using a copy-only trial map from this branch (not Quest/Android). After changing only `end_hardcorps_heli1` from SiN `npc_generic_actor` to HL2 `generic_actor`, the running HL2 engine displayed the original model `models/vehicles/hardcorps_helicopter/helicopter_prop.mdl` and reported that the actor exists with classname `generic_actor` and schedule `SCHED_SCRIPTED_WAIT`. The owner then manually executed:

```text
ent_fire end_hardcorps_heli1_sequence BeginSequence
```

The observed console reported the following event chain:

```text
(132.76) input : end_hardcorps_heli1_sequence.BeginSequence()
(150.68) output: (scripted_sequence,end_hardcorps_heli1_sequence) -> (end_hardcorps_heli1,Kill,2.0)()
(150.68) output: (scripted_sequence,end_hardcorps_heli1_sequence) -> (end_hardcorps_heli2,Wake,2.0)()
(152.68) input end_hardcorps_heli1_sequence: end_hardcorps_heli1.Kill()
Cancelling script: highrise_scene
unhandled input: (Wake) -> (end_hardcorps_heli2), from (scripted_sequence,end_hardcorps_heli1_sequence); target entity not found
```

**Validated:** the first actor exists in actual HL2 and is renderable; `BeginSequence` is accepted; the associated `OnEndSequence` outputs fire; the +2 second kill input reaches the first helicopter. The screenshot also depicts a helicopter airborne with rotating rotor blades, consistent with the animation. The trailing `Cancelling script` occurs after the first actor's scheduled removal and must not alone be interpreted as failed sequence startup.

**Next diagnosed blocker:** `end_hardcorps_heli2` is missing when the first sequence's completion output sends `Wake`. Identify the original BSP classname, model, targetname, and any `scripted_sequence` fields of helicopter 2 before adding another targeted actor substitution. Test once with the new isolated BSP rather than mutating the working helicopter-1 version. This finding does not establish full automatic endgame flow, scene.cache / Finale_Cinematic VCD playback, complete SiN generic-actor behavior, or Quest 3 compatibility.
