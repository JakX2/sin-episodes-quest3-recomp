# Phases 35–36: assault rifle, scattergun and spawned projectiles

**Status:** recovered reverse-engineering findings, not byte-for-byte historical ZIP archives. Selected instructions in these notes were **rechecked against the original 2006 DLL** by `phase23-43-backfill-verify.py`. Other behavior remains inferred or unverified.

## Reused Source weapon hierarchy

- `CWeaponAssaultRifle` directly inherits `CHLSelectFireMachineGun`, which inherits Source machinegun classes. Its original primary aligned vtable shared **296 of 319** pointers with its direct parent in the same SiN DLL.
- `CWeaponScatterGun` directly inherits `CBaseHLCombatWeapon`. Its vtable shared **298 of 316** pointers with that parent.
- These pointer counts demonstrate inheritance within SiN, **not** binary compatibility with unrelated 2023 HL2 or Quest SourceVR binaries.

## Scattergun's two distinct firing paths

At **`0x2231C450`**, the scattergun bullet-like attack prepares a Source-style firing request with **11** as its shot/pellet count (instruction `0x2231C5A9`). The exact spread, damage, ammo consumption, range and tracer behavior were not established.

At **`0x2231C810`**, its alternate path enters a loop with counter **8** (instruction `0x2231C9B4`), spawning a separate entity named `flak`. A direction-like vector is scaled by **1500.0** before an entity movement-related function; the final parameter semantics have not been fully proven. The flak class is `CFlak`. Never substitute a vanilla HL2 shotgun secondary fire for the released scattergun's flak attack.

## Flak callback chain

Spawn setup installs its actual **contact callback at `0x2231BD70`** (assignment at `0x2231CF88`). Scheduled think callbacks include `0x2231CC70`, `0x2231B990` and the delayed removal function `0x2231B8A0`. Reported delays in the uninterrupted chain are **0.25 s, 0.5 s and 2.0 s**, respectively. These are scheduling intervals, **not** proof of a simple fixed total lifetime. The string `FlakTouch` corresponds to a deferred callback name; the actual installed collision callback is the code address above, not the name alone.

Precise collision response and damage to targets remain unresolved.

## Assault rifle alternate grenade

The assault-rifle alternate path lives at **`0x22313C60`** and creates a `grenade_ar` entity, of type `CGrenadeAR` (derived from Source `CBaseGrenade`). The projectile references a custom model and `GrenadeARThink` / `GrenadeARTouch`.

Launch-vector calculations use **2000.0** and **1000.0** scalars along different branches. Don't assume one fixed projectile velocity without establishing the original branch condition.

Spawn installs the grenade contact callback **`0x223142F0`** (at `0x22314BAB`) and its think callback **`0x22314360`**. Its first think is scheduled about **0.1 s** after spawn; later thinks can use **0.05 s** intervals. Contact and think can invoke a grenade detonation virtual at **`0x22314570`**. Contact code checks the target against the grenade thrower; complete fuse, collision, damage radius and explosion timing remain to be recovered.

## Porting boundary

Rebuild these *new* entities on the selected SourceVR branch while reusing compatible licensed Source weapon primitives. These findings are **not** a working weapon DLL and have no Quest 3 runtime integration.
