# Phase 150: recovered SiN 2006 short-term advisor listener classes

**Original DLL decompilation, 9 November 2026.** Follows Phase 149's real advisor listener fanout and aggregation. The owner-held Windows 2006 server.dll and client.dll retain their previously established full SHA-256 fingerprints. No original DLL bytes or game assets are uploaded.

## Two previously unnamed concrete listener types

Original MSVC RTTI names confirm **CShortTermStatGate** (type name at 0x22522CA0, primary vtable 0x2245E4A8) and **CShortTermStatGateConjugate** (type name at 0x22522CC4, primary vtable 0x2245E4C8). Both concrete objects are allocated as 20-byte structures, with original field offsets: scale +0x04, source pointer +0x08, cached float +0x0C, and gain +0x10.

| Original vtable slot | Normal function | Conjugate function | Verified operation |
|---|---|---|---|
| 0 | 0x222C6680 | 0x222C66D0 | Deleting destructor |
| 1 | 0x222C6670 | 0x222C6670 | Return cached float |
| 2 | 0x222C6370 | 0x222C6370 | Cache raw input and forward normalized source input |
| 3 | 0x222C63A0 | 0x222C6420 | Recompute ordinary/complement cached value |
| 4 | 0x222C63F0 | 0x222C63F0 | Forward underlying virtual +0x10 |
| 5 | 0x222C6400 | 0x222C6400 | Forward underlying virtual +0x30 |
| 6 | 0x222C6410 | 0x222C6410 | Forward underlying virtual +0x14 |

Unverified forwarded-method signatures and names for slots 4–6 are **not** implemented or guessed.

## Six original method bodies independently SHA-256 verified

| Role | Original VA | Exact bytes | Full SHA256 |
|---|---|---:|---|
| Normal constructor | 0x222C6600 | 104 | 0eed18121db5af0bdb4d71133be223da9a4af07c1191216d1ac34967c45c2449 |
| Conjugate constructor | 0x222C66A0 | 46 | 1ee9d538fe20c62e12d711747c23e2ce070dd8e2aef451f310fde9b80ce01a73 |
| Cache getter | 0x222C6670 | 4 | 933a6c85af8213a87a347402fdaa62905c7b53039e4c483855521c3da6fd9509 |
| Normalized slot2 | 0x222C6370 | 34 | 1124fe36fec84be51a5879424064d5ee1330ab355932395d24d3f639566ba11d |
| Normal refresh slot3 | 0x222C63A0 | 76 | 34d384c82ff5819c9ef5d021e4e6fd31b575a53ca619e88c511e6fba0d608b6e |
| Conjugate refresh slot3 | 0x222C6420 | 94 | 367cfdf63e773c461edc8a3e87b6bc9390bf03d935f6d32bcf0a80bcfb169c54 |

The ordinary constructor calls the underlying stat's virtual slot2, multiplies by gain, clamps to [0,1] for ordered IEEE754 values, and caches scale times the normalized value. The conjugate constructor first calls the ordinary constructor, switches to its own vtable, and recomputes the cached value using **scale times (1 minus the normalized value)**. This causes two source reads during conjugate construction, consistent with the verified x86 calls.

The original status-flag branches do not coerce NaNs into zero; unordered values pass through. Both slot3 refresh implementations ignore the float argument from the Phase 149 advisor fanout, re-reading the underlying statistic instead.

The shared slot2 method stores the **unmodified input float** at +0x0C and passes input / (gain * scale) to the underlying stat virtual slot3. The original routine has no saturation or zero-denominator guard. The clean-room model preserves those arithmetic operations without inventing gameplay policy.

## Original call graph witnesses

Constructor calls verified from the original server include normal at 0x222BC163, 0x222BC602, 0x222BC6B6, 0x222BC7AE, 0x222BC951; conjugate at 0x222BC1C3. The derived constructor calls the normal constructor at 0x222C66B4 and its own refresh at 0x222C66C3. This ties the concrete classes to the original adaptive-difficulty advisor creation code.

Original Phase 149 server callback 0x222C6750 calls listener slot3, then aggregator 0x222C6830 reads cached value through slot1. The score is eventually delivered to client.dll by the previously verified DDAdvisorScores message. Class and function names beyond these RTTI labels are not guessed.

## New source and reproducibility

- include/sin_compat/original_short_term_stat_gates.hpp: clean C++17 representation of both gate classes using a host-neutral source-stat interface and Phase149's actual advisor-listener interface.
- tests/test_original_short_term_stat_gates.cpp: constructor update order, clamp, NaN/infinity, slot2 normalization, conjugate behavior, and Phase149 advisor fanout/gate integration.
- research/tools/audit_sin_original_short_term_stat_gates.py: owner-only dual-DLL SHA chain, six full function SHA checks, 47 instruction anchors, 8 original CALL edges, both exact 7-slot vtables and RTTI descriptors.
- research/tests/test_sin_original_short_term_stat_gates.py: synthetic original-free corruption checks.
- CMakeLists.txt: independent CTest target.

Owner-only static audit command:

    python3 research/tools/audit_sin_original_short_term_stat_gates.py --client /private/client.dll --server /private/server.dll --output /private/phase150.json

No original C++ source syntax, binary-compatible engine ABI, exact bitwise x87 parity or gameplay execution is claimed. Next investigate the concrete source-stat classes feeding these gates and the related long-term gate variants.
