# Phase 1723: three original shared entity constructor bodies, 60 direct upstream creators

## Exact source evidence

User's original SiN Episodes (2006) PE32 client.dll whole SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Phase1722 authenticated the complete original 142 common entity
ClientClass creator functions, which reference **73 distinct original
constructor TARGET entrypoints**. Two targets dominate that registry:
one has 34 direct source callers, and another has 26.

Disassembly of the unchanged original executable proves a native
call chain of three COMPLETE source functions:

| Original source function | Whole native executable bytes | Original first direct CALL | INT3 padding |
| --- | ---: | --- | ---: |
| `0x24092BD0` (A) | 206 | `0x24092BD3` -> `0x240BD7C0` (B) | 2 |
| `0x240BD7C0` (B) | 861 | `0x240BD7C7` -> `0x240CCE30` (C) | 3 |
| `0x240CCE30` (C) | 573 | `0x240CCE86` -> `0x24195260` (unported) | 3 |

Original complete function SHA256, A/B/C:

- A `28e8f086a532a606b78304522eaaeb571f63922feaee80aaaeeacb7e8a03659a`.
- B `194b3ef35fa66ab726f06f1ce37539c6901ced7a3d22995a228f00aa04e62db0`.
- C `63fc9f1bbd6c8fe318f7faa1cd4aef448dd19553a6dbdc0d182b768855b34eca`.

The 1,640 original executable method bytes concatenated A+B+C have
SHA256 `3a02d8d7ff3c50af8c1096bcc00be731e02426a0b4f06112138e72cb99ec70ec`.
These source bytes remain private; the public repository stores
hashes, virtual-address labels and source-identity metadata only.

Original Phase1722 class creator source counts:

- **34** of 142 directly call A, so their original native method
  enters B and then C through the verified A->B->C first-CALL chain.
- **26** of 142 directly call C.
- **82** of 142 call other constructor targets, which are not claimed
  to pass through this chain without their own independent disassembly.

This means **60/142 source creator entrypoints** directly
depend on at least one of the source-confirmed A/B/C chain members.
Function C itself has a further original direct dependency
`0x24195260`; its body and side effects are not reconstructed here.
These counts identify a useful next porting priority, NOT
60 fully constructed/working Source entities.

## Non-executable source plan

`include/sin_compat/original_client_shared_constructor_chain.hpp`
provides original first-CALL dependency edges, exact function lengths,
original source padding, source VA labels and source-only caller grouping.
Compile-time checks cross-correlate the full original Phase1722
142-creator target catalog, counts 34/26/82 and original 73 distinct
target identities. `CanRunOriginalConstructorChain()` is false.

Original-free source audit:
`python research/tools/audit_sin_original_client_shared_constructor_chain.py --self-test`.

Optional private unchanged retail original-code audit:
`python research/tools/audit_sin_original_client_shared_constructor_chain.py --client /private/client.dll`.

The private audit checks the whole original client SHA, the three
complete whole-function SHA256 digests, exact native E8 displacement
targets, return/padding bytes, combined 1,640 original bytes SHA,
and authenticated upstream 34/26 constructor-target counts.

Native C++17 regression:
`tests/test_original_client_shared_constructor_chain.cpp`.
Tests original first-CALL dependency records, 60 directly connected
source creator identities, 82 unsupported targets, all 191 original
ClientClass source identities and NULL entity/temp-event creator
pointers. Linux strict/ASan/UBSan, Windows MSVC Win32 and
Android CMake builds include the original-free checks.

## Actual game execution remains blocked

The three full method bodies are authenticated **as source evidence**.
The original 2006 methods and their source arguments are not executed.
The remaining dependencies include further nested constructors,
original class vtables, object memory layouts, allocator services,
destructors, RecvProxy ownership and Source networking callbacks.
The original retail `VClient011.slot5` remains intentionally
guarded. No original DLL file was installed or overwritten and
no Windows game boot or Android/Quest 3 gameplay is established.
