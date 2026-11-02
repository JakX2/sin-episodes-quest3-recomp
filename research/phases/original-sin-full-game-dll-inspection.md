# Original SiN client.dll and server.dll: full known factory registration audit

**Source:** original proprietary 2006 PE32 x86 game DLLs, fetched from the owner's Google Drive `sin files/bin.zip». They have the same hashes as the originals already pinned in `research/binary-provenance.json». No original game executable bytes have been added to GitHub.

| File | Bytes | SHA-256 |
|---|---:|---|
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

## Verified exports and dependencies

**Client:** exports only `CreateInterface». Imports WINMM, KERNEL32, USER32, GDI32, ADVAPI32, vstdlib and tier0 DLLs.

**Server:** exports `CreateInterface» and four decorated grenade methods for `CGrenadeHomer::Detonate», `CGrenadePathfollower::Detonate», `CGrenadeSpitMutant::Detonate» and `CGrenadeSpitPoison::Detonate». Imports KERNEL32, USER32, vstdlib and tier0. The grenade exports are not separate Source game interface factories.

## 510 distinct original server entity registration call-shapes

The previous Phase 73 scanner pinned 32 selected entity factory registrations. An expanded scan discovers **510 distinct, complete 22-byte routines** whose instructions call the exact registry getter at `0x2228B110», pass a NUL-terminated ASCII classname pointer in `.rdata» and a distinct entity factory object pointer in `.data», then make the virtual registration call and return. **All 32 earlier anchors are present unchanged in these 510.**

| Literal name prefix | Count |
|---|---:|
| npc_ | 54 |
| weapon_ | 8 |
| item_ | 19 |
| prop_ | 35 |
| trigger_ | 27 |
| logic_ | 15 |
| func_ | 50 |
| env_ | 78 |
| ai_ | 23 |
| info_ | 29 |
| point_ | 16 |
| phys_ | 17 |
| game_ | 9 |
| grenade_ | 6 |
| path_ | 3 |
| Other | 121 |
| **Total** | **510** |

Examples of exactly verified original registration function VAs:

- `npc_sintek_grunt» at `0x223C77A0», `npc_sintek_jetpack» at `0x223C77C0»
- `npc_recombinant_major» at `0x223C74E0», `npc_quadralex» at `0x223C7350», `npc_jessica» at `0x223C6E10»
- `weapon_magnum» at `0x223C5150», `weapon_scattergun» at `0x223C54C0»
- `item_antigen_dispenser» at `0x223C40A0», `prop_phone» at `0x223C47A0»
- `trigger_dynamic_difficulty» at `0x223BE4A0», `prop_vehicle_sin» at `0x223C5BE0», `info_challenge» at `0x223C3ED0»

These are **factory registrations**, not gameplay implementations or a claim that 510 distinct intelligent/gameplay systems are complete. The count is exhaustive **only for this specific 22-byte original compiler pattern**, not for every possible registration mechanism.

## Client and server RTTI

Searching exact NUL-terminated MSVC class/struct type descriptors produces **770 distinct client RTTI names**, **977 server RTTI names**, with **144 names shared**. Names include original SiN client weapons `C_WeaponMagnum», `C_WeaponAssaultRifle», `C_WeaponScatterGun», HUD `CHudGrenades» and `CHudPoisonTimer», plus server classes `CWeaponMagnum», `CNPC_SintekGrunt», `CNPC_SintekJetpack», `CNPC_Quadralex», `CNPC_Jessica», `CAntigenDispenser» and `CPoisonGas».

Compared with the owner's separate Windows HL2 client/server binaries, the SiN class-string sets contain **151 client-only** and **207 server-only** type names. This comparison identifies divergence, **not** a count of methods needing porting or proof of matching C++ ABI.

## Shipped weapon scripts are not the same as compiled factories

Google Drive `scripts.zip» and `SinEpisodesSDK.zip» both carry 11 weapon script names. The original server registers exactly **8** names in its recognized `weapon_» call pattern:

`weapon_assault_rifle», `weapon_chaingun», `weapon_grenade», `weapon_magnum», `weapon_melee», `weapon_npc_pistol», `weapon_scattergun», `weapon_smg».

The other script names are `weapon_flaregun», `weapon_rocket_launcher», and `weapon_sniper_rifle». Their absence **from this particular registry pattern** does not prove they never spawn or are unused. Client RTTI still contains `C_WeaponRocketLauncher» and `C_WeaponSniperRifle».

## Reproduce from local user-owned files

~~~powershell
py research\tools\audit_full_sin_game_dlls.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --weapon-scripts-zip "C:\SiN\scripts.zip" --output "sin-full-dll-audit.json"
~~~

This standard-library Python tool requires exact original hashes and produces a detailed JSON report of all 510 registration locations, factory pointers, and both RTTI name sets. No proprietary DLL or reconstructed source is distributed.

## Next programming gates

1. **Real gameplay side:** build a legal, engine-compatible entity registration and I/O adapter, beginning with `worldspawn», `info_player_start», `player», `logic_auto», `logic_relay», `trigger_once», `func_door».
2. **SiN player and guns:** map the registered weapons and networked/predicted client classes, then implement real engine-hosted player/controller/weapon behavior instead of assuming a mock-host weapon runtime is enough.
3. **AI and campaign:** reconstruct the actual SinTEK grunt schedules, scripted sequences, antigen/poison logic and map transitions on top of a host that actually accepts new client/server game modules.

**Unproven:** ARM64 SourceVR vtable compatibility, live registration into the packaged engine, working SiN BSP/entity execution, VR gameplay. The original Windows x86 DLLs are not Quest 3 modules.
