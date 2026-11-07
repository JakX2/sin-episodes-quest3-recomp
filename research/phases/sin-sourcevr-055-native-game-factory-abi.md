# Native game factory ABI: original SiN versus SourceVR 0.5.5

**7 November 2026. Direct original-PE and Android ARM64 ELF inspection.**

The owner's original SiN Episodes 2006 bin.zip and separate three-volume
SourceVR 0.5.5 archive were inspected privately. The source archive was
rejoined and seven native profile manifests were followed to their exact
game-module blobs. Every original and native blob was independently verified
against its full SHA256 fingerprint. **None is committed to GitHub.**

## Main result: both CreateInterface game-module contracts differ

| Role | Original SiN factory string | Native SourceVR 0.5.5 registered factory | Result |
| --- | --- | --- | --- |
| Game server | ServerGameDLL003 | **ServerGameDLL010** | Exact-name mismatch |
| Game client | VClient011 | **VClient017** | Exact-name mismatch |

The original server.dll SHA256 is
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
The original client.dll SHA256 is
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Only seven native game profiles are present, with **five distinct**
Android libserver.so builds and **six distinct** Android libclient.so builds:

| Profile | Server SHA256 prefix | Client SHA256 prefix |
| --- | --- | --- |
| HL2 | fdc8bb8777e0 | 0a8d1082e86d |
| Episode 1 | a81b04bdb016 | e4384399bd36 |
| Episode 2 | a81b04bdb016 | 9ad8e9eff680 |
| Lost Coast | fdc8bb8777e0 | 0a8d1082e86d |
| Portal | 6c8b6df1db32 | f14083ceaeb4 |
| Portal 2 | ecb111cb112f | 0f555f09a2ab |
| Entropy: Zero | 16e461d91ab5 | 720602b9643c |

No SiN game-specific native manifest exists in the examined release
archive. Presence of familiar NPC entity strings in other Source games
does not establish compatibility with SiN's original NPC layouts.

## Real AArch64 registration call proofs

These are **actual** native registration paths, not mere string
searches. Addresses are ELF virtual addresses within each ELF image.

| Module SHA prefix | Exported CreateInterface | Name VA | Registration ADRP x2 | Registrar |
| --- | --- | --- | --- | --- |
| HL2 server fdc8bb8777e0 | 0x94AF80 | 0xA4291 | 0x532A68 | 0x94AEE0 |
| EP1/EP2 server a81b04bdb016 | 0x9CE060 | 0xAE196 | 0x551F48 | 0x9CDFC0 |
| Portal server 6c8b6df1db32 | 0x9226A0 | 0xA1034 | 0x44C6E8 | 0x922600 |
| Portal2 server ecb111cb112f | 0xA50600 | 0xB5904 | 0x4935C8 | 0xA50560 |
| Entropy: Zero server 16e461d91ab5 | 0x9DCEA0 | 0xAF74D | 0x556AA8 | 0x9DCE00 |
| HL2 client 0a8d1082e86d | 0x722400 | 0x49489 | 0x370848 | 0x722360 |
| EP1 client e4384399bd36 | 0x72EEE0 | 0x49B0A | 0x3745C8 | 0x72EE40 |
| EP2 client 9ad8e9eff680 | 0x72EEE0 | 0x49AF9 | 0x3745C8 | 0x72EE40 |
| Portal client f14083ceaeb4 | 0x6FA780 | 0x480D6 | 0x2D1AA4 | 0x6FA6E0 |
| Portal2 client 0f555f09a2ab | 0x770180 | 0x4E04B | 0x2EC6A4 | 0x7700E0 |
| Entropy: Zero client 720602b9643c | 0x731A60 | 0x49DE5 | 0x3755A8 | 0x7319C0 |

For HL2 server, original AArch64 code at 0x532A54 initializes
the writable registration node in x0; 0x532A60 loads the singleton
factory callback 0x52CC10 into x1; 0x532A68 loads the
ServerGameDLL010 string in x2; and 0x532A70 calls registrar
0x94AEE0. The registrar builds a 24-byte linked registry node
(callback/name/next), and exported CreateInterface at 0x94AF80
performs case-sensitive name comparison through the *same* head.

The auditor repeats those independent instruction and singleton
callback checks across **all eleven** native ELF64 game modules,
including direct AArch64 BL call resolution and exported dynamic
symbol lookup. All ELF text/register witnesses are conditional
on the exact verified native SHA256 fingerprints.

These sites establish static native registration paths. Runtime
constructor timing, complete C++ interface method ABI, and Android
game-module loader acceptance remain separate unverified questions.

## Reproducible source-only research

- research/tools/audit_sourcevr_native_game_factories.py: standard-library
  SHA256-pinned private-ZIP/original-PE/ELF section, symbol and ARM64
  instruction auditor; JSON metadata only.
- research/tests/test_audit_sourcevr_native_game_factories.py:
  13 original-asset-free synthetic mutation and positive regressions.
- include/sin_compat/sourcevr_game_factory_preflight.hpp:
  compile-time hard failure of both direct interface compatibility gates.
- research/tests/sin_sourcevr_game_factory_preflight_reference.cpp:
  independent strict C++17 regression; same-name aliases alone cannot
  establish equivalent method ABI.
- CMake and original decomp GitHub Actions integration.

Run the private audit with the original user-owned bin.zip and a
privately joined SourceVR archive:

    zip -s 0 "source vr.zip" --out sourcevr-joined.zip
    python3 research/tools/audit_sourcevr_native_game_factories.py --sin-bin-zip /private/bin.zip --sourcevr-zip /private/sourcevr-joined.zip

The .z01 and .z02 split-volume names must be restored before joining.
The original binary scan was completed locally using the owner's data.
CI runs synthetic tests **without any original DLLs or Android APK**.

## Actual integration blocker

A name alias or DLL rename would not bridge the original x86
ServerGameDLL003/VClient011 classes and SourceVR's ARM64
ServerGameDLL010/VClient017 C++ method layouts or lifecycle.

The next needed evidence is the **actual native engine/launcher
game-module loading contract** for these profiles: module selection,
dlopen paths, game factory resolution and version handling.
Only then can a legally usable ARM64 game-module source
or a complete, independently verified adapter be attempted.

**Not proven:** game-module source availability, custom SiN profile
loading, real NPC engine bindings, campaign map execution, Quest 3
on-device gameplay, or a playable APK. No proprietary code or asset
was redistributed.
