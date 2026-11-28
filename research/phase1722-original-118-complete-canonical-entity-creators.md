# Phase 1722: 118 full original native entity creator bodies authenticated

Original unchanged retail SiN Episodes (2006) PE32 client.dll SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## Genuine original source functions

Of the 142 common allocation creators classified in Phase1718, **118**
are recognizable as two **complete** original x86 function body templates:

| Native body | Count | Original padding | Constructor targets |
| --- | ---: | ---: | --- |
| 60-byte entity creator | 68 | 4 `INT3` | 68 different original constructor VAs |
| 86-byte inline-vtable entity creator | 50 | 10 `INT3` | 6 shared original base ctor VAs |

Each original function begins with PUSH ESI; PUSH original entity size;
CALL original zeroing allocation wrapper `0x240C4FF0`. Both families
call an ORIGINAL constructor and return NULL on the allocation/ctor
failure path. The source successful path passes two original network
arguments to a vtable method at **byte offset +0x94**, then returns
an **interior networkable object pointer at base +8**.

The 60-byte family calls an original class-specific constructor whose
return value is checked before the engine initialization virtual call.
The 86-byte family calls one of six shared base constructors, writes
four distinct original 32-bit vtable pointer labels into native entity
fields at +0, +4, +8, +12, then calls the same virtual +0x94.
Source constructor vtable labels are in original `.rdata`, NEVER
host-executable or Android-compatible function tables.

Remaining **24** of the 142 common-source allocation creators contain
unique extra initialization logic, additional native branches, or
other side effects. They are explicitly excluded from the complete
canonical body classification. Their first 11-byte allocation
prologues are still source-authenticated; the rest require separate
instruction-level review.

Original concatenated full executable method SHA256 in ordered classes:

- 68 x 60-byte: `401286913c256886a575d8d55985c7b3579143e9623b897fecb13a6cc548d10a`
- 50 x 86-byte: `ee7e1deea1a0608ca47aefc56bf8b2c6ca389ca19f35d86e641c1435cf27d84b`
- Combined 118: `a0fe29c6f806dd85c7f1e46ae18c5d167a5e50493faf48640425245ca2131cfa`

After masking only original size/relative-call immediates and, for
the 86-byte family, original vtable pointer DWORDs, every member of
each group is byte-for-byte identical. The two complete original
normalized instruction template SHA256 values are:

- 60B: `7b4cd8eb8b67ef453cf3e8c586708b78fb34e6fff0bf5e4c5b02c36b25e1ca3d`
- 86B: `f72e0856a850946a80e7332dd14ed37c15150062e8b154c940a81a9b4e8e0a06`

## Source-only code and source audits

`include/sin_compat/original_client_canonical_creator_bodies.hpp`
lists the 118 original native creator class ordinals and exact
constructor function VAs as **source labels**. The C++17 reference
provides an immutable, fail-closed classification with original
requested allocation size, body length, number of inline vtable
writes, original return adjustment and original virtual byte offset.

`research/tools/audit_sin_original_client_canonical_creator_bodies.py`
checks public phase1716/1718 original-class and allocator metadata
and validates exactly 68+50 source class/constructor identities.
Private `--client` mode additionally SHA-pins all 118 complete original
x86 function bodies against the SHA-pinned unchanged retail client,
checks original E8 allocator/constructor destinations, source vtable
Dword pointers and both precise original RET/INT3 boundaries.
Original executable function bytes are NOT committed to GitHub.

Public check:
`python research/tools/audit_sin_original_client_canonical_creator_bodies.py --self-test`

Owner-private original-only check:
`python research/tools/audit_sin_original_client_canonical_creator_bodies.py --client /private/client.dll`

`tests/test_original_client_canonical_creator_bodies.cpp`
cross-checks 118 source classifications against original class list
and allocation sizes, 68 unique external constructor targets, six
shared inline constructors, exact virtual/return offsets, 24
deferred common creator bodies, seven original special entity creators
and all 42 temp-event getters. Every native32 ClientClass record
continues to have NULL at original creator function slots +0 and +4.
Strict C++17, Linux ASan/UBSan, Windows x86 and Android CMake CI
are wired to this reference test.

## Real runtime boundary

Complete 60B/86B entrypoint SHA does not mean original constructor
implementation is ported. Three of the six shared inline targets also
occur among the 68 class-specific targets. All 71 distinct constructor TARGET function
bodies and their engine dependencies still require analysis, as do
native vtables, object ownership and real receiver callbacks.
VClient011.slot5 remains intentionally guarded. No actual engine
gameplay, Windows map load or Android/Quest 3 startup is established.
