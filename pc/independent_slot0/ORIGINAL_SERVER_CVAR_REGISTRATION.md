# Phase 333: console registration following initial game events

## Evidence

Read-only disassembly of original server.dll SHA-256
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
confirms the immediately following original DLLInit calls:

| RVA | Operation |
| --- | --- |
| 0x1E7842 | Call InitializeCvars-style helper at 0x1E6450 |
| 0x1E6455 | Register pending ConCommandBase list through 0x25B350 |
| 0x1E646A | ICvar slot 3, const FindVar("developer") |
| 0x1E784C | FindCommand-style local list lookup ("sv_cheats") at 0x25BC00 |
| 0x1E785B | Branch to original failure return if sv_cheats is absent |

The original accessor at RVA 0x1E6490 sets game-DLL flag 0x4,
tests replicated flag 0x2000, and for replicated variables saves
the pre-link default through helper 0x25BB70. It clears the next
pointer, invokes ICvar slot 0 to register, then ICvar slot 1 to
obtain a command-line override by name. It sets a non-command's
value to the override if non-null, otherwise to its saved replicated
default when available. The original manager saves each successor
before invoking virtual offset 0x20 (Init). Original Init at 0x25B410
skips already-registered records, delegates to the accessor and sets
the registration byte.

The owner's SDK 2004 src_mod/dlls/game.cpp, tier1/convar.cpp,
public/tier1/convar.h and public/icvar.h corroborate these roles.
Original command records have next pointer +4, registered byte +8,
name +0xC, help +0x10, flags +0x14. The complete ConVar hierarchy,
parent-linked values, callback ABI, static constructors, and actual
command inventory remain unresolved.

## Independent implementation

server_cvar_registration_core.h models the manager and accessor
with independent owned records and explicit register, override,
value and engine-lookup callbacks. It preserves the pre-link successor
and replicated default across engine-like relinking, does not write
values to commands, and distinguishes absent overrides from empty
ones. It resolves developer and performs case-insensitive sv_cheats
lookup on the linked list after registration. It refuses malformed
bounded lists and reentry before mutation. Registration is not
repeated after a missing sv_cheats result because external effects
have already occurred. These are independent guards, not asserted
retail behavior.

The test begins with Phase 332's own state initialization and first
game-event load, then exercises five records and an already-registered
record using independently owned synthetic engine hooks. It verifies
mutation of next pointers by the registration adapter, preserved
replicated default, override precedence, command skipping, empty
override, absent sv_cheats, and no duplicate side effects.

This is not a full ConVar object or engine-facing implementation.
Do not pass ConsoleRecord to the retail ICvar: its layout is
intentionally independent. Phase 332's validated owner event probe
remains the next retail experiment. No original code or DLL committed.
