# Phase 337: terminal retail registration of one original console variable

The owner-private SHA-authenticated original server was examined
at RVA 0x3C2FA0. The direct call at 0x3C2FB1 targets the original
three-argument ConVar constructor at 0x25BD20. The declaration name
is dd_log_events, default "0", flags 0, object RVA 0x5923E8.
The constructor supplies null help/callback and no bounds.
The original SHA-pinned client file was also searched for the exact
NUL-terminated name, which was not found. This does not guarantee
that no engine or plugin command with the same name exists.

Phase 332 owner evidence established successful sound connection,
scalar initialization and gameevents load returning 47. Phase 336
reconstructed the full original registration accessor in a synthetic
engine fixture. This separate probe deliberately selects ONE original
nonreplicated variable, not the full original constructor/registration
order. It uses verified VEngineCvar002 slots 0, 1, 3 and 4 and the
native 68-byte variable object. No original C++ class is cast to a
synthetic implementation; raw native callbacks use the observed ABI.

The probe checks the existing variable lookup AND bounded registered
command list before mutation. On conflict it refuses to register
anything. The independently owned variable has no gameplay callback,
bounds or external allocator ownership. It sets the original game-DLL
flag, invokes RegisterConCommandBase, queries the override, and
checks pointer identity, self-parent, retained string storage, flags,
default value and actual engine lookup. Unexpected root relinking or
override is refused rather than writing an unfamiliar root. A
registered object is kept alive for the entire disposable process.
The routine never returns to engine initialization, unloads the DLL
or destroys an engine-retained object. TerminateProcess is explicit.

The owner launcher requires all four opt-ins in Server-only mode:
EnableServerFactoryProbe, EnableServerSoundProbe,
EnableServerEventProbe and EnableServerCvarProbe. The final gate
requires a positive event-load return. Exit 0x53494E49 indicates the
single variable was verified by engine lookup; 0x53494E4A indicates
a recorded refusal. Prior sound/event-only diagnostic paths are
unchanged. The classifier checks the ordered same-process evidence
and does not report successful DLLInit. This has not been run in the
retail engine; wait for passing Windows CI and use a confirmed
disposable copy only.

The distributed server DLL synthetic child regression exercises
successful verification, existing variable and command name conflicts,
lookup mismatch, override, unexpected relinking, cyclic list, and
refusal to register following a zero event result. All existing
sound/event paths also remain tested. No original binary or code
committed. Full 625-declaration registration and gameplay remain
unfinished.
