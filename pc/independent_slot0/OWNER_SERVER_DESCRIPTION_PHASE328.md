# Phase 328: actual retail slot 11 and original game description

## Observed owner experiment

Phase 325 retained the owner-original client and replaced only the server.
The independent server accepted ServerGameDLL003, ServerGameEnts001, and
ServerGameClients003. It rejected HLTVDirector001, then logged a terminal
guard at ServerGameDLL003.slot11. Exit was the deliberate 0x53494E45
diagnostic stop. No DLLInit entry or dependency requests were observed.
Restoration was VERIFIED_COMPLETE.

## Original evidence

The private original server.dll was locally SHA-256 verified as
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Slot 11 of the original main server table points to RVA 0x1E7C40.
It delegates to RVA 0x2F0CC0. If the global game-rules pointer is null,
the helper returns the static string "SiN Episodes" at RVA 0x466DF8.
Otherwise it dispatches a method on the existing game-rules object
(virtual byte offset 0x94). The SDK 2004 IServerGameDLL and
CServerGameDLL::GetGameDescription signatures corroborate the no-argument
const-char-pointer return. The game-rules virtual method's full behavior
is outside the scope of pre-init reproduction.

The original separately registers HLTVDirector001 at RVA 0x3C1B60.
The live trace shows its rejection did not prevent slot 11 entry.
This does not establish that it will be optional in later startup.

## Implementation

Replace slot 11's terminal guard with the authentic pre-game-rules
description and a diagnostic callback trace. The independent DLL does
not yet initialize game rules, so its only reachable branch is the
pre-init behavior. Test the actual returned vtable method, stable string
storage, and every remaining guard. Extend the owner-local metadata
inspector and replay the actual Phase 325 evidence.

Do not treat this as full game-rules support or successful DLLInit.
Use the same explicit server-only diagnostic opt-in for the next test.
No retail binaries or original method code are committed.
