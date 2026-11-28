# Phase 1722: all 142 original common entity ClientClass creator methods, complete

## Exact original code recovered, without importing executable bytes

Owner original unmodified SiN Episodes (2006) PE32 client.dll SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Phase1718 had authenticated ONLY the first eleven native bytes
of 142 common entity creators: `PUSH ESI; PUSH original byte SIZE;
CALL 0x240C4FF0`. Phase1722 now authenticates the **complete original
executable method body** of all 142, with individually pinned
per-class SHA256 and a concatenated whole-function digest.

All 142 source methods are ordered in the authentic VClient011.slot5
191-class linked-list order, excluding the seven exceptional entity
creators that were already whole-function SHA-pinned in Phase1721.
Concatenated complete method bytes: **10,770**.
Concatenated 142-body SHA256:
`0badb693b0f8c3977a74b182c1fa696bbd6f620e40c0acbbd33746933a9ac902`.

## Two original native method families

| Original source family | Count | Full native lengths |
| --- | ---: | --- |
| Constructor-first, then virtual | **68** | 60 bytes (all) |
| Vtable-stamped, then virtual | **74** | 86, 92, 96, 97, 106, 107, 113, 117 bytes |

All 142 contain one original native virtual call instruction
`FF 90 94 00 00 00`, which invokes the actual object's vtable
method at byte offset **+0x94**. On a successful object creation,
all return the original **networkable subobject address** at
allocation/constructed object base **+8**.

The constructor-first family calls the source initializer at
`originalCreatorVA+20`, has an early `xor eax,eax; pop esi; ret`
for a failed construction, and ends with
`lea eax,[esi+8]; pop esi; ret` after the virtual call.
The vtable-stamped family calls its source constructor at
`originalCreatorVA+23`, then stores native class vtable pointers
and invokes the original virtual call. Its successful result
is `edi`, which was initialized to `esi+8`; it ends with
`mov eax,edi; pop edi; pop esi; ret`, followed by
`xor eax,eax; pop esi; ret` for allocation failure.

Every one of the source constructor CALL targets was recovered
from its *actual* E8-relative32 displacement. There are exactly
**73 distinct original constructor target VAs** across the 142
original creator bodies. This gives the next phase a concrete
target-function inventory, not yet portable constructors.

Of the 142 complete native functions, 130 have 4,5,6,10,11
or 15 original `INT3` padding bytes after their final RET.
**Twelve** complete native functions have **no padding** and
directly abut further executable code. Their boundaries were
verified using the original success/failure return instructions
instead of searching for `RET; INT3` alone, which would
incorrectly extend them into other original functions.

Full source CSV:
`research/manifests/original_client_142_full_entity_creator_bodies_phase1722.csv`.

It records class ordinal/name, original function VA, requested
allocation bytes, complete native function length and SHA256,
original constructor CALL site and target, original virtual byte
offset, networkable subobject return adjustment, 68/74 family
and exact native padding length.
LF CSV SHA256:
`3393a7ef2ddaefa2ff0c4a3d0cb6539ae165d764f3a97a42f4b177e5483f2530`.

## Native C++ metadata remains non-executable

`include/sin_compat/original_client_full_entity_creator_bodies.hpp`
contains constexpr source-identity records for all 142 complete
original methods. Compile-time checks verify every record matches
the earlier independently source-verified Phase1718 first-11-byte
prologue, 142 class/creator identities, source lengths and categories,
68/74 families, 12 unpadded methods, 73 unique source constructor
targets, 10,770 total executable byte count, and the earlier
all-149 entity entrypoint family classification.

Every original VA here is a **source-only label**. No original
Win32 function address is dereferenced, cast to a host function
pointer, inserted into a 20-byte ClientClass node, or executed.
The host native32 image still has BOTH +0/+4 creator fields null.
No entity vtable, constructor target, destructor or allocation
lifetime is executed or fabricated.

## Audits and cross-platform tests

Original-free:
`python research/tools/audit_sin_original_client_full_entity_creator_bodies.py --self-test`.

Optional private unchanged owner original-code SHA audit:
`python research/tools/audit_sin_original_client_full_entity_creator_bodies.py --client /private/client.dll`.

The private path authenticates the whole original client DLL,
all **142 complete original source x86 method SHA256** values,
the complete concatenated 10,770-byte original source body SHA,
the common zero-init callsite/size, exact second constructor
CALL targets, one original virtual+0x94 instruction per method,
the two exact original return-family opcode suffixes and
INT3 padding (or the twelve explicitly unpadded source methods).

C++ test:
`tests/test_original_client_full_entity_creator_bodies.cpp`.
Requires complete source CSV/native/class catalog consistency,
return+8 / vtable+0x94 for each method, exactly 68/74 source
families and 73 constructor targets, all 142 class source
allocation sizes, seven excluded special entity creators and
42 distinct temporary event creators. Every native32 ClientClass
still has uncallable zero creator pointers; retail registry
exposure remains impossible.

Strict C++17, Linux ASan/UBSan, Windows MSVC Win32 and
Android original-free CMake checks are wired.

## Actual engine/startup limitation

This is a significant **original entrypoint decompilation/ABI
metadata** milestone, not 149 working entity constructors.
The 73 distinct native constructor target routines, their
subcalls, full original object/vtable layouts, engine allocator
ownership, lifecycle/destructors, RecvProp callback ABI, special
native arrays/overlay and network table mutation must still be
reimplemented before returning a live ClientClass graph.
The owner's last retail-game diagnostic remains the intentional
`VClient011.slot5` stop, and no commercial executable was
overwritten. No native Windows game boot, Android/Quest 3
gameplay, physics or renderer is claimed.
