# Phase 237: original client PVS map placement search and index reuse

15 November 2026. Stacked draft on Phase236 PR #291. Owner-original **SiN Episodes: Emergence (2006)** retail Windows x86 client/server binaries, independently authenticated before original-method credit.

Full retail original provenance:
- client.dll: **3,592,192 bytes**, SHA256 **6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d**
- server.dll: **6,049,792 bytes**, SHA256 **568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650**

No proprietary commercial DLL, native body bytes, complete original disassembly or game assets are committed.

## Two newly whole-original-SHA-reviewed native client functions

| Original client entry | Role | Complete original native bytes | Whole original body SHA256 | Return / trailing INT3 |
| --- | --- | ---: | --- | --- |
| **0x24106440** | Comparator-driven map insertion parent/side location search | **255** | **23e29d786cbdb4acf862213297595c65259484779730a3c7788ea6325046006a** | RET 16 at method +0xFC, 1 trailing INT3 |
| **0x24106700** | Map free-index reuse or next-unused index allocation | **86** | **15307e759a5ad044b5f781b7eb42914c95b1be056748356d53d664f68fd3344c** | C3 at +0x37/+0x55, 10 trailing INT3 |

**Original reviewed ledger: 339 → 341 (server 147, client 192 → 194). Two new full native code SHA reviews, 341 original x86 instruction bytes.** Counts are scoped original function reviews, not a percent of entire DLL recovery or proof of a playable replacement DLL.

## Original comparator placement search 0x24106440

The original calls an indirect native comparator through DWORD manager+0x00 at original CALL location **0x2410648A**, with the caller key followed by each 16-byte record's payload address (+0x08). The map root is the WORD at manager+0x10. On function entry it initializes the output parent index WORD to **0xFFFF** and output side BYTE to zero.

For each visited non-absent record it writes the current index into the output parent WORD, calls comparator **once** on (key, current_node_payload), and follows record **left WORD+0x00 on true**, or record **right WORD+0x02 on false**. It writes side BYTE=1 for a left branch, or 0 for a right branch, and repeats until absent index. Equal keys follow the false/right branch; the frontend does not independently reject duplicates. After a root sentinel or traversal end it returns via **RET 16**, preserving the original four-argument stack cleanup. Only three input/output argument pointers are directly accessed in this whole native body; the other caller-supplied stack value is not read by the reviewed function.

A lazy original static nil/sentinel initialized under global flag at **0x2433FDC4** and words at **0x2433FDBC..0x2433FDC2** can participate in missing-child navigation. The host reference uses an explicit 0xFFFF sentinel without claiming the original CRT/global singleton lifecycle is reconstructed.

## Original map allocator 0x24106700

- manager+**0x04 DWORD**: map record storage pointer; **+0x08 DWORD**: capacity; **+0x14 WORD**: free-head index; **+0x16 WORD**: next never-used index; **+0x18 DWORD**: refreshed cached storage pointer.
- If free head is **0xFFFF**, read WORD next-unused; when this equals capacity call the original **0x24106140** capacity-growth method with PUSH 1 and ECX=manager+0x04. Return next-unused WORD and increment the WORD using native wrap semantics.
- If free head is not absent, push index and call original node-pointer resolver **0x24105F40**; read its returned record's **+0x02 WORD** as next free-head index, update manager+0x14, return former free-head.
- Both paths refresh cached storage pointer at manager+0x18. Both native exits use plain **C3**, not RET4.
- Original 0x24106140 growth and 0x24105F40 node-pointer resolver bodies remain outside this phase. The host model injects explicit callbacks and safely rejects invalid indices/overflow rather than pretending undefined original behavior is validated.

## Original call link evidence

| Verified original E8 rel32 callsite | Native target | Observed role |
| --- | --- | --- |
| **0x2410671C** | **0x24106140** | Allocator asks map record manager for capacity growth |
| **0x2410673D** | **0x24105F40** | Allocator resolves recycled free-chain node |
| **0x2410876B** | **0x24106440** | Phase236 map insertion finds parent index and branch side |
| **0x24108772** | **0x24106700** | Phase236 map insertion gets new/recycled record index |

The Phase237 original-only auditor checks the complete two code SHA256 hashes after both full DLL SHA256s, **30 sparse native opcode witnesses**, native RET16 + two C3 returns, 11 original INT3 pad bytes, four direct E8 rel32 calls, and native indirect comparator CALL DWORD PTR [EBP] at **0x2410648A**. Synthetic fixtures never count as recovered original methods.

## Source, host reference and CI

- research/tools/audit_sin_original_client_pvs_map_search_index.py: original whole-SHA-gated x86 code and call audit
- research/tests/test_sin_original_client_pvs_map_search_index.py: synthetic-only mutation regressions across all original instruction witnesses, returns, padding, call edges, PE base and text section
- include/sin_compat/original_client_pvs_map_search_index.hpp: independent C++17 host functions for comparator placement and map index reuse, with callback-injected opaque growth, pointer resolution and tree attachment
- tests/test_original_client_pvs_map_search_index.cpp: empty/left/right/equal-key placement, virgin/recycled index allocation, growth boundary, malformed chain/cycle rejection, and a cross-phase Phase236 insert/release/reinsert path
- root CMake and native MSVC Win32 x86 CTest registration: portable synthetic host testing, **not** original Source engine game module loading

Owner original audit:

    python3 research/tools/audit_sin_original_client_pvs_map_search_index.py --archive /private/bin.zip

Synthetic mutation suite:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_map_search_index.py -v

**Remaining original map internals:** 0x24106140 storage growth, 0x24105F40 node resolution, 0x241076E0 tree attachment/balancing, and 0x241083C0 tree removal/rebalancing. Next meaningful Windows milestone is loading reconstructed gameplay DLLs in the *original* retail game engine; host CTests do not prove this. Target order: Windows PC → Android ARM64 → Quest 3 VR.
