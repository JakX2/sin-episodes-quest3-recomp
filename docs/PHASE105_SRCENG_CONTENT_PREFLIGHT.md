# Phase 105: srceng Android content-compatibility reconnaissance

**Purpose:** determine whether the user's legally owned *SiN Episodes: Emergence* maps and models have Source format headers within the ranges **advertised** by nillerusr's Android Source engine, before attempting any rendering test. This is a **read-only** owner-local preflight, not a SourceVR/Quest port and not an endorsement to redistribute any third-party engine.

## Verified upstream information

- The upstream source README at https://github.com/nillerusr/source-engine states Android/ARM and 64-bit support, BSP v19–21 (v21 partial) and MDL v46–49. These are **declared support ranges**, not verified SiN compatibility.
- The upstream Android build wiki at https://github.com/nillerusr/source-engine/wiki/Source-Engine-(EN) gives an older WAF build workflow involving Android NDK r10e, Clang 11 and `--togles --android=...`; it must not be confused with our own pinned NDK 27 CMake builds. No nillerusr engine build or version-specific gameplay ABI has been reproduced by this PR.
- Android content setup described by community instructions usually places game folders directly under the `srceng` root, alongside `hl2` and `platform`; the program only checks this layout if a root is supplied. It **does not** construct or modify Source `gameinfo.txt` mount rules.
- The upstream repository says its engine source is based on leaked Valve code. Its `LICENSE` is the Source 1 SDK License, which restricts redistribution and has special third-party rights conditions. **Do not copy, fork, embed, or redistribute this engine in our MIT repository**, and do not assume it is a legally redistributable Quest host. Local owner-held comparison remains separate from a lawful product integration.
- The Valve Developer Community `Source_on_Android` page was not directly retrievable in our research environment (403). This report does not claim to have validated that page's contents.

## Run against an owner-held game folder (no copying)

Use Python 3.9+ and a real unpacked game folder containing maps, models and/or other assets. The input is read-only and stays on the user's machine.

~~~powershell
py scripts/srceng_sin_preflight.py "D:\\Games\\SiN Episodes Emergence\\SE1" -o srceng-sin-preflight.json
# Optional: inspect an existing srceng folder layout without writing to it:
py scripts/srceng_sin_preflight.py "D:\\Games\\SiN Episodes Emergence\\SE1" --srceng-root "D:\\Android\\srceng" -o srceng-sin-preflight.json
~~~

Replace the example path with your **actual** game-content directory (e.g. wherever maps and gameinfo reside). The tool does not guess game folder names. It scans up to 250,000 files, skips symlinked files/directories and reads at most 1,036 bytes from each sampled BSP, 8 bytes from each MDL, 16 from each VTF, 12 from each VPK and 256 KiB from a gameinfo file. It never reads full maps/models, extracts archives, attempts network access, copies anything to `srceng`, or executes binary code.

The JSON reports:

- Resource counts by type and up to 24 sampled Source-format headers per format.
- BSP version, map revision and any lump file offsets exceeding the file size. **Within 19–21 is a candidate**, not proof that srceng can load or render the BSP; outside means **outside advertised range**, not conclusive impossibility.
- MDL version compared to upstream's **46–49** advertisement. VTF/VPK header versions are reported as observations only.
- The mere presence of `GameInfo`/`SearchPaths` tokens, *not* resolved mount rules; whether optional `hl2` and `platform` content folders exist.
- Samples of case-sensitive filename conflicts that could cause broken material/model references on Android.

The report contains **source-relative file names** and small metadata only, not asset bytes. Review it before sharing, and **never commit original game content or private generated reports to GitHub**. On synthetic test fixtures:

~~~sh
python3 -m unittest discover -s research/tests -p 'test_srceng_sin_preflight.py' -v
~~~

## Actual owner-local preflight: 2 November 2026

The owner ran the read-only scanner on an existing Steam SiN Episodes `SE1` installation and supplied a JSON report. The content itself **has not** been uploaded or copied to the repository. This section reports verified **header metadata**, not engine runtime compatibility.

| Observation | Owner-local result | Consequence |
| --- | --- | --- |
| File scan | 17,962 files, no truncation | Complete path inventory, but sampled format headers |
| Maps | 32 `.bsp`; first 24 reported samples all `VBSP` version **19** with zero out-of-bounds lumps | Within srceng `MINBSPVERSION=19`, `BSPVERSION=21`; *map-load test still required* |
| Models | 1,121 `.mdl`; first 24 samples all **version 44** | Outside advertised README MDL 46–49, **not necessarily rejected by engine code** |
| Textures | 3,216 `.vtf`; first 24 samples all version **7.1** with sane header sizes | Reader/shader/runtime must still be tested |
| Materials | 3,095 `.vmt` | Many loose textures and material scripts; mount/search paths may still fail |
| Model companions | 3,318 `.vtx` and 1,106 `.vvd` | Geometry dependency filenames/extensions exist, no pair validation yet |
| gameinfo | Present; `GameInfo` and `SearchPaths` tokens found | Search paths **not** yet resolved |
| Collisions | No case-insensitive path collisions detected in scan | One Android filesystem risk reduced |
| VPK | No `.vpk` in resource count or samples | This installation appears to use loose content in the scanned subtree |

### Important MDL 44 nuance from upstream source

The upstream **README is not the entire compatibility contract**. The source tree at https://github.com/nillerusr/source-engine/blob/ed8209cc35c61fbd8ddff8480962a01c981eef2f/public/studio.h has a `Studio_ConvertStudioHdrToNewVersion` routine with branches for versions below 46/47. Its fork intentionally leaves the original studio header version untouched instead of blindly changing it to 49. The upstream `utils/mdlcheck/mdlcheck.cpp` code treats versions below **44** as outdated, suggesting version 44 was anticipated in at least some tooling. This gives us an **engineering reason to test MDL 44 before attempting model recompilation**, not proof that every renderer path accepts it. Also verify `.vtx`/`.vvd` pair formats and dynamic animation behavior during an eventual test.

The exact map/header subset examined is sample-limited. Do **not** claim all 32 maps or all 1,121 models share the sampled version until an exhaustive scan confirms it.

### Candidate first private Android rendering test

- `maps/arena_U4Factory.bsp` (1,652,792 bytes, BSP 19, 36 populated lumps, zero out-of-bounds lumps) is a compact first **BSP loading** candidate. As an arena map it may still require SiN-specific entity code for actual gameplay.
- `maps/SE1_U4Lab05.bsp` (559,272 bytes, BSP 19, 36 populated lumps) is an even smaller smoke candidate, though small size does not establish that it is a self-contained playable level.
- `maps/se1_docks01.bsp` (18,995,812 bytes, BSP 19, 43 populated lumps) is a later full-size validation target once materials, world geometry and prop loading work.

No one has yet loaded one of these actual maps with `srceng` in this project. Keep the test **private, optional, and non-redistributed** due to third-party engine provenance. No proprietary game assets or source code should enter the public repo.

## Follow-up: actual PC Half-Life 2 runtime observations (2 November 2026)

**A new owner-local desktop test now exists.** With only `maps/`, `materials/`, `models/` and `sound/` mounted under `hl2/custom/SE1` (and colliding `background01`–`background06` BSPs excluded), the owner showed real SiN maps rendering convincingly in the **PC Half-Life 2 engine**. The owner additionally demonstrated a breakable supply crate with spawned pickups, both SiN vending-machine variants using `models/hl2_props/vendingmachine.mdl`, and console errors for unregistered SiN-specific `npc_jessica`, `npc_jc` and `npc_blade` classes. At least one console screenshot identifies `se1_docks02`. This is **not** an Android `srceng` render test.

**Newly identified Android migration risk:** the disk directory `models/HL2_props` and some uppercase `VendingMachine` companion files do not match the casing of the in-engine lower-case reference. The existing scanner's zero case-folded **collisions** does not prove a case-correct asset/reference tree on Android. Investigate actual `srceng` filesystem behavior before normalization or conversion.

The full reproducibility notes, evidence limitations and next-step gate are in **[Phase 105 PC HL2 runtime findings](PHASE105_PC_HL2_SIN_RUNTIME_FINDINGS.md)**. These observations lower perceived *content-format* risk on PC, **not** the legal host/game-DLL/Android/VR blockers.

## Concrete next in-engine experiment, contingent on the preflight

1. Record exact BSP/MDL versions and whether the game layout actually contains the loose dependencies. **Do not assume** that a VPK/material mounting rule works unchanged.
2. With a separately acquired, authorized `srceng` installation and owned assets, attempt **one original SiN BSP** in a private test folder, while retaining a pristine original installation. Keep Source engine binaries out of our public repo.
3. Capture a real Android engine log and screenshot. Classify the outcome separately as: header rejected; mount/materials failed; BSP loaded/rendered; entity spawning/script failed; or real gameplay working. **Only the first three outcomes can be tested without porting SiN-specific code.**
4. Evaluate which SiN entities and scene scripts require an adapted Android game DLL. The original Windows x86 `client.dll` / `server.dll` cannot be loaded directly on Android ARM64.
5. Preserve **R1 blocked** until an appropriately licensed engine host and genuine gameplay callbacks have been compiled and tested. Our Phase 104 diagnostic APK is independent of `srceng` and is not a Source game.

**No SiN BSP, MDL, VTF, VPK or native game module was supplied to this Phase 105 CI.** It can validate only deliberately fabricated fixture headers, not whether a SiN map renders.
