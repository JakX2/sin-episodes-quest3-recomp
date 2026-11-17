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

## Owner-original VClient011 first-method metadata (Phase 310)

A bounded local inspection of the **original SHA-authenticated** x86
`client.dll` (not the independent diagnostic) verified a Source-style
`CreateInterface` registration for `VClient011`, whose factory returns
the original client interface object. Its first virtual method is at
**RVA `0x0FE0E0`** (image base `0x24000000`). These are address/identity
metadata for private targeted analysis, not copied original code.

The beginning of that original client Init performs a run of ordered
factory requests, each checked before proceeding. The **first five**
interface names are:

| Order | Original client factory query | Independent test status |
| --- | --- | --- |
| 1 | `VEngineClient012` | Observed available in owner retail run |
| 2 | `VEngineModel012` | Observed available in owner retail run |
| 3 | `VEngineEffects001` | Found in original Init, not yet owner-run tested |
| 4 | `VEngineCvar002` | Found in original Init, not yet owner-run tested |
| 5 | `EngineTraceClient003` | Found in original Init, not yet owner-run tested |

The Phase 310 non-forwarding client preflight now **only queries these first
five in the original order**, stops at the first null, and always returns
failure. It never calls a method through returned third-party interface
pointers, cannot safely advertise successful client initialization, and
does not alter the unreviewed server DLLInit candidate sequence. Native
synthetic x86 tests verify all five names and early-stop behavior at
each missing interface. Do not re-run the owner game until originals are
restored and the updated launcher is validated.

## Second actual owner Windows experiment: all five found (Phase 311)

A subsequent owner run of the Phase 310 diagnostic against the disposable
retail installation confirmed the following **in the real engine**, in order:

1. `ServerGameDLL003` accepted, `ServerGameEnts001` rejected (no server
   `DLLInit` entry observed).
2. `VClient011` accepted, client `Init` entered.
3. `VEngineClient012` returned a non-null interface.
4. `VEngineModel012` returned a non-null interface.
5. `VEngineEffects001` returned a non-null interface.
6. `VEngineCvar002` returned a non-null interface.
7. `EngineTraceClient003` returned a non-null interface.
8. `sdk2004-preflight-only` emitted, independent Init intentionally
   returned zero, still **not a playable boot**.

The owner launcher recorded one matching session trace, actual PID 9724,
372 best-effort module samples, a tracked process exit of `0x00000000`,
and `DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF`. Sampling never showed either
gameplay DLL despite the direct in-process diagnostic logs. Do not treat
these sampling misses as no-load evidence.

**Restoration failure recurred:** server.dll was restored and hash-verified;
client.dll remained locked with `UnauthorizedAccessException`.
The newer Phase 308 launcher correctly preserved diagnostics, reported
`RESTORE_PENDING` for the backed-up original client, and flagged
`INCOMPLETE_MANUAL_RECOVERY_REQUIRED`. Owner must close any remaining
game process and restore client.dll from its SHA-pinned saved original
before another run. No in-process handle owner is proven by these logs.

### Research impact

The first five original client factory names and their presence on the
actual SiN engine are now corroborated by two distinct evidence types:
authenticated original-binary Init order and opt-in owner retail runtime
diagnostics. But the virtual methods on these interfaces, later client
Init work, the original server `ServerGameEnts001` implementation, and
the server `DLLInit` ABI are **not yet reconstructed**. Further work
should pursue the real implementation, not simply request more
already-present interfaces. The classifier regression now also replays
all 15 second-run independent diagnostic lines.
