# Phase 223: original client type-4 quaternion serializer specialists

14 November 2026. Stacked draft on Phase222 PR #277. Privately examined original 2006 SiN Episodes PE32 game DLLs in owner-provided bin.zip. The whole-client SHA256 is 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d; the whole-server SHA256 is 568a0086fde607345adf31d7cfb2a5bf4eefe9a86f416faa97076ddf9d97650. Neither commercial DLL, complete native disassembly, nor original assets are committed.

## Three previously unreviewed original native code regions

| Native client.dll entry | Original bytes | Complete original function SHA256 | Router CALL | RET pop / INT3 pad |
| --- | ---: | --- | --- | --- |
| 0x24198F50 | 258 | b5d783421c4b15d965de57fde1cb89fe99b659ddac4bfecf2a68aeee3b603fb4 | 0x241998EC | 12 / 14 |
| 0x241987C0 | 406 | 20111b64456070863f319501c20ba6c558dba85c4b5d3200f54024dcd1ea4323 | 0x24199908 | 16 / 10 |
| 0x241989A0 | 58 | 86c7478c414c10d3e4e9247e47cbdbad416e67b4f2e936df2987cb756b3a0afb | 0x24199917 | 16 / 6 |

Three complete SHA-reviewed original native method bodies, 722 bytes in total. The previously recovered Phase221 type-4 16-byte forward-copy method at 0x24198DE0 is not counted again. All four native direct-call destinations in the original type-4 router case now have individual full original code SHA reviews. Distinct original nested formatter calls remain UNRECOVERED implementation bodies: 0x241988FC → 0x24197910, 0x24198945 → 0x241979C0, 0x241989CF → 0x24197FE0.

The original rdata format strings are:

- quaternion[] differs (1st diff) (net %f %f %f %f - pred %f %f %f %f) delta(%f %f %f %f)\n
- quaternion (%f %f %f %f)\n
- quaternion (%f %f %f %f)

## Authentic x87 comparison control flow and major portability caveat

Native comparator 0x24198F50 returns 0 if byte(this+0x14) is zero; returns 1 on descriptor bit 0x04 forced match or signed count <=0; otherwise loops through four binary32 floats per 16-byte record. Original FUCOMPP, FNSTSW AX, TEST AH,0x44, JP detects ordered unequal values through x87 status parity. With the x87 invalid-operation exception masked, unordered NaN comparisons also take the "not different" branch, unlike ordinary C++ float equality.

On a nonexact component the original does NOT calculate four independent vector component differences. The native sequence loads the FOURTH predicted component, subtracts the FOURTH net component, and FSTP DWORD stores one single-rounded delta at address ESP+0x18+ESI*4, where ESI is the CURRENT RECORD INDEX, not a vector component index. Four fixed local stack slots ESP+0x18, +0x1C, +0x20 and +0x24 are then independently loaded, fabs-transformed and compared with a strictly positive original descriptor tolerance at +0x30. For the first nonexact record only the first of these four slots is written by the method. The other three may be uninitialized. A record index >=4 writes outside these four slots. Do not silently reinterpret this as a conventional four-axis epsilon comparison or assume the original game must crash.

To prevent a misleading "faithful" approximation, the clean-room host model returns deterministic exact/forced/disabled paths normally. A nonexact record with positive tolerance requires caller-supplied ExplicitScratch values, otherwise it refuses with MissingOriginalScratch. Explicit scratch is diagnostic injection, NOT proven original runtime stack data. The host also refuses writes beyond those four slots. Other x87 control words, invalid exceptions and surrounding stack state have not been emulated.

## Diagnostics and preview

Native 406-byte 0x241987C0 tests this+0x14, invokes original mismatch formatter on status 0 and original general quaternion status formatter on enabled paths. The original constructs substantial by-value binary32/binary64 argument groups, but this phase does NOT claim to know every final native printf-style argument mapping. The host reports the witnessed first-record net/predicted values and fourth-component subtraction, and marks complete format-argument ordering as UNRESOLVED.

Native 58-byte 0x241989A0 tests DWORD(this+0x34) equal to DWORD(this+0x18), then formats the four first-predicted floats with the original quaternion preview string; this gate is independent of this+0x14. This method is distinct from the adjacent original preview at 0x24198960, and exact separate RET, padding and next-entry witnesses are pinned.

Original router direct-call order: comparison 0x24198F50 → Phase221 forward stride16 copy 0x24198DE0 → diagnostics 0x241987C0 → preview 0x241989A0. When copying is enabled, diagnostics may observe the destination after it is updated.

## Reproduce and test

- research/tools/audit_sin_original_client_quaternion_specialists.py: requires BOTH whole original DLL SHA256s; checks all 3 entire code-region hashes, 46 sparse original opcode witnesses, four exact router calls, three formatter calls, three original string literals, original zero float constant, x87 record-indexed stack writer and four fixed slot readers, native RET/pop/INT3 and neighbor entries.
- research/tests/test_sin_original_client_quaternion_specialists.py: original-free synthetic mutation of each evidence witness, including indexed x87 writer; never counts synthetic PE bytes as original reviewed methods.
- include/sin_compat/original_client_snapshot_quaternion.hpp: independent portable C++17 exact gates, optional explicit scratch-based diagnostic modeling, conditional preview, conservative trace callback and selected direct four-call composition.
- tests/test_original_client_snapshot_quaternion.cpp: exact/forced/disabled/zero-count tests, NaN masked-unordered parity, explicit scratch requirement, fourth-only delta, post-copy logs and preview gates.
- Native Win32 x86 and Linux CTests exercise host references, NOT the original game engine.

Original owner-only verification command:

    python3 research/tools/audit_sin_original_client_quaternion_specialists.py --archive /private/bin.zip

Synthetic tests:

    python3 -m unittest discover -s research/tests -p 'test_sin_original_client_quaternion_specialists.py' -v

## Checkpoint

Authentic whole-body SHA-reviewed native method ledger increases 267 → 270, split 128 server and 142 client. Of the strictly scoped original 36-target Phase212 snapshot router direct-call graph, four destinations remain without complete individual SHA reviews (7 → 4). This is NOT total-game completeness, percentage decompiled, or proof of Windows engine runtime loading. The next work is to review the final four original serializer targets, then a real Windows retail engine startup gate, followed by Android ARM64 and Quest 3.
