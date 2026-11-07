# SourceVR 0.5.5 ARM64 game-client/server virtual tables

7 November 2026. Read-only inspection of the owner's private joined
SourceVR 0.5.5 archive. The original native ELFs and commercial assets
are never committed.

## Recovering complete game-object topology

All 11 distinct native game modules from the seven SourceVR profiles
were SHA256-verified against the native profile manifests. Their
registered CreateInterface factory callbacks return singleton objects,
and each singleton's real initializer installs the concrete vtable:

- Native CServerGameDLL inherits IServerGameDLL and has exactly
  **47 consecutive executable AArch64 virtual method pointers**.
- Native CHLClient inherits IBaseClientDLL and has exactly
  **78 consecutive executable AArch64 virtual method pointers**.

The evidence traces each factory registration, callback and singleton;
concrete and base RTTI; vtable offset-to-top; the complete executable
method pointer table and its terminating boundary; complete SHA256 of
all pointers; and exact ADRP/ADD/STR code writing the vtable into the
same factory-returned singleton.

Across five unique server and six unique client ELFs this verifies
**703 actual native virtual method pointers**, not merely interface names.

## Exact native layout addresses

| SourceVR profile | Server primary vtable VA | Client primary vtable VA |
| --- | --- | --- |
| Half-Life 2 and Lost Coast | 0xA11488 | 0x81FE10 |
| Episode One | 0xA94550 | 0x82D898 |
| Episode Two | 0xA94550 | 0x82D898 |
| Portal | 0x9C4818 | 0x7BDAA0 |
| Portal 2 | 0xAF47F0 | 0x834C98 |
| Entropy: Zero | 0xAA1990 | 0x82EA28 |

All seven profiles share the same **47/78 interface table lengths**.
Some share exact binaries; notably Episode One and Episode Two share
the same server SHA256 and their distinct client binaries have the
same complete 78-pointer vtable hash. Full 64-character ELF/table
hashes and all constructor sites are in the new verifier.

The original Half-Life 2 native game server, for example, registers
ServerGameDLL010 using factory callback 0x52CC10 returning singleton
0xC7E448. At native VA 0x532A4C the constructor loads the vtable
0xA11488, and STR at 0x532A5C stores it to the singleton. The
concrete RTTI starts at 0xA11600. The first table method pointer
is 0x52CC20; its 47th pointer is 0x52F100.

The original native client callback 0x36C4F0 returns singleton
0x9DAAF8. Constructor 0x370808/0x370810 writes the CHLClient
table 0x81FE10 into that object. Its first method is at 0x36C820
and its 78th method is at 0x36F7C0.

## Genuine original SiN 2006 Windows x86 game vftables

The owner's original server.dll and client.dll from bin.zip were
also checked against their exact SHA256 values, then parsed as real
PE32 x86 binaries with Microsoft RTTI TypeDescriptors,
CompleteObjectLocators (COL), class hierarchies and virtual tables.
This exposes a material difference in interface shape:

| Game DLL | Original MSVC class | Locator VA | Vtable VA | Methods | Complete table SHA256 |
| --- | --- | --- | --- | ---: | --- |
| Original 2006 server.dll | CServerGameDLL | 0x224C51E8 | 0x22442EFC | **31** | 97e42f1905b28e6aad0e3de21473d12725c49b25980be4a5db285333774a8299 |
| Original 2006 client.dll | CHLClient | 0x242E5818 | 0x24296C6C | **48** | 9d28eccf91315f7a6ecc2394386d7b4077919d8649d34297a951eb7a1cafa664 |

Every original 32-bit entry is a pointer into the corresponding
original DLL's executable .text section, and the following word is
not an executable function pointer. The first/last original server
methods are 0x221E7640/0x221E7CF0; the first/last original client
methods are 0x240FE0E0/0x240FE670. Both tables are SHA-pinned in full.

Comparing with the AArch64 vtables:

- Server: original **31** slots versus ARM64 **47**, a delta of 16.
- Client: original **48** slots versus ARM64 **78**, a delta of 30.

Those deltas do **not** establish that the newer interfaces simply
append 16/30 methods. Versions may insert, reorder, remove or change
earlier methods. No blind virtual-pointer index translation is safe.

The auditor now accepts optional --sin-bin-zip to validate original
PE32/MSVC RTTI, COL pointer, both entire 31/48 pointer tables and
their exact fingerprints. A separate synthetic-only test file checks
corrupted PE headers, RTTI pointers, locator links, method execution
bounds, table boundaries and SHA failure.

## Source SDK labels remain candidates only

Valve's publicly available Source SDK 2013 header eiface.h declares
IServerGameDLL hooks beginning with DLLInit, ReplayInit, GameInit,
LevelInit and ServerActivate. cdll_int.h declares IBaseClientDLL
hooks beginning with Init, PostInit, Shutdown, ReplayInit and
ReplayPostInit.

Those headers provide potential next-step names and signatures, but
are **not** automatically binary-compatible with SourceVR 0.5.5.
The public revisions have platform-specific and later virtual methods;
the actual native vtable topology is stronger than raw declaration
counts. Every used method needs an independent native callsite and
signature audit before dispatch through a replacement C++ object.

Public references, not copied into this repository:
https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/eiface.h
https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/cdll_int.h

## Deliverables and reproduction

- research/tools/audit_sourcevr_game_module_vtables.py verifies all
  eleven native files against their full manifest SHA, prior real
  factory registration/lookup proof, class/base RTTI, every method
  pointer, the exact pinned pointer-table SHA and constructor write
  to the returned singleton.
- research/tests/test_audit_sourcevr_game_module_vtables.py contains
  13 synthetic-only positive/negative cases, including 47-slot and
  78-slot fixtures, malformed RTTI/ctor and table boundaries.
- include/sin_compat/sourcevr_game_vtable_layout.hpp and C++ CTest
  assert the native topology while refusing all unverified native
  game-object virtual dispatch. GitHub CI has no original game files.

Run privately against the user's joined SourceVR ZIP:

    python3 research/tools/audit_sourcevr_game_module_vtables.py --sourcevr-zip /private/sourcevr-joined.zip

Run synthetic regressions in any checkout:

    python3 -m unittest discover -s research/tests -p test_audit_sourcevr_game_module_vtables.py -v

## Integration decision and remaining blocker

A real replacement libserver.so or libclient.so must eventually
satisfy native engine ServerGameDLL010..004 / VClient017 contract.
Original Windows SiN uses ServerGameDLL003 and VClient011, so
blind factory aliases or copies of the old x86 objects are unsafe.

Recover method signatures, calling conventions and engine lifecycle
before ever returning a replacement 47/78-method game object from
CreateInterface. Then bridge the already rebuilt SiN NPC and XR
components to actual engine-controlled entities and map events.

This milestone is **native game-object ABI topology research**, not
a functioning SourceVR plugin or a playable Meta Quest 3 APK.
