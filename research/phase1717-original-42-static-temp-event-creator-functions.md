# Phase 1717: all 42 original 2006 PE32 static temporary-event creator functions

## Source-backed breakthrough: original event creators do not allocate

The original unchanged SiN Episodes client.dll is SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

From the 191 source-authenticated ClientClass nodes, exactly
42 have a native temporary-event creator at native ClientClass+4.
All 42 original executable getter bodies are exactly SIX BYTES:

  MOV EAX, <constant address of an original writable static object>
  RET

Every one of these 42 six-byte functions is immediately followed
by exactly ten original INT3 padding bytes. The pointer operand
is unique per event; all 42 original address immediates lie in
original PE32 .data (virtual region 0x242F3000..0x2438F320).
They are **not executable callback addresses** and are not
process-independent object handles. The original process must have
initialized those static objects and their vtables before calling
the creator functions.

The SHA256 of all 42 original complete six-byte methods
concatenated in authenticated VClient011.slot5 class-list order is:
266efe94cd2323cd097a9ba2af229d5f57ca547889e4dc0fe244a04374a4db30.

This demonstrates a different original creator family from the
149 entity-type ClientClass creators: most entity creators allocate,
construct and initialize objects, whereas all 42 temp-event creator
functions simply return a known original static-object pointer.

## Original immutable source records

Research CSV with source ordinal, original ClientClass name,
original creator function VA, original static-object VA, and native
CRT initializer index:

research/manifests/original_client_42_temp_event_static_getters_phase1717.csv

LF-normalized SHA256:
b40182b4b5257a02bcce50b85da6123665b551b2f6615b3f759e0dccc23f4b50.

This is cross-checked against the Phase1716 191-class creator map:
42 unique events, 42 unique source functions, 42 unique original
static target addresses. No original executable bytes are committed.

A second private-source audit independently SHA-pins the entire
unchanged original DLL, confirms all 42 exact MOV EAX immediate +
RET six-byte functions, confirms ten original INT3 padding bytes
per function, and verifies the native source CRT class registration
and target identity. Public source-only mode can run without the
user's proprietary game files.

Public source audit:
python research/tools/audit_sin_original_client_temp_event_static_creators.py --self-test

Optional owner-private read-only original source check:
python research/tools/audit_sin_original_client_temp_event_static_creators.py --client /private/client.dll

## Bounded portable host-only reference

Header:
include/sin_compat/original_client_temp_event_static_creators.hpp

This provides 42 compile-time source mappings from class ordinal
and original creator VA to its source original static-object VA.
These source addresses are **labels only** and can never be used
as real native function or object pointers on Win32 replacement
client.dll, Linux or Android.

The optional host-modeled getter accepts a separately authenticated
synthetic token into an owned byte-vector. The host token must be
non-null, four-byte aligned, beyond reserved synthetic 0x1400,
and inside the actual owned buffer. A wrong creator/class ordinal,
unbound token, original PE32 data pointer, out-of-bounds token
or unaligned token is refused without modifying any host data.
The method only returns a validated token; it does NOT recreate
the original static object's fields, native vtable, constructor,
initialization order, or network callback side effects.

Native test:
tests/test_original_client_temp_event_static_creators.cpp

Tests all 42 unique original class/creator combinations and the
source native singleton address, correct bounded return tokens,
unbound and unsafe-address refusals, alias/short-buffer failure,
no mutation of owned bytes, and exact original ClientClass kind.
Critically every native32 ClientClass record still has both
entity creator (+0) and temp-event creator (+4) set to NULL.
Nothing becomes an executable callback simply because the source
getter's 6-byte behavior is understood.

Strict C++17, Linux ASan/UBSan, Windows MSVC Win32 and portable
Android CMake builds include the tests; real executable retail
original game modules are not replaced.

## Next actual creator ABI

A first-instruction scan of the 149 original ENTITY creator
function addresses shows:
- 142 begin with PUSH ESI; PUSH original object size;
  CALL original allocator 0x240C4FF0.
- Three call another original allocation helper 0x2416FCF0
  with small allocations.
- Four require individual analysis: CWorld, CBaseEntity,
  CRagdollPropAttached, CInfoTeleporterCountdown.

Those counts describe ORIGINAL method prologues, not reconstructed
entity creation. The entity factory virtual method, object
size/layout, constructor dependencies and native lifetime still
need to be source-authenticated before any original engine
creator pointer can be published.

All CanPublishRetailCreatorPointers() and
CanExposeToRetailEngine() guards remain false. The retail
VClient011.slot5 unsupported diagnostic stop remains enabled.
No actual engine packet decoding, map boot, Android/Quest 3
gameplay, physics, props/models or renderer is claimed.
