# Gameplay Phase 596: Retail Grunt +0xC18 candidate triage

## Evidence and limits

This pass analyzes the owner's uploaded Phase595 candidate-only JSON from unchanged retail SiN Episodes: Emergence (2006) server.dll, SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. The prior 72-byte gate at 0x2202D930 remains SHA-authenticated (969cc5659f53449812b567734994fbb656edc52fabffaa810dd15c0152f8f5a2). This particular run used --server and did not verify a matched client.dll SHA. Phase596 authenticates **zero new native function bodies**: Phase595 scanned syntactic x86 byte patterns and did not prove their true instruction boundaries or class ownership.

## Actual counts

- 245 syntactic sites with displacement +0xC18.
- 51 possible stores: 33 register-stores and 18 immediate-stores (neither category proves assignment of a pointer).
- 146 read-register, 29 x87-memory, 10 arithmetic/compare-immediate and 9 address-of shapes.
- Four E8 byte-pattern callsites aimed at the original 0x2202D930 gate.

| Candidate E8 callsite | Distinction |
| --- | --- |
| 0x2235C282 | Previously authenticated direct edge in inherited Grunt event parent, Phase592 |
| 0x2235C436 | Previously authenticated direct edge in inherited Grunt event parent, Phase592 |
| 0x2208C00E | New pattern candidate; verify Ghidra instruction boundary/target and enclosing function |
| 0x2208C054 | New pattern candidate; verify Ghidra instruction boundary/target and enclosing function |

The last two callsites are **not** independently authenticated native calls yet.

## Priority sites for constructor and member provenance

| Original VA | Phase595 shape | Research priority |
| --- | --- | --- |
| 0x223527AC | write-immediate [esi+C18] | Potential initializer, owner unknown |
| 0x22353C3C | write-immediate [esi+C18] | Early Grunt-area candidate; inspect receiver and containing function |
| 0x22353D4C | write-immediate [esi+C18] | Nearby candidate, not proven to be a Grunt constructor |
| 0x223596E9 | read-register [edi+C18] | Neighboring Grunt-family context |
| 0x22359FC7 | read-register [esi+C18] | Neighboring Grunt-family context |
| 0x2235A0CA | read-register [esi+C18] | Near previously recovered Grunt activity mapping |
| **0x2235A375** | read-register [esi+C18] | **Inside authentic 326-byte Phase587 Grunt +0x338 method**. Independent evidence ties this field to the optional activity virtual +0x7C |
| 0x2235AABC | read-register [esi+C18] | Neighboring Grunt-family context |
| 0x2235DBA3 | read-register [esi+C18] | Neighboring inherited methods |
| 0x2235DF97 | read-register [esi+C18] | Neighboring inherited methods |

The independently verified native gate read is at 0x2202D93E, inside the Phase592 72-byte event intermediary. These two authenticated read sites (0x2235A375 and 0x2202D93E) are stronger receiver anchors than the unconfirmed candidate write patterns.

### Same-offset false friend

0x2236473F matches +0xC18 inside authenticated Phase588 Jetpack activity mapper 0x22364700, where +0xC18/+0xC1C/+0xC20 belong to an **attached flying entity's three-float vector**, not the Grunt owner's optional component pointer. Never merge fields from different classes because offsets match.

## Useful regional totals

| VA range | Candidate uses | Potential writes |
| --- | ---: | ---: |
| 0x2202D930..0x2202D977 | 1 | 0 |
| 0x22352000..0x22355FFF | 3 | 3 |
| 0x22359000..0x2235DFFF | 7 | 0 |
| 0x22360000..0x2236FFFF | 24 | 2 |
| 0x22370000..0x2238FFFF | 80 | 18 |

Ranges are investigation conveniences, **not authenticated class boundaries**. They overlap with methods and subclasses from other research.

## Next work

Use Ghidra on original server.dll to check whether 0x22353C3C/0x22353D4C are actual instruction starts and which object the store base register names. Trace pointer allocation, assignment, destructor and vtable initialization, starting from the separately authenticated owner +0xC18 reads in Phase587 and Phase592. If the same component vtable can be established, examine **virtual slot +0x60** (animation event), **virtual slot +0x7C** (activity mapper), and writes to component byte +0x08. Determine actual behavior for event 0x54. Verify the two new 0x2208C0xx callsite candidates independently.

An original-free Phase596 triage utility and optional Ghidra metadata-only script accompany this research. **No original game binary, proprietary disassembly dump or game assets are committed.** This does not produce a loadable replacement DLL, Windows port, Android build or Quest 3 runtime.