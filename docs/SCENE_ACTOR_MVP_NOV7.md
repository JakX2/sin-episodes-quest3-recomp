# November 7 milestone: JC/Jessica scripted-character compatibility slice

**2026-11-05. Status: buildable host-only C++17 bridge + optional copy-only BSP experiment + original VCD archive preflight.** This is **not** a working Source game DLL, native Android actor module, in-headset test, nor original SiN companion AI. It does not solve the SourceVR game-module ABI mismatch.

## Primary original-game evidence

Original SiN 2006 `server.dll` SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`; original `client.dll` SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. Independent reports: `SiN_Original_Jessica_JC_AI_DeepDive_07.md`, `SiN_Original_Scene_Actor_Binding_DeepDive_06.md`, `SiN_Original_Scenes_and_Scripted_Sequence_IO_DeepDive_04.md`.

| Original map classname | Original inheritance | Original default model | Compiled initial health | **Experimental stock HL2 substitute** |
|---|---|---|---:|---|
| `npc_jc` | `CNPC_JC : CAI_BaseActor` | `models/characters/JC/jc_torso.mdl` | 8 | `npc_citizen` |
| `npc_jessica` | `CNPC_Jessica : CNPC_PlayerCompanion : CAI_PlayerAlly` | `models/characters/Jessica/Jessica.mdl` | 80 | `npc_alyx` |
| `npc_generic_actor` | custom generic actor | map-provided model | map-provided | `npc_citizen`, optional |

The substitutes are **engineering hypotheses**, not verified source-level equivalences. Stock `npc_alyx` can force its own model, voice/response rules or animations. Bone/gesture compatibility and scene actor resolution must be tested inside HL2 before treating them as a success. A character appearing in a map is not evidence its scenes work.

## Build the standalone compatibility bridge (C++17)

When integrated into the project's Phase 103 CMake branch:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_scene_actor_bridge_tests
ctest --test-dir build -R sin_scene_actor_bridge_tests --output-on-failure
```

The new `SceneActorBridge` is an **independent host-facing adapter contract**. It requests an actor from a candidate existing Source base, applies original model/health defaults, checks choreography capability, preserves map targetnames, forwards exact VCD actor tokens, and routes Jessica's `ThrowItemAtPlayer`, `SetPrimaryAmmoCountToThrow`, `SetSecondaryAmmoCountToThrow`, and `HolsterWeapon` inputs. Throwing is triggered only by the host's registered `AE_JESSICA_THROW_ITEM` event, never just by the input. Host implementation of attachment lookup, entity creation, velocity, ammo and scene-manager calls is *not* supplied.

The original class's wrong-type `variant_t` input behavior is **not reproduced**: this bridge rejects wrong input types rather than silently setting fields to zero. Throw-event duplicate suppression is a safe port policy rather than an assertion about original decompilation.

## Local HL2 engine experiment without compiling a game DLL

The *opt-in* `research/tools/sin_bsp_scene_actor_trial.py` edits **only the entity-lump classname values in a new copy** of a user-owned Source BSP. It leaves targetnames, scene names, scripts, all other lump headers, and original lump bytes unchanged. This allows a first HL2 run of ordinary actor/companion proxies and pinpoints which scene errors remain. It does NOT implement Jessica's four original inputs, custom schedules or a working full scene actor.

```powershell
py research/tools/sin_bsp_scene_actor_trial.py "C:\SiN\maps\se1_finale01.bsp" --report
py research/tools/sin_bsp_scene_actor_trial.py "C:\SiN\maps\se1_finale01.bsp" --write "C:\SiN_Test_Only\se1_finale01.bsp"
```

**Crucial test discipline:** work on a *copy* of the map only; do not replace original game assets. The tool refuses overwrite and rejects unknown/compressed BSP entity lump formats. For HL2 to load the trial scene using `map se1_finale01`, you must mount that copy in your test mod's `maps/` folder under the same basename and mount the proper source game assets legally. If the engine rejects the modified BSP, revert to the untouched source. If you need to test `npc_generic_actor` too, add `--include-generic`, but do this only after examining the initial output.

Expected results from the temporary trial are limited: `npc_jc` and `npc_jessica` should no longer appear as *unknown classnames* **if the replacements exist in that HL2 build**. Model selection, animations, AI and VCD bindings may still fail. The tool does not register original classnames in the game DLL, and does not repair the many other missing SiN entities.

## VCD mounting and actor token verification

```powershell
py research/tools/sin_scene_archive_preflight.py "C:\SiN\scenes.zip" --log "C:\Logs\sin stuff finale.txt" --log "C:\Logs\sin stuff highrise.txt" --json scene-preflight.json
```

Against the previously provided `scenes.zip` and five HL2 logs: **261 VCDs, four unique reported missing VCD paths, all four VCDs present in the archive**. Examples:

| VCD file | Exact choreography actor tokens |
|---|---|
| `scenes/se1_finale/teaser_jc_hospital.vcd` | `JC_Hospital` |
| `scenes/se1_finale/teaser_radek_and_jessica.vcd` | `Roddick`, `Jessica`, `Helicopter` |
| `scenes/se1_highrise/jess_wakeupblade.vcd` | `!Target1` |

The presence of VCD text is **not proof of a working `scene.cache`**. The provided `scenes.zip` does **not** include `scene.cache`; its generation or Source-host runtime support remains a separate blocker. Source's `logic_choreographed_scene` has actor-target alias resolution and scene-event I/O; keep its original map structure intact.

## Acceptance gates for a true playable prototype

1. **Desktop HL2:** Original SiN map loads from test mod; JC/Jessica or valid proxies spawn without `unknown entity` errors; actor models animate, not T-posing. Capture a fresh console log.
2. **Scenes:** Correct VCD resources mount, scene cache loads, choreography actor names bind, speech/gesture playback works, and map I/O events complete. Test one JC scene (`teaser_jc_hospital.vcd`) and one Jessica scene (`jess_wakeupblade.vcd`).
3. **Companion:** Jessica follows/stands/plays scripted sequences without interrupting choreo; separate throw-item action may be postponed if not required by the chosen scene.
4. **Quest 3:** Only after real SourceVR engine/game-module integration is available; native arm64 module and headset test needed. Building the portable C++ bridge or loading the WebXR proof alone **does not pass this gate**.

Until gate 1 and gate 2 are observed, claim **"actor compatibility test harness"** rather than **"working SiN NPCs"**. Do not merge stacked draft branches automatically or publish proprietary game content.
## Automated scene/console diagnostics

Use the [November 7 actor trial diagnostics playbook](SCENE_ACTOR_TRIAL_DIAGNOSTICS_NOV7.md) to inspect a user-owned BSP's actor targetnames and scene aliases, summarize the five original logs and compare one fresh before/after HL2 console capture. The historical five-log baseline contains **6 JC, 17 Jessica and 49 generic-actor failed creation attempts**, with repeated loads possible. It does **not** prove any actor spawns or animates. The new tests are synthetic; live HL2/Quest trials remain outstanding.
