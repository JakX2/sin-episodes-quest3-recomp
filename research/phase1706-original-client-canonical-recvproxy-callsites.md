# Phase 1706: original 2006 PE32 canonical RecvProxy identities and safe ABI refusal

## Context

The last authentic original commercial-engine session remains a guarded
independent replacement PE32 `client.dll` connection, stopping deliberately
at `VClient011.slot5` rather than returning an invalid native class list.
No retail game startup, packet decoding, Android gameplay or VR rendering
is claimed by this phase.

Original owner `client.dll` is SHA256-pinned to
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
The original source constructor callsites are read privately and matched
to the existing 211-table/1,110-property original-static network CSV.
No proprietary executable bytes are committed.

## Exact source callback classification

The original 1,110 static network properties comprise:

| Native constructor class | Properties | What is established |
| --- | ---: | --- |
| Canonical original 2006 RecvProp constructors | **1,082** | Exact native source callback VAs from source push/call operands, original 60-byte descriptor type and member offset, +8 constructor flags **zero** |
| Canonical callbacks using seven standard native proxy methods | **999** | Source callback identity points to original byte/word/DWORD/float/vector/string/datatable receiver routine |
| Canonical callbacks referencing custom original code | **83** | Original source function identity only, **not yet executable** |
| Native type4 array alternative constructors | **13** | Independent original array callbacks/count/stride route; callback identity not part of this canonical +32 mapping |
| Native type5 generated outer constructors | **14** | Special nested table creation path; missing callback execution not counted here |
| Specialized animation overlay constructor | **1** | Original +44 zero known, **native callback and child-table semantics still blocked** |

**Source construction semantics**:
the ordinary native type0/1/2/3 descriptors keep their
`RecvVarProxyFn` pointer at descriptor `+32`, and static type5
datatable descriptors keep their `DataTableProxyFn` at `+36`.
Source flags at descriptor `+8` are zero across all **1,082**
ordinary original constructor routes. This is constructor-proven
initialization, not proof that no later subsystem may mutate flags.

The seven source-standard methods are:
`0x24126C70` (byte), `0x24126C80` (word),
`0x24126C90` (integer DWORD), `0x24126C40` (float DWORD),
`0x24126C50` (three-DWORD vector),
`0x24126CA0` (bounded source string receiver) and
`0x24126CE0` (datatable receiver).
Phase251 already contains independent C++17 reference copies for the
first **five** primitive receiver methods. That reference is
**not** a production network proxy runtime; Phase1706 does not
duplicate it or claim source packet serialization.

The 83 custom assignments include entity-handle remapping,
movement/network interpolation, color, camera/FOV and game-specific
callbacks. Their real source code behavior is not guessed or replaced
with a raw copy. The native x86 function VAs cannot be called on
Android or used as pointers inside the reconstructed independent DLL.

## Committed source catalog and hashes

`include/sin_compat/original_client_source_proxy_catalog.hpp`
stores the 1,110 source-order property callback labels using a
**one-byte index** into 44 distinct original 32-bit PE function
addresses, with index zero reserved for **28 separate constructor
routes**. It is the same original 191-class-linked table order followed
by 20 auxiliary original-table VAs as the prior research phases.
The catalog never exports callable pointers.

Original-source ID stream: **1,110 bytes**, SHA256
`e74fe1d930d61ba70651fcb0a22dd6d13df222633d83a3812e2ddaffbff7b4f3`.

Expanded little-endian 1,110-DWORD source callback VAs with
unclassified values represented by zero: SHA256
`eb7f8427a064fe266cde69b8d587bc3b7835653da8c7b9f11f8fc9fa8ae9fc1f`.

Native `static_assert` verifies exact 1,082/28 route census,
999 standard/83 custom, original numeric type association,
source executable VA range and no property type4 masquerading as
a canonical primitive callback. The optional
`research/tools/audit_sin_original_client_source_proxy_catalog.py`
owner-private mode reconstructs all callback operands directly from
the unchanged original client PE, authenticates helper function bodies,
original descriptor names and static property order, then demands
byte-exact parity with the public catalog.

```sh
python research/tools/audit_sin_original_client_source_proxy_catalog.py --self-test
python research/tools/audit_sin_original_client_source_proxy_catalog.py --client /private/client.dll
```

## Owned host-native image and regression tests

`Image::OriginalSourceConstructorProxyVA(table,slot)` exposes only
**source-address labels** for research. The simulated private arena
never writes those original executable VA values into its native
callback slots. `AuditVisitedSourceTable` refuses any nonzero
callback pointer in the canonical `+32` or `+36` fields and
refuses nonzero `+8` flags, while preserving the original 191-root
reachability distinction: 224 tables / 1,796 descriptors, versus
225 / 1,801 exhaustive research inventory.

The existing Windows/Linux/Android CTest
`test_original_client_static_recvprop_names.cpp`
counts every original callback classification and mutates one valid
primitive descriptor and a type5 datatable descriptor by inserting
their **genuine original PE32 function addresses**. Both must fail
closed, without ever executing those pointers. Assertions continue
to require `CanExposeToRetailEngine()==false`. Linux strict C++17,
ASan/UBSan and Windows native builds are wired through existing
workflows. No original proprietary DLL is checked into GitHub.

## Next implementation boundary

Recover enough original-source callback behavior to implement a
safe, executable **host-owned C++ callback ABI** and test real
resulting state changes (starting with direct primitive and string
receivers and source-safe specialized callbacks). Complete the
special animation overlay's table/callback ownership and the actual
191 `ClientClass` entity/temp-event creators before considering
the retail `VClient011.slot5` guard. The user's excluded HL2
equivalent physics, surfaces, props/models and renderer work remains
untouched.
