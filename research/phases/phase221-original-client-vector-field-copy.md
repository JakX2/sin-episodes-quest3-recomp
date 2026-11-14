# Phase 221: native type-3/type-4 prediction field copy helpers

14 November 2026. **Stacked draft on canonical Phase220 PR #274.** The user's original retail 2006 SiN Episodes: Emergence PE32 server/client DLLs were directly inspected in a private bin.zip. Neither proprietary binary, complete original disassembly/method body nor commercial game asset is committed.

Original full-DLL SHA256 provenance:
- client.dll: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d (3,592,192 bytes).
- server.dll: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 (6,049,792 bytes).

## Two new complete SHA-verified native x86 methods

| Original client.dll entry | Native bytes | Whole-method SHA256 | Original router CALL |
| --- | ---: | --- | --- |
| 0x24198DA0 | **53** | 4a10b9e0f382e4732ecd64991a5a6ea8fcd67caf8a8ac8521c9281f7dd36eac0 | 0x241998C3 |
| 0x24198DE0 | **49** | 4fe077835ba1cc25f17844afb49b481eabcf130515231479bc3fe22e45ccc33b | 0x241998FD |

**102 total original x86 bytes, two new method entrypoints.** The authentic RET pop is 16 bytes for both. Original INT3 padding after each method is 11 and 15 bytes, respectively, up to the next function entries 0x24198DE0 and 0x24198E20. Both 39-CALL-graph edges are already in Phase212's SHA-pinned serializer router. This phase checks them again against the actual owner DLLs, along with 34 exact sparse complete instruction witnesses.

## Original native semantics

**Type 3 at 0x24198DA0:** BYTE(this+0x2C)==0 or comparison/status argument ==1 returns without reading pointers/count. Otherwise, the unsigned 32-bit record count is multiplied by **12 modulo 2^32** using LEA count*3 followed by two doublings. The original moves that many bytes in ascending address order, using REP MOVSD for complete DWORDs followed by REP MOVSB for the remainder. The direction flag is assumed clear. This proves a 12-byte record stride, not its high-level class name or memory ownership.

**Type 4 at 0x24198DE0:** identical native early gates and forward REP MOVSD/REP MOVSB pattern, but left-shifts count by four, representing an unsigned **16-byte record stride modulo 2^32**.

The two methods preserve the same early skip predicate as Phase213 (this+0x2C set and status !=1). The C++17 independent host reference reuses Phase213's previously validated word-by-word forward copy, preserving the unusual overlapping-memory behavior of REP MOVSD rather than silently replacing it with memmove. Original 32-bit count*stride overflow wraps; the host reference rejects it only when a copy is active, as an explicitly host-only protection. Native invalid pointers, CPU exceptions and arbitrary Source engine memory are not emulated.

## Still unreviewed type-3/type-4 callees

Type 3: comparison 0x24198E20, **copy 0x24198DA0 recovered in this phase**, diagnostic 0x241985A0, preview 0x241986C0.
Type 4: comparison 0x24198F50, **copy 0x24198DE0 recovered in this phase**, diagnostic 0x241987C0, preview 0x241989A0.

Those six non-copy callee bodies remain unreviewed. Some use x87 floating-point instructions; a future phase must preserve unordered and rounding behaviors and not assume a guessed Source SDK type.

## Implementation and validation

- research/tools/audit_sin_original_client_vector_copy.py: SHA-gated whole original DLL audit plus both complete original native method hashes, 34 sparse instruction anchors, exact two native CALL rel32 destinations, native return arities, original INT3 pads and next function heads.
- research/tests/test_sin_original_client_vector_copy.py: original-free synthetic mutations for each sparse instruction witness, original router CALL, RET, INT3, adjacent native entry, original base/section. A synthetic fixture never claims to authenticate original native methods.
- include/sin_compat/original_client_snapshot_vector_copy.hpp: C++17 host reference using established Phase213 forward DWORD copy with original skip gates and host-only buffer/overflow guards.
- tests/test_original_client_snapshot_vector_copy.cpp: stride12/stride16 copies, offsets, zero-length, count=0, status0/status1/status2, disabled gate, overlapping source/destination, host-only overflow and bounds refusal.
- Root Linux and real MSVC native Windows x86 CTest registrations. These test the independent reference, **not retail SiN engine gameplay**.

Owner-local original verification:

    python3 research/tools/audit_sin_original_client_vector_copy.py --archive /private/bin.zip
    python3 -m unittest discover -s research/tests -p 'test_sin_original_client_vector_copy.py' -v

## Checkpoint

Original verified-function ledger **262 → 264** (server **128**, client **136**), pending final CI. The narrow Phase212 36-target snapshot router call graph has **10** still-unreviewed distinct native callee destinations (12 minus 2). This is not the game's decompilation completion percentage. Real Windows engine startup using reconstructed gameplay DLLs is still unproven; PC first, Android ARM64 second, Quest 3 third.
