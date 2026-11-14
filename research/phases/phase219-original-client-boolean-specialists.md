# Phase 219: original client type-6 Boolean snapshot specialists

14 November 2026. **Stacked draft on Phase218 PR #272.** Privately examined both original retail 2006 Windows x86 SiN Episodes: Emergence DLLs from the user's original bin.zip. Whole-file SHA256 fingerprints match:

- client.dll: SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d, 3,592,192 bytes
- server.dll: SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650, 6,049,792 bytes

No original DLL, complete original native function body, raw disassembly, map or commercial game asset is committed.

## Three new complete original functions

| Original client.dll VA | Complete native bytes | Entire native SHA256 | Phase212 router CALL |
| --- | ---: | --- | --- |
| 0x24197E30 | **82** | ed96e875573440132a3652b4a5a419966e46722da843d655b63ac3efd506b334 | 0x24199982 |
| 0x24197C30 | **110** | 2e200672f44e367c38412b1d9d0e6d19ad6aebfd6b3a4eb923ef04f74a17295b | 0x2419999E |
| 0x24198440 | **45** | e0cb1517e7cab7e23b5d5fe5d6442515c73dd474ead4050d96b9e0831ae98e74 | 0x241999AD |

Three complete SHA-reviewed original functions, **237 native x86 bytes**, original RET stack cleanup 12/16/16 bytes and original 14/2/3 INT3 padding bytes. Three nested original formatter CALLs are also pinned: 0x24197C71 → 0x24197910; 0x24197C90 → 0x241979C0; and 0x24198462 → 0x24197FE0. Those formatter implementations remain opaque and are NOT included in the new reviewed-function count.

## Native behavior reconstructed

1. **Boolean byte comparison at 0x24197E30.** Original byte this+0x14 == 0 returns status 0 immediately. Otherwise descriptor flags byte at [this+0x18]+0x13 bit 0x04 forces status 1. When comparison proceeds, the third stack argument is a signed DWORD count: count <= 0 yields status 1. Positive counts compare actual bytes forward, returning 0 at the first mismatched byte and 1 if all compared bytes equal. Distinct bytes 0x01 and 0xFF compare UNEQUAL even though both display as true. Native invalid pointers are not safely bounds checked.

2. **Boolean diagnostics at 0x24197C30.** If original this+0x14 == 0 there are no formatter calls. Otherwise operation/status 0 first calls formatter 0x24197910 with the real original literal "bool differs (net %s pred %s)\n", mapping the first nonzero byte of each pointer to original "true" at 0x242D373C and zero to "false" at 0x242A3514. Then any enabled status calls 0x241979C0 with "bool (%s)\n" for the first predicted byte. These original strings, including the two truth labels, are separately authenticated in .rdata. The real formatter object and console effects are not reconstructed.

3. **Boolean preview at 0x24198440.** Tests DWORD(this+0x34) == DWORD(this+0x18) and if equal calls 0x24197FE0 with original "bool (%s)" using the first destination/predicted byte. This is independent of the this+0x14 enable byte.

Phase212 original case-6 native call order is: **0x24197E30 compare → Phase213 0x24198A40 byte copy → 0x24197C30 diagnostics → 0x24198440 preview**. Thus all four direct original type-6 callee bodies have now been individually SHA-reviewed. Copy is separately gated by byte this+0x2C != 0 and operation != 1, and uses forward REP MOVSD/MOVSB on an unsigned byte count. A compare mismatch can be logged *after the copy*, so a format message may show two identical Boolean labels after the destination was updated.

## Files and verification

- research/tools/audit_sin_original_client_boolean_specialists.py authenticates the two whole original DLL hashes, three complete native method SHA256s, 44 short sparse x86 instruction witnesses, all four original type-6 router calls (including already-reviewed Phase213 copy), three nested formatter calls, five real format/truth strings, native RET arities and INT3 padding.
- research/tests/test_sin_original_client_boolean_specialists.py uses synthetic-only PE fixtures to mutate code anchors, CALL sites, padding, return cleanup, .rdata strings and base address. Synthetic verification must return **zero newly authenticated original methods**.
- include/sin_compat/original_client_snapshot_boolean.hpp is an independent C++17 host model preserving bytewise exact comparison, nonzero-to-truth-label formatting, pre-copy comparison and post-copy diagnostic/preview order, and explicit host-only bounds/unsafe-count refusal.
- tests/test_original_client_snapshot_boolean.cpp checks exact byte versus displayed Boolean semantics, early-mismatch, state gates, forward overlapping copy, zero/negative signed comparison count, unsigned-copy guard and callback ordering.
- Root Linux and genuine native Windows MSVC x86 CTest registrations test the independent model. Neither proves the game engine loads reconstructed DLLs.

Owner-local commands:

    python3 research/tools/audit_sin_original_client_boolean_specialists.py --archive /private/bin.zip
    python3 -m unittest discover -s research/tests -p 'test_sin_original_client_boolean_specialists.py' -v

## Status and limitations

Original reviewed-function inventory **257 → 260** (**128 server, 132 client**) if all owner-original binary audits pass. Of Phase212's narrowly scoped 36 distinct snapshot-router CALL targets, **14 remain without a reviewed entire function body**. That is NOT a completion percentage or a count of functions left across the whole game.

Actual Source engine serializer ownership, native formatter implementations, full Windows replacement DLL startup, playable PC campaign, Android game and Quest 3 VR gameplay remain open. Priority is **Windows PC engine first → Android ARM64 → Quest 3**.
