# Original SiN Episodes 2006: gameplay calls behind grunt, Quadralex, weapons, phone

**Source provenance:** user's original Windows x86 DLLs, client SHA-256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d, server SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.

**Scope:** conditional branch and direct-call traversal from the vtable changes already validated in PR #66. The original binaries were analyzed locally but not uploaded to this public repository. Results are metadata and original x86 addresses, **not full C++ reconstruction**.

## One-hop expansion of selected game classes

| Original DLL/class comparison | Distinct changed virtual seed addresses | Additional unique direct-call targets examined | Per-group total |
|---|---:|---:|---:|
| server: CNPC_GruntBase / CAI_BaseHumanoid | 71 | 113 | 184 |
| server: CNPC_Quadralex / CAI_BaseNPC | 85 | 107 | 192 |
| server: CWeaponAssaultRifle / CHLSelectFireMachineGun | 23 | 29 | 52 |
| server: CPropPhone / CBaseAnimating | 9 | 20 | 29 |
| client: C_WeaponAssaultRifle / C_BaseHLCombatWeapon | 4 | 2 | 6 |
| client: C_WeaponMagnum / C_BaseHLCombatWeapon | 4 | 2 | 6 |

These are **per-group entry-address counts**; different groups can share callees. Addresses may identify thunks or other compiler-generated routines, not necessarily one distinct C++ method each. GNU objdump linear decoding can misidentify embedded code/data, indirect dispatch and jump tables; the new analyzer explicitly marks unresolved branches.

## Verified original code references

### Grunt combat base
- Original server 0x2235DE00 (changed virtual slot 26): 77 reachable instructions, 4 conditional branches and 4 indirect call instructions. Direct callsites to 0x22149B70 (7), 0x22287CB0 (5), 0x221B6170 (3), 0x22031B30 (1). Literal references: NPC_grunt.helmet_impact, NPC_grunt.cough, NPC_Grunt.WeaponBash, MobGrunt/head gib models, weapon_grenade, item_healthvial, item_ammo_duag, item_ammo_flak, item_ammo_ar_grenade.
- Original server 0x2235E0B0 (slot 58): 306 reachable instructions, 16 conditional branch instructions and 11 distinct direct callee addresses. References weapon_assault_rifle, weapon_npc_pistol, weapon_scattergun, weapon_grenade and assorted ammunition.
- Original server 0x2235F2A0 (slot 25): 244 reachable instructions, 14 conditional branches. References npc_druglab_grunt_assault_rifle and weapon_scattergun.
- Original server 0x223582A0 (slot 376): literal SCHED_GRUNT_KICK_GRENADE reference.
- A direct-call descendant at 0x2235B790 references weapon_*, item_health*, item_ammo*. This is evidence of code touching wildcard-name resources, not proof of spawn probabilities.

### Quadralex
- Server 0x223746B0 (slot 26): grenade_spit_poison, shockwave, movement/voice/damage sound cues, and Quadralex model resource.
- Server 0x22370C70 (slot 96): NPC_BigMutant.ChargeLoop, .RunHurt, .Run and .Walk.
- Server 0x22370D00 (slot 355): NPC_BigMutant.Die.
A resource reference alone is **not** proof of a particular AI schedule or attack transition.

### Assault Rifle
- Server 0x22313780 (slot 26): 9 decoded instructions, grenade_ar resource reference, direct call to 0x22287CB0.
- Server 0x22313C60 (slot 227): 278 reachable instructions, 17 conditional branch instructions, references ARNADE and grenade_ar. Direct targets include 0x222AD370 (9 callsites), 0x222D54B0 (4), 0x2230B240 (2), 0x221E5D50 (2), plus indirect calls. These are useful candidates for original secondary-fire/ammo behavior; the precise semantics remain unproven.

### Payphone
- Server 0x223027C0 (slot 26): 86 reachable instructions, 9 conditional branches. References PhoneBook, prop_keypad, Failed to load phone book!, and models/props_general/payphone_01.mdl. Direct target addresses include 0x22264410, 0x22264730, 0x22264200, 0x22264340, 0x22266000, 0x22264350. Mapping every callee to parsing/spawning/audio effects requires more instruction-level work.

### Original client side
The client weapon subclass's four changed virtual targets give a comparatively small set of one-hop helper targets (two additional addresses each for C_WeaponAssaultRifle and C_WeaponMagnum). Original client prediction and weapon presentation code may be inherited or nonvirtual, and this narrow result **does not establish** that client gameplay is small or complete.

## Reproduce privately

From the research branch, with GNU objdump and the same private original DLLs:

    py research/tools/audit_sin_gameplay_control_flow.py --binary "C:\SiN\bin\server.dll" --side server --focus-class CNPC_GruntBase --depth 1 --output "grunt-callgraph.json"

Other original server classes: CNPC_Quadralex, CWeaponAssaultRifle, CPropPhone. Original client classes: C_WeaponMagnum, C_WeaponAssaultRifle.

The script uses the SHA-pinned original RTTI/vtable analyzer, follows decoded branch/fallthrough and direct-call instruction targets with strict size/budget limits, and produces JSON metadata only. Synthetic tests exercise conditional paths, indirect versus direct calls, crossing into another known vtable entry, and jump-table uncertainty.

## Remaining work

1. Identify precise helper meanings with instruction-level argument/side-effect analysis, not string-name speculation.
2. Build full validated CFGs with reliable boundaries and switch tables; GNU objdump linear decoding is only a provisional foundation.
3. Reconcile actual original scripts (PR #65), client/server network tables (PR #64), and per-class vtable offsets (PR #66) into true gameplay contracts.
4. Port those behaviors to the real ARM64 SourceVR client/server ABI when verified. SiN BSP maps already load in Android HL2 and VR HL2, so map loading is not this project's current bottleneck.


## Shared nonvirtual game routines called across class systems

Cross-referencing the **original server DLL depth-zero virtual-entry calls** across GruntBase, Quadralex, CWeaponAssaultRifle and CPropPhone yields **33 different callee addresses used by at least two separate class groups**. This is stronger evidence of reusable server-side helpers than identifying each class in isolation. These are function *addresses*, not confirmed C++ function names.

Three original direct-call targets appear in **all four** investigated groups:

| Original server x86 helper address | GruntBase call sites | Quadralex | Assault Rifle | Payphone |
|---|---:|---:|---:|---:|
| 0x22287CB0 | 5 | 2 | 1 | 1 |
| 0x22143610 | 2 | 1 | 1 | 1 |
| 0x22138980 | 1 | 1 | 1 | 1 |

Two likely resource/service-related original targets appear in three:

- **0x221B6170:** GruntBase 3 call sites, Quadralex 16, phone 2.
- **0x22149B70:** GruntBase 7, Quadralex 1, phone 1.

Original instruction example: both Assault Rifle 0x22313780 and payphone 0x223027C0 push a specific entity classname string, call 0x22287CB0, then continue into their own class-level logic. The shared routine itself manipulates engine entity/string tables, so **entity precache/registration is a hypothesis worth testing**, not a proven source-level function name. Identifying exact helper semantics requires inspecting all call sites and the original helper's side effects.

The new tool research/tools/audit_sin_shared_helpers.py derives this cross-class inventory from the previous per-class metadata JSON, requiring the matching original DLL fingerprint and rejecting client/server mixtures. It compares **distinct class groups**, not inflated counts from multiple call sites within a single class, and excludes second-hop calls from the shared-first-hop tally.

Next: reconstruct these three shared helpers and their callers once, then integrate their verified behavior into the future ARM64 server modules where appropriate. This is more efficient than treating every weapon/NPC/phone subsystem as wholly isolated.
