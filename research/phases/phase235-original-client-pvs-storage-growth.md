# Phase 235: original client PVS storage growth, 8- and 16-byte managers

15 November 2026. Stacked on Phase234 PR #289; leave as a draft pending review.

Original owner-supplied retail **SiN Episodes: Emergence (2006)** PE32 game DLLs:

- client.dll (3,592,192 bytes): SHA256 **6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d**
- server.dll (6,049,792 bytes): SHA256 **568a0086fde607345adf31d7cfb2a5bf4eefe9a86f416faa97076ddf9d97650**

**Only after both entire original DLL hashes match** may the verification tool count complete native methods. No proprietary original DLL, whole method bytes, code dump or retail asset is committed.

## Two NEW entire native function SHA reviews

| Original client entry | Function | Complete native x86 bytes | Complete original-code SHA256 | RET 4 offsets | Trailing INT3 |
| --- | --- | ---: | --- | --- | ---: |
| **0x24106010** | Grow underlying 8-byte auxiliary-list storage | **116** | **4b28687cce6676118984f4bb2c61efedb173f6e8093cd355181020e5e8c9c160** | +0x59, +0x71 | **12** |
| **0x24106090** | Grow underlying 16-byte PVS-list storage | **110** | **39507159482f561fce8300268e7f9798266b8b6592fb76513959267c669274ba** | +0x56, +0x6B | **2** |

Both complete original code bodies, 24 independent sparse opcode witnesses, exact native returns/INT3 boundaries and six E8 rel32 CALL sites were checked from the owner-original images.

**Reviewed ledger 333 → 335 (server 147; client 186 → 188), +226 newly SHA-reviewed original code bytes.** These are scoped research counts, not a decompilation percentage, game DLL boot, or complete replacement.

## Native layout and control flow

Both functions are x86 ECX/thiscall with one stack DWORD argument and RET 4. Manager layout used by these methods:

- manager +0x00: DWORD storage pointer
- manager +0x04: DWORD current capacity
- manager +0x08: DWORD growth-policy parameter, **-1** means immediate return without changing storage
- stack argument: requested **additional record count**
- record stride: eight bytes in 0x24106010, sixteen bytes in 0x24106090

Original operation:

1. If manager.growth_policy is **-1**, return immediately without invoking either memory helper.
2. Otherwise compute target = capacity + requested additional records (native 32-bit arithmetic) and use **signed** comparisons for the growth loop.
3. If capacity is below target, repeatedly expand it: from zero, seed capacity **4** (8-byte version) or **2** (16-byte version); if nonzero and growth_policy is zero, **double capacity**; otherwise **add growth_policy** until capacity >= target.
4. **Important:** even when capacity was already sufficient, the original jumps past the growth loop but continues to a memory helper. It does not return early. This includes a caller requesting zero additional records.
5. For non-null storage, call original **0x2416F960** with the storage pointer and updated byte size (capacity * record stride). For null storage, call **0x2416F8F0** with the byte size. Both native memory helper implementations are outside this phase.
6. Write the returned pointer back to manager+0x00 and return, with no native error handling visible in these growth wrappers.

The original auxiliary implementation computes byte size with three doublings (times eight); the PVS implementation uses a shift left by four (times sixteen). The safe portable reference rejects arithmetic overflow, nonprogressing growth and declined backend requests rather than mirroring unsafe x86 wraparound, infinite loops or bad allocations. Its synthetic storage is vector-owned and value-initialized, **not** the real 2006 Source engine allocation strategy.

## Six verified original E8 rel32 calls

| Native callsite | Original target | Meaning |
| --- | --- | --- |
| **0x2410605E** | **0x2416F960** | 8-byte manager existing-storage memory helper |
| **0x24106076** | **0x2416F8F0** | 8-byte manager null-storage memory helper |
| **0x241060DB** | **0x2416F960** | 16-byte manager existing-storage memory helper |
| **0x241060F0** | **0x2416F8F0** | 16-byte manager null-storage memory helper |
| **0x2410658A** | **0x24106010** | Phase234 8-byte index allocator -> 8-byte growth |
| **0x2410662A** | **0x24106090** | Phase234 16-byte index allocator -> 16-byte growth |

## New code and regressions

- research/tools/audit_sin_original_client_pvs_storage_growth.py: dual-original-DLL SHA gate, whole-method hashes, opcode witnesses, native returns, INT3 padding and six original call edges
- research/tests/test_sin_original_client_pvs_storage_growth.py: synthetic-only PE mutation tests. Synthetic tests never receive original-method credit.
- include/sin_compat/original_client_pvs_storage_growth.hpp: independent C++17 8/16-byte growth model, injected host memory backend and explicit bounds/overflow guards
- tests/test_original_client_pvs_storage_growth.cpp: initial capacity (4/2), fixed policy -1, doubling vs fixed increment, sufficient-capacity call-through, zero-count memory dispatch, initial allocation vs reallocation, preserved synthetic payload, declined backend, invalid values and overflow
- Root Linux CTest and real MSVC Win32 x86 CTest build registrations

Private original-binary auditor:

    python3 research/tools/audit_sin_original_client_pvs_storage_growth.py --archive /private/bin.zip

Synthetic mutation tests:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_storage_growth.py -v

**Remaining boundaries:** Original memory helper internals 0x2416F960/0x2416F8F0, PVS map registration/insertion 0x24108750, map lookup 0x24106B20, map removal 0x24108840, real game-engine entity virtual callbacks, and full Windows-compatible rebuilt client/server gameplay DLL loading. Prioritize original Windows PC first, Android ARM64 second, Quest 3 VR last.
