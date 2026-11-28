# Phase 1704: all 1,110 original 2006 PE32 static RecvProp names in source-exact order

## Actual Windows executable boundary remains guarded

The latest proven original-engine test loaded the independently reconstructed
PE32 `client.dll`, connected 21 mandatory engine interfaces, sound, prediction,
entity list, centerprint and leaf system, then deliberately stopped on
`VClient011.slot5` (unsupported native ClientClass network registry).
This phase **does not** return pointers to the commercial engine or enable
Windows/Android gameplay. The owner-original retail `client.dll` SHA256 is
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## Original 2006 property constructor evidence

The original 211 statically constructed RecvTables declare exactly **1,110**
60-byte RecvProp slots. Of these, **1,082** are canonical original constructor
calls with a literal native property name, destination RecvProp and original
helper target. The remaining **28** require their actual original alternative
paths: **13 type4 arrays**, **14 type5 dynamic nested-array constructors**,
and **one specialized `DT_OverlayVars.m_AnimOverlay`**. Every property
name is reconstructed in the original **191 class-table linked-list order
followed by 20 source auxiliary tables ordered by actual native VA**.

All **1,110/1,110** static properties now have exact original names;
those contain **665 distinct source strings**, including the exact unusual
`DT_Team` name `"player_array"` with its two embedded quotation marks.
The census is source numeric type0..5 **414 / 300 / 164 / 12 / 13 / 207**.
Of the 1,110 native field `+44` records, 1,082 canonical fields plus
14 generated parents plus 13 special type4 constructor-zero fields yield
**1,109 source-accounted**. Only the specialized animation-overlay field
remains explicitly guarded. None of those counts imply executable RecvProxy
or network serializer code.

The complete public source-name CSV:
`research/manifests/original_client_static_recvprops_phase1704.csv`
contains original table ordinal, table name, property slot and name,
native type tag, source `+44` value/semantics and original x86 constructor
callsite labels. These addresses are **source provenance only**, never
dereferenceable host or Android pointers.

Independent integrity fingerprints:

- Ordered original `table|slot|name` sequence, joined with LF but no final LF:
  SHA256 `5a6420cc24c1ed11aa6babcdc98a035c04985eefc4a8c0a6891e8466ac918dd4`.
- Matching 64-bit FNV-1a for native C++ original-free checks:
  `0xb37b011a57f1d08b`.
- Public CSV UTF-8 bytes with LF line endings:
  SHA256 `91099b2b87c1ea84cc73a2a61d1abe1f85a605d1a846c54e8a74fbec9e359f4b`.

## Native C++ integration

The safe 32-bit owned-token `Image` gained `AttachStaticSourceNames()`
taking a strict CSV input. It accepts all **1,110 source-order original
names atomically** after checking the type tags, array and canonical
offsets already present, exact source-order FNV64, **665 distinct names**,
valid printable ASCII and correct native descriptor boundaries. A rejected
manifest does **not** partly populate the image. On success, the arena
receives owned copies of name strings at each native RecvProp `+0`.
`AuditVisitedSourceTable()` then refuses a mutation to a reached
original name, even if the pointer still refers to another owned
string. No original retail string pointers, class creation callbacks,
proxy functions or serializer objects are exposed.

The test `tests/test_original_client_static_recvprop_names.cpp` uses the
exact original 191-class catalog and source CSV to verify every copied
property name, all 665 unique strings, the `"player_array"` special
source label, and the same original graph traversal:

- Retail-equivalent 191 rooted classes: 224 tables / 1,796 descriptors.
- Complete 211-table research inventory: 225 tables / 1,801 descriptors.

The one auxiliary table `DT_Animationlayer` remains absent from the
original class-root traversal. Mutating its name is rejected by the
full-inventory walk but does not change the actual source-root walk.
Tests also mutate source names, point to other valid but wrong
in-arena names, refuse malformed CSV metadata and test
transactional rollback. Linux strict C++17 and ASan/UBSan tests and
MSVC Win32 x86 CTests are wired to the phase. Android compilation
can compile the same original-free header/test, but **Android Source
engine/Quest 3 gameplay is not claimed**.

The Python original-free SHA audit:

```sh
python research/tools/audit_sin_original_client_static_recvprop_names.py --self-test
```

The optional private original-DLL source audit:

```sh
python research/tools/audit_sin_original_client_static_recvprop_names.py --client /private/client.dll
```

The latter validates the original SHA-pinned PE32 constructor calls and
name literals without committing proprietary original executable bytes.

## Remaining real engine blockers

Full original 60-byte static RecvProp flags and proxy methods; specialized
overlay callback and allocation semantics; 20-byte RecvTable lifetime and
mutable engine side effects; 191 actual ClientClass creator/temp-event
functions; full original Source networking ABI and packet serialization;
and correct platform-independent ownership. All
`CanExposeToRetailEngine()` remain **false**, and the Windows owner
`VClient011.slot5` remains deliberately guarded. User-deferred HL2
equivalent physics, models/props, surfaces and renderer work remain excluded.
