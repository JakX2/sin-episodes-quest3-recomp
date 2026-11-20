# Phase 475: genuine owner-retail Stats hooks and independent bit cursor

## Authenticated retail owner results, November 20, 2026

Private owner report: SiN_FileSystem_Test_Results_20261120-025708-0fc6c96503.zip
SHA256: d7e55c218ac660c7ada4a3dcbb84715b41059c53adcebd23ea054ff65c1e8c0f.
All eleven report items were inspected directly. No private original retail DLL,
sensitive path, raw game asset or per-session trace is committed to the repo.

| Phase | Original Windows engine exit | Grounded observation |
|---|---|---|
| Presence | 0x53494E68 | VFileSystem012 FileExists both resources |
| Read | 0x53494E6B | Open/Size(handle)/Read/Close and 62:8:156:66 |
| Owned Stats hooks | **0x53494E74** | Original rb/GAME and rt/null staged reads, 66-ID Stats catalog, independently owned user-message IDs **27:28:5** |

The last session emitted:
- engine-fs-staged-counts: 66:62:8:156:66
- engine-fs-staged-stage: 7:7
- engine-fs-staged-result: owned-staged-verified-not-engine-init
- engine-fs-hooks-ids: 27:28:5
- engine-fs-hooks-count: 3
- engine-fs-hooks-result: three-owned-hooks-bound-not-dispatched
- CLIENT_ENGINE_FS_HOOKS_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF.

All three process transcripts contained OWNER_INDEPENDENT_RESTORATION:
VERIFIED_COMPLETE; SUMMARY confirmed VERIFIED_AND_REMOVED_DISPOSABLE_COPY.
This is genuine original 2006 Windows engine evidence for original ABI
filesystem reads and independently owned hooks. The hooks were **NOT**
registered in original engine CUserMessages, no real game packet was
received, no normal VClient011 Init completed and no menu/map/game boot
was reached.

## Next reconstruction dependency: bit-accurate Source bf_read

The SHA-authenticated client Stats handlers call ReadUBitLong(16) for
their first stat ID field. The older independent InspectHeader function
was only valid on **byte-aligned** buffers. Public Valve Source SDK 2004
tier1/bitbuf.h reads from bit (cursor & 7) of byte (cursor >> 3) and
packs results least-significant-bit first. A real message cursor may
begin at a non-byte offset.

The new independently authored client_stats_bitbuf_cursor.h supports
explicit byte/bit lengths and an initial cursor position; valid bit
widths 1..32; a bounded 256 KiB packet limit; LSB-first reads; cursor
clamping and overflow on attempted raw overrun; and transactional
InspectBitHeader parsing that leaves a caller cursor untouched when
a message is refused. This is NOT a binary-layout-compatible original
bf_read object or an engine dispatcher.

The new inspector recognizes original user message IDs StaticStat=27
(static stat indices 0..54), DynamicStat=28 (dynamic 55..65) and
ResetHUD=5 (one-byte envelope).

### New independent owner-original value-field discovery

The connected Drive bin.zip was examined **read-only**. Its client.dll
and server.dll SHA256 exactly match the two authenticated owner-retail
hashes already present in the repository. The original DynamicStat
entrypoint (0x241FB7E0) delegates to 0x241FB370, which first invokes
ReadUBitLong(16) at 0x240A5600, then an allocated string reader at
0x240A5740. That string helper calls core ReadString at 0x240A5670
through a **0x800-byte (2048) stack buffer**, then the dynamic record
update reaches virtual method **+0x4c**. StaticStat delegates through
0x241FE590 to the same vtable offset after its 16-bit ID.

The stronger SHA-bound auditor now hashes and authenticates the
four new helper bodies and the exact callsite/virtual-call edges. The
public repository contains the auditor and *derived metadata only*,
never an extracted original executable, disassembly, decompiled body,
or raw commercial assets.

The new InspectDynamicText method independently consumes the recovered
16-bit ID plus a **NUL-terminated 8-bit bit-buffer string**, with
strict <=2047 data bytes + terminal NUL and transactional failure. It
does not guess the later per-type stat mutation semantics. The native
fixture covers unaligned messages, empty strings, exactly 2047 payload
bytes plus NUL, missing/oversized strings, invalid IDs and independent
cursor copies.

The original functions have unusually broad ID conditions: StaticStat
skips IDs 55..65 while the dynamic helper ignores valid nonnegative
IDs below 55; the original callbacks do not visibly reject all IDs
>=66. Our independent parser **deliberately refuses IDs >=66** until
those error cases are completely understood, rather than risking a
bad pointer dereference.

The native original-free synthetic MSVC x86 test covers bit offsets
0..8 including word boundaries, 32-bit reads, 16-bit ID bounds,
one-byte ResetHUD, incomplete input, 256 KiB maximum, invalid starting
offsets and copied independent listener cursors. The rebuilt game's
slot33 DispatchUserMessage remains intentionally fail-closed until the
original bf_read bridge and stat value decoder are genuinely rebuilt.

This bitcursor is SDK-grounded and tested against synthetic bitstreams,
not against original network traffic or retail game packet dispatch.

## Original 16-case stat value-type factory (additional SHA-grounded recovery)

A further **read-only audit** of the exact original client.dll reveals
the stat-value allocation factory at **0x24205BC0** and its dispatch
jump table at **0x24205DF0**. It checks a type discriminator against
15, jumps through exactly **16** concrete case labels for types 0..15,
and returns a null result for values outside that switch. Thus the
network stat ID (0..65) and the **factory value-type discriminator**
(0..15) are **different** dimensions. There is no basis yet to assign
descriptive semantic names to the 16 factory codes.

Each case constructs a virtual value object and its vtable entry at
offset **+0x4C** contains a small thunk that invokes another virtual
method on a supplied reader. The recovered byte offsets on the reader
vtable (NOT bit widths and NOT yet named decoding functions) are:

| Factory types | Reader virtual byte offset | Original allocation size |
|---|---:|---:|
| 0 | 0x24 | 16 bytes |
| 1 | 0x20 | 16 bytes |
| 2 | 0x14 | 20 bytes |
| 3 | 0x10 | 20 bytes |
| 4, 5 | 0x0C | 20 bytes |
| 6 | 0x08 | 20 bytes |
| 7 | 0x1C | 20 bytes |
| 8, 9, 10 | 0x18 | 28 bytes |
| 11 | 0x04 | 32 bytes |
| 12, 13 | 0x20 | 16 bytes |
| 14, 15 | 0x20 | 20 bytes |

Types **8, 9, 10** share the same constructor, virtual table and
reading method but pass different literal 32-bit initialization values:
**0x3F800000, 0x3C888889, 0x3991A2B4** respectively. Type **11**
passes 0x3F800000 to a different constructor. These are *observed
configuration argument bits*, not an assertion that the corresponding
wire payloads are ordinary float32 values.

Another original dynamic-record wrapper constructor at 0x24206280
installs vtable 0x242AF854. Its own +0x4C method at 0x242063C0
forwards to the wrapped value object's +0x4C entry. This explains
the two levels of indirect dispatch visible in the original callback.

New independent files:
- client_stats_value_factory_contract.h: safe, bounded original
  factory metadata, with NO original pointer execution.
- audit_sin_original_stat_value_factory.py: SHA-authenticated private
  owner audit of all 16 jump-table targets, allocation sizes,
  constructor-vtable assignments, +0x4C method pointers and actual
  reader-slot forwarding thunks. Public CI uses only its synthetic
  self-test and never reads commercial binaries.
- test_client_stats_value_factory_contract.cpp: native Win32 fixture
  that verifies each type, allocation size, reader slot and rejection
  of unknown type discriminants.

**Local original-binary audit:** all **16/16** factory cases passed
exact pointer/constructor/thunk verification against the private
SHA-authenticated retail client.dll, with **zero mismatches**. This
proves type-dispatch metadata, not the meaning of each reader vtable
method. Those target methods and exact value mutation rules must be
reconstructed and tested before they are enabled in runtime.

## Further SHA-grounded recovery: all 66 stat IDs mapped to wire types

The retail client contains a compile-time **66 x 12-byte table** at
**0x2431E660**. The helper at **0x241FD220** returns the first
32-bit field of record `statId`, using the addressing expression
`0x2431E660 + statId*12`. Its value selects the **0..15** factory
cases described above. The adjacent helpers at 0x241FD230 and
0x241FD240 read the second and third fields, whose semantics are
not yet proven. Their original numeric values are kept unchanged in
the independent registry, not given guessed names.

All **66** records, 792 bytes, have original SHA256
`63c1ee30f9b2a021115150b8b98a298530f4f2e0ebd621fca3ff7132d7b80ffb`.
Our independently authored `client_stats_original_type_registry.h`
contains all 66 triples; a compile-time canonical little-endian
FNV64 check `0xfb1f4af74099b337` verifies the **entire** original
dataset, not just spot checks. The SHA auditor validates the original
file's exact bytes, accessors and type distribution privately.
Only 11 of the 16 possible type codes occur in these 66 records.

Original type-code frequency across all 66 stat IDs:
- type 0: 38; type 1: 3; type 2: 3; type 5: 4
- type 6: 1; type 7: 5; type 8: 3; type 9: 3
- type 10: 1; type 11: 3; type 14: 2

The actual **live wire** reader is MSVC RTTI class **CStatReader**,
vtable **0x242AE06C**, distinct from the file/persistence class
**CStatBufferReader** at **0x242AF06C**. All nine CStatReader methods
selected by the 16-type factory were audited against original code.
Their underlying source bit-buffer methods are either:

- **0x240A5630**, 32 raw bits interpreted as IEEE754 float;
- **0x240A5620**, 32 raw bits interpreted through signed-integer x87
  `FILD` conversion to float.

Factory **type 2** reads two integer32 fields; type **3** reads two
float32 fields. The remaining types read one 32-bit raw field. This
is authenticated by 9/9 original virtual-method targets and the
correct count of their original 32-bit read call edges.

New `client_stats_wire_value_decoder.h` performs bounded, independent
bit-accurate extraction of those fields after the known message
header and optional DynamicStat string. It returns **raw 32-bit
patterns** rather than changing actual stat records, because the
original value-object mutation semantics are not yet fully recovered.
Its main entrypoint `InspectFrameForRegisteredId` reads the stat ID,
looks up its **authenticated original type** from all 66 records, then
invokes the independently authored numeric decoder. It never guesses
the type from user-controlled packet bits and refuses malformed IDs.

Native original-free test `test_client_stats_original_type_registry.cpp`
now exercises ALL **66/66** mappings and static/dynamic messages,
the correct one/two-field widths, unaligned input, truncation, unknown
IDs and transactional rollback. Further wire reader fixtures exercise
all 16 possible types, not just those used by the original registry.

**Evidence separation:** these are verified structural facts from
the original *retail DLL* plus independent native synthetic payload
tests. We have **NOT** processed a real multiplayer/gameplay packet
in the original engine. This new decoder must not be wired to the
engine's live slot33 until the original bf_read ABI and all necessary
stat update side effects are reconstructed.

## Next tasks

1. Reconstruct the actual *mutation semantics* behind the
   original value-object reader virtual calls. The factory types,
   66 statID/type associations and 32/64-bit wire field counts are
   now recovered. The post-string dynamic name helper 0x241FFCD0
   and mutation virtuals still need careful original-class modeling.
2. Recover original 24-byte bf_read ABI from SHA-grounded original
   CUserMessages::DispatchUserMessage and Source SDK evidence; build
   a verified, side-effect-safe adapter before enabling real engine calls.
3. Implement bounded, type-specific independent stat value parsing and
   dispatch, then successful normal Windows client initialization.
4. Integrate server entities and network sender/receiver paths for
   independent PC gameplay. Android then Quest 3 are later phases.

The previous retail 0x53494E74 result does not need a duplicate owner
run simply to test synthetic bitcursor fixture work.
