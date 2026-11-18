# Phase 327: first shared post-factory sound-emitter callback

## Original evidence and scope

Both original SHA-authenticated Win32 game DLLs call the same
VSoundEmitter001 Connect interface immediately after their verified
initial factory acquisitions. Client Init RVA 0xFE0E0 issues 21
requests and invokes Connect at RVA 0xFE320. Server DLLInit RVA
0x1E7640 issues 16 requests (15 engine and one filesystem) and
calls Connect at RVA 0x1E77E5. Each passes its original engine/app
factory and tests the boolean result before proceeding.

The owner's SDK 2004 public/SoundEmitterSystem/isoundemittersystembase.h
declares bool Connect(CreateInterfaceFn) as the first virtual operation
without a virtual destructor. The SDK gameinterface.cpp and
cdll_client_int.cpp confirm the order and factory argument.

## Independent implementation and tests

sound_emitter_connect_core.h provides the typed x86 call boundary
and explicit missing-interface/factory/callback and rejected/connected
results. The standalone native regression validates both caller
factory identities, service callback dispatch, rejection, slot zero
and native stack cleanup. No engine Init/Shutdown is called.

Both original startup prefixes now preserve the sound pointer locally.
Two separately compiled synthetic-only DLL targets use the same
production source prefix, then actually invoke the shared Connect
boundary. Both verify every missing dependency prevents Connect,
the exact engine/app factory reaches Connect only after the last
acquisition, rejection stops startup, and success still returns
failure instead of pretending the remaining startup is complete.

The fixture-only targets are built outside the distributable bin path.
The production client.dll and server.dll do NOT call Connect; this is
enforced at compilation, not an environment-variable override.
Owner retail testing continues with the guarded Phase 325 factory
negotiation build until actual server DLLInit entry is observed.

Actual engine sound-state ownership, globals, math setup, cvars,
events, game-system initialization and shutdown still need implementation.
These are independent synthetic boundary tests, not a playable build.
No commercial DLL, original code or raw disassembly is included.
