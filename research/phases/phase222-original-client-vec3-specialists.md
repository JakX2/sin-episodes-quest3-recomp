# Phase 222: original client type-3 vec3 comparison, diagnostics and preview

14 November 2026. **Stacked draft on Phase221 PR #276.** Direct original-binary investigation used the user's authentic 2006 SiN Episodes: Emergence Windows i386 client.dll and server.dll from the private bin.zip mounted in the project environment. Both SHA256 values match the pre-existing whole-DLL provenance:

- client.dll: SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d (3,592,192 bytes).
- server.dll: SHA256 568a0086fde607345adf31d7cfb2a5bf4eefe9a86f416faa97076ddf9d97650 (6,049,792 bytes).

No commercial DLL, original whole function body, full native disassembly, game map or asset is committed.

## Three newly reviewed complete original x86 methods

| Original client.dll VA | Complete original native bytes | Whole-original method SHA256 | Exact Phase212 router call | RET cleanup / INT3 padding |
| --- | ---: | --- | --- | --- |
| 0x24198E20 | **289** | 1b13b7672ab3ff4932694d934d76d6f7bbdb656c1f09941b827d31d4c7842360 | 0x241998B2 | 12 / 15 |
| 0x241985A0 | **219** | d5abf2cbbbf02ba5dac02c32fb7c98f999d363042aae3413f7661808de71ec39 | 0x241998CE | 16 / 5 |
| 0x241986C0 | **51** | 19c07ccae5e728b90a818a248a449719bc02e81bc6e41eccc1b03c4c184bb6a4 | 0x241998DD | 16 / 13 |

**Three newly SHA-verified complete native methods, 559 original i386 instruction bytes.** The fourth direct type-3 call, 0x241998C3 → 0x24198DA0, was separately fully SHA-reviewed in Phase221 as a 53-byte record copy with 12-byte stride. It is **not counted twice**.

## What the original machine code actually does

**Type-3 comparison at 0x24198E20.** When native BYTE(this+0x14) is zero it immediately returns status **0**. When original descriptor DWORD(this+0x18), flags BYTE[descriptor+0x13] bit 0x04 is set it returns status **1**. A signed 32-bit count <=0 also returns status **1**. Otherwise each record consists of **three contiguous IEEE754 binary32 floats**. Native x87 uses FLD/FUCOMPP/FNSTSW and the parity-sensitive TEST AH,0x44 to check all three components for ordered numeric equality. Exact +0/-0 and equal infinities count as equal; NaNs are unordered and do not satisfy equality.

When a vector is nonidentical, the original computes **three separate predicted-minus-net** x87 subtractions, storing each component with FSTP DWORD to single-rounded binary32 temporary memory. It only accepts this non-exact vec3 when the descriptor tolerance at +0x30 is strictly positive and ordered when compared with the authentic +0.0 constant at 0x242DE2F8, **and all three absolute single-rounded component differences are <= tolerance**. Otherwise it returns **0** at the first failing record. At least one approximate vector and no failing records yields status **2**; all vectors exact yields status **1**. The independent C++ host reference uses long-double intermediate subtraction then an explicitly rounded float, but does not fully emulate alternative x87 control-word settings, exceptions, or traps.

**Type-3 mismatch/status at 0x241985A0.** Enabled only by the independent native this+0x14 BYTE gate. If status equals 0, the first native formatter call at 0x24198645 → 0x24197910 reports the **first** vector's net/predicted three-component values and the three **single-rounded predicted-minus-net** deltas, using the original literal:

    vec[] differs (1st diff) (net %f %f %f - pred %f %f %f) delta(%f %f %f)\n

Regardless of nonzero/zero status, enabled cases then call the separate native formatter at 0x2419866A → 0x241979C0 with the authentic "vector (%f %f %f)\n" literal and the first predicted vector. Both formatter implementations remain *callee-only evidence* and are NOT counted as recovered. This function returns with RET 16 after its 219-byte original body.

**Type-3 preview at 0x241986C0.** Tests original DWORD(this+0x34)==DWORD(this+0x18). Only if true does it call original formatter 0x241986E8 → 0x24197FE0 with the three floats in the first predicted vector, using "vector (%f %f %f)". The preview gate is independent of this+0x14.

**Call ordering in Phase212** is native comparison 0x24198E20, Phase221 forward copy 0x24198DA0, diagnostics 0x241985A0, preview 0x241986C0. Therefore the comparison tests pre-copy values, while the later diagnostic callbacks can observe **post-copy** predicted values, including a mismatch report whose displayed delta has become zero after copying. The model preserves that order without inventing original Source class ownership or external console wiring.

## Reproduce, test and limits

- research/tools/audit_sin_original_client_vec3_specialists.py: original owner's dual-full-DLL SHA gate; 3 whole method SHA256s; **37** independent sparse complete x86 opcode witnesses; 4 exact type-3 Phase212 router CALL rel32 instructions including Phase221 copy; 3 exact original nested formatter CALLs; 3 original .rdata formatting literals; authentic positive-tolerance float-zero constant; exact RET and INT3 boundaries.
- research/tests/test_sin_original_client_vec3_specialists.py: synthetic-only mutation suite for all 37 opcode witnesses, 7 native direct CALLs, every padding byte and return cleanup, 3 literals, zero constant and base/section validation. Synthetic fixtures never count as original SHA-verified recoveries.
- include/sin_compat/original_client_snapshot_vec3.hpp: independent portable C++17 vec3 IEEE754 comparison, x87-inspired explicit single-rounded delta, 0/1/2 status semantics, tracing callback interface, conditional preview and selected four-original-direct-CALL composition using the already reviewed Phase221 copy helper.
- tests/test_original_client_snapshot_vec3.cpp: exact/approximate/mismatch results, per-component positive tolerance, NaNs/unordered and +/-0, signed empty/negative count, early mismatch, independent disabled/forced gates, diagnostic/preview order, post-copy traces and host-only invalid-buffer/count guards.
- Root CMake Linux CTest and native MSVC Windows x86 CTest target are registered, but neither test actually loads original gameplay modules in the retail engine.

Owner-local audit:

    python3 research/tools/audit_sin_original_client_vec3_specialists.py --archive /private/bin.zip
    python3 -m unittest discover -s research/tests -p 'test_sin_original_client_vec3_specialists.py' -v

## Checkpoint

The reviewed original-function ledger advances **264 → 267**: server **128**, client **139**. Within the **strictly local** Phase212 36-distinct-target prediction router CALL graph, targets without individual whole-original SHA review decrease **10 → 7**. These are not the total game's remaining functions and not a playable-port completion percentage.

The type-4 four-component (16-byte) record path still has comparison, diagnostic and preview routines at 0x24198F50, 0x241987C0 and 0x241989A0 pending review. Its x87 comparison has nontrivial float-slot indexing and should not be conflated with the three-component path until independently verified. Other original Native CALL targets beyond this serializer also remain unresolved. Main integration priority remains **running reconstructed server.dll/client.dll in the actual Windows Source engine**, followed by Android ARM64 and Quest 3.
