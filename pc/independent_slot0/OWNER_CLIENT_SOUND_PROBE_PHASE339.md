# Phase 339: original client initial acquisition and terminal sound probe

Owner-private original client.dll SHA-256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Direct read-only PE32 inspection confirms the 48 contiguous x86
VClient011 vtable entries at RVA 0x296C6C. The original slot 0 Init
at 0xFE0E0 issues the 21 previously recovered initial app-system
factory requests. After acquiring VSoundEmitter001, it calls its
slot zero with the original app-system factory at RVA 0xFE320 and
tests AL, branching to failure on false. Subsequent original
operations include -textmode, the optional VMaterialSystemStub001
factory request, required MaterialSystemHardwareConfig012 from
material-system slot 8, and later MatSystemSurface003 acquisition.
They are NOT executed by this bounded probe.

The distributed independent client now has terminal guards covering
slots 2..47. The two earlier typed methods remain unchanged except
for an optional Connect path after all 21 acquisitions. Guards
log their index, terminate the diagnostic process and do not assert
parameter types or implement any remaining gameplay method.

EnableClientSoundProbe requires Client-only owner launcher mode,
retaining the SHA-pinned original server in the disposable game copy.
The explicit environment gate also requires a valid trace session.
Both original files are authenticated and the selected file restored.
The default still returns initialization failure without invoking
sound. In opt-in mode the first real sound callback is called and
its boolean return logged before a deliberate process stop:
0x53494E4B for true, 0x53494E4C for false. A missing ABI input
uses the existing terminal unsupported-callback code. No complete
client initialization, rendering, or game boot is claimed.

Native independently owned child fixtures validate all 48 table
entries, all 46 unfinished guards, default-off and invalid-session
cases, refusal of the last required interface, and both Connect
return paths. A separate classifier records client-specific evidence
and rejects mixed-role synthetic traces. The existing server probe
remains unchanged. The original metadata inspector verifies the
entire client vtable in addition to the original acquisition prefix.

Phase 337 remains the next server retail experiment. Run this client
experiment only after its own Windows CI is green, with an explicit
Client-only disposable copy. No original binary or code body is
committed.
