# Phases 37–43: Magnum DUAG, iron sights and quick grenades

**Status:** recovered research notes, not copies of historical ZIPs. Specific code signatures and three shared zoom vtable slots were independently rechecked against the user-owned 2006 DLLs using the new [backfill verifier](../verifiers/phase23-43-backfill-verify.py).

## Phase 37: the pistol is a custom Magnum

SiN's released sidearm is represented in the server by `weapon_magnum` / `CWeaponMagnum`, with a matching client `C_WeaponMagnum`. Its primary vtable shares **301/316** aligned targets with SiN's `CBaseHLCombatWeapon`, while a customized secondary attack points to **`0x22316CF0`**.

The original client registers separate `+zoom` / `-zoom` and `+grenade` / `-grenade` commands. It packs **`0x00080000`** for zoom and **`0x02000000`** for quick grenade input. The game owner's observed stock **G** throw key is user gameplay knowledge; the DLL confirms the command but does not by itself establish the keyboard default binding.

The player confirmed that **all three released firearms have iron sights**, while grenades are thrown directly instead of selected like a vanilla HL2 weapon. Preserve offhand hand grenade actions separately from the rifle's `grenade_ar` projectile. A server `CWeaponGrenade` type exists but its presence does not imply an equipable grenade in normal gameplay.

## Phase 38: identify the Magnum's special shot as DUAG

Contemporary gameplay terminology calls the shot **DUAG (Depleted Uranium Anti-Gen)**. Internal DLL strings include `DUAG`, `item_ammo_duag`, `CItemAmmoDUAG`, `DUAGDecal1` / `DUAGDecal2`, and an ammo icon and shell model.

The Magnum's secondary routine checks secondary clip state (at `weapon+0x4F0` in this x86 build), prepares a one-shot bullet request, reads an alternate ammo-type field (at `+0x4E8`), and dispatches a player firing virtual. A timed action counter at `+0x57C` with a roughly **0.05 s** follow-up scheduling interval was also seen, but its initiating path was not identified and **must not be described as a proven two-shot attack**.

## Phases 39–40: ammo flags, dispatch and a corrected layout

DUAG registers the damage-type bitmask **`0x00400002`**, compared with MAG **`0x00000002`**. Its special bit is therefore `0x00400000`. Original `CAmmoDef` ammo records use **60-byte** strides. The original writer rearranges source-registration arguments into fields, so raw arguments must not be treated as known direct health-damage or penetration-thickness values.

A key correction: the first ammo getter used by shared player bullet code reads damage bits at entry offset **`+0x0C`**, while the second reads an independently registered property at **`+0x28`** (DUAG value **0**). The tracer-related registration argument is at entry offset **`+0x10`**. An earlier phase incorrectly called the second lookup a tracer lookup.

A field named `m_PenetratedAmmoType` belongs to a separate **`CSniperBullet`** save-data map at offset **`0x3C4`**, and is **not** established as the Magnum's DUAG penetration mechanism.

## Phase 41: player forwards impact visuals to equipped weapon

The Magnum secondary handler calls the player's shared bullet virtual at offset `+0x194`; the player code resolves to a `FireBullets`-style function at **`0x2214C8A0`**. Its conditional impact path dispatches to the player impact virtual, which in turn forwards to the currently equipped weapon. The Magnum's override at **`0x22316910`** calls base impact effects and, if the incoming damage bits contain **`0x00400000`** and a separate trace flag does not contain **`0x04`**, requests the distinctive `DUAGDecal1` / `DUAGDecal2` effects.

This proves a **weapon-specific visual impact path**, not a wall-thickness, repeated-trace or damage-loss algorithm.

## Phase 42: target damage filter and TraceAttack callbacks

The shared `FireBullets` implementation invokes the target's virtual at **`+0xD8`** (`0x2214D0AD`), checks its Boolean return, and if accepted invokes the target's `+0xDC` virtual (`0x2214D0CD`). This is consistent with Source's `PassesDamageFilter` / `TraceAttack` interfaces. Rejected target damage can still reach the **later visual impact decision** subject to other gates.

The inherited base filter has exact-equality checks for `0x00400000` and `0x00010000`. Combined DUAG bits **`0x00400002`** do not equal the former literal. The original rule for physically passing through walls remains **unverified**.

## Phase 43: aim state networking

Both SiN DLLs register `m_bZooming` and `m_bZoomScope`. `ZoomFOV` is also referenced in their weapon-related code. The SiN 2006 build differs from the user's HL2 2023 comparison build in the presence of `m_bZoomScope` and `ZoomFOV` strings; exact string comparison is not proof that the newer engine lacks equivalent features.

A shared weapon virtual getter at **`0x222AC030`** is inherited by Magnum, assault rifle and scattergun. Later Phase 44 research, already in this repository, further traces `ZoomFOV` and requested FOV transition durations.

## Still required

There is **no confirmed DUAG wall-penetration algorithm**, original numerical per-gun ZoomFOV values, finished reload animation timings, physical sight offsets, complete render prediction, or verified SourceVR gameplay module in these recovered notes. Preserve normal fire, weapon-specific secondary fire, zoom and quick grenade as distinct actions, and validate against the original released campaign.
