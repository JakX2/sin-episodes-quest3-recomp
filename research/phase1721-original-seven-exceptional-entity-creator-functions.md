# Phase 1721: authenticate all seven exceptional native entity creator functions

Original owner unchanged 2006 PE32 client.dll whole SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

## Source entrypoint classification completed

Phase1718 authenticated the first 11 original executable bytes of 142
entity creator entrypoints, all calling common zeroing wrapper `0x240C4FF0`.
This phase audits the **entire original x86 function body** of each of
the other seven native entity creator entrypoints, including whole-method
SHA, function RET/INT3 boundary, actual allocator argument/call (if any),
original returned networkable subobject pointer offset and original
virtual method dispatch position.

| Class | Original allocation/singleton source | Requested bytes | Return adjustment | Virtual slot byte offset |
| --- | --- | ---: | ---: | ---: |
| CInfoTeleporterCountdown | shared zero wrapper `0x240C4FF0` | 1168 | +8 | +0x94 |
| CRagdollPropAttached | shared zero wrapper `0x240C4FF0` | 3408 | +8 | +0x94 |
| CWorld | source global `0x2433F3F4`, no direct allocation | 0 | +8 | +0x94 |
| CBaseEntity | direct allocator `0x2416F8F0`, manual DWORD zeroing | 1152 | +8 | +0x94 |
| CTEParticleSystem | original raw allocator `0x2416FCF0` | 28 | +4 | +0x30 |
| CTEBaseBeam | original raw allocator `0x2416FCF0` | 76 | +4 | +0x30 |
| CBaseTempEntity | original raw allocator `0x2416FCF0` | 16 | +4 | +0x30 |

The return adjustments identify a pointer to an interior original
networkable/event subobject rather than simply the base allocation.
They matter for an eventual genuine Creator ABI, but they are NOT
safe host pointer arithmetic against original game addresses.

Every one of these seven complete native x86 method bodies is independently
SHA-pinned, with size and padding recorded in the source CSV:
`research/manifests/original_client_seven_creator_exceptions_phase1721.csv`.

LF CSV SHA256:
`24b00c39fbf90a0af79448496cd4f1d91221dc752dc0021ae6a755c7f77f38e7`.

The complete original native method lengths are 110, 116, 40, 76,
88, 54 and 82 bytes in the original class ordinal order.
Each terminates in an original RET before verified INT3 padding.

Native `CWorld` is special: its creator accesses an existing source
global singleton, makes a virtual call and returns the networkable
subobject pointer if present. It does NOT issue an allocation
request within this ClientClass creator method. The global object's
construction and lifetime are still not reconstructed.

`CBaseEntity` directly allocates 0x480 bytes and performs its own
DWORD zeroing, unlike the usual common helper. `CTEParticleSystem`,
`CTEBaseBeam`, and `CBaseTempEntity` allocate small structures through
another source allocator and explicitly initialize vtables/fields.
This phase does not infer unwritten object contents as zero.

Thus the source original entity-creator ENTRYPOINT family is
**142 common allocation requests + six special allocation requests
+ one CWorld global object reference = all 149 original entities**.
This is source-level classification, not 149 working constructors.

## Public source mapping and safe test guard

`include/sin_compat/original_client_seven_creator_exceptions.hpp`
provides original-source class ordinal/VA/strategy/size/allocator-or-global
mapping for the seven exceptional paths. Its constexpr test requires
every one of the source 149 entity classes to belong to exactly
one authenticated source classification, with 148 allocation requests
and exactly one CWorld singleton.

Public source audit:
`python research/tools/audit_sin_original_client_seven_creator_exceptions.py --self-test`

Optional private unchanged owner original-code SHA audit:
`python research/tools/audit_sin_original_client_seven_creator_exceptions.py --client /private/client.dll`

This private audit verifies each complete original x86 function's whole
SHA256, executable length, RET/INT3 end, original PUSH-size/E8 call target,
virtual method call and returned subobject adjustment. No proprietary
original native executable bytes are committed to GitHub.

Native regression:
`tests/test_original_client_seven_creator_exceptions.cpp`.
Requires the exact 142+6+1/149 classification, and verifies all 191
host-owned native32 ClientClass +0/+4 creator callback fields stay NULL.
Linux C++17 and ASan/UBSan, Windows Win32 MSVC, and portable Android
NDK CMake tests are wired.

## Real runtime restrictions

Even though source entrypoint allocation paths are mapped for all 149
entity creators, only the seven exceptional WHOLE functions have been
verified here; 142 generic functions still need their complete constructor
chains, native 2006 vtables, virtual slot implementation and memory
ownership. No original x86 function pointer is executable in the
replacement Windows DLL or on Android. Original VClient011.slot5 remains
guarded and host-owned image callbacks remain NULL.
No user game binaries were overwritten and this is not retail map boot,
real network packet processing, Android/Quest 3 gameplay, renderer
or physics implementation.
