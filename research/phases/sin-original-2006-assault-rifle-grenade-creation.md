# Original SiN Assault Rifle: actual grenade entity creation

Source: user-owned original 2006 server.dll, SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Original Windows x86 static evidence, NOT Quest code.

## Precache vs actual projectile launch

- At original 0x22313780, CWeaponAssaultRifle calls UTIL_PrecacheOther for **grenade_ar**. This prepares the projectile type only.
- A distinct routine at **0x22313C60** contains real grenade creation and weapon-state logic.
- At **0x22313E25**, original x86 pushes a .rdata pointer to the literal **grenade_ar** at 0x22477230.
- At **0x22313E2A**, a direct CALL targets **0x22146C40**, which accepts the classname and other arguments and performs entity creation.
- At **0x22146C48**, that helper calls **0x22266B00**, the original **CreateEntityByName** entry. At **0x22266B34**, it invokes the independently verified registry getter **0x2228B110**.
- After the entity is created, original 0x22313E3B calls 0x22141A00 with a computed vector, 0x22313E46 calls 0x22143610 on the entity and 0x22313E55 calls 0x222B3430. The exact original C++ names of these state methods are not proven.
- The firing function also references the original ammo/script literal **ARNADE** at 0x2245F43C.

These are **actual machine-code CALL edges** validated against the unchanged original DLL, not an inferred code path from strings alone.

## Verified original numeric literals

| Original x86 rdata VA | Exact float | Verified role |
|---|---:|---|
| 0x224771F0 | 2000.0 | One branch's direction-vector multiplier |
| 0x223E97F0 | 1000.0 | Alternate branch's direction-vector multiplier |
| 0x223D9778 | 0.5 | Added to global game time in a weapon timing path |
| 0x223D22F8 | 1.0 | Added to global game time in another timing path |

The 1000/2000 branch selection occurs around 0x22313DE7; its triggering condition, physical units, and full gameplay purpose still require recovery. At 0x22313FBB the original calls 0x22173260 for a state near weapon+0x49C; at 0x22313FDE it calls 0x221732A0 near weapon+0x4A0. These appear to be timed state-update paths, not yet source-identified prediction fields or verified visible cooldowns.

## Repeatable audit

Run from a checkout with the owner-held original private server.dll:

    py research/tools/audit_sin_original_ar_grenade_spawn.py --server "C:\private\SiN\server.dll" --output "ar-grenade-creation.json"

This standard-library tool verifies 7 exact original x86 CALL destinations, original grenade_ar and ARNADE string bytes, 4 IEEE754 numeric literals and the pinned original DLL SHA-256. Its synthetic tests check damaged opcodes, mutated branch targets, constants and original fingerprint enforcement.

## Actual next implementation work

Resolve the original ammo consumption and attack checks, the created projectile's position/velocity/owner initialization and damage and explosion behavior, and the matching client.dll predicted fields and animation events. Only then reproduce the behavior in real SourceVR-compatible ARM64 game code.

No SiN native Quest client/server gameplay is claimed in this research phase, and original x86 function addresses must not be reused as ARM64 pointers.
