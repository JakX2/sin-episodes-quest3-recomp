# Phase 194: original server cache tree slot allocators

13 November 2026. Owner-original 2006 SiN Episodes: Emergence Windows i386 PE32 binaries. Draft stacked on Phase193 PR #246; no original binaries, extracted game assets or complete function disassemblies are distributed. PC gameplay first, Android second, Quest 3 third.

## Complete original native function evidence

The owner's original DLL fingerprints independently match the existing project provenance:

- server.dll: 6,049,792 bytes, full SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client.dll: 3,592,192 bytes, full SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

| Function | Original VA | Length | Full original function SHA256 |
| --- | --- | ---: | --- |
| Scene slot allocator | 0x221A6890 | 119 bytes | 1e267f1ece8a0520553fbdcececa0c3efaca4d652a2d1d8b30c3906dbea29923 |
| Model-sounds slot allocator | 0x22140900 | 119 bytes | 3733292d698d8ac51d8b814834070a68375630d65d14655b422f5e7d9877395c |

Two new complete native server functions, 238 bytes total. Both complete native bodies are byte-for-byte equal after zeroing ONLY the two CALL rel32 displacement DWORDs at local offsets +0x1D and +0x32. Exact normalized whole-body SHA256: 66d48e1aaa823eeb953c3c70acf9c8095bb601326fe1bad696dd517266d15a51. Both end at RET followed by nine original INT3 alignment bytes.

### Exact original CALL edges

| Role | Scene target | Model-sounds target |
| --- | --- | --- |
| Conditional indexed-slot array growth at +0x1D | 0x221A3A10 | 0x2213AAC0 |
| Reused/free-slot next-node retrieval at +0x32 | 0x221A4D90 | 0x2213E990 |
| Incoming Phase193 callsite | 0x221ACD22 | 0x22146F12 |

The original auditor verifies all four outgoing CALLs and both incoming CALLs. Target routines remain partially opaque. Their semantic labels describe observed call-site behavior, not a complete decompilation of those callees.

## Original native tree header and algorithm

| Offset | Recovered role | Original width |
| --- | --- | --- |
| +0x00 | Phase191 comparator callback pointer | DWORD |
| +0x04 | 24-byte slot array pointer | DWORD |
| +0x08 | Capacity of slot array | DWORD |
| +0x10 | Root index | WORD |
| +0x12 | Live record count (incremented by Phase193) | WORD |
| +0x14 | Free-list head index | WORD |
| +0x16 | Next never-used index | WORD |
| +0x18 | Latest slot-base pointer snapshot | DWORD |

If free-list head equals 0xFFFF, original code compares capacity against the 16-bit next-unused counter, conditionally calls an array growth routine with a one-slot request, returns the previous next-unused index and increments that WORD. If a free slot exists, it selects the free-list head and replaces that head with the WORD at offset +2 in the returned node from its repository-specific helper.

Both paths initialize four DWORDs beginning at 24-byte slot offset +8 with values 0, 0, 0xFFFFFFFE, and 0xFFFFFFFF; copy the current slot-base pointer into the object at +0x18; and return the chosen 16-bit slot index in AX. The live count at +0x12 is NOT changed by this method. Original unchecked pointer arithmetic does not constitute a bounds or allocation-success check.

Phase193 subsequently writes a 16-byte record key into the same slot fields; Phase190 eventually overwrites the fourth DWORD at slot +20 with its record payload pointer. No native record-key serialization or actual Source runtime ABI is inferred.

## New independently authored code and validation

- include/sin_compat/original_server_cache_slot_allocation.hpp: C++17 fixed-size original 28-byte 32-bit tree header, modeled free-list/growth branches and exact slot-field initialization, with host-owned vector and explicit safe callback interface.
- tests/test_original_server_cache_slot_allocation.cpp: verifies both allocation paths, 16-bit counters, sentinels, reuse and integration with the recovered Phase193 key-copy model.
- research/tools/audit_sin_original_server_cache_slot_allocation.py: strict whole-original-DLL and whole-function hashes, 40 sparse x86 witnesses, six exact CALL edges, normalized whole-body comparison and native function boundaries.
- research/tests/test_sin_original_server_cache_slot_allocation.py: eight synthetic negative/mutation test groups that never treat fabricated evidence as original native code.
- Root CTest and native Win32 MSVC 32-bit CTest registrations added.

Local SHA-pinned original binary audit PASS; original-free Python mutation groups PASS (8/8). Local optimized GCC/Clang C++17 -Werror contract tests passed against matching previous-phase header signatures. The actual repository-wide and Win32 CI results must be observed independently.

## Unresolved and next step

The reviewed atlas advances from 183 to 185 distinct original functions (93 server and 92 client). This is NOT a whole-DLL decompilation percentage.

Original vector-growth and freed-node lookup callees, original tree link/rebalance implementation, the filesystem vtable +0xD4 input, engine-owned lifecycle and true Windows executable-hosted reconstructed game-module playback are still unresolved. Continue source-grounded method recovery, but prioritize an original Windows engine startup trace and actual compatibility proof before claiming Android/Quest3 gameplay.
