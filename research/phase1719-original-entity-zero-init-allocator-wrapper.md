# Phase 1719: original 2006 PE32 40-byte zero-initializing allocation wrappers

## Source-backed helper inspection

Unchanged user-owned original SiN Episodes client.dll:
SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Phase1718 authenticated the first 11 x86 bytes of 142 original
entity creator routines. They all CALL original native helper
0x240C4FF0 with an immediate requested allocation byte count.

That original helper is a **complete 40-byte method**:
SHA256 e9428fee2e9eccc2fb344b2d86b9e238e56d5ad2b12cc128db314f7416513e9d.

Its actual source behavior:

  PUSH ESI; MOV ESI,[ESP+8]; PUSH EDI
  PUSH ESI; CALL 0x2416F8F0
  MOV EDX,EAX
  MOV ECX,ESI; SHR ECX,2; XOR EAX,EAX
  MOV EDI,EDX; REP STOSD
  MOV ECX,ESI; ADD ESP,4; AND ECX,3; REP STOSB
  POP EDI; MOV EAX,EDX; POP ESI; RET

It calls an ORIGINAL allocator service at native 0x2416F8F0.
That service's internal vtable dispatch / actual heap-ownership
semantics are not established by this 40-byte helper alone.
The helper itself has no null-pointer check, so a failed allocation
could trigger the original x86 REP STOS behavior on a null pointer.
The portable preflight deliberately refuses allocation failure
instead of trying to reproduce an unsafe fault.

The original helper zeroes DWORDs for floor(size/4) and bytes
for size modulo 4, then returns its original allocated pointer.
The 142 currently recovered entity request sizes are divisible
by four, but tail handling is still part of the authenticated
native method.

Three adjacent original functions at 0x240C5020, 0x240C5050
and 0x240C5080 are each also exactly 40 executable bytes and
are the same zero-initializing algorithm; all four are followed
by eight INT3 bytes and call 0x2416F8F0. Their whole independent
SHA256 digests, in ascending source VA order:

- 0x240C4FF0: e9428fee2e9eccc2fb344b2d86b9e238e56d5ad2b12cc128db314f7416513e9d
- 0x240C5020: c3e3bd04c0614fb56f3111181075c0c8e6d85f7e2b16f165d1b971a36cfce502
- 0x240C5050: dac60fe2bee174356d2923ebdac3777e89b621f2b87bb29098e8c158e6d74b7a
- 0x240C5080: b9cd8e7a18106e5363019f1da995542a7ae2da1909a136f588e561ca6bb877e1

The four original byte sequences differ only in the relative
CALL displacement, consistent with one underlying original allocator
address at 0x2416F8F0.

## Safe synthetic host-memory preflight

Source code:
include/sin_compat/original_client_source_zero_init_allocator.hpp

This does not call original Win32 allocation pointers or create
actual entities. It offers two narrow host-owned operations:

- ZeroOriginalSizeIntoOwnedBuffer checks that a caller-provided
  std::vector<uint8_t> has at least the requested bytes, then
  zeroes 32-bit DWORD units plus 1..3 remainder bytes using a
  safe host implementation of the original source algorithm.
  It preserves bytes after the requested extent and fails without
  writes if storage is short.
- PrepareSyntheticOriginalEntityZeroedBytes requires an exact
  original Phase1718 (ClientClass ordinal, original creator VA)
  for one of the 142 authenticated routines. It prepares a NEW
  host-owned zeroed vector with the exact source allocation size
  and swaps it into the caller only after success. All seven
  special entity creators and all 42 temp-event getters are
  refused; invalid class VAs and source addresses are refused.
  This vector is not a native entity instance, and contains
  no original constructor-initialized fields or vtable pointers.

All exposure/creation helpers remain false. All native32
ClientClass +0/+4 creator callback fields remain null.

## Source auditing and tests

Public original-free:
python research/tools/audit_sin_original_client_zero_init_allocator.py --self-test

Optional read-only original-owner source:
python research/tools/audit_sin_original_client_zero_init_allocator.py --client /private/client.dll

The private mode SHA-pins all four whole 40-byte functions, all
four 8-byte INT3 boundaries, individual REP STOSD/STOSB opcodes,
verified relative CALL target 0x2416F8F0 and bytewise equality
of the instruction sequences except differing relative call
displacements. Public mode also validates the pinned 142 original
creator-size manifest and checks fabricated-function refusal.

Native source regression:
tests/test_original_client_source_zero_init_allocator.cpp

The test covers arbitrary requested byte counts 0..129,
correct original DWORD/tail behavior, preservation of bytes
outside the zeroed range, all-or-nothing refusal on short memory,
142 exact host-zeroed sized buffer results, 7 special creators,
42 temp creators and wrong source-VA refusal, and preserved null
creator fields across all original native32 ClientClass objects.

Linux strict C++17 and ASan/UBSan, Windows MSVC Win32 and portable
Android CMake builds test the safe source reference implementation.
No real original game DLL was overwritten, no retail startup,
Source entity factory ABI, genuine heap ownership, vtable, network
deserializer or Android/Quest 3 gameplay is established.

## Actual next runtime boundary

The remaining work is substantial: reconstruct each source
entity constructor chain and native 2006 object class layout,
the original allocator service's lifecycle and ownership,
native event static-object constructors, and executable
receive-property callbacks. Only after matching these requirements
can VClient011.slot5 return a safely executable ClientClass
linked list to the actual original engine. The retail guard
remains in place.
