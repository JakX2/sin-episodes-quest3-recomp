# Phase 96: original SiN factory requirements vs actual SourceVR 0.5.5 ARM64 registrations

## Original binary evidence and scope

The user's original 2006 Windows x86 SiN Episodes client.dll and server.dll were SHA256-verified against all prior Phase95 original-PE checks:

- client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

The user-supplied SourceVR 0.5.5 last-volume ZIP provides **two directly extractable native ARM64 ELF64 libraries**, privately inspected but never committed:

| SourceVR module | SHA256 | CreateInterface exported VA |
|---|---|---|
| libsoundemittersystem.so | 484e5bb2c1457ee6e62f39d47c83bc68b8cee491c2f1479886a8e0d93740c454 | 0x1A300 |
| libvgui2.so | 7b7b7ec9f8c3994f9bd72a6a9820dd0f1057e569e86f01a8cb40462d94e3fa12 | 0x40EA0 |

**All seven** native profile manifests (hl2, ep1, ep2, lostcoast, portal, portal2, entropyzero) explicitly select these two identical SHA-pinned blobs. There is no SiN native game profile in that archive. The large libclient.so, libserver.so and libengine.so blobs are not all readable without earlier multipart archive volumes. Do not confuse these two successful reads with a complete native runtime audit.

## Server: original VSoundEmitter001 vs native VSoundEmitter002

Original server.dll ServerGameDLL003 Init strictly requests **VSoundEmitter001** at original x86 VA 0x221E7799, stores it at 0x225721D0, fails on a missing result, then requires virtual slot +0 success at 0x221E77E6. Original CSceneCache/CModelSoundCache call the returned interface through byte offsets +0x0C (name-like input to token) and +0x18 (token to name-like result).

SourceVR's **actual native ARM64 libsoundemittersystem.so instead registers VSoundEmitter002**:

- Registered interface literal at ELF VA 0x2879.
- Startup initializer is in native .init_array at 0x18D50; original AArch64 instructions 0x18DA0 through 0x18DC0 pass node 0x48518, callback 0x16330 and name 0x2879 to registrar 0x1A260.
- Callback 0x16330 returns singleton address 0x483E8.
- Exported CreateInterface at 0x1A300 uses exact case-sensitive strcmp and a 24-byte linked registry (callback +0, name +8, next +16). Return code is 0 on found, 1 plus null on absent.
- 18 original AArch64 instruction anchors and full selected ELF SHA256 verify these facts.

**Finding:** a direct request for VSoundEmitter001 cannot match the observed VSoundEmitter002 native registration. There may be additional engine routing, but none is established. Do not alias names or claim C++ virtual methods are compatible without checking actual method ABI.

## Client: VGUI factory registration names match, but methods remain unverified

Original client.dll VGUI initialization at x86 0x241EDFB0 requires **VGUI_InputInternal001** and **VGUI_System010**, verified against original literal addresses 0x242AC50C and 0x242D32C0.

In the actual native ARM64 libvgui2.so both interface registrations are **proven by init-array instruction paths**, not just strings:

| Interface | Native name VA | Registration node | Callback |
|---|---|---|---|
| VGUI_InputInternal001 | 0x54D4 | 0x6F558 | 0x222F0 |
| VGUI_System010 | 0x654E | 0x71568 | 0x3D4D0 |

- VGUI_InputInternal001 is registered from .init_array function 0x269D0, via original ARM64 instructions 0x26A3C..0x26A5C.
- VGUI_System010 is registered from .init_array function 0x3DFB0, via 0x3E04C..0x3E06C.
- Both use registrar 0x40E00 and exported native CreateInterface 0x40EA0 with exact strcmp, 24-byte registry and return status 0/1. 22 AArch64 instruction anchors verify the paths.

**Finding:** the SiN client names are present in actual native registration. This establishes **factory name compatibility only**, not native object ABI, VGUI method layout, ownership or CLoadingDiscPanel functionality.

## Code and verification

- include/sin_compat/sourcevr_factory_preflight.hpp: C++17 strict gate that distinguishes unregistered, mismatched versions, matching names with unknown methods, and fully verified compatible interfaces. No automatic aliases. In the observed set, it deliberately allows **zero** unsafe native vtable calls.
- research/tools/audit_sourcevr_native_sin_factories.py: pure-stdlib ELF64 dynamic symbol/section parser and manifest auditor, exact ARM64 registration opcodes, SourceVR blob/full SHA, chained original SiN client/server SHA and VGUI/sound interface names, metadata JSON output only.
- research/tests/test_sourcevr_native_sin_factories.py: **12 synthetic original-free Python regression groups**, mutating all AArch64 opcode anchors, registered interface names, exports and .init_array entries; checking ELF architecture, SHA, malformed manifests and factory return codes.
- research/tests/sin_sourcevr_factory_preflight_reference.cpp: strict C++17 gate tests.
- CMakeLists.txt: adds new CTest regression target.

**Local:** real original PE and native selected ARM64 audits PASS; synthetic Python 12/12 PASS, isolated strict C++17 CTest 1/1 PASS. GitHub CI tracked separately after publishing. The private source archive and original Windows/Android binary bytes are not stored in GitHub.

## Next implementation step

Phase97 should inspect and prove SourceVR VSoundEmitter002 virtual method signatures and compare them with original 001 callsites; only then implement an explicit, documented sound-emitter adapter. Validate the two matching VGUI interfaces' methods in the real native runtime as well.

Not achieved: a compatible native SiN game-specific libserver.so/libclient.so, actual gameplay, campaign map loading, or a Quest 3 APK.
