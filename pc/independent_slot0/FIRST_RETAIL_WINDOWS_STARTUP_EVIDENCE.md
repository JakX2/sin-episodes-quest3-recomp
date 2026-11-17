# First actual owner retail Windows startup probe, observed 2026-11-17

## Evidence provenance and boundary

The owner ran the opt-in reversible Windows x86 diagnostic launcher against a
disposable copy of SiN Episodes: Emergence, using the independently compiled
non-forwarding `client.dll` and `server.dll`. The owner subsequently
supplied the session-isolated **independent diagnostic lines** and the
engine's visible error. No original game code, disassembly, proprietary
pseudocode, or commercial binaries are included here.

The owner separately supplied the original retail `client.dll`; local
inspection confirmed PE32 Intel i386, 3,592,192 bytes, the only named
export `CreateInterface` at RVA `0x164180`, and SHA-256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
The 2006 PE timestamp in its image header is **not** proof of compilation
date. Embedded interface-name strings include `VClient011`,
`VEngineClient012`, `VEngineModel012` and `VEngineCvar002`. Embedded
strings alone are not verified call sites.

## Observed runtime sequence (same process log; ordered)

1. Independent server `CreateInterface("ServerGameDLL003")` accepted.
2. Independent server `CreateInterface("ServerGameEnts001")` rejected
   (this independent diagnostic factory currently does not expose that name).
3. Independent client `CreateInterface("VClient011")` accepted.
4. Independent client virtual slot-zero `Init` **entered**.
5. Client queried `VEngineClient012` from an engine factory: found.
6. Client queried `VEngineModel012` from an engine factory: found.
7. The independent client logged `sdk2004-preflight-only` and deliberately
   returned 0. The retail engine displayed:
   `Client.dll Init() in library client failed.`

The classifier recorded one matching log, both roles observed, explicit
Init entry **only for client**, no missing-interface/Init blocker, and
`DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF`. The PowerShell probe's tracked
process returned `0x00000000` after the dialog, which is **not boot proof**.

The module sampler did not observe either DLL in its snapshots, even though
the same process's independent diagnostic trace conclusively shows
executed factory/Init code. Therefore `MODULE_NOT_OBSERVED` cannot be
interpreted as proof the module was never loaded. The original server DLL
was restored, but the independent client DLL remained file-locked, causing
partial restoration. A later run correctly refused the preexisting backup.
Owner must verify restoration before any further launch.

## Engineering consequences and remaining unknowns

**Confirmed:** the observed 2006 engine can negotiate the SDK2004-compatible
`VClient011` and `ServerGameDLL003` factory names, call the client's
initial virtual `Init` slot using the existing independent ABI implementation,
and supply working factories for `VEngineClient012` and `VEngineModel012`.
This is evidence only for those narrow calls and the current safe fail-closed
probe, **not** evidence that all arguments, subsequent vtable slots, client
state, or server initialization are compatible.

**Potentially important but unproven:** `ServerGameEnts001` was requested
and rejected. The game proceeded into client Init, so an individual
rejection is not demonstrated to be a fatal blocker. Before advertising
that interface, reconstruct its original server-side object and virtual
contract from the separately SHA-authenticated original `server.dll`.
Do not return a dummy pointer to an incorrectly shaped vtable.

**Priority:**
1. Recover and verify original client DLL before another local run.
2. Study the original `VClient011` Init body and immediate follow-up
   lifecycle calls privately, preserving exact ABI and object layout.
3. Reconstruct `ServerGameEnts001` using owner-private original server
   method evidence, rather than treating a factory-string match as an
   implementation.
4. Introduce real client lifecycle behavior incrementally, with positive
   initialization only after its mandatory call surface is implemented.
5. Only then repeat the disposable-copy owner test. Never change the
   diagnostic `Init` return value to success simply to suppress the dialog.

The `Test-OwnerIndependentStartupVerdict.ps1` synthetic replay of this
observed sequence guards against reclassifying an unknown/possibly optional
module factory request as a fatal client Init blocker, and against inventing
server Init entry or a playable retail boot.
