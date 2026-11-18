# Phase 325: owner server-only factory negotiation diagnostic

## Observed retail result (Phase 324)

The owner ran a server-only independent diagnostic while retaining the
original SHA-verified client.dll. The session trace contains precisely:

```text
[SiN independent] role=server phase=factory-accepted version=ServerGameDLL003
[SiN independent] role=server phase=factory-rejected version=ServerGameEnts001
```

The process exited zero with no independent DLLInit entry or dependency
queries. Module sampling did not observe server.dll, but the in-process
trace establishes execution. The selected original server was restored
and the retained original client was unchanged. The verdict was
GAME_MODULE_FACTORY_ONLY_NO_INIT_EVIDENCE.

This does not prove the rejected factory was fatal. It identifies the
next controlled experiment.

## Original metadata

The SHA-pinned owner-private PE32 server.dll was inspected directly.
The original singleton table spans are 31 ServerGameDLL003 entries,
7 ServerGameEnts001 entries, and 15 ServerGameClients003 entries.
The SDK 2004 main server interface has 29 entries; do not assume its
whole table is identical. The original GetPlayerLimits at RVA 0x1E63F0
writes minimum 1, maximum 32, default 1. The existing metadata tool
now authenticates the full contiguous table entry ranges.

## Diagnostic experiment

By default the extra factory names remain rejected. The owner launcher
requires -ProbeRole Server and -EnableServerFactoryProbe together.
It explicitly sets the process environment and validates an opt-in
session trace; the original client remains installed. Existing original
hash, disposable copy, backup and restore guardrails still apply.

The two main initialization methods remain fail-closed. The additional
main table entries and client table entries are index-labelled terminal
traps, not recovered C++ signatures or gameplay methods. The typed
entity table implements the debug-base pointer save and null conversions
only; unimplemented entity calls stop the current probe process without
dereferencing game objects. The clients table implements the recovered
GetPlayerLimits behavior. A terminal diagnostic writes the exact slot
or method and exits the current process with code 0x53494E45. The exit
classifier and startup classifier distinguish this from actual DLLInit
entry. None of this establishes a playable or ABI-complete game DLL.

All 49 terminal guards are covered in independently built synthetic
child processes in the new Windows test; no retail executable runs in CI.
The owner must not rerun a probe if restoration is incomplete.

Once this branch passes native CI, repeating the owner server-only test
with the explicit opt-in switch will distinguish further factory
negotiation, slot entry, DLLInit and missing dependency observations.
Original binaries, proprietary implementation code and raw disassembly
are not committed.
