# Phase 132: partial original SiN grunt schedule selector

## Evidence

This reconstruction uses the preserved Library report `sin_re_phase7.md`, which records static inspection of the owner-supplied 2006 PE32 `server.dll` (SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`). The original DLL was not executed. Raw bytes were not available for a fresh verifier run in this development pass.

Phase 7's registration extraction mechanically pairs grunt-local condition strings with numeric IDs and schedule strings with numeric IDs. This allows the confirmed selector branches to use symbolic names rather than the ambiguous integers in the earlier Phase 6 report.

## Implemented branch segments

The implementation is deliberately a partial selector. It preserves the recovered branch order and delegates three gaps to an explicit extension interface rather than pretending that omitted branches do not exist.

| Order / original evidence | Condition or state | Original schedule |
|---|---|---|
| First branch at `0x2235E4E0` | `npcFlagsB6 & 0x02` and descriptive one-shot field at `+0xCB0` is clear | `SCHED_GRUNT_SITTING` (`0x6F`) |
| Early branch | `COND_GRUNT_ON_FIRE` (`0x46`) | `SCHED_GRUNT_BURNING_STAND` (`0x68`) |
| Early branch | `COND_GRUNT_KICKABLE_GRENADE` (`0x4C`) | `SCHED_GRUNT_KICK_GRENADE` (`0x73`) |
| Early branch | `COND_GRUNT_SHOT_IN_HELMET` (`0x48`) | `SCHED_GRUNT_HELMET_FLINCH` (`0x75`) |
| `0x2235E591` | Shared condition `0x37`; name unresolved | `SCHED_GRUNT_DODGE` (`0x6D`) |
| `0x2235E6FB` | `COND_GRUNT_SQUADMATE_NEEDS_HELP` plus a nonzero NPC field | `SCHED_GRUNT_HELP_STUNNED` (`0x6C`) |
| `0x2235E719` | `COND_GRUNT_HELP_UP`; clear both help-up and shot-in-knee conditions | `SCHED_GRUNT_GETUP` (`0x71`) |
| `0x2235E763` | `COND_GRUNT_SHOT_IN_KNEE` without the help-up branch | `SCHED_GRUNT_STUNNED` (`0x6B`) |
| `0x2235E787` | `COND_GRUNT_HELPER_FOUND_PLAYER` (`0x4B`) | `SCHED_GRUNT_MOVE_TO_PLAYER` (`0x6E`) |
| `0x2235E883`–`0x2235E8AA` | `COND_GRUNT_SHOULD_PATROL` (`0x44`) after an additional guard | `SCHED_GRUNT_PATROL` (`0x64`) |

Phase 7 also establishes `0x4D` as `COND_GRUNT_IN_PLAYER_SIGHTS`. PR #183's Phase 131 call-site enum and documentation were corrected to use that recovered name.

## Explicit unknown gaps

The original selector has additional control flow between the recovered segments. `IGruntSelectorExtension` is called at these boundaries:

1. After the first-priority conditions and before the shared-condition dodge branch.
2. After dodge and before squad-help selection.
3. After helper-found-player and before the guarded patrol branch.

An extension can return an `OriginalGruntScheduleId` for another recovered original branch. It must not reinterpret the old ID as a SourceVR/HL2 schedule ID.

The first branch's field labels are descriptive, not recovered C++ symbols. The squad-help nonzero field and patrol guard are host-supplied facts because their original member names/complete guard expressions are not established by the preserved report. Shared condition `0x37` is intentionally not given a guessed semantic name.

When this partial selector returns no schedule, the original function's tail at `0x2235E8B0` delegates to `CAI_BaseHumanoid` slot 269 at `0x22089700`. The portable function returns `nullopt`; the future engine adapter must perform the parent call.

## Build and tests

The new `sin_original_grunt_ai` static target compiles the implementation for host builds and for the existing Android NDK smoke-build configuration. Its regression target covers priority, one-shot sitting state, condition clearing, the unknown shared condition, guarded branches, extension gaps and parent-fallback signaling.

This is portable C++ behavior reconstruction, not a native Source NPC, not a complete grunt AI, and not a Quest execution test. No proprietary DLL, APK, original ELF, BSP, model, map, script or game asset is included.
