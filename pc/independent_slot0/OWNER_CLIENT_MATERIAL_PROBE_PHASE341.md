# Phase 341: original client material configuration acquisition

## Retail observation

The Phase 339 owner Client-only test retained the original server,
accepted VClient011, entered Init, acquired all 21 initial
dependencies and successfully executed the engine sound-emitter
Connect. Terminal exit was 0x53494E4B, and both original DLLs were
verified after restoration. The actual 47-event trace is replayed
in the classifier regression. This is not complete client Init.

## Original evidence

The owner-private client.dll SHA-256 was verified locally:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
Direct inspection of the original code after the sound success
branch confirms:

| RVA | Operation |
| --- | --- |
| 0xFE32A | vstdlib CommandLine import (IAT RVA 0x276250) |
| 0xFE339 | CommandLine FindParm slot 10, "-textmode" |
| 0xFE340 | Set textmode byte when result nonzero |
| 0xFE34E | app factory for VMaterialSystemStub001, optional |
| 0xFE367 | VMaterialSystem076 slot 8, version MaterialSystemHardwareConfig012, null status |
| 0xFE371 | Branch to initial failure RVA 0xFE59E if null |
| 0xFE377 | Next operation, screen-size setup (not executed) |

The owner SDK 2004 ICommandLine and IMaterialSystem headers
corroborate the signatures. Only these verified slots are used,
not a guessed complete material class. The metadata inspector
authenticates the call offsets, names, and required-null branch.

## Independent diagnostic

client_material_startup_core.h implements the ordered callbacks
on opaque engine objects and owned state. The client captures
VMaterialSystem076 from the original initial sequence. The probe
resolves CommandLine only from already loaded vstdlib.dll, then
checks textmode, requests the optional stub and calls hardware
configuration. It preserves the distinction between missing
optional stub and missing mandatory config. It terminates after
the acquired/refused result without executing screen-size setup.
EnableClientMaterialProbe requires Client-only mode and the
existing EnableClientSoundProbe gate; the latter remains intact.
Exit codes 0x53494E4F acquired, 0x53494E50 refused.

Native tests use the actual distributed client DLL and a
fixture-only commandline provider, owned material objects, and
both stub outcomes. Tests check slot 10 and slot 8, correct
arguments, order, textmode present/absent, missing hardware,
missing commandline provider, and refusal after sound failure.
The fixture never enters the retail artifact. Evidence
classification requires ordered role-specific material events.
The original server is retained in owner testing. No playable
client, rendering, or full startup claim. No original binary
or code bodies committed.
