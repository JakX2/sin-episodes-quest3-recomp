# Phase 133: partial original SiN grunt schedule translator

## Evidence

This implementation uses the preserved Phase 8 static-analysis report for the SHA-pinned 2006 Windows x86 `server.dll` (SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`). The original DLL was not executed and raw bytes were not available for a fresh verifier run in this development pass.

The grunt's primary vtable slot 271 at `0x2235D5B0` is a high-confidence `TranslateSchedule`-style method. It handles 14 special input IDs in a 90-entry dispatch range; ordinary inputs delegate to the humanoid implementation at `0x220897A0`.

## Implemented decisions

| Input | Reconstructed result | Evidence |
|---|---|---|
| `0x12` | `0x52` (`SCHED_GRUNT_VICTORY_DANCE`) | Direct return |
| `0x1A` | `0x5E` (`SCHED_GRUNT_TAKE_COVER_FROM_BEST_SOUND`) | Direct return |
| `0x4F` | `0x51` when the observed NPC virtual result is nonzero; otherwise preserve `0x4F` | `0x2235DAA4` |
| `0x50` | Promote `SCHED_GRUNT_SUPPRESS` to `0x56` (`SCHED_GRUNT_SIGNAL_SUPPRESS`) only when all four host-resolved guards pass; clear the latch at `+0xC67` | `0x2235DA1B`–`0x2235DA98` |
| `0x30` | If grenade/reload guards pass, `0x63`; otherwise call CSE1Rules mobility slot 104 with factor 20.0, selecting `0x54` when true or unnamed shared ID `0x31` when false | `0x2235D83E`–`0x2235D898` |
| `0x64` | Patrol becomes `0x67` (`SCHED_GRUNT_PATROL_ENEMY`) only when the enemy resolves and its observed `+0x8C` field is zero; otherwise preserve `0x64` | `0x2235DAC0`–`0x2235DAEF` |

The input `0x30` name and output `0x31` name remain unresolved. The mobility factor reuses the Phase 131 helper and its caller-supplied random sample. The input `0x4F` virtual's semantic name and the enemy field at `+0x8C` are not guessed.

## Dispatch safety

The 14 original grunt-specific inputs are recognized. Six known cases are implemented above. The remaining eight special cases are returned as `UnresolvedSpecialCase`; they are not incorrectly sent to the parent. This includes `0x6B`, whose path delegates to the humanoid translator only after local side effects that are not yet reconstructed.

Inputs outside the 14-entry special set return `DelegateToHumanoid`, matching the observed default path. The result is a typed disposition; an eventual engine adapter must call the target build's symbolic parent translator. No 2006 numeric schedule ID is passed directly into a new engine.

## Tests and scope

The new regression target covers the suppression latch side effect, all suppression guards, grenade/reload precedence, factor-20 difficulty behavior, patrol-enemy conversion, combat-failure conversion, all 14 special-case IDs, unresolved-case containment and ordinary parent delegation.

This is a portable partial behavior reconstruction, not a complete grunt translator, native Source NPC or Quest execution. No proprietary DLL, APK, original ELF, BSP, model, map, script or game asset is included.
