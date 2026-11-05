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
