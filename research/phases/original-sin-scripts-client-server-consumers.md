# Original SiN Episodes scripts × client.dll/server.dll evidence

**Status:** read-only evidence for the **2006 Windows x86 DLLs** and user-owned original game scripts. This is directly relevant to the gameplay missing in Android HL2/HL2VR even if SiN BSP geometry can already be loaded there. **Map-loading work is not part of this investigation.**

## Provenance and reuse

- Original client.dll: SHA-256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
- Original server.dll: SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
- The user's game scripts.zip is independently hashed in analyzer output; its local inspected copy includes 123 ZIP entries and 11 weapon definitions. Keep private script and DLL bytes out of the public repo.
- The previous codebase already contains game-script research (Phase 55 weapon parser, Phase 90 combat spread), a separate C++ weapon-script loader in PR #54, and native portable weapon implementation. Reuse these; the new script/consumer audit complements rather than duplicates those components.

## Actual original DLL script consumer sites

Literal pointers are recovered from PE32 .rdata and verified **push imm32** instructions in .text. These confirm an x86 instruction directly references the named key. The higher-level helper semantics still require disassembly.

| Script key | Original client x86 push VA | Original server x86 push VA |
|---|---|---|
| clip_size | 0x241E6765 | 0x222B45D5 |
| clip2_size | 0x241E6779 | 0x222B45E9 |
| default_clip | 0x241E6798 | 0x222B4608 |
| ZoomFOV | 0x241E6A96 | 0x222B4906 |
| MeleeWeapon | 0x241E68BC | 0x222B472C |
| kickMin | 0x241E68D8 | 0x222B4748 |
| kickRampSecs | 0x241E697A | 0x222B47EA |
| HUDColor | 0x241E67D1 | 0x222B4641 |

**Both** original DLLs contain their own weapon configuration consumers. The exact offsets and field sizes were confirmed in prior static analysis, e.g. original WeaponInfo clip size fields +0x160/+0x164 and default clips +0x168/+0x16C. These are **original x86 layouts** and cannot be copied into modern ARM64 SourceVR C++ structs.

The combat-rule lookup code is server-specific. Direct x86 push-immediate references exist to:
- class_names at 0x222D7FD6
- weapon_class_names at 0x222D8018
- target_class_names at 0x222D8053

The server also refers directly to the PhoneBook KeyValues block at 0x22302844. The presence of these strings plus script assets establishes **a gameplay-side lookup path to reconstruct**; reading a script on its own cannot make the old Windows handler work in HL2VR.

## Original scripts: substantive examples

| Weapon script | clip_size | ZoomFOV | primary ammo | secondary ammo |
|---|---:|---:|---|---|
| weapon_magnum | 20 | 50 | MAG | DUAG |
| weapon_assault_rifle | 45 | 40 | AR | ARNADE |
| weapon_scattergun | 6 | 60 | BUCK | FLAK |
| weapon_chaingun | 30 | 40 | CHAIN | field not specified |
| weapon_smg | 45 | 45 | AR | ARNADE |
| weapon_npc_pistol | 20 | 60 | MAG | DUAG |
| weapon_flaregun | 3 | 60 | FLARE | explicitly empty string |

Do not equate a specified secondary ammo type with a second physical clip. For example, Magnum, Assault Rifle and Scattergun all configure clip2_size=-1, despite separate ammo types. Script fields left absent are not assigned inferred numeric defaults by this audit.

Original weapon script names absent from the **single previously verified 22-byte server entity factory registration pattern**: weapon_flaregun, weapon_rocket_launcher, weapon_sniper_rifle. That fact does not establish that their classes are absent, unsupported, or never used.

### Combat spread

Original scripts/rules_combat.txt defines an AttackSpreadRules root with class_names, weapon_class_names and target_class_names maps. The configured player generic target spread is 0.0. The default NPC rule has 0.13053 against the player and 0.04362 against other targets, as literal game-script values. Specialized drug-lab NPC mappings exist, so substituting generic HL2 NPC aiming code will not reproduce original SiN combat spread behavior by itself.

### Campaign interactions beyond weapons

- scripts/phonebook.txt maps **56 unique dial strings** to named sound cues. **52** of these literal mappings start with phonebook.*, while other mappings route to police radio or Jessica cues. scripts/game_sounds_phonebook.txt defines a subset of the sound cues, but remaining cues can be declared in other manifest files, so do not label them missing from only the dedicated phonebook sound file.
- scripts/vehicles/sincar.txt and scripts/vehicles/sincar_pit.txt are **distinct** 5,599-byte presets. They contain engine parameters, intentionally repeated gear declarations, axle/wheel/suspension data and vehicle_sounds blocks. Parsing these as a flat key/value dictionary would destroy gear/axle ordering and lose behavior.

## Port implementation work now prioritized

1. **Preserve and bind original script values** to the native weapon session (already developed in other branches). Do not hardcode magazine size, ammo types, recoil curves or flat-screen ZoomFOV behavior.
2. **Trace and reproduce actual server weapon firing/combat rules**, including the AttackSpreadRules mapping and weapon-specific recoil, hit processing, NPC/class resolution and automatic difficulty effects. Compare against existing Phase 90 portable implementation before writing new code.
3. **Port client presentation/prediction**: original HUDColor, SoundData/TextureData, animation and viewmodel hooks. In VR, ZoomFOV is legacy input and should not change the headset's physical camera projection.
4. **SiN interactions and vehicles**: recover the server PhoneBook lookup, input handlers and phone sound dispatch; distinguish vehicle script parsing (which an HL2 engine may share) from SiN-specific handling.
5. **Interface/ABI integration**: supply compatible ARM64 SourceVR game client and server interfaces after validating their actual host headers/vtables. Original x86 parser addresses are debugging/reimplementation evidence, never directly loadable Quest libraries.

No new APK/launcher changes, BSP parser or engine-level game implementation are claimed by this research.

## Local, reproducible metadata audit

Run the new tool with the user-owned original files, not decompiled or substitute HL2 DLLs:

    py research/tools/audit_sin_script_dll_consumers.py --client C:\private\SiN\client.dll --server C:\private\SiN\server.dll --scripts-zip C:\private\SiN\scripts.zip --output C:\private\sin-script-consumer-report.json

The tool has strict expected original game DLL hashes, reports explicit KeyValues values and all qualifying code-push xrefs, preserves repeated SoundData/TextureData member names and repeated gear/axle entries, and labels incomplete factory recognition correctly. Original binaries/scripts are not republished.
