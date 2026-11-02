# Phase 105: owner-local SiN maps inside the PC Half-Life 2 engine

**Observed:** 2 November 2026. **Platform tested:** Windows PC, Half-Life 2 Source engine with owner-held *SiN Episodes: Emergence* assets. **Not tested:** SiN assets on Android `srceng`, Quest 3 VR, complete SiN gameplay, or a distributable engine build.

This is a manual compatibility observation, backed by screenshots and a console screenshot supplied by the owner during experimentation. It is **not** an automated CI test. It should guide the next investigation without changing the project's R1 blocker or implying the public repository contains proprietary assets.

## Reproducible owner-local content arrangement

Initial experiment: the owner copied the **entire** SiN `SE1` tree into `Half-Life 2/hl2/custom/SE1`. This produced severe flat-color/black object and world rendering anomalies and a separate map-load failure involving conflicting `background01.bsp`–`background06.bsp` menu/background maps. One observed error was `func_breakable: UTIL_SetModel: not precached: *10`, associated with a menu-background map. `*10` is a BSP brush-model index, not a file-based `.mdl` asset.

After isolating the content to the following **test copy** (not the user's original SiN installation), the owner supplied screenshots showing substantially correct rendering:

```text
Half-Life 2/
  hl2/
    custom/
      SE1/
        maps/
        materials/
        models/
        sound/
```

Other folders and files from the copied root (`bin`, `cfg`, `expressions`, `media`, `resource`, `scenes`, `scripts`, `shaders`, `GameInfo.txt`, caches, game metadata) were excluded for the clean-mount experiment; the conflicting `background0[1-6].bsp` files should stay excluded. **Do not conclude which removed folder caused the anomalies without one-variable-at-a-time reintroduction.** The cleaned content arrangement is a useful experiment, not a guaranteed minimum runtime dependency set. SiN-specific script resources may later need selective reintroduction.

## Observed real PC runtime results

- Screenshots show textured SiN environments, brick/concrete/metal surfaces, industrial doors, fences, trees and grass, dock/street scenery and an illuminated skyline. Local and colored lighting, environment props and first-person HL2 weapons visibly render.
- A console screenshot specifically identifies `Map: se1_docks02` in a running Half-Life 2 session. Do not silently generalize that verified map name to every other screenshot: the map names of the earlier screenshots were not individually confirmed.
- SiN-themed vending machines display Hammer Cola and Grape Drink branding, and both receive the **same in-engine model path** via `r_staticpropinfo 1`: `models/hl2_props/vendingmachine.mdl`. The owner also showed an `SE1/models/HL2_props` folder containing the vending-machine model and companion files. This establishes a shared model *reference* and successful rendering of SiN-specific appearance. A byte-for-byte identical HL2 mesh/model is **not yet established**; hash `.mdl`, `.vvd`, and `.vtx` independently to verify.
- `prop_crosshair` reported `hit prop 881` for a static prop. This is a **static prop index**, not a model path. `r_staticpropinfo 1` displays model names above `prop_static` instances; `r_staticpropinfo 0` disables it. This was verified against the upstream Source implementation at https://github.com/nillerusr/source-engine/blob/ed8209cc35c61fbd8ddff8480962a01c981eef2f/engine/staticpropmgr.cpp.
- A SiN-styled supply crate was shown intact, then removed with pickup-like objects on the ground. This is evidence that **some mapped breakable/entity behavior and spawning are active in HL2**; it does **not** establish SiN's intended loot tables, pickup types, collision across every object, or complete SiN gameplay.
- The owner observed many rendered props and some functioning shared entities. This does not validate all SiN models, `prop_dynamic` animation, full NPC AI, physics, mission triggers, doors or scripted sequences.

### Confirmed missing SiN gameplay registrations

The console screenshot showed errors for missing game-specific classes, including:

```text
Attempted to create unknown entity type npc_jessica!
Attempted to create unknown entity type npc_jc!
Attempted to create unknown entity type npc_blade!
```

This is **strong negative evidence for SiN-specific game DLL/entity registrations in the HL2 host**: HL2 can load the world but does not know those classes. It does not prove that recompiling the original Windows `client.dll` and `server.dll` is possible; they are binaries, not available source, and cannot simply be retargeted to Android ARM64. SiN-specific entity implementations or appropriately sourced compatible code are still needed. The separately implemented `libsin_compat_host_abi.so` and `libsin_compat_event_bridge.so` do not register these classes in an actual Source engine.

Other observed console warnings (not necessarily hard failures):

- `Need to rebuild map to get bumped lighting...` for some `se1_docks02` materials, suggesting old/branch-specific bump-lightmap details may differ.
- `Invalid sample rate (32000)` for `music/docks_lighthouse.mp3`; audio decoding support may need examination.
- A physics warning about `brush_hinge` and `shitter_door` involving constrained shadow objects. Keep this as evidence for targeted physics validation rather than declaring doors or constraints fully working.

### Important Android case-sensitivity trap discovered

The owner showed the asset directory name `models/HL2_props`, while the **in-engine** model label is `models/hl2_props/vendingmachine.mdl`. Companion filenames shown include differently capitalized `VendingMachine.dx80.vtx`, `VendingMachine.dx90.vtx`, `VendingMachine.phy`, `VendingMachine.sw.vtx` alongside lower-case `vendingmachine.mdl` and `vendingmachine.vvd`.

**Windows file lookup is commonly case-insensitive, Android/Linux filesystem lookup is often case-sensitive.** The existing owner-local scanner found no *case-folded path collisions*, but that is **not a reference-to-path case-mismatch audit**. Before moving the working PC assets to Android, validate whether `srceng`'s virtual filesystem normalizes case and check all referenced model/material/companion paths. Do not mass-rename the original files; work on a temporary copy and fix only reproducible failures.

## Engineering implications and next evidence gate

1. **Preserve the PC configuration** as a reproducible baseline, noting engine version, the exact map command, mounted directories, console warnings and screenshots. Initial bad rendering vs the clean four-directory mount is evidence for a resource/mount or configuration conflict, not proof of its exact source.
2. **Test the same owned assets privately on Android `srceng`** in an isolated content tree, initially with a known-good HL2 control map and then a small SiN BSP. Record *actual device* logs, engine build/ABI, resolved asset paths and screenshots. No Android SiN rendering has yet been shown.
3. **Audit case-sensitive references and asset companions** before attempting bulk model conversion. MDL v44 samples (from the preceding preflight) are not, by themselves, a reason to rewrite models.
4. **Build an entity-class gap inventory** from actual map entity lump data and original SDK/binary research: separate generic Source entities which work, shared classes with wrong behavior, unknown SiN classes, and client-only features. Prioritize a single actual SiN entity registration through a legally usable test host rather than another synthetic JNI-only demonstration.
5. **Treat DLL portability separately**: Windows x86 SiN game binaries do not load as Android ARM64 Source modules. Independently authored implementations, verified APIs and a legitimate host integration are necessary. Original owner-held binary files and third-party engine source must remain outside the MIT public repo.

**Status:** convincing **owner-reported PC Source environment/prop/shared-entity compatibility**. **Still blocked:** lawful, reproducible Android ARM64 engine + game-module integration, full SiN-specific gameplay, and Quest 3 stereo rendering, input and performance. Phase 104's Android probe remains synthetic and must not be conflated with this PC experiment.
