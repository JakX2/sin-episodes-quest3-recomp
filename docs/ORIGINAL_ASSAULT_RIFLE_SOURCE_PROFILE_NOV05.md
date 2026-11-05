# Original SiN assault rifle: verified sources (Nov 5, 2026)

This project milestone uses the owner's original SinEpisodesSDK.zip, shipped scripts.zip, and 2006 client.dll/server.dll. No proprietary input files are committed.

## Cross-checked original data

- SDK SinEpisodes.fgd: npc_druglab_grunt_assault_rifle inherits BaseGrunt, uses models/characters/MobGrunt/mobgrunt.mdl, and defaults to weapon_assault_rifle. Druglab elite defaults to weapon_chaingun.
- SDK base.fgd: spawnflags 516 = Fall to ground (4) + Fade Corpse (512). sleepstate 3 = Waiting for input.
- Owner's original rooftop grunt roof_grunt_1_seq1: additionalequipment weapon_assault_rifle, NumGrenades 5, damagefilter grunt_unkillable. Preserve these pending native entity implementation and map-script validation.
- Shipped weapon_assault_rifle.txt: viewmodel models/weapons/v_assault_rifle/v_assault_rifle.mdl; world model models/weapons/w_assault_rifle/w_assault_rifle.mdl; 45-round clip, 45 default, clip2_size=-1, ammo AR and ARNADE, ZoomFOV=40, anim_prefix=smg2, weight 7, plus original recoil fields.
- SDK weapon script and shipped game script match byte-for-byte; SHA-256: 491cdb481d9bd43941e36c6733f32f2ce3a1e10e2cc4ab992ead7b9b2d593f5f.
- Shipped game_sounds_weapons.txt contains every sound event named in rifle's SoundData section, including Weapon_Assault.NPC_Single.
- Original server.dll SHA-256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. It contains CWeaponAssaultRifle and its CEntityFactory specialization RTTI plus original grenade_ar projectile name; x86 grenade spawn literal/model confirmed against research verifier addresses.
- Original client.dll SHA-256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d. It contains assault-rifle class/name markers.

## Added code and verification

The original_assault_rifle_profile.hpp profile feeds verified values into the existing engine-neutral WeaponConfig. Its test exercises weapon model binding, original 45-round reload accounting, flat-screen ZoomFOV 40, and no artificial HMD FOV narrowing. The read-only verify_original_assault_rifle_game_sources.py cross-checks the actual original ZIPs and pinned binaries. Synthetic tests require no redistributed game files.

To rerun from original files, use:

    py research/tools/verify_original_assault_rifle_game_sources.py --sdk-zip YOUR_SDK.zip --scripts-zip YOUR_SCRIPTS.zip --client YOUR_client.dll --server YOUR_server.dll --output new_evidence.json

## Engine integration boundary

The profile is NOT a functioning registered weapon entity. Stock HL2 cannot resolve a missing SiN classname by adding this C++ data alone. The original Windows x86 DLLs cannot simply run in an ARM64 Android game.

Next: add an independently written native weapon_assault_rifle factory to the selected legal SourceVR/Android game module; use this config for asset precache, viewmodel, ammo registry, primary fire, reload event, sound and network lifecycle. Reconstruct the original grenade_ar secondary projectile with the existing hash-pinned original x86 instruction research, not invented firing behavior. After native gun validation, port one unique MobGrunt NPC while preserving weapon_assault_rifle, the wake input state, named damage filter, and scripted squad logic.

The previously successful HL2 Combine and weapon-pickup proxies remain desktop smoke tests, not the target shipping behavior.