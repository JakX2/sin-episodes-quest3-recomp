# Phase 344: bounded client Gaussian stream attachment

Phase 343 passed all four CI workflows, 38/38 native tests. The actual
retail screen query is still awaiting owner execution. Phase 344 extends
the same initial path, not full client initialization.

The owner-private client SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
was checked again. Its static initializer and startup call use the vstdlib
imports ??0CGaussianRandomStream@@QAE@PAVIUniformRandomStream@@@Z
(IAT RVA 0x276240) and
?AttachToStream@CGaussianRandomStream@@QAEXPAVIUniformRandomStream@@@Z
(IAT RVA 0x276244). The second call at 0xFE388 receives the acquired
VEngineRandom001 stream. The owner SDK2004 random.h defines the
nonvirtual pointer/bool/float 12-byte x86 layout and the same argument
types. No original client memory or original implementation is used.

This terminal diagnostic captures VEngineRandom001, resolves both
decorated exports only from already loaded vstdlib.dll, constructs
owned storage with null and attaches the actual engine stream AFTER
screen query. Constructor is invoked at this diagnostic boundary,
not at DLL static initialization time. Its returned receiver, attached
pointer, cleared cached-value flag and adjacent guard words are checked.
Guard words detect certain unexpected writes but are not memory isolation
or proof of complete provider layout compatibility. No random samples,
seed changes or engine global-stream overrides are performed. The
storage is not destroyed while the separate disposable process runs.
No subsequent game initialization occurs.

The new EnableClientGaussianProbe switch requires all three preceding
Client-only switches. Exit 0x53494E53 means attachment verified;
0x53494E54 means diagnostic refusal. Old paths remain unchanged.
Native tests execute the actual DLL with fixture-only decorated
vstdlib exports, successful and refused attachment and sound/material
failure before attachment. The classifier checks the ordered trace.
No claim of playable graphics, random algorithm parity or full Init.
No original binary or commercial implementation body committed.
