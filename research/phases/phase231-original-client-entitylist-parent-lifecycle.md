# Phase 231: original client entity-list parent lifecycle

15 November 2026. STACKED DRAFT on Phase230 PR #285. Owner-original 2006 retail SiN Episodes PE32 x86 DLL provenance is SHA256-gated in the new verifier:
- client.dll, 3,592,192 bytes: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll, 6,049,792 bytes: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

No original DLL, proprietary complete native function-body sequence, disassembly dump, map or assets committed.

## Distinct parent vtable

Original entity-list containing object: 0x2433FDC8. Separate, embedded VClientEntityList003 interface at parent+0x10014 has final vtable 0x24297390 (9 methods reviewed in Phases229/230). The actual containing-object parent vtable is 0x242973B8, installed by original constructor native instruction at 0x241081A2, with a verified PE32 HIGHLOW relocation on its immediate operand. Its three entries (each independently HIGHLOW-relocated in original .rdata) are:
- slot0 0x241089D0: register entity and auxiliary table associations
- slot1 0x24108A70: unregister entity and auxiliary associations
- slot2 0x241082A0: scalar deleting-destructor wrapper

## Three NEW complete original-native SHA256 reviews

| Original function VA | Whole x86 code bytes | SHA256 of complete original method bytes | Native stack cleanup | Trailing INT3 |
| --- | ---: | --- | --- | ---: |
| 0x241089D0 | 157 | 4c3874fcb23e149e035f7b7c853ae45b2c89c4cb5f9c64d7dc95ba4debfddd4e | RET 8 at two exits | 3 |
| 0x24108A70 | 161 | 0d6734b30784e88c1e0aee96c4eef76cae0aa221f95086a62838bd37743996f0 | RET 8 | 15 |
| 0x241082A0 | 30 | 069ece106e6326af9c4449dc188f4a08c0d1b2e5479cad2a53dd79ea29a2d191 | RET 4 | 2 |

Three newly reviewed whole-original function bodies, 348 native x86 bytes. These entry VAs were individually checked against the existing Phase230 catalog to avoid double counting.

Original direct CALL rel32 evidence:
- 0x24108A13 → 0x24108880 (entity register helper)
- 0x24108A34 → 0x24108680 (secondary entity auxiliary-slot acquisition)
- 0x24108AA3 → 0x24105B20 (Phase230 SHA-reviewed highest-live-slot rescan)
- 0x24108AAF → 0x24108910 (entity unregister helper)
- 0x24108AEF → 0x24106C20 (secondary auxiliary slot release)
- 0x241082A3 → 0x24107430 (parent object destructor)
- 0x241082B0 → 0x2416FD60 (conditional deallocation)

The six still-opaque nested method bodies and engine-owned virtual methods are not claimed as recovered here.

## Observed registration behavior (parent slot0)

The original masks the second 32-bit argument with 0xFFF, an index 0..4095. Only when this masked index is BELOW 2048 does native code increment DWORD[parent+0x10018], raise the recorded signed high-water index at DWORD[parent+0x10024] as needed, call the entity vtable at byte offset +0x10, and store the returned pointer into the separate EIGHT-BYTE-stride mirror at DWORD[parent+0x10028+index*8].

Native code then calls 0x24108880 regardless of the mirrored-count guard, followed by entity virtual byte offset +0x1C. When a nonnull secondary entity exists, 0x24108680 acquires an auxiliary WORD index placed at parent+0x1002C+index*8; secondary entity virtual +0xC4 yields flags. The native TEST AH,0x04 checks bit TEN, 0x400, incrementing a separate flagged count at DWORD[parent+0x10020]. If secondary entity is null, auxiliary WORD is assigned absent-index sentinel 0xFFFF. These auxiliary WORD accesses can occur for all 4096 masked indices, not just the 2048 count-tracked entries.

## Observed unregistration behavior (parent slot1)

When index <2048, original zeros the mirror entity pointer and DECREMENTS the tracked count unconditionally. When removed index is at or above the previously recorded signed highest slot, it invokes Phase230 function 0x24105B20 to rescan the PRIMARY 16-byte-stride entity table.

Critical native order: highest-slot rescan occurs BEFORE 0x24108910 unregisters the primary entity. The reported highest index may therefore still include an entity about to be removed. This is a genuine observed call-order constraint; a more intuitive post-removal rescan would be a behavioral change.

Then the method calls original 0x24108910, entity virtual +0x1C and (if secondary entity nonnull) secondary virtual +0xC4. If the flags include bit 10 it decrements parent+0x10020. Finally it reads the auxiliary 16-bit index at parent+0x1002C+index*8. If not 0xFFFF, it calls native 0x24106C20 to unlink that association, and sets the mirror WORD to 0xFFFF. The internal native list mutations of 0x24106C20 are still opaque.

## Observed deleting destructor (parent slot2)

Original 0x241082A0 invokes destructor 0x24107430. Only if low bit of first stack flag is set does it call native memory deallocation 0x2416FD60 with the containing-object pointer. The wrapper returns that pointer, RET 4. The host reference reports the event order but does not execute an original destructor or free real engine memory.

## New project source and testing

- research/tools/audit_sin_original_client_entitylist_parent_lifecycle.py: checks both whole-original DLL hashes, all three complete original function SHA256s, 29 sparse instruction witnesses, RET 8/RET 4 paths, exact INT3 pads, 3 original parent vtable slots and their HIGHLOW sites, original ctor vptr write/relocation and seven native direct CALL instructions.
- research/tests/test_sin_original_client_entitylist_parent_lifecycle.py: synthetic-only mutation tests for each original witness, return, padding, slot, relocation, constructor vptr and callsite. Artificial data NEVER increments the owner-original function review count.
- include/sin_compat/original_client_entitylist_parent_lifecycle.hpp: clean-room C++17 event and byte-buffer reference, with injected callbacks for 0x24108880, 0x24108910, 0x24108680, 0x24106C20, entity virtual calls, parent destructor and allocator. These callbacks do not fabricate opaque original C++ engine object behavior.
- tests/test_original_client_entitylist_parent_lifecycle.cpp: verifies masked index 4095 versus the guarded 0..2047 count-tracked slots; event order, pre-removal high-water scan, bit10 secondary counters, original auxiliary sentinel, and conditional deleting destructor. Linux and native MSVC Win32 x86 CTest targets registered.

Original-owner verification: python3 research/tools/audit_sin_original_client_entitylist_parent_lifecycle.py --archive /private/bin.zip

Original-free mutation regression: python3 -m unittest discover -s research/tests -p test_sin_original_client_entitylist_parent_lifecycle.py -v

## Scoped checkpoint

Reviewed native whole-method ledger: 322 → 325 (server 147; client 178). Together, the original containing object's three-slot PARENT vtable and the prior nine-slot EMBEDDED VClientEntityList003 vtable have complete-original method SHA evidence for their selected entries. This does not complete the underlying entity implementations, registry helpers, CRT/object ownership, nor prove Windows retail Source engine gameplay DLL replacement.

Next targets: original helper implementations 0x24108880, 0x24108910, 0x24108680, 0x24106C20, and actual entity virtual-method callees. Windows PC first → Android ARM64 → Quest 3.
