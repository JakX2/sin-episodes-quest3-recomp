# Gameplay Phase610: original Assault x87 time state and marker +0x26

## Genuine native source and scope

The owner's original retail SiN Episodes: Emergence 2006 server.dll and client.dll, from connected Drive SE1/bin.zip, have SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 and 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d respectively.

The Assault eligibility virtual +0x18 is original 0x220CEAF0 (exact 255 bytes, full SHA256 0c5b688341149ae7ed28a49f9f930c87116810c67f1107a444e48742ba570466). **Phase607 ALREADY counted these 255 bytes.** Phase610 recovers the previously unresolved *ordered finite-time control flow* of this same function and adds a portable, original-free C++ host model. This phase counts **zero newly recovered complete native methods** rather than double-counting the same original code.

## Original 2006 control flow (finite, stable inputs)

1. Call owner vfunc +0x450; false returns ineligible. Query condition raw ID 0x3B through native 0x220397C0; present returns ineligible.
2. Original x87 compares float at behavior+0x2C with current engine time. When finite and ordered, +0x2C > now returns false; equality passes. Requires raw DWORD behavior+0x1C nonzero.
3. Check two original serial-validated target handles at behavior+0x14 then +0x18 through Phase609's missing-target predicates. If either is invalid/stale/null, clear behavior raw DWORD+0x1C, BYTE+0x24, BYTE+0x25 and return false. Earlier failures do not clear them.
4. Call owner vfunc +0x154. If it returns nonzero, write now to behavior float+0x28.
5. Re-resolve the original first entity handle at behavior+0x14 **TWICE** after the pair guards. If first lookup returns null, return (BYTE+0x26 == 0). The second lookup reads the entity BYTE+0x3B9; if zero, return (BYTE+0x26 == 0), without clearing that marker. A stale second pointer can be unsafe in the original.
6. If entity BYTE+0x3B9 is nonzero, FUCOMPP compares behavior float+0x28 against original constant 0.0f at 0x223D2590. **Only if +0x28 equals exactly zero** does the next x87 comparison of (now - +0x28) against original 4.0f constant at 0x223DDCF4 occur. Finite elapsed **>=4.0f**, including equality, writes BYTE+0x26=1 and returns false.
7. Otherwise the original checks BYTE+0x26. When nonzero it calls native 0x22039760 for raw condition ID 0x18. Then it clears BYTE+0x26 and returns true. When marker already zero it simply clears and returns true.

**Important:** Valid targets alone do not mean Assault is eligible, and the 4-second threshold is strict. The entity-byte-zero path preserves +0x26, unlike the normal final success/reset path. Historical symbolic names for +0x26 and +0x3B9 are not proven.

## Native provenance checks

Research tool: research/tools/audit_sin_original_assault_timer_state_phase610.py

Checks exact retail SHA pair; original Assault vtable+0x18 and 255-byte full function SHA; single INT3 byte at exclusive end 0x220CEBEF and distinct native method at 0x220CEBF0; original .rdata floats 0.0 and 4.0; **33 independently checked native opcode/field witnesses**; and **6 exact original E8 callsite targets**:

- 0x220CEB07 -> 0x220397C0, condition 0x3B;
- 0x220CEB3A -> 0x220CE240, first target missing check;
- 0x220CEB4A -> 0x220CE350, second target missing check;
- 0x220CEB73 -> 0x220CE210, first handle resolve;
- 0x220CEB7E -> 0x220CE2F0, second resolve of first handle;
- 0x220CEBCE -> 0x22039760, condition 0x18.

Native original audit of all 33 witnesses and 6 call targets **passed locally**, as did full body SHA confirmation. Only addresses, metadata and a few instruction witnesses are recorded, no commercial executable bytes or bulk disassembly.

## Independently authored portable host code

- include/sin_compat/original_grunt_assault_eligibility_phase610.hpp: accepts a stable finite game-time tick; composes the Phase609 dual-target handle guard; maintains raw+0x1C/+0x24/+0x25, float+0x28/+0x2C and byte+0x26; explicitly calls original owner gates, repeated handle resolution and condition 0x18 through host callbacks.
- tests/test_original_grunt_assault_eligibility_phase610.cpp: owner and condition rejection, inclusive deadline equality, no state on early gates, two missing-target resets, stale/missing repeated handle, target byte+0x3B9 bypass, strict 3.999 vs 4.000 deadline, marker recovery and exact condition 0x18 signaling, and NaN refusal.
- New native source verifier with original-free finite-state self-test and synthetic callsite mutation checks. CMake adds Python and C++ CTests.

**Explicit deviations:** Native x87 NaN/unordered behavior is not replicated. The model refuses nonfinite inputs. It receives a stable now tick even though the native code reloads global time. Stale second target resolution fails closed, not with a native null dereference. Engine-owned entity and owner virtual semantics remain external callbacks. This is a finite/stable-input parity subset, not a playable native game-DLL replacement.

The standalone host code compiled locally using GCC C++17 with -Wall -Wextra -Wpedantic -Werror, and its regression passed.

Run:
    python research/tools/audit_sin_original_assault_timer_state_phase610.py --self-test
    python research/tools/audit_sin_original_assault_timer_state_phase610.py --bin-zip /private/SE1/bin.zip

**Cumulative Phases601-610: 38 separately authenticated complete original methods, 11,004 native code bytes.** Count unchanged this phase. No generic HL2 surfaces/models/props/physics work, Windows engine boot or Android/Quest integration asserted.
