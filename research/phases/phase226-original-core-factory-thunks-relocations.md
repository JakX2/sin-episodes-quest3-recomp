# Phase 226: original core interface singleton thunks and PE32 relocations

14 November 2026. Stacked DRAFT on Phase225 PR #280. Both owner-provided original 2006 SiN Episodes Windows x86 DLLs are privately SHA256 checked by the verifier, using the retained Phase177 provenance:

- server.dll: SHA256 568a0086fde607345adf31d7cfb2a5bf4eefe9a86f416faa97076ddf9d97650 (6,049,792 bytes).
- client.dll: SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d (3,592,192 bytes).

The commercial DLLs, proprietary full machine-code dumps and game assets are not committed.

## Exactly nine newly reviewed methods, not eighteen

Phase177 previously authenticated both exported CreateInterface routines, 12-byte linked-list registration nodes, and complete manifest digests for **10 server and 55 client registrations**. Phase178 had already individually SHA-verified **nine** six-byte core factory callbacks (four server, five client). This phase completes a unified inventory of **18 static singleton-return factories**, but credits ONLY **nine new** original methods, each six native x86 bytes, for **54 new code bytes**. The other **47** client registrations are dynamically allocating material-proxy factories, not singleton thunks.

All 18 have the original six-byte native pattern MOV EAX, absolute singleton VA; RET C3, followed by precisely 10 INT3 padding bytes. The four-byte immediate operand at native thunk VA+1 has a real PE32 HIGHLOW relocation. The corresponding original registered interface name and thunk VA are checked against the previously SHA-validated Phase177 ordered native registration manifest.

### New nine

| Module | Registered interface | Original thunk VA | Returned original singleton VA | Entire original 6-byte SHA256 |
| --- | --- | --- | --- | --- |
| server | IEffects001 | 0x22129680 | 0x224ECE00 | 2eac16d4c0cfda2bcb592a089d6762c3c6697740eac955659c0abe3362ed83ef |
| server | PluginHelpersCheck001 | 0x221FC4F0 | 0x22511A14 | 9171cdfc432102e603561d2be6b8ab08815e29993474946b32f7b5bf03e83d72 |
| server | PlayerInfoManager001 | 0x22257780 | 0x2251A048 | e828c0b8f05a5fb5ae09c15fab91f334910e50d6084e79e4faddc65d47addb10 |
| server | PlayerInfoManager002 | 0x222577A0 | 0x2251A048 | e828c0b8f05a5fb5ae09c15fab91f334910e50d6084e79e4faddc65d47addb10 |
| server | BotManager001 | 0x222577B0 | 0x2251A04C | c9d32770c94fb85fde54f6d6d895aa092924ab0574d4242fc104013bbd0b13c5 |
| server | HLTVDirector001 | 0x2225F8C0 | 0x2257AEA0 | 9a50234d1328f0c8cfd361e0d497e1df4d3a5310eaeb5e7f18f97c5bfb4d689a |
| client | ClientLeafSystem001 | 0x24108B20 | 0x24357F30 | 9328e36548ae5755e8637e94a5a3712571f822f49b9ea290e973bef7e9ea6de6 |
| client | IEffects001 | 0x24127780 | 0x242FB554 | 13b0ec8347529f2c88a47912819ef52895a47e95b6da8dcd0e85a66a930a99e4 |
| client | VCENTERPRINT002 | 0x241EC540 | 0x2431DBF0 | 81267ad71e6a917e5251cb606e3180b7d36d219e6be716ed03bc97674e9c0aca |

**Original factory alias:** the two distinct server PlayerInfoManager factory functions for versions 001 and 002 both return original address 0x2251A048, with the same original file-backed vtable pointer and first virtual method. Different factory entrypoints do NOT imply different runtime singleton instances.

## Critical original Windows PE relocation and vtable contract

For twelve of eighteen factory singleton addresses (eight server, four client) the original file has a static vtable pointer in its raw .data section. The verifier checks its exact pointer, the original .rdata vtable slot-zero target into .text, and the HIGHLOW relocation on both the singleton vptr word and the first vtable entry. The factory return immediate itself is independently relocation-checked for all eighteen.

The other six singleton addresses (two server, four client) are located in the **virtual unbacked tail of .data**. The on-disk original DLL cannot reveal their eventual vptr values. The verifier asserts these are unbacked, rather than manufacturing zero vtables or claiming they are initialized. These include HLTVDirector001, server/client GameMovement001, VClientEntityList003, ClientLeafSystem001 and VClientPrediction001.

Native absolute VAs in original code are subject to the real Windows image loader's HIGHLOW rebasing. The independent portable C++17 reference calculates **expected rebased address tokens** with explicit 32-bit overflow checks; it does not map actual PE memory, run original constructors or return usable original engine pointers.

## New sources and tests

- research/tools/audit_sin_original_core_factory_thunks.py: read-only owner-original provenance gate reusing Phase177 dual whole-DLL SHA, export and registration-manifest verification; all 18 complete six-byte SHA hashes, original factory names and addresses, 18 required immediate HIGHLOW sites, 12 paired static vptr/slot0 address and relocation checks, exact 10-byte INT3 padding, and six virtual unbacked singleton checks. Only nine newly reviewed methods count after original whole-DLL provenance is proved.
- research/tests/test_sin_original_core_factory_thunks.py: synthetic mutation checks for every factory instruction byte, each INT3 pad, relocations, vtable pointers, unbacked singleton boundaries, original interface registrations and a standalone PE32 relocation-directory fixture. Synthetic bodies never count as authentic new recoveries.
- include/sin_compat/original_core_factory_thunks.hpp: portable C++17 inventory of all eighteen static factories, precise name lookup, original singleton and vtable identity tokens, and safe rebasing. No host pointers are dereferenced.
- tests/test_original_core_factory_thunks.cpp: verifies 10 server / 8 client core registrations, exact version matching, PlayerInfoManager aliasing, independent client/server IEffects singleton identities, rebased HIGHLOW return addresses, and refusal of unknown or overflowed vptr tokens.
- Root Linux CTest and genuine native Win32 MSVC x86 CTest exercise only the original-free host model.

Owner-only reproduction commands:

    python3 research/tools/audit_sin_original_core_factory_thunks.py --archive /private/bin.zip
    python3 -m unittest discover -s research/tests -p 'test_sin_original_core_factory_thunks.py' -v

## Status

Original complete-function SHA catalog advances **277 to 286**: 128+6 = **134 server**, 149+3 = **152 client**. No prior Phase177/178 function is double-counted. Complete interface object layouts, constructors, vtable function implementations, dynamic material-proxy allocations, real factory object lifetime and actual Windows gameplay engine startup remain incomplete. This is not a percentage of total DLL decompilation. Windows PC integration remains the primary milestone; Android ARM64 and Quest 3 are downstream.
