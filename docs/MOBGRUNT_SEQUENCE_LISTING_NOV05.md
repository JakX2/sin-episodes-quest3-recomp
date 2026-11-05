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
