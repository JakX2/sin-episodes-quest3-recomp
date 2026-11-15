# Phase 242: original 2006 client PVS red-black deletion balancing

15 November 2026. Stacked draft on Phase241 PR #296. This work uses the owner-supplied original **SiN Episodes: Emergence (2006)** PE32 Windows game DLLs. No retail DLLs, proprietary complete original function bytes, whole original disassemblies or assets are committed.

## Whole original provenance required before method credit

| Original PE32 module | Whole DLL size | Complete retail SHA256 |
| --- | ---: | --- |
| client.dll | 3,592,192 bytes | 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d |
| server.dll | 6,049,792 bytes | 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 |

## One NEW whole-original-SHA-reviewed native method

| Original client entry | Original function | Complete native code | SHA256 of entire original code | Native return and padding |
| --- | --- | ---: | --- | --- |
| **0x24107750** | Red-black tree delete fixup after replacing a physically removed black node | **2,609 bytes** | **7bed48e41a436fb3a0e2fc69e69b3cfe83c42aee02be58f5fd6872e4c7f0c12a** | RET 4 at +0xA2E, **15 trailing INT3** through 0x2410818F |

**Original method review ledger 348 → 349 (server 147; client 201 → 202), +2,609 complete SHA-reviewed original x86 code bytes.** A complete original native function digest does **not** prove source-level or x86 register-by-register equivalence, and the inventory is not a percentage of game completion.

The owner-original auditor checks the complete two DLL SHA256s first, then this complete native code hash, **25 independent sparse opcode witnesses**, native RET4 position, 15 following INT3 bytes and **seven exact E8 rel32 CALL edges**.

## Native original behavior

ECX is the map manager; one stack DWORD carries replacement index, returning via **RET 4**. A 16-byte red-black node has left WORD+0, right WORD+2, parent WORD+4 and color WORD+6; original color values are **red=0 and black=1**, and absent index is **0xFFFF** with the lazily initialized **global nil sentinel 0x2433FDBC**. The original initializer is guarded by flag 0x2433FDC4.

- If selected replacement is already the map root (WORD manager+0x10), the method blackens that node and exits.
- Otherwise, while replacement is not the root and its color is black, it obtains the parent and sibling. A **red sibling** is blackened; the parent is reddened and rotated to transform the problem into a black-sibling case.
- For a **black sibling with two black children**, recolor the sibling red and propagate the repair upward to the parent.
- For a **black sibling with red inner child and black outer child**, recolor and rotate the sibling first to turn this into the outer-child case.
- For a **black sibling with red outer child**, copy the parent's color into the sibling, blacken parent and outer child, rotate the parent, then complete the loop.
- Both left-child and right-child paths are handled symmetrically. Before RET4, the selected/current node's color WORD is blackened.

Original Windows compiler output repeats global lazy-nil setup and inline 16-byte indexed record lookups many times, accounting for much of the 2,609-byte method. The C++17 host model uses the separately reviewed Phase238 nil-index/sentinel representation and Phase240's left/right rotations. It tracks case/rotation counters for synthetic tests and adds fail-closed guards for malformed links/loops; neither host safety checks nor CRT globals are claimed as the original x86 implementation.

**Important Phase241 dispatch requirement:** Original tree deletion at **0x241083C0** calls this method only when the selected replacement is **not 0xFFFF** AND the physically removed node's color is black. It skips delete fixup for absent replacements even if a black leaf was removed. The host preserves that observed native gate, instead of silently implementing a different textbook red-black deletion algorithm. Therefore exhaustive red-black invariants are **not** promised for arbitrary original black-leaf deletions.

## All seven original direct E8 rel32 calls

| Original instruction | Original native target | Path |
| --- | --- | --- |
| 0x241078E6 | 0x241067C0 | left rotation |
| 0x24107B81 | 0x24106D40 | right rotation |
| 0x24107CBF | 0x241067C0 | left rotation |
| 0x24107D70 | 0x24106D40 | right rotation |
| 0x2410800A | 0x241067C0 | left rotation |
| 0x24108146 | 0x24106D40 | right rotation |
| 0x24108669 | 0x24107750 | Phase241 original eraser invokes delete fixup |

These are all six direct rotation calls found within the entire reviewed method plus the one direct caller from the original tree erase routine. The complete SHA code-body check is stronger than sparse witness testing; synthetic PE fixtures cannot count as original methods.

## Source and tests

- research/tools/audit_sin_original_client_pvs_delete_fixup.py: dual original complete-DLL SHA gate, entire native method SHA, 25 distributed original opcode witnesses, RET4, INT3 and exact seven E8 links.
- research/tests/test_sin_original_client_pvs_delete_fixup.py: synthetic-only mutations for all witnesses, return bytes, 15 INT3 bytes, seven complete native E8 instructions, PE base and complete text section boundaries.
- include/sin_compat/original_client_pvs_delete_fixup.hpp: independent C++17 host red-black deletion repair implementing Phase241's explicit DeleteBalancingBackend, using both Phase240 reviewed rotation models.
- tests/test_original_client_pvs_delete_fixup.cpp: root and red-replacement blackening, black-nephew recoloring, left and right outer-nephew rotations, both mirror-image inner-nephew double rotations, mirrored red-sibling rotations, original Phase241 black-node one-child release callback, cross-phase insert/lookup/release/index-reuse, and malformed graph guards.
- Root CMake and native MSVC Win32 x86 CTest registrations, plus SHA-verified reviewed method catalog update to 349 entries.

Private whole-original SHA auditor:

    python3 research/tools/audit_sin_original_client_pvs_delete_fixup.py --archive /private/bin.zip

Synthetic PE mutation unit tests:

    python3 -m unittest discover -s research/tests -p test_sin_original_client_pvs_delete_fixup.py -v

## Remaining integration limits

This reconstructs the **behavioral path** for Phase236 map insertion, Phase237 placement and indices, Phase238 storage and nil node, Phase239 attach, Phase240 insertion rotations/balancing, Phase241 deletion and Phase242 removal balancing. It does NOT reconstruct the original engine-owned map/comparator class, all memory allocation internals, original native ABI/vtable integration, or prove **independent replacement client.dll and server.dll are loaded by the retail Windows game**.

The original-retail Windows PC gameplay DLL compatibility milestone remains first, Android ARM64 second, Quest 3 VR third. Host synthetic tests and the proxy that forwards to retail DLLs are not a playable independent recompilation.
