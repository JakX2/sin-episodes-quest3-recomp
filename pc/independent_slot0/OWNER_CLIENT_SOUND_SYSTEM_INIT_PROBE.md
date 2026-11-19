# Original Windows sound game-system Init, gated after surface query

The owner-original 2006 client.dll SHA 6535f13e... demonstrates
CSoundEmitterSystem::Init at 0x241BC950 is 11 bytes and tail jumps into
slot 1 of sound-emitter engine interface held at global 0x2433F6E4.
Its RTTI, original object and exact slot method VA are verified by
research/tools/audit_sin_client_static_gamesystem_rtti.py.

The independent client captures its already-factory-acquired
VSoundEmitter001 pointer and (ONLY with the additional environment flag
SIN_INDEPENDENT_CLIENT_SOUND_SYSTEM_INIT_PROBE=1, plus all earlier
terminal gates) invokes the authenticated slot 1 via module and
VirtualQuery checks. Success returns diagnostic 0x53494E63; a false
return or refusal yields 0x53494E64. This runs after successful
MatSystemSurface003 query (owner-confirmed earlier checkpoint
0x53494E61) and therefore does not change previous default behavior.

This is a *single genuine game-system engine ABI operation*, not an
original gamesystem-vector pass, normal VClient011::Init return,
completed sound playback, level load or gameplay. No original code or
commercial binaries committed.
