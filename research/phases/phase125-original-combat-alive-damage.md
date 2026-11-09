# Phase 125: Original SiN common alive-damage reduction

**Evidence provenance:** Existing owner-supplied, SHA-pinned 2006 server.dll analysis record, SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. This phase reuses the preserved static trace and RTTI/vtable evidence; it does not claim a fresh disassembly in this run because raw-byte materialization of the owner ZIP was unavailable.

## Verified call graph

The preserved trace identifies the original CBaseEntity TakeDamage entry at 0x2213F460 and virtual +0xE0 dispatch. The relevant routes converge on the common integer health update at 0x222A7B20:

- CHL2_Player 0x2239FA00 -> CBasePlayer 0x2224C020 -> CBaseCombatCharacter 0x222A57C0.
- CBasePlayer alive-damage virtual +0x370 at 0x2224B130 -> 0x222A7B20.
- CAI_BaseNPC alive-damage virtual +0x370 at 0x22040140 -> 0x222A7B20.
- Recombinant Major +0x370 at 0x22379100 -> mutant base 0x22342CB0 -> CAI_BaseNPC 0x22040140 -> 0x222A7B20.
- Recombinant Minor +0x370 at 0x2237D0C0 -> mutant base 0x22342CB0 -> CAI_BaseNPC 0x22040140 -> 0x222A7B20.

At 0x222A7CE5 the x87 control word selects truncation toward zero; 0x222A7CF2 performs the float-to-signed-integer conversion; 0x222A7CFA subtracts that integer from health at object +0x9C; 0x222A7D0E stores the result. The combat-character caller tests updated health and can dispatch the death virtual +0xE8 at 0x222A598C. Generic NPC death evidence separately shows lifeState +0x8C set to 1, but that write is not generalized to every player/NPC death override.

## Independently authored implementation

- include/sin_compat/original_alive_damage.hpp
- src/original_alive_damage.cpp
- tests/test_original_alive_damage.cpp

The portable reducer accepts final, already-filtered damage and a host-owned generation-aware entity reference. It uses safe finite/range validation, truncates toward zero, subtracts in a wider integer type, writes through typed host callbacks, and dispatches the class-specific death hook only after the health write when new health is <= 0.

The route description records only the call chains established in the preserved evidence. Unknown classes are rejected instead of guessed. The host must map health/life-state and the death virtual through its own lawful engine APIs; original x86 offsets are never dereferenced.

## Explicit boundaries

This is one common alive-health stage, not complete TakeDamage or RadiusDamage. It does not implement damage-type filtering, global/vehicle scaling, armor (the player +0xBC4 branch is only partially resolved), physics-active deferral/replay, game-rules victim selection, knockback, network replication, or the exact death behavior of each subclass. Nonzero life-state rejection and integer-overflow rejection are safety policies in the portable host contract, not claims that every original path performs those checks.

The code does not load a Source entity, map, DLL, or SourceVR module. No proprietary binary, APK, ELF, BSP, model, map or game asset is committed. No C ABI surface is changed.

## Local verification

GCC 14.2 Release CMake/CTest: 1/1 test executable passed (7 focused test groups). GCC AddressSanitizer + UndefinedBehaviorSanitizer Debug: 1/1 passed. Clang 17 Release: 1/1 passed. GitHub Actions must independently verify repository integration and the Android NDK build.
