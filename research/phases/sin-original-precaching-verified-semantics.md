# Original SiN server.dll precaching: verified behavior and corrections

**2026-11-02.** This work uses the owner's **unaltered original SiN Episodes: Emergence 2006 Windows x86 server.dll** (SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`). The machine-code addresses in this report are **not Quest 3 ARM64 addresses**. No original game binary bytes are committed.

## Function identification from original compiled code, not guesses

Original SiN code includes these diagnostic literals:

| Original 2006 VA | Literal text / meaning |
|---|---|
| `0x224561BC` | `NULL Ent in UTIL_PrecacheOther` |
| `0x224542A4` | `CreateEntityByName( %s, %d ) - CreateEdict failed.` |
| `0x2240BAFC` | `Attempting to precache model, but model name is NULL` |
| `0x2240BB34` | `Late precache of %s` |
| `0x22431544` | `PrecacheScriptSound '%s' failed, no such sound script entry` |

Verified original 2006 machine-code chains:

- `0x22287CB0` is **UTIL_PrecacheOther**: checks a classname cache, calls `0x22266B00` (CreateEntityByName) on a first use, invokes methods on the temporary entity, and calls `0x22286C10` to remove/destroy it after precaching. Other paths skip repeated precaches.
- `0x22266B00` calls the exact original server entity registry getter `0x2228B110`; the diagnostic above explicitly names CreateEntityByName. That identifies the factory entry but **does not** make every caller an actual live spawn.
- `0x22149B70` is a model precaching/index helper: explicitly checks null models and warns about late precaching through the string/model subsystem.
- `0x221B6170` wraps a call to `0x221B5DB0`; the latter checks the original sound emitter interface and contains the `PrecacheScriptSound` diagnostic. This identifies script-sound preparation, not a sound actually playing.

## Original class entrypoint-to-resource callsite inventory

The new analyzer uses exact bounded `.text` function ranges and decodes actual `call rel32` with GNU objdump. A nearby `push` immediate is resolved to a real NUL-terminated `.rdata` string only when present. Dynamic arguments remain unresolved.

| Original class pre-cache entry | Entity helper calls | Model helper calls | Sound-script helper calls | Total |
|---|---:|---:|---:|---:|
| CWeaponAssaultRifle `0x22313780` | 1 | 0 | 0 | 1 |
| CPropPhone `0x223027C0` | 1 | 1 | 2 | 4 |
| CNPC_GruntBase `0x2235DE00` | 5 | 7 | 3 | 15 |
| CNPC_Quadralex `0x223746B0` | 2 | 1 | 16 | 19 |
| **Total** | **9** | **9** | **21** | **39** |

Original **Assault Rifle**: the only classified entity precache here is `grenade_ar`. Its function then tail-jumps into a shared base routine; **this is not the grenade firing method**.

Original **Phone**: `prop_keypad` is precached. The base model and two script-sound arguments are **dynamic**, so the callsites themselves do not identify their final values. The routine separately contains the default model `models/props_general/payphone_01.mdl` and a script KeyValues `PhoneBook` load. Do not assume this routine is the dial-number input handler.

Original **GruntBase**: entity resources `item_healthvial`, `item_ammo_ar_grenade`, `item_ammo_flak`, `item_ammo_duag`, and `weapon_grenade`. Model resources include `models/characters/armor/MG_helmet1.mdl`, the decapitation head and four head gib models; its first model argument is dynamic. Sound cues: `NPC_Grunt.WeaponBash`, `NPC_grunt.helmet_impact`, `NPC_grunt.cough`. Precaching does **not** prove a specific drop probability, NPC health value, damage event, or decapitation condition.

Original **Quadralex**: entities `shockwave` and `grenade_spit_poison`; model `models/characters/Quadralex/Quadralex.mdl`; sixteen `NPC_BigMutant.*` sound cues such as Walk, Run, RunHurt, Shove, HitHard, Roar, Die, Fallover and growls/breathing. These sound names are **precached**, not evidence of when the corresponding attack/animation transitions occur.

The derived per-resource CSV and JSON generated locally hold each original helper callsite VA, script/model/entity literal, and explicit dynamic-argument status.

## Important correction to preceding investigation

Some earlier PR #66 notes described `0x2235DE00` as a possible grunt weapon-drop behavior and `0x22313780` as evidence toward Assault Rifle grenade firing. **Those interpretations were too strong**. Their actual original machine-code calls go to **UTIL_PrecacheOther / PrecacheModel / PrecacheScriptSound**, so these entrypoints mainly prepare assets and temporary entity types. They do not give spawn/drop odds or firing conditions.

That does **not** invalidate independent analysis of `CWeaponAssaultRifle` routine `0x22313C60`, which has distinct `ARNADE`-related state/branch logic. That routine remains a candidate for deeper alternate-fire analysis, without declaring it fully recovered.

## Next original-game implementation research

1. From the grunt's **non-precache** virtuals and their call graph, recover when the item and weapon classnames are chosen and actual entity creation occurs, including the exact control-flow branches and player/NPC state writes.
2. For Assault Rifle, trace `0x22313C60` and helper calls into ammo checks, attack cooldowns and projectile creation, separating actual spawn from `UTIL_PrecacheOther`.
3. For the phone, trace the non-precache use of `PhoneBook`, keypad input handlers, dial validation and sound emission.
4. Trace `CNPC_Quadralex` behavior/action methods, not the precache list, to identify poison spit scheduling and transitions.
5. Rebuild gameplay only after comparing recovered original behavior with the client network/prediction contract in PR #64 and script values in PR #65.

**Validation:** the read-only standard-library analyzer `research/tools/audit_sin_precache_semantics.py` requires the user's exact original server SHA, checks the five diagnostic strings and three original call-chain relationships, and scans four bounded entrypoints with GNU objdump. Synthetic regression tests deliberately keep the commercial DLL out of GitHub. The original game executable and scripts are not republished.
