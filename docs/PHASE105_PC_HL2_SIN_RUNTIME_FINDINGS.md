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

## MAJOR FOLLOW-UP: actual Android srceng SiN BSP rendering (2 November 2026)

**Supersedes the earlier statement that no Android SiN rendering has been shown.** Following the above PC tests, the owner supplied **four Android screenshots** of original SiN Episodes dock content displayed and navigated in a mobile `srceng` Source engine, with translucent on-screen touchscreen controls. One Android console screenshot explicitly identifies `Map: se1_docks02` and `Source Engine 25 (build 8491853)` at its upper right. The Android console also shows a path under `/storage/emulated/0/srceng/hl2/`, corroborating the Android source-engine file layout. The exact Android device chipset/OS version and the exact mount/search-path arrangement for SiN assets were not supplied.

**Observed via owner screenshots:**

- Original SiN `se1_docks02` world geometry is loaded and rendered on **Android**, with recognizably correct industrial architecture, metal rolling doors, warning signage, docks perimeter, concrete roads, props, sky and baked/local lights. This is **a real Android Source-host BSP/content test**, not Phase104's standalone synthetic JNI probe.
- The mobile host renders a first-person HL2 pistol and crowbar and shows its touch-control overlay. Screenshots taken at multiple map locations show movement/navigation through the world; the screenshots do **not** demonstrate SiN weapons or VR controls.
- A mobile screenshot of the dumpster/warehouse area shows a set of objects scattered near the dumpster, visually consistent with the PC supply-crate/drop experiment. Their exact spawn mechanism, pickup behavior and whether the original SiN loot logic was invoked **were not verified** by a console trace.
- Graphics quality and effects can differ from desktop, but screenshots show recognizable textured world and assets without the original severe flat-shading failure.

**Android console findings seen in the screenshot:**

```text
Saving /storage/emulated/0/srceng/hl2/hl2_sound_misc.vpk.sound.cache
Constraint () attached to two shadow objects (brush_hinge and shitter_door)!!!
Redownloading all lightmaps
MP3 initialized with no sound cache, this may cause janking. [ music/docks_lighthouse.mp3 ]
*** Invalid sample rate (0) for sound "#music/docks_lighthouse.mp3".
```

The audio error is important: **rendering success does not establish working soundtrack playback**. The map's physics constraint warning also persists across hosts. We have **not** established whether all SiN props, map entities, animated NPCs or scripted triggers work on Android.

### Revised proof/status matrix

| Goal | Evidence | Status |
| --- | --- | --- |
| PC HL2 engine loads original SiN dock BSPs | PC in-game/console screenshots | **Owner demonstrated** |
| PC SiN textures/models/lighting | PC screenshots in the above observations | **Owner demonstrated, sampled maps/areas** |
| Android `srceng` loads `se1_docks02` | Android console and gameplay screenshots | **Owner demonstrated** |
| Android SiN content textures, world geometry and many static props render | Android gameplay screenshots | **Owner demonstrated, sampled areas** |
| Android mobile controls navigate the map | Four screenshots from several positions with touch-control overlay | **Strong visual evidence** |
| Android music, full physics and entity-specific gameplay | Warning and screenshots, no direct behavioral traces | **Not validated; audio warning remains** |
| SiN entity registration and NPC behavior | PC HL2 reported `npc_jessica`, `npc_jc`, `npc_blade` unknown | **Still missing in HL2 host, Android not established** |
| Original Windows SiN `client.dll`/`server.dll` running on Android ARM64 | Windows PE binaries cannot be directly used as Android ARM64 modules | **Not achieved** |
| Distributable lawful Source-compatible engine + SiN gameplay module | Upstream `srceng` provenance/licence concerns, no integrated game module | **R1 still blocked** |
| Standalone Quest 3 stereo VR gameplay/performance | No Quest device/VR renderer/input evidence | **Not achieved** |

### Next narrow engineering gate after this Android success

1. Archive an **owner-local** Android console log and note device/OS, `srceng` build, map command and actual SiN mount paths; avoid publishing copyrighted original files.
2. Verify a few representative SiN map model/material references on Android and investigate whether case-insensitive lookups are supplied by `srceng` before mass-renaming content. Successful rendering means case mismatch is not an automatic universal blocker, **not** that every asset reference resolves.
3. Address `music/docks_lighthouse.mp3` audio decoding/initialization as a separate test, without letting it block gameplay entity porting.
4. Move the next implementation focus to **real SiN game-class registration and engine callbacks**, with a legally usable native host target; preserve the already proven Android map-render evidence and distinguish it from our synthetic CMake/JNI smoke.
5. **No direct lift-and-shift of SiN Win32 client/server DLLs:** require source-compatible reimplementation/integration and legal engine provenance review.

**Repository policy:** these are owner-reported screenshots/console observations; no engine or SiN proprietary binaries/assets/screen-captured asset bytes are checked in. This milestone reduces a major Android *content-render* uncertainty but does **not** finish the Quest 3 port.

## Engineering implications and next evidence gate

1. **Preserve the PC configuration** as a reproducible baseline, noting engine version, the exact map command, mounted directories, console warnings and screenshots. Initial bad rendering vs the clean four-directory mount is evidence for a resource/mount or configuration conflict, not proof of its exact source.
2. **Test the same owned assets privately on Android `srceng`** in an isolated content tree, initially with a known-good HL2 control map and then a small SiN BSP. Record *actual device* logs, engine build/ABI, resolved asset paths and screenshots. No Android SiN rendering has yet been shown.
3. **Audit case-sensitive references and asset companions** before attempting bulk model conversion. MDL v44 samples (from the preceding preflight) are not, by themselves, a reason to rewrite models.
4. **Build an entity-class gap inventory** from actual map entity lump data and original SDK/binary research: separate generic Source entities which work, shared classes with wrong behavior, unknown SiN classes, and client-only features. Prioritize a single actual SiN entity registration through a legally usable test host rather than another synthetic JNI-only demonstration.
5. **Treat DLL portability separately**: Windows x86 SiN game binaries do not load as Android ARM64 Source modules. Independently authored implementations, verified APIs and a legitimate host integration are necessary. Original owner-held binary files and third-party engine source must remain outside the MIT public repo.

**Updated status (2 Nov 2026):** **owner-demonstrated PC and Android Source-host SiN dock world rendering**, including mobile on-screen control navigation and textured props. **Still blocked:** a legally distributable/reproducible Android ARM64 engine build and genuine SiN gameplay module integration, full game logic, and Quest 3 stereo rendering, tracked controller input and performance. Phase 104's separate synthetic Android probe is not the same as the owner's real Android `srceng` test.
