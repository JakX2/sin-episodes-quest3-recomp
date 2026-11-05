# MobGrunt original animation sequence discovery

## What the owner tested

- A named desktop HL2 `prop_dynamic` called `sin_mobgrunt_anim_test` loads `models/characters/MobGrunt/mobgrunt.mdl` and its included `mobgrunt_anims.mdl`.
- `ent_dump sin_mobgrunt_anim_test` reports `classname: prop_dynamic`.
- `ent_fire sin_mobgrunt_anim_test SetAnimation idle` reaches the prop but reports `Dynamic prop sin_mobgrunt_anim_test: no sequence named:idle`.
- Therefore model load and entity input dispatch succeeded, but the guessed sequence `idle` is not accepted. This does NOT indicate a missing/corrupt model or incompatible animation format.

## Read the actual names (original assets remain on owner's Windows machine)

```powershell
cd "D:\sin-scene-mvp-updated"
Invoke-WebRequest -UseBasicParsing "https://raw.githubusercontent.com/JakX2/sin-episodes-quest3-recomp/feature/sin-mobgrunt-sequence-inspection-20261105/research/tools/list_source_mdl_sequences.py" -OutFile ".\list_source_mdl_sequences.py"
py .\list_source_mdl_sequences.py "D:\SiN\models\characters\MobGrunt\mobgrunt.mdl" "D:\SiN\models\characters\MobGrunt\mobgrunt_anims.mdl"
```

The parser reads only `IDST`/little-endian Source 1 studio headers, version 36-53, and lists the local `numlocalseq` descriptors using `szlabelindex` offsets relative to each descriptor. It fails closed on inconsistent tables. It does not attempt to merge included models' virtual runtime sequence list; use both files. If this old studio version differs from the expected header/descriptor layout, use the SDK's `hlmv.exe` to examine the same model's sequence selector instead of guessing labels.

Once a **non-empty, verified** sequence name is found, test it on the named prop, e.g. `ent_fire sin_mobgrunt_anim_test SetAnimation <exactly-reported-sequence-name>`. Do not hardcode `idle`, `walk`, `run` without evidence. A listed local animation may still depend on engine activities, pose parameters or include remapping. A successful sequence transition is visual animation proof, not proof of restored SinTEK AI or weapon mechanics.

Data boundary: no original model, SDK binaries, client.dll/server.dll or game assets are distributed in the repository.

## Original model listing, owner-confirmed

The owner ran the read-only sequence inspector on the extracted SiN models. For `D:\SiN\models\characters\MobGrunt\mobgrunt_anims.mdl`, it reported **IDST version 44** and **240 local sequences**. The complete 240-entry output was provided in chat, including these indexed names:
- 38 `Idle1`, 39 `Idle1_SMG1`, 40 `CombatIdle1`, 41 `CombatIdle1_SG`, 42 `CombatIdle1_SMG1`, 43 `CrouchIdle`
- 81 `grenThrow`, 85 `reload`, 86-91 reload gestures, 92 `kick`
- 98 `shootAR2s`, 99 `shootAR2c`, 101 `shootAR2alt`, 145 `RunALL`, 151 `Walk_all`, 162 `gesture_shoot_ar1`, 163 `gesture_shoot_ar2`
- 223 `Idle_Unarmed`, 234 `checkpoint_grunt_check`, 235 `checkpoint_grunt_idle`

This is **sequence metadata discovery**, not animation playback. The first tester previously sent `SetAnimation idle`, which returned "no sequence named:idle". The exact source-listed spellings to try next against `sin_mobgrunt_anim_test` are `Idle1`, `CombatIdle1`, and `reload`, one at a time. Check for transition out of reference pose and absence of "no sequence named". If none work, inspect `mobgrunt.mdl` include-table mapping/virtual sequences and consult original SDK HLMV before treating included local sequences as directly callable.

Original assets remain local; no binary files are committed.

## Owner-confirmed MobGrunt animation playback (Nov 5, 2026)

**PASS: at least one of the exact listed original model sequences plays in desktop HL2 on the owner-created named prop `sin_mobgrunt_anim_test`.** Owner confirmed "workin" and supplied a screenshot with the original MobGrunt moved out of its earlier rigid T-pose into a distinct arm/torso pose. The preceding local-source test plan used `Idle1`, `CombatIdle1`, and `reload`; the screenshot alone does not establish which specific sequence(s) were tested or guarantee that all three passed. Record only at-least-one-sequence playback.

This independently demonstrates the original 2006 Source model assets can be loaded and animated by HL2's prop animation path with `mobgrunt_anims.mdl` included. It **does not** prove combat motion blending, attachment and correct weapon grip, original NPC schedules or damage, original firing, or a native Quest port.

Next useful incremental desktop smoke: with the same named prop (and no BSP rewrite), try the original local firing sequence `shootAR2s`:

```
ent_fire sin_mobgrunt_anim_test SetAnimation shootAR2s
```

Despite the AR2 label, this is a sequence name from the original SiN MobGrunt animation bundle, **not** a request to restore HL2 rifle gameplay. Check whether the animation plays and whether its arms align with the separately rendered original rifle. Do not equate an animation playing with working projectiles/sounds/ammo or original SinTEK AI.

## Verified original firing sequence playback (owner, Nov 5)

**PASS:** The owner ran the next proposed `ent_fire sin_mobgrunt_anim_test SetAnimation shootAR2s` test and confirmed "it's workin". This adds verified desktop HL2 animation playback for the original SiN MobGrunt firing sequence `shootAR2s` (original included `mobgrunt_anims.mdl`, local sequence index 98). The confirmation does **not** establish an actual projectile, gun entity, muzzle flash, original sound, equipment attachment, AI integration or Quest runtime.

**Next isolated hypothesis:** check whether stock HL2 `npc_combine_s` can be instantiated with `model models/characters/MobGrunt/mobgrunt.mdl` using a map-spawn keyvalue rather than changing an active NPC's model. Test one named NPC in a disposable copy of the known working map, no global BSP rewrites. A different game-class animation activity table may leave the NPC idle, in reference pose or unable to attack; even success remains a Combine AI proxy, not original Ritual's SinTEK AI.
