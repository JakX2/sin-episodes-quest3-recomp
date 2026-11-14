# Phase 228: original embedded HLTV and client entity-list initializers

14 November 2026. **STACKED DRAFT on Phase227 PR #282.** Direct original-binary research using the owner's complete retail 2006 PE32 game DLLs. Original server SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 (6,049,792 bytes). Original client SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d (3,592,192 bytes). Neither original game DLL, whole native proprietary code body, disassembly dump nor game asset is published.

## Last two runtime-only factory returns are embedded subobjects

**Server HLTVDirector001** factory VA 0x2225F8C0 returns 0x2257AEA0, **containing object 0x2257AE98 + offset 0x08**. Its file-backed and HIGHLOW-relocated .data static initializer cell at 0x224D7A04 points to function 0x223C1B40. That wrapper loads containing-object address 0x2257AE98 in ECX, calls complete original constructor 0x222625E0, pushes teardown wrapper 0x223CE6A0 and registers it via call 0x223A5639. Teardown loads ECX with **containing object** and tail-JMPs to original destructor 0x222626A0. Authentic constructor vptr store sequence: parent+4 -> 0x224313FC; parent+8 -> 0x22453CB4; parent+0 -> 0x22453E04; parent+4 -> 0x22453DC8; parent+8 -> **0x22453DA0** final embedded interface vtable. The first verified executable vtable entry is 0x22262690.

**Client VClientEntityList003** factory VA 0x241056B0 returns 0x2434FDDC, **containing object 0x2433FDC8 + offset 0x10014**. Original .data static initializer cell 0x242F3998 points to wrapper 0x24268370. It loads containing object 0x2433FDC8 in ECX, calls full original constructor 0x24108190, pushes teardown wrapper 0x242734C0 and registers via 0x24248788. Teardown likewise uses **containing object**, then JMPs to destructor 0x24107430. Original native constructor vptr stores: parent+0x10014 -> 0x24297368; parent+0 -> 0x242973B8; parent+0x10014 -> **0x24297390** final embedded interface vtable. Its first executable vtable entry is 0x241056F0. Constructor also calls 0x24127E80 and 0x241061E0, but the full nested bodies are not newly credited.

The original entity-list +0x10014 offset is observed directly in code; it is not evidence of a separately allocated standalone class. Both containing objects and embedded interface addresses lie in the virtual unbacked .data tail. The vtable pointers cannot be read as initialized raw on-disk object contents: native constructors install them at runtime.

## Six newly authenticated complete original x86 methods

| DLL | Native VA | Role | Code bytes | Whole original native SHA256 | Following INT3 |
| --- | --- | --- | ---: | --- | ---: |
| server | 0x223C1B40 | HLTV initializer | 22 | 0a8559afff5663dd07f3a188c57dbf67c4400392f671ff5bda47bb26c593db22 | 10 |
| server | 0x223CE6A0 | HLTV teardown trampoline | 10 | d828ee3fcabdccea745ae1e1e46471a4758208133e02dc4feee729cd9a959790 | 6 |
| server | 0x222625E0 | HLTV containing-object constructor | 158 | deb717a3779cf5019a19d8f259009314c670dc5e6bbe4fa36759f8364ab3782a | 2 |
| client | 0x24268370 | Entity-list initializer | 22 | 446b15473c55849da8f446f80f090e459e2e1d6073464871d62803b977e896a6 | 10 |
| client | 0x242734C0 | Entity-list teardown trampoline | 10 | c6f9ff177371c32624dcd45c17b338cc235c3b9dcc173ddcc1e288bf30e35c08 | 6 |
| client | 0x24108190 | Entity-list containing-object constructor | 264 | c7811e3f10bbf60d9beb9dd54fdc8251fe59445b59d3be0050ae02e3fda209fd | 8 |

**Six whole original SHA-verified methods; 486 native instruction bytes.** Existing Phase178/226 CreateInterface factory thunks, Phase227 initializer functions and the original nested destructor/registration helper bodies are NOT counted again. Authenticated opcode/relocation witnesses include five server and three client native vptr-write instructions, both relocated .data static-init pointers, exact constructor/registrar CALLs, original teardown tail-JMP and correct parent pointers, and final relocated .rdata vtable slot-zero pointers.

## Evidence, host model, limitations

- research/tools/audit_sin_original_embedded_interface_initializers.py: owner-original dual-full-DLL SHA gate; exact six complete native function SHA256s, C3 vs JMP boundaries and INT3; static initializer and factory pointers; 8 original constructor vptr instructions and actual PE HIGHLOW entries; two nested constructor CALLs; and original final executable vtable entries.
- research/tests/test_sin_original_embedded_interface_initializers.py: synthetic-only mutation tests for original evidence. Synthetic fixture byte patterns cannot establish original SHA-reviewed methods.
- include/sin_compat/original_embedded_interface_initializers.hpp: portable C++17 **event-only** representation of containing-object constructor/vptr writes, registered cleanup and destructor tailcall using original address **tokens**. It does NOT create any engine-owned object.
- tests/test_original_embedded_interface_initializers.cpp: exact parent/interface relationship, offset, original vptr stores, native callback ordering and host-only invalid lifecycle guard.
- Root Linux and native MSVC Win32 x86 host tests registered. No retail Windows game-engine gameplay DLL loading is demonstrated.

Owner-only SHA verifier:

    python3 research/tools/audit_sin_original_embedded_interface_initializers.py --archive /owner-private/bin.zip

Synthetic tests:

    python3 -m unittest discover -s research/tests -p 'test_sin_original_embedded_interface_initializers.py' -v

## Scope

Original whole-function SHA-reviewed method ledger **297 -> 303** (server **140**, client **163**). All six originally unbacked core singleton factory return targets from Phase226 now have verified *startup chain evidence*: four Phase227 direct singleton objects and two Phase228 embedded subobjects. This is NOT a total decompilation percentage or proof of playable replacement DLLs. The exact CRT execution order, destructor behavior, virtual method implementations and genuine Windows retail engine integration remain unresolved. **Windows PC first, Android ARM64 second, Quest 3 VR last.**
