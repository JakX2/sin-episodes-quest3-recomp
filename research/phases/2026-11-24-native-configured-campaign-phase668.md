# Phase 668: configured campaign start in the independent server DLL

Original evidence: Phase88, `sin-original-client-server-shutdown-and-caches.md`.
Original ServerGameDLL003 slot 1, VA 0x221E7B00, calls reset helper
0x221F4980, queues literal `exec game.cfg\n` via VEngineServer021
virtual +0x90, then executes via +0x94. Phase314 documents acquisition
of VEngineServer021 and VEngineCvar002.

The new explicit Win32 port export SiN_StartConfiguredCampaign calls
the two original-guided command-buffer slots 36 and 37, then samples
skill via Phase667 and starts the same DLL-owned campaign. It does not
implement the preceding original reset helper or claim full GameInit.
The caller must supply live correct interfaces at a ready game-thread
boundary. All three vtable slots are checked before any effect.
It runs no arbitrary caller-provided commands, retains no interface,
and refuses reentrant mutations and duplicate active campaign starts.

Results distinguish preflight rejection, uncertain queue/execute
effects if a C++ callback throws, command-buffer completion with no
valid skill, and campaign creation. External command effects cannot
be rolled back: a caller must not blindly retry a partial outcome.
A returned success means both command callbacks returned, not proof
that game.cfg existed or contained any particular setting.

The actual loaded-module test supplies native synthetic Win32
interfaces. It proves the post-execute value (not the pre-config value)
selects the campaign; validates exact command and slot order, invalid
interfaces, callback exceptions, missing/invalid skill, reentrancy,
duplicate start, live synchronization, damage and cleanup. No retail
engine calls, real map loading, or successful DLLInit are claimed.
Phase666 portable ABI and Linux consumer unchanged.
