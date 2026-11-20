# Three SHA-authenticated original 2006 soundscape child-array cleanup helpers

## Source identity

This stage extends the separate original soundscape auxiliary cleanup
and vtable wrapper work in PRs #506 and #507, not the parallel
startup-evidence automation PR chain.

All bytes were checked read-only against the user's exact original
2006 client.dll SHA256
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
from their original bin.zip archive.

The original vtable-slot11 deleting-destructor-style wrapper calls
four child arrays in order B4, A0, 1C, 08, using **three distinct
but structurally identical 83-byte methods**:

| Original helper | VA | Complete 83-byte SHA256 |
| --- | --- | --- |
| Child at this+0x08 and this+0x1C | 0x240F3970 | da7aa4756e4619c54c29a8a2093621e74b2eec7bb687b9bff8d1cd89c676c005 |
| Child at this+0xA0 | 0x240F3A20 | 07714c3f94df41a0385d66d5816262ee1a366200ad9f20739530459b79f16f9f |
| Child at this+0xB4 | 0x240F3AD0 | 1ad7502a6062d8dabbd7ef7b3fa766ba73209c2787ed6215da6ab2592016d071 |

**The 83-byte instruction bodies are byte-identical after normalizing
only two CALL rel32 displacement fields at offsets +23 and +60**.
Both CALL sites in each helper resolve to original allocator
free function 0x2416F940. Every other instruction byte is identical.

The private auditor
audit_sin_original_soundscape_child_cleanup_trio.py --archive
checks all three complete original method hashes, every
selected opcode witness, six resolved free CALLs, and full
normalized-body equality. Its public CI self-test uses
fabricated instruction windows, rejects changed branch/opcode
and allocator-target witnesses and does not include original
client DLL bytes.

## Source-observed child field behavior

For each base child pointer (this+0x08, +0x1C, +0xA0, or +0xB4),
use local child-relative fields:

| Child-relative offset | Source use |
| --- | --- |
| +0x00 | owned buffer pointer |
| +0x04 | element count cleared when flag permits |
| +0x08 | sentinel flag tested twice against -1 |
| +0x0C | count cleared unconditionally |
| +0x10 | pointer mirror written after first pass |

The 83-byte source sequence:

1. Compare flag +8 with -1; unconditionally write count +C=0.
2. If flag not -1: test pointer+0; when non-null, call
   0x2416F940 once and write pointer+0=0; whether pointer
   was non-null or null, write count+4=0.
3. Compare flag+8 with -1 *again*, reload pointer+0
   and unconditionally write mirror+10=pointer+0.
4. If flag not -1: retest pointer+0; if non-null, call
   original allocator free and zero the pointer; always
   write count+4=0; then return.
5. If flag -1: preserve pointer+0 and count+4, but
   count+C is still zeroed and mirror+10 receives
   original pointer+0.

**Why two free instructions but at most one actual free:**
In a normal consistent helper invocation, the first valid
non-null pointer branch frees and explicitly zeroes
pointer+0 before the second check. A valid null input also
stays null. Therefore the second allocator call is statically
present but cannot be reached on the straightforward
single-threaded source path. This conclusion does not
extend to memory corruption, concurrent unsynchronized
mutation or a mutated binary.

## Independent owned reconstruction

client_soundscape_original_child_array_owned.h defines one
typed handler for all three variants. Its only state is
independently owned integer counts, a sentinel flag,
pointer-*presence* booleans, mirror presence and a
monotonic revision number. It never stores or dereferences
a retail engine allocation.

The source-exact ordered event plan contains the flag
comparisons, count clears, conditional allocator CALL
placeholder, conditional pointer zero store, mirror
copy and second branch. The event order and per-variant
original addresses are authenticated before any
independently owned changes occur.

The model has at most one original-allocator placeholder
per valid cleanup, **not two actual frees**, and it never
calls original 0x2416F940. Source buffer bytes/addresses
and actual original class layout remain outside the model.

Native MSVC x86 tests cover:
- sentinel invalid flag with a live pointer and retained +4 count;
- valid live buffer for all three variants and one only free event;
- valid null buffer, no free and no explicit pointer-zero store;
- two source writes of count+4 and unconditional +C clearing;
- exact mirror+10 result from post-first-pass pointer;
- corrupted allocator target, forged event counts and source order;
- unknown helper variant and stale plan after new owned lifetime.

This reduces unknown child-array teardown behavior but
does not instantiate or destroy actual original Source objects.
Normal independent VClient011::Init still fails closed,
original soundscape audio playback is not functioning,
and no Android/Quest 3 gameplay is claimed.
Last user-observed real Windows checkpoint remains 0x53494E74.
