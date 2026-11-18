# Phase 326: original server tick interval and complete initial client prefix

## Owner-original evidence

Direct local PE32 inspection of the owner-authenticated Windows files:

- server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client.dll SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

ServerGameDLL003 virtual slot 9, RVA 0x1E7A90, is the actual
GetTickInterval method. Unlike the SDK 2004 fixed TICK_INTERVAL return,
SiN's original method imports CommandLine from vstdlib.dll (IAT RVA
0x3D2180). It invokes the existing command-line object's CheckParm
(slot 3, null optional value output) for -tickrate. Only if present
does it obtain the singleton again and invoke the integer ParmValue
(slot 7, default 0). It returns 1.0 / rate only for a value greater
than 10; absent, invalid, negative, zero or <=10 falls back to 0.015
seconds. The original threshold is 10.0, the numerator is 1.0, and
the embedded default is the binary32 representation of 0.015.
The SDK's public/vstdlib/ICommandLine.h corroborates both slot
signatures. No raw executable code or original DLL is committed.

VClient011 Init at RVA 0xFE0E0: all 21 initial required app-system
factory version requests are now authenticated (previously 5). The
bounded 0xFE109..0xFE31B call region contains 21 ordered interface
names and every result's null branch reaches RVA 0xFE59E. The exact
list is in Inspect-OwnerOriginalClientPE.py and the native test.
At RVA 0xFE31B the original continues by invoking virtual slot zero
on the final VSoundEmitter001 pointer, passing the app-system factory
and checking its bool result. Server DLLInit similarly calls
VSoundEmitter001::Connect after its 16 initial lookups. The owner's
SDK SoundEmitterSystem/isoundemittersystembase.h confirms the signature.

## Changes

The independent server DLL now implements the real tick callback in
its original slot 9 instead of an index-only guard. It resolves
CommandLine only from an already-loaded vstdlib.dll, with explicit
terminal diagnostic refusal if unavailable. It does not load another
DLL or forward any method to the original game DLL. A synthetic
vstdlib fixture lives in fixture-only, never the bin artifact path.
The native test invokes slot 9 through the DLL's returned vtable,
checks nine rate cases and the exact engine callback sequence, and
also tests the absent-provider guard in a child process.

The independent client diagnostic now requests all 21 initial
interfaces in original order and refuses at the first null result.
The native test checks every boundary. The client still deliberately
returns zero even if all dependencies are available. Owner-local
client metadata inspection authenticates the exact SHA, registration,
singleton, Init address and common failure destination; its synthetic
test checks branch validation and hash rejection.

## Limits

Both initial dependency lists are only prefixes of the full startup.
Sound-emitter Connect and later subsystem initialization have not yet
been invoked by the independent diagnostics. The original-client
Phase 325 guarded server-only experiment is still the next real retail
boundary test. This branch does not establish successful startup, game
entity allocation, sound initialization or gameplay.
