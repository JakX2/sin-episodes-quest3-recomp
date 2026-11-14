# Phase 227: original runtime singleton initialization and teardown

14 November 2026. **Stacked draft on Phase226 PR #281.** The complete original 2006 SiN Episodes: Emergence PE32 game DLL pair was inspected privately:

- server.dll: 6,049,792 bytes; original SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
- client.dll: 3,592,192 bytes; original SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

No proprietary original DLL, full native disassembly or full original method body is published. All function-body hashes below are calculated over the **complete original native method regions**, not synthetic reference code.

## Four observed runtime singleton initialization chains

The Phase226 report established that six core singleton factory return addresses lie in the original **virtual unbacked .data tail**, with no on-disk vptr at their object addresses. Phase227 identifies the static initializer cell, constructor entry, native teardown registration and final vptr for four of the six. The .data initializer cell is a **file-backed relocated pointer to executable startup code**; this proves a static native callback entry, but not the precise order in which the CRT invokes every callback.

| DLL and interface | Relocated original .data initializer cell | Native startup wrapper | Singleton | Constructor called | Teardown wrapper / final destructor |
| --- | --- | --- | --- | --- | --- |
| server GameMovement001 | 0x224D7CC0 | 0x223C33F0 | 0x225934B0 | 0x221EBF40 | 0x223CF2C0 → 0x221EAAC0 |
| client ClientLeafSystem001 | 0x242F39AC | 0x24268440 | 0x24357F30 | 0x2410C790 | 0x24273500 → 0x2410CA80 |
| client GameMovement001 | 0x242F4304 | 0x2426E940 | 0x24379920 | 0x24136F80 | 0x242757A0 → 0x24136270 |
| client VClientPrediction001 | 0x242F480C | 0x24271430 | 0x2437FA00 | 0x24195A10 | 0x24275E50 → 0x24195700 |

All four startup wrappers pass the exact singleton address via x86 ECX to a native constructor, push the matching teardown callback pointer, and call a native shutdown-registration helper. Original helper destinations are 0x223A5639 (server) and 0x24248788 (client); these helpers' **complete bodies are not claimed** as recovered here. Teardown wrappers are a native `MOV ECX, singleton; JMP original destructor`, each with precisely six following INT3 bytes.

## Original constructor and vptr evidence

- **Server GameMovement001:** constructor 0x221EBF40 first writes vtable **0x224433D4**. After its return, native wrapper 0x223C33F0 writes final vtable **0x2245EC04** directly to the singleton, before registering teardown. Original final vtable slot 0 targets executable **0x222D01B0**.
- **Client GameMovement001:** constructor 0x24136F80 first writes vtable **0x2429A684**, then wrapper 0x2426E940 overwrites the singleton's first word with final vtable **0x242B1B5C**. Final vtable slot 0 points at **0x2420FC60**.
- **Client VClientPrediction001:** constructor 0x24195A10 installs vtable **0x242A363C** during construction, without a later wrapper overwrite. That final table's first entry points at **0x24195A50**.
- **Client ClientLeafSystem001:** wrapper 0x24268440 calls constructor **0x2410C790** and registers teardown. Three concrete original constructor instructions at 0x2410C7A0, 0x2410C7A6 and 0x2410C7AD install vtable pointers **0x2429756C** at object +0, **0x24297534** at object +4 and **0x2429752C** at object +8. The first primary vtable entry is **0x2410D8E0**. **The complete leaf constructor is not SHA-reviewed or modeled here**, only these selected vptr stores. Other leaf constructor behavior remains opaque.

Every reported final primary vtable is verified in original .rdata, its first DWORD points into original executable .text and has a real PE32 HIGHLOW relocation. All original absolute pointer operands used for static .data initializer cells and constructor vptr writes have corresponding HIGHLOW relocations. For both GameMovement wrappers the post-constructor singleton store and final-vptr operand are independently relocation-checked.

The remaining two originally unbacked singleton return addresses, **server HLTVDirector001 at 0x2257AEA0** and **client VClientEntityList003 at 0x2434FDDC**, do not have an equivalent independently authenticated startup chain in this phase. They may be initialized through different owning objects; treating them as automatically reconstructed would be incorrect.

## Eleven newly reviewed complete native functions

| DLL | Native original entry | Role | Whole native bytes | Whole native code SHA256 |
| --- | --- | --- | ---: | --- |
| server | 0x223C33F0 | GameMovement initializer | 32 | c4968533bfb17b863ef43b4238b056c1163f73e3c0d7d41c76329c488d03eb73 |
| server | 0x223CF2C0 | GameMovement teardown callback | 10 | e62c172da3df757cdcb4b417da1711ba42441d265ba6ddd7038f8ee5f8958a9e |
| server | 0x221EBF40 | GameMovement constructor | 58 | b41623a8cfab6393ae2b33ae1d89a290cd4a99528c4568fea25e10c58b89edbf |
| client | 0x24268440 | ClientLeafSystem initializer | 22 | ba92fca69ae0bacce3f0eaf0caaba7371b9ca216aff659d76eb8b67bb0582adb |
| client | 0x2426E940 | GameMovement initializer | 32 | d0e6bafd640d3ed0dc19c2a1b7d2f50805ab7103c6105de11d6dfa6fa46e64cf |
| client | 0x24271430 | VClientPrediction initializer | 22 | cd1e62790abbf1083835c043be37815a772d807424a3d0661ce0669380ed7c79 |
| client | 0x24273500 | ClientLeafSystem teardown callback | 10 | 398f89ccc6255fd4c5a31a22092e38bdfc614326cb4ccd90999e352f3886bfe0 |
| client | 0x242757A0 | GameMovement teardown callback | 10 | 2ea1c0a296a9dc53c9c987b679fbd1594db882bfe585af1495f1b68d17ff39b7 |
| client | 0x24275E50 | VClientPrediction teardown callback | 10 | c6a242c2156899d53244328a8edc28bb1d1ae2b79c12cacf44cde38782098651 |
| client | 0x24136F80 | GameMovement constructor | 58 | 4d6388f8810aac5e43bf305ad75231f3e48ab2a5b77e77179e6364083f9fac24 |
| client | 0x24195A10 | VClientPrediction constructor | 50 | 9b0b5c77c72ae369a286600971788b3820d79ffff740fc6c7721b3fc5277f8a7 |

Total **11 newly reviewed original native functions, 314 original x86 bytes**. Not previously present in the Phase226 method ledger; **server 3, client 8**. None of the prior Phase177/178/226 CreateInterface factory thunks are counted again.

The 11 complete original SHA reviews cover four initializer wrappers, four teardown trampoline bodies and three constructors; the larger client leaf constructor, four full destructor implementations and CRT registration implementation remain outside the new body count.

## Code, verification and tests

- `research/tools/audit_sin_original_runtime_singleton_initializers.py`: verifies BOTH complete retail DLL SHA256s, all eleven complete original method hashes and exact native INT3 boundaries, eight native direct ctor/registration CALLs, four teardown tail JMPs, four relocated .data startup-pointer entries, native constructor initial vptr stores, both post-constructor vptr overwrites, relocated original .rdata vtable slot-zero pointers and virtual-unbacked .data singleton locations. Synthetic code cannot increment authenticated original-method totals.
- `research/tests/test_sin_original_runtime_singleton_initializers.py`: synthetic-only mutation tests rejecting altered startup-pointer cells, HIGHLOW relocations, ctor/registration CALLs, teardown JMPs, vptr opcodes and pointer operands, final vtable entries, return bytes and native INT3 boundaries.
- `include/sin_compat/original_runtime_singleton_initializers.hpp`: C++17 host **event-only** model of four original constructor/initial-vptr/post-constructor-vptr/teardown registration paths, plus known leaf subobject vptrs; intentionally no runtime class instances, CRT execution or real engine memory.
- `tests/test_original_runtime_singleton_initializers.cpp`: verifies all four identity mappings, two movement vptr overwrites, client prediction and leaf no-overwrite paths, full-body leaf constructor explicitly unresolved, correct server/client cleanup helper targets, callback timeline and host-only invalid shutdown protection.
- Linux CTest and native MSVC Windows x86 CTest registrations are included, but these are synthetic reference tests, **not** the original retail engine loading replacement game DLLs.

Owner-only verification:

```sh
python3 research/tools/audit_sin_original_runtime_singleton_initializers.py --archive /private/bin.zip
python3 -m unittest discover -s research/tests -p 'test_sin_original_runtime_singleton_initializers.py' -v
```

## Checkpoint

Reviewed original whole-function SHA ledger **286 → 297** (**137 server, 160 client**). Four of six Phase226 unbacked singleton factory return targets now have independently traced native static initializer/teardown paths. Two remain untraced. This is not the total game's completed function count, not a percentage of total code decompiled, and not demonstration of playable replacement Windows gameplay modules.

**Next:** trace initialization of HLTVDirector001 and VClientEntityList003, then hook real engine callbacks and verify a reconstructed game DLL under the actual Windows game process. Android ARM64 and Quest 3 follow the PC runtime milestone.
