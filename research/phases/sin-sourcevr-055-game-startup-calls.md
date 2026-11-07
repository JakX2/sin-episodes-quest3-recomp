# SourceVR 0.5.5: real ARM64 client/server startup callsites

7 November 2026. Original owner-provided SourceVR archive, read-only.
This extends PR #148, which established the concrete game-module
factory singleton vtables. This phase follows three **engine-side
virtual calls** and the real native game methods at their destinations.
No commercial binaries or assets are committed.

## Verified native engine dispatches

| Observed native operation | Vtable index | ARM64 pointer byte offset | Call and return evidence | Public Source SDK candidate |
| --- | ---: | ---: | --- | --- |
| Client initialization | 0 | +0x00 | x0 object, x1 pointer, x2 copy of x1, x3 pointer, then BLR and CBNZ w0 | IBaseClientDLL::Init |
| Client cleanup | 2 | +0x10 | x0 object, BLR; return ignored | IBaseClientDLL::Shutdown |
| Server description | 12 | +0x60 | x0 object; returned x0 moved into x2 and passed as a string | IServerGameDLL::GetGameDescription |

Real engine ELF virtual addresses by native profile:

| Native game profile | Client startup slot0 | Client cleanup slot2 | Server description slot12 |
| --- | --- | --- | --- |
| HL2, Episode One, Episode Two, Lost Coast, Entropy: Zero | 0x328228 | 0x328294 | 0x2F3B1C |
| Portal | 0x327A08 | 0x327A74 | 0x2F32FC |
| Portal 2 | 0x32B848 | 0x32B8B4 | 0x2F713C |

For the HL2 ELF, the actual client call sequence at
0x328228 to 0x32823C reads [x0] as the object's vtable,
loads the 0th method, sets pointer arguments in x1, x2,
x3, invokes BLR x8, and branches on the resulting w0.
The client cleanup code at 0x328294 to 0x32829C explicitly
reads the +0x10 virtual slot and invokes it. The server
description code at 0x2F3B1C to 0x2F3B3C reads the +0x60
slot, invokes it, and passes its x0 result to the engine's
native string logger. The actual format string reads:
server%s loaded for "%s" followed by newline; .so is the
first string argument. The second string argument is the
server object's description return.

The same AArch64 call/return and log-format witnesses were
confirmed in all seven SHA-pinned libengine.so builds, including
the shifted Portal and Portal 2 code addresses.

## Actual ARM64 game-callee evidence

Each of the 11 distinct SHA-pinned client/server native game ELF
blobs was verified against the selected manifest and previous
CreateInterface factory, singleton and entire virtual table
evidence. The directly invoked game methods are genuine executable
functions within those files.

All six client implementations have identical method0 initial
register saving for x1, x2 and x3. Their method2 implementations
have the expected executable cleanup prologue. Across all five
server game implementations, method0 similarly preserves
four pointer-sized inputs x1, x2, x3 and x4. In the HL2
native server its address is 0x52CC20. This four-register
shape is **consistent** with the public Source SDK's DLLInit,
but the native engine callsite for server method0 is not yet
independently proven. We do not claim to know its full typed ABI.

All five server implementations have an executable method12
target. HL2 native server method12 is at 0x52E360,
branching into game-specific description code. Engine-side
string consumption independently supports its description role.

Public Source SDK 2013 references are *candidate names and
signature guides*, not proof of SourceVR 0.5.5 ABI identity:
https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/cdll_int.h
https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/eiface.h

## Reproducible code and testing

research/tools/audit_sourcevr_game_startup_dispatch.py:
read-only SHA256-pinned owner-private archive scanner, verifies
real native engine callsites and real native game-method prologues.

research/tests/test_audit_sourcevr_game_startup_dispatch.py:
12 original-asset-free synthetic tests including wrong vtable
index, missing w0 success check, altered pointer registers,
server log format and native method-prologue mutations.

include/sin_compat/sourcevr_game_startup_abi.hpp and
research/tests/sin_sourcevr_game_startup_abi_reference.cpp:
typed C++17 metadata-only native register-level facts
and deliberate fail-closed virtual dispatch gate. No
unverified SourceVR C++ virtual call is made.

CMake/CTest and original decomp CI run without any proprietary
APK, DLL, ELF, BSP, model or assets.

Private owner-archive reproduction:

    python3 research/tools/audit_sourcevr_game_startup_dispatch.py --sourcevr-zip /private/sourcevr-joined.zip

Synthetic tests:

    python3 -m unittest discover -s research/tests -p test_audit_sourcevr_game_startup_dispatch.py -v

## Next genuine integration gate

The original Windows 2006 game vtables are 31 server / 48 client
method slots; SourceVR ARM64 game modules expose 47 / 78.
ServerGameDLL003 vs ServerGameDLL010 and VClient011 vs VClient017
are separate major interface contracts. Do not alias original
game objects, pad unknown virtual methods or reinterpret_cast
their vtables merely to satisfy a game module factory.

Next verify the native engine's server initialization method0
callsite and full argument types, then implement an independently
validated, safe engine-owned game startup adapter. Real entity
registration, campaign map loading, Meta Quest 3 native gameplay,
headset rendering and any playable APK remain unproven.
