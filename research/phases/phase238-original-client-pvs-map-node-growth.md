# Phase 238: original client PVS map node resolution and 16-byte capacity growth

15 November 2026. Stacked draft on Phase237 PR #292. Grounded in the owner's **original SiN Episodes: Emergence (2006)** retail Win32 DLLs. Do not commit original binary files, complete native code bodies, commercial game assets or whole disassemblies.

## Original SHA256 provenance required before any review credit

| Retail PE32 | Whole file length | Complete owner-original SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

## Two newly complete original native method SHA reviews

| Original client VA | Purpose | Complete original code bytes | Whole original SHA256 | Native return and INT3 |
| --- | --- | ---: | --- | --- |
| **0x24105F40** | resolve sixteen-byte map node or lazy nil sentinel | **81** | **b9b6f5ec6f6a914b682e44461b33942b02006a864bb2bd5e0f8bf695f52028c1** | RET4 +0x46/+0x4E, 15 trailing INT3 |
| **0x24106140** | grow 16-byte PVS map record storage | **110** | **8adf0bd0a02d5471c712cbbe4b97bca99a4d6c92b30450f309ab865df37914ae** | RET4 +0x56/+0x6B, 2 trailing INT3 |

**Ledger 341 → 343 reviewed complete original native methods: server 147, client 194 → 196. +191 original x86 code bytes.** The owner-local audit checks both full-DLL SHA256s first, then two whole native function SHA256s, **36 sparse original opcode witnesses**, four authentic RET4 exits, 17 INT3 bytes and seven exact original E8 call destinations. Synthetic tests never count as original.

### Native map node resolver: 0x24105F40

Thiscall ECX is the map manager, and a single stack argument supplies a 16-bit index. The function checks a process-global initialization bit at original **0x2433FDC4**. On first call it sets the bit and initializes the first four WORDs at **global nil sentinel 0x2433FDBC** to **FFFF, FFFF, FFFF, 0001**. For index 0xFFFF, return the native pointer to this global sentinel; for any other index, return manager+0x04 storage pointer plus index*16. The original code **does not check bounds**. The clean-room host reference uses a separately injected lazy resolver state, a typed view of only the proven eight-byte sentinel header, and bounds checks for live synthetic record pointers. It does **not** fabricate the other global bytes or original CRT lifecycle.

### Native map record growth: 0x24106140

ECX points to the map's embedded storage manager at absolute map+0x04: pointer WORD-sized DWORD at this+0, capacity DWORD at this+4 (map+8), growth policy DWORD at this+8 (map+0xC). A policy of -1 immediately returns. Otherwise, the method computes a signed capacity target from the additional-record DWORD; zero capacity seeds **2** entries, nonzero policy increments by policy, zero policy doubles repeatedly. It **still dispatches to the memory helper when current capacity is already sufficient**, including a zero-additional request (unless policy is -1). Bytes = record_count*16. Original non-null storage calls **0x2416F960** (reallocate); null storage calls **0x2416F8F0** (allocate). The returned pointer replaces the manager storage pointer.

This is a **separate native 110-byte method** from Phase235's 0x24106090. They share observed growth policy, not the same original native code SHA. The host reference reuses Phase235's portable independent C++17 PvsSixteen policy model to avoid duplicating the algorithm and wires it into Phase237's index allocator.

### Seven original E8 rel32 call targets, independently authenticated

| Native callsite | Target | Context |
| --- | --- | --- |
| 0x2410673D | 0x24105F40 | Phase237 free-index reuse node resolver |
| 0x2410671C | 0x24106140 | Phase237 map index allocator growth |
| 0x24105FA5 | 0x24105F40 | Original sibling node flag accessor |
| 0x24106105 | 0x24105F40 | Original sibling flag predicate 0 |
| 0x24106125 | 0x24105F40 | Original sibling flag predicate 1 |
| 0x2410618B | 0x2416F960 | Existing storage reallocation |
| 0x241061A0 | 0x2416F8F0 | Initial storage allocation |

### New tests and code

- research/tools/audit_sin_original_client_pvs_map_node_growth.py: both complete owner-original DLL SHA gates, entire original method hashes, independent native opcode, return, padding and E8 call checks.
- research/tests/test_sin_original_client_pvs_map_node_growth.py: synthetic-only mutations of every native witness/RET4/INT3/E8 byte, original PE base and text bounds. Artificial fixtures never count as genuine original code.
- include/sin_compat/original_client_pvs_map_node_growth.hpp: independent C++17 sentinel-header model, checked synthetic record address resolution and growth adapter implementing Phase237's IndexCallbacks, calling Phase235's independent 16-byte capacity model through a host-provided memory backend.
- tests/test_original_client_pvs_map_node_growth.cpp: lazy sentinel initialization, valid/invalid map entries, exact initial two-entry growth, doubling versus increments, already-sufficient-capacity memory dispatch, fixed-growth policy -1, 16-bit index bounds, backend allocation failure, and cross-phase insert→lookup→release→free-index-reuse→reinsert host integration.
- CMakeLists.txt and pc/windows_x86_proxy/CMakeLists.txt: portable Linux and native MSVC Win32 x86 CTests for synthetic host code, not original gameplay DLL loading.

Owner-original audit:

    python3 research/tools/audit_sin_original_client_pvs_map_node_growth.py --archive /private/bin.zip

Mutation tests:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_map_node_growth.py -v

**Remaining blockers:** actual original tree attach and balancing at 0x241076E0 and tree removal/rebalancing at 0x241083C0, allocator internals 0x2416F960 and 0x2416F8F0, engine-owned map/comparator C++ objects and native virtual callbacks, original Windows gameplay DLL loading. Proxy forwarding and synthetic CTests are not proof of a native reconstructed game.

Target order remains Windows PC original retail game first, Android ARM64 next and Quest 3 VR last.
