# Phase 76: Original SiN Episodes SDK research

**Evidence:** Static inspection of the owner's SinEpisodesSDK.zip, not live game execution. The independently authored C++ helper and SDK auditor do not contain original Ritual map/SDK text. No binaries, VMFs, VPKs, FGDs, models or original assets are committed.

## Provenance

- ZIP SHA-256: 5a934e6c3f99151f2ba1a90fd080c7d6cd3f80a2b2b8118e5ef9cb2f3bb3d20b
- SiN FGD SHA-256: 4e803f2e23e09ee2dd275064ee71be133536549d34321ff2c7237320ac8d60f9
- base.fgd SHA-256: a3b69cd6129d10a64c2277cfab4721b8913ce5081ae9915889f12a930b23f16d
- weapon_magnum.txt SHA-256: 2f77a013eb591e787ba139647a6d7ca340ac3456c13c7fed1a5ad255d42aef3d
- weapon_assault_rifle.txt SHA-256: 491cdb481d9bd43941e36c6733f32f2ce3a1e10e2cc4ab992ead7b9b2d593f5f
- weapon_scattergun.txt SHA-256: 09f000e235a5c9e14cbd38fe1f4a2ba8a913a96e29823b474dc51cd5a6aebc6c

## Original WeaponData values

| Setting | Magnum | Assault rifle | Scattergun |
|---|---|---|---|
| ZoomFOV | 50 | 40 | 60 |
| clip_size / default_clip | 20 / 20 | 45 / 45 | 6 / 6 |
| clip2_size / default_clip2 | -1 / 0 | -1 / 0 | -1 / 0 |
| primary_ammo | MAG | AR | BUCK |
| secondary_ammo | DUAG | ARNADE | FLAK |
| viewmodel | models/weapons/v_magnum/v_magnum.mdl | models/weapons/v_assault_rifle/v_assault_rifle.mdl | models/weapons/v_scattergun/v_scattergun.mdl |

These close the **Phase 44 per-weapon ZoomFOV data gap**. The opt-in original_sdk_weapon_config(WeaponKind) C++ helper supplies only model path, FOV, ammunition capacities and default ammo. A {0,0,0} origin offset is a **placeholder, not a verified aiming pose**. It is not automatically enabled in the existing v1 C ABI. The portable VR implementation must not reduce headset projection FOV.

## Three map sources packed in VPK

The original VMFs are inside vpks/depot_1318_dir.vpk and vpks/depot_1318_000.vpk, not loose ZIP files. VPK v2 directory records identify the files, and the new private auditor validates original VMF payload CRC-32 before inspection.

| Original VMF | Entity blocks | Hammer output connections | Dynamic difficulty triggers | Changelevel triggers | SiN vehicle |
|---|---:|---:|---:|---:|---:|
| se1_docks04 | 3,405 | 436 | 6 | 2 | 0 |
| se1_highrise01 | 3,138 | 713 | 9 | 2 | 1 |
| arena_office | 1,146 | 60 | 0 | 0 | 0 |
| **Total** | **7,689** | **1,209** | **15** | **4** | **1** |

None of these 1,209 explicit connections has a malformed 5-component output shape under the scanner's simple parser, but that does not prove targetname resolution or actual game execution.

The two campaign VMFs contain 74 npc_template_maker instances combined (20 docks, 54 highrise), plus doors, relays, scripted sequences and actual transitions. Original trigger_dynamic_difficulty estimated-difficulty values are tier 1: 3, tier 2: 10, tier 3: 2 across the two sample campaign maps. In SinEpisodes.fgd, that entity declares EstimatedTime, EstimatedDifficulty (editor options 0 to 4), EndOfGame and a repeat-related spawnflag.

The original prop_vehicle_sin FGD defines SetView, SetCamera, MonitorOn, MonitorOff and several car radio, window and door inputs/outputs. This is consistent with Phase 22's binary camera research; it does not establish native gameplay.

## Reproduction

Run on a separately held, lawfully obtained SDK ZIP:

    python3 research/tools/audit_sin_sdk.py /private/path/SinEpisodesSDK.zip --output /private/path/sdk-audit.json
    python3 -m unittest discover -s research/tests -p 'test_*.py'
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

The private JSON should stay outside git. Only selected aggregate metrics and WeaponData fields are emitted. The VMF scanner understands original tab-indented Hammer 2006 source; it is not a universal Source parsing implementation.

Nine new synthetic Python tests exercised VPK parsing, CRC and bounds, VMF connection multiplicity, non-On outputs and malformed data locally. One native CTest was added for the original weapon values and the VR FOV guard. GitHub CI is responsible for independently rerunning the repository tests.

## Remaining limits

- No original game executable or compiled BSP has been launched.
- No SourceVR engine adapter, loader, Quest module or physical headset proof exists.
- No engine-side target resolver, save/restore or map transition implementation.
- The script values do not determine recoil execution, sight pose, animation events or reload duration.

**Next:** strengthen Phase 74's synthetic entity I/O coverage using independently authored representations of VMF patterns, then integrate with an accessible engine only after R1 issue #12 is solved. Also tracked by gameplay issue #20.
