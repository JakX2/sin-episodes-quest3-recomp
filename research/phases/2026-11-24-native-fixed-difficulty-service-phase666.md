# Phase666: fixed difficulty in the actual independent server DLL

Phase665 passed its full exact-head matrix (148 Win32 / 477 Linux).
This phase links the existing Phase663 fixed policy and Phase665 challenge
handlers into the actual non-forwarding independent Win32 server.dll target,
rather than just compiling another static reference test.

A **new, port-specific** C ABI export SiN_GetFixedDifficultyApi negotiates
version 1 and the exact API table size. It is not an original engine
CreateInterface version and does not replace any original vtable slot.
All state lives in the DLL, with an explicit begin/end campaign lifetime.
The initial challenge value is captured when the campaign begins. Skill
changes, 14 metric queries, original damage rules and NPC delay math,
trigger observations, normalized challenge snapshots and output descriptors
all consume the same owned profile. The output descriptor does not fire a
real Source map event; the future entity host must dispatch it.

The service never reads or writes a retail PE32 address. It also does not
register engine ConVars, invoke original Source callbacks, or claim that
the retail engine has called its new API. DLLInit remains fail-closed.
The ABI assumes synchronous game-thread use and caller-owned valid output
pointers. No STL allocations or C++ objects cross the module boundary.

The new native loader loads the real built server.dll, obtains the export
with GetProcAddress, checks original CreateInterface is still present,
and tests campaign/skill lifecycle, all 14 baseline metrics, original
damage bypass and delay consumers, EndOfGame low-byte rule, normalized
challenge input/output descriptors, invalid arguments, and campaign
reset. Linux separately builds the same service into a shared library
and calls it via dlopen under Clang ASan/UBSan.

This is an executable module integration milestone, not a playable
replacement DLL. The next step is to route actual game-owned console,
map entity and weapon/NPC callbacks into this service, rather than
adding further independent metric representations.
