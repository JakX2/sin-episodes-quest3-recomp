# SourceVR native module loading: ARM64 R1 checkpoint

**Basis:** The owner's private SourceVR HL2 and Entropy: Zero ARM64 libengine.so files. Offline disassembly, no APK modifications or headset execution. No proprietary binaries committed.

## Confirmed observations

| Finding | ELF-relative address | Evidence |
|---|---|---|
| Client module lookup | 0x327f5c..0x327fdc | `client.dll» and `GAMEBIN», filesystem module-load call |
| Client factory | 0x327ff0..0x328000 | requests `VClient017» |
| Server module lookup | 0x2f38c0..0x2f3930 | `server.so» and `GAMEBIN», filesystem module-load call |
| Server factory | 0x2f3958..0x2f39d4 | formats `ServerGameDLL%03d», starts at 10, decrements down to 4 |
| Shared permission stub | 0x287fe0..0x287fe4 | literally returns integer 1, then returns |
| Client permission call sites | 0x327f74 and 0x327fa0 | both call 0x287fe0 |
| Server permission call sites | 0x2f38d8 and 0x2f3904 | both call 0x287fe0 |
| Native loader backend | around 0x53f0c0..0x53f594 | `dlopen», `APP_LIB_PATH», filesystem/path fallbacks and preloaded SONAME diagnostic |

These are *virtual addresses relative to the ELF*, not live Android ASLR addresses. An always-allowing local permission stub does **not** prove arbitrary custom-module loading: profile staging, filesystem, Android linker and ABI are separate gates.

## HL2 versus Entropy: Zero

The two engines have **different SHA-256 hashes and .text sections**. The entire client-load instruction window from 0x327f50 to 0x328060 and the permission-check stub are byte-identical. The server-load window differs at string-address instructions but has the same observed version negotiation sequence. The individual .load descriptors pre-stage profile-specific `libclient.so» and `libserver.so» before `liblauncher.so». Native stage selection may dominate the GAMEBIN fallback.

## SDK compatibility gate

Valve Source SDK 2013 `src/public/cdll_int.h» declares `VClient017». Current `src/public/eiface.h» declares default `ServerGameDLL012», although it retains a `ServerGameDLL010» version identifier. This SourceVR engine **only attempts ServerGameDLL010 through ServerGameDLL004**. An unmodified latest SDK module exposing only 012 will not be found. Renaming the interface version is unsafe until its vtable and lifecycle are matched.

Original 2006 SiN Windows game DLLs advertise `VClient011» and `ServerGameDLL003» and are x86, not Android ARM64.

[Valve client SDK header](https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/cdll_int.h) and [server SDK header](https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/eiface.h).

## Reproduction

Use the independently authored Python 3 stdlib tool, *locally on the user's copies*:

~~~sh
python tools/sourcevr_module_gate_audit.py /private/hl2/libengine.so /private/entropyzero/libengine.so --output module_gate_xrefs.json
python -m unittest discover -s tools/tests -v
~~~

The scanner identifies ELF strings, ARM64 ADRP/ADD xrefs and nearby BL calls to simple return-1 stubs. It is a narrow analyzer, not a whole-program proof: confirm findings with a proper ARM64 disassembler.

## Next hard gate

**Continue** with an isolated test of the actual Android module search outcome: which staged/preloaded SONAME or GAMEBIN module wins. Only then construct a Source `CreateInterface» game-module prototype using verified version-010/017 interface headers. The existing C weapon bridge is *not* a substitute for client/server game modules.

R1 remains **unimplemented**. No SiN map or game module has run in SourceVR yet.
