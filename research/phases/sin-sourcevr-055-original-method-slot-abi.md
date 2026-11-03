# Phase 97: actual original SiN vs SourceVR 0.5.5 ARM64 method-slot ABI

## Primary binary evidence and scope

The user's original 2006 Windows PE32 SiN client.dll/server.dll are verified by the full Phase95 SHA256/RTTI/Init chain. Real SourceVR 0.5.5 libsoundemittersystem.so and libvgui2.so are independently verified by Phase96's full native ELF SHA256, registration and startup audit. Neither set of proprietary binaries is committed.

Additional new evidence: **three entire native virtual method tables** with every 8-byte pointer checked, exact SHA256 for the whole contiguous table, proof each entry points into the actual executable .text section and that the next pointer is not a method.

| SourceVR native module | Vtable VA | Methods | Original complete vtable SHA256 |
|---|---:|---:|---|
| libsoundemittersystem.so VSoundEmitter002 | 0x41F98 | 47 | 4aacb2497a53c6e96462fa27a4107344924d42b3cc2a9da95f99ff692cf65c88 |
| libvgui2.so VGUI_InputInternal001 | 0x670C8 | 89 | f80d17a906a17c5cd70da0ac42f2a8dc9ff44e5b6df98c4ff3db72eb3b5160b6 |
| libvgui2.so VGUI_System010 | 0x67CB0 | 40 | 1a6186a109e05a47b21e700847683df27f0ff8a8ca4e8afbec4989b0aea4d58c |

Actual ARM64 constructor writes pin native singleton vptrs: sound constructor 0x127A0 writes vptr at 0x127C0, native factory callback 0x16330 returns singleton 0x483E8; Input constructor 0x22300 writes vptr 0x670C8, callback 0x222F0 returns singleton 0x6E6E0; VGUI System constructor 0x3D4E0 writes vptr 0x67CB0, callback 0x3D4D0 returns singleton 0x71218. The auditor checks **27 native opcode sites** in addition to the previous Phase96 factory registration checks.

## PROVEN mismatch: VSoundEmitter001 old method index 3 and 6 vs native 002

| Virtual method index | Original SiN VSoundEmitter001 semantics | Actual ARM64 VSoundEmitter002 method at SAME index | Conclusion |
|---:|---|---|---|
| 3 | Name-like string to numeric token, original x86 offset +0x0C, invalid token 0xFFFFFFFF | AArch64 VA 0x12970: MOV W0, #1 then RET; method offset +0x18 | PROVEN semantic mismatch |
| 6 | Integer token to name-like pointer, original x86 offset +0x18 | AArch64 VA 0x13380: decrement state counter at this+0xD0, potentially perform teardown; method offset +0x30 | PROVEN semantic mismatch |

Original code evidence: x86 server.dll calls at 0x221456EE and 0x2214542E for index 3, and 0x221A739F and 0x2214380F for index 6. The real native singleton vtable entries and native instructions at 0x12970/0x12974 and 0x13380..0x13390 are checked byte-exactly. **This is more than a factory version mismatch: direct method-slot reuse is unsafe.**

Some other native methods (e.g. near 0x13570 and 0x13E50) perform name-like or index-like handling; these are *candidates for further analysis only*, not proven drop-in replacements. No 001 -> 002 adapter is published or enabled.

## Client: six original-used VGUI slots with real native method pointers

Phase96 verified the exact factory names VGUI_InputInternal001 and VGUI_System010 are actually registered in SourceVR 0.5.5. Phase97 additionally follows real consumer code in original client.dll at 0x240FA345, 0x240F97C9, 0x240F97E2, 0x240F97FB, 0x240F9814, 0x240F987A, 0x240F98C5 and 0x240F9870:

| Original interface | Original x86 vtable offset | Index | ARM64 same-index byte offset | Native method VA |
|---|---:|---:|---:|---:|
| VGUI_InputInternal001 | +0xDC | 55 | +0x1B8 | 0x25C00 |
| VGUI_InputInternal001 | +0xE0 | 56 | +0x1C0 | 0x25CA0 |
| VGUI_InputInternal001 | +0xE8 | 58 | +0x1D0 | 0x25F30 |
| VGUI_InputInternal001 | +0xF4 | 61 | +0x1E8 | 0x26160 |
| VGUI_InputInternal001 | +0xF8 | 62 | +0x1F0 | 0x261A0 |
| VGUI_System010 | +0x60 | 24 | +0xC0 | 0x3DE30 |

**These are actual executable native vtable pointers, NOT proven ABI-compatible methods.** The old 32-bit and new 64-bit pointer widths change offsets, and parameters/object lifetime/side effects are still unverified. All six are explicitly gated as METHOD ABI UNKNOWN.

## Implementation and tests

- include/sin_compat/sourcevr_method_slot_compat.hpp: C++17 architecture-neutral original slot requirements, verified sound mismatches, unverified VGUI methods and strict refusal policy. No reinterpret_cast to ARM64 function pointers.
- research/tools/audit_sourcevr_method_slot_compat.py: original client/server SHA chain, real ARM64 ELF SHA/registration chain, complete vtable fingerprints, 27 native ARM64 instruction checks, 12 original x86 virtual callsites, metadata-only output.
- research/tests/test_sourcevr_method_slot_compat.py: 12 original-free mutation test groups including all 176 pointer entries, 27 native opcodes, wrong section pointers, table boundaries, SHA enforcement and original DLL fingerprint chain.
- research/tests/sin_sourcevr_method_slot_reference.cpp: C++17 strict compile-time and runtime refusal assertions.
- CMakeLists.txt: additional independent desktop/NDK CTest target.

Local original-owner binary audit PASS; Python 12/12 PASS and CTest 1/1 PASS with strict C++17 warnings. GitHub CI is recorded separately.

## Limits and next investigation

This phase does **not** implement a compatible original SiN libclient.so or libserver.so, an actual 001-to-002 sound-emitter bridge, VGUI rendering or a game that runs on Meta Quest 3. Phase 98 should inspect native sound name/token candidate method signatures and compare their call chains to the old originals, then bridge only methods with fully established ABI. Independently examine each of the six VGUI method signatures on ARM64 before invoking them.