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

## Concrete next in-engine experiment, contingent on the preflight

1. Record exact BSP/MDL versions and whether the game layout actually contains the loose dependencies. **Do not assume** that a VPK/material mounting rule works unchanged.
2. With a separately acquired, authorized `srceng` installation and owned assets, attempt **one original SiN BSP** in a private test folder, while retaining a pristine original installation. Keep Source engine binaries out of our public repo.
3. Capture a real Android engine log and screenshot. Classify the outcome separately as: header rejected; mount/materials failed; BSP loaded/rendered; entity spawning/script failed; or real gameplay working. **Only the first three outcomes can be tested without porting SiN-specific code.**
4. Evaluate which SiN entities and scene scripts require an adapted Android game DLL. The original Windows x86 `client.dll` / `server.dll` cannot be loaded directly on Android ARM64.
5. Preserve **R1 blocked** until an appropriately licensed engine host and genuine gameplay callbacks have been compiled and tested. Our Phase 104 diagnostic APK is independent of `srceng` and is not a Source game.

**No SiN BSP, MDL, VTF, VPK or native game module was supplied to this Phase 105 CI.** It can validate only deliberately fabricated fixture headers, not whether a SiN map renders.
