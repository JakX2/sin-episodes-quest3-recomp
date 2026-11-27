# Phase 1620: Original recombinant Major/Minor failure-schedule overrides

Direct read-only inspection of the owner-supplied 2006 PE32 `server.dll` verified SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Original primary mutant vtable +0x438 points to Major `0x22377740` and Minor `0x2237d2a0`, both distinct from shared base `0x2204e6d0`. Original bytes stay private.

The Major override first writes the invalid-handle sentinel `0xffffffff` to actor `+0xc68`, then tail-calls inherited `0x2204e6d0` with its three arguments unchanged. The effect remains even if a delegated host implementation rejects the call.

The Minor override interprets the first argument as an input failure-schedule/decision ID, the second as its reason/context, and preserves the third argument for inherited fallback. If the ID is 85, or ID 23 with reason 100, the method checks local NPC condition 10 through inherited `0x220397c0`; when present it returns schedule 41. If that check rejects, or when ID is 23 with another reason, 72, or 73, it calls its local `0x2237c190` ground-clearance predicate; false returns schedule 86. Other inputs, and a true clearance predicate, delegate to inherited `0x2204e6d0`. The inherited base method returns its stored `actor+0x7bc` schedule, or 79 if zero; this generic behavior is explicitly delegated rather than reproduced.

Original Minor clearance predicate `0x2237c190` starts with grounded bit `actor+0xbc & 1`; otherwise false. On the grounded branch it refreshes dirty origin when necessary and performs a class-specific downward collision/trace query. Because trace and collision internals belong to the shared Source engine, the native export takes a host callback for this predicate. This phase recovers the calling decision only, not the trace engine.

Native `SiN_GetServerRecombinantFailScheduleApi` is an additional separate exported API, preserving all prior API layout/version contracts. The existing native NPC condition mapping resolves local condition 10 without guessing a bit position. The loaded Windows DLL regression exercises Major pre-delegate invalidation, deferred owner failure, Minor conditions and reason discrimination, probe positive/negative/error, parent fallback with all three arguments, stale input/wrong class/undersized actor and factory version rejection. No original executable bytes committed.

Authenticated code region evidence (VA / bytes / SHA-256):
- Major `22377740 / 15 / 1bb8bf9a94ec39b0f2fc61af78f59275cadf10ebf4d30e8ab4b8a11b4cb33ddb`
- Minor `2237d2a0 / 104 / 34d3a33fbf552bb81ba0c1b0b998911fca3a9da5f6715ede5c7f76736211667c`
- Delegated Minor clearance `2237c190 / 196 / 29205b3cc791f38206bc936e96cd87cbf7a44375440cd656ed0b0320f7247ed9`

No generic Source physics, surface/material, prop, model loading or rendering implementation. Tests are native synthetic Windows DLL tests, not in-engine gameplay. CI pending.
