# SourceVR 0.5.5 ARM64: native engine game-module loader contract

7 November 2026. Read-only investigation of the owner's privately supplied three-volume SourceVR 0.5.5 archive and original SiN Episodes PE32 DLL ZIP.

## Verified native behavior

There are seven separate SHA256-pinned Android AArch64 libengine.so builds, one selected by each profile manifest (HL2, Episode One, Episode Two, Lost Coast, Portal, Portal 2, Entropy: Zero). Their SHA256 and native game-module pairing are pinned in research/tools/audit_sourcevr_engine_module_loader.py.

Direct native ARM64 analysis establishes two distinct stages:

1. The engine requests module handles via an engine-side **virtual callback at vtable byte offset +0xC8**, passing the module name and the search group GAMEBIN. For the server the module name is **server.so**; for the client it is **client**. This call is indirect, so the concrete loader class and its implementation are NOT yet recovered.
2. The engine obtains CreateInterface via a factory helper that loads the exact literal CreateInterface and branches to the ELF-imported **dlsym@PLT** entry. The audit follows the actual R_AARCH64_JUMP_SLOT relocation for dlsym, rather than relying on coincidental string presence. The returned factory is invoked for each game-side interface request.

The server initializes its desired interface version to **10** and formats **ServerGameDLL%03d**. On failure it decrements until the original branch excludes values at or below **3**. Thus the statically proven retry range is **ServerGameDLL010 through ServerGameDLL004**. The client path requests **VClient017** without a similar version retry loop at the inspected site.

The original SiN 2006 game DLLs instead expose ServerGameDLL003 and VClient011. The native factory-name evidence is in the preceding PR #145. SourceVR's server fallback still does not reach 003, and the client demands 017. These differences cannot be fixed safely by renaming a DLL or aliasing an interface string without separately proving all called C++ methods and native object layouts.

## Verified native addresses

Addresses are ELF image VAs, not live process addresses.

| Native game profile | Full engine SHA256 prefix | Server format ADRP | Client version ADRP | Shared dlsym factory helper |
| --- | --- | --- | --- | --- |
| HL2 | 265c675d6f0d | 0x2F395C | 0x327FF0 | 0x53F5C0 |
| Episode One | 8891aced91ea | 0x2F395C | 0x327FF0 | 0x53F5C0 |
| Episode Two | f3acb5b5461c | 0x2F395C | 0x327FF0 | 0x53F5C0 |
| Lost Coast | 0ad9c1909443 | 0x2F395C | 0x327FF0 | 0x53F5C0 |
| Portal | fb0daa545f48 | 0x2F313C | 0x3277D0 | 0x53E1A0 |
| Portal 2 | e675c7dac606 | 0x2F6F7C | 0x32B610 | 0x5503C0 |
| Entropy: Zero | 81c6041998 | 0x2F395C | 0x327FF0 | 0x53F5C0 |

For example, HL2 libengine.so:

- VA 0x2F390C and 0x2F3914: ADRP/ADD pairs select server.so and GAMEBIN, respectively.
- VA 0x2F3920 to 0x2F3928: vtable lookup at +0xC8, then BLR x8.
- VA 0x2F3940: BL to factory-resolving helper at 0x53F5C0.
- VA 0x2F3958: MOV w8,#10. VA 0x2F395C: ServerGameDLL%03d format pointer.
- VA 0x2F3980: initial BLR of returned factory.
- VA 0x2F39A4: decrement. VA 0x2F39AC: CMP w8,#4. VA 0x2F39B0: B.LE to failure path. ServerGameDLL003 cannot be reached through this original loop.
- VA 0x327FA8 and 0x327FB4: client and GAMEBIN.
- VA 0x327FC4 to 0x327FCC: identical virtual +0xC8 loader slot, BLR x8.
- VA 0x327FE0: same factory-resolving helper. VA 0x327FF0: VClient017.
- VA 0x53F5C4..0x53F5CC: CreateInterface string and direct branch to actual dlsym PLT relocation at 0x671CA0.

The auditor independently validates the variant-specific instructions and dlsym relocation on all seven fully SHA-pinned native libengine.so files.

## Reproduction

Source code and tests:
- research/tools/audit_sourcevr_engine_module_loader.py: owner-private ZIP/manifest/ELF64/ARM64 read-only audit, outputs metadata JSON.
- research/tests/test_audit_sourcevr_engine_module_loader.py: sixteen fabricated instruction, relocation, version and callback tests without private source bytes.
- .github/workflows/sin-original-decomp-tests.yml: CI synthetic-only runner.

Example invocation, using locally accessible legally held archives:

    python3 research/tools/audit_sourcevr_engine_module_loader.py --sourcevr-zip /private/sourcevr-joined.zip --sin-bin-zip /private/bin.zip

The SourceVR archive is multipart. Restore the .z01 and .z02 filenames before joining the split ZIP privately with zip -s 0; no original DLL, Android APK, .so, map or game asset is checked into GitHub.

## Precise limitations

This establishes GAMEBIN module names, the indirect loader virtual slot, dlsym(CreateInterface), and the native interface request range. It does **not** prove the virtual loader callback's implementation, accepted custom module installation path, APK asset extraction policy, profile discovery/selection, game-module vtable ABI, native NPC factory, original combat behavior, headset rendering or a playable Quest 3 port.

Next investigate the Android asset-manifest/module extractor and real loader slot implementation. Do not add a pretend SiN profile or alias incompatible game interfaces until those boundaries have been verified.
