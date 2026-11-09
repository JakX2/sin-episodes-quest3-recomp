# Phase 148: original CStatAdvisor evaluation methods (server.dll ↔ client.dll)

**9 November 2026.** Direct owner-held original SiN Episodes: Emergence (2006) Windows x86 static analysis. This builds on the previously archived Phase 76 DDAdvisorScores / DDAdvisorChannels and Phase 81 CStatAdvisor state-flow work, rather than duplicating their already-reconstructed wire protocols.

## Evidence and provenance

The original DLLs were independently verified before disassembly:
- server.dll: SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client.dll: SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The validated MSVC RTTI name is `CStatAdvisor` (name at `0x22521924`, RTTI complete-object locator at `0x2245CBD0`). The original primary vtable at `0x2245CBD4` points to three previously unresolved evaluator functions:

| Vtable slot | Pointer-cell VA | Function VA | Full-code SHA-256 |
| --- | --- | --- | --- |
| 9 | `0x2245CBF8` | `0x222C4D00` | `ba4446916b2baada2f5229979341353f023ebaa439771cd95f00718d6dda4aa6` |
| 10 | `0x2245CBFC` | `0x222C4D40` | `26fd8d85153ce0f15d8f330a47c6e578def1ca78f5523e50e1dd480eee14417d` |
| 11 | `0x2245CC00` | `0x222C4D80` | `f81dfaa3b110b49a0b389c8a6ac3e33e6c57a7740464ed32763d6062e9c66bd1` |

The same audit pins adjacent nonvirtual `0x222C4D90` (SHA-256 `87727a576cb0f316ce7ce278056bb560756a83d6b36b4703fadca42a34986250`) and `0x222C4DC0` (`1ae1d1ce3a05553c94fa47aad02c2d8dd4bb0c1082b43d0bdef9f89ec50a4372`) as exact original machine-code regions. The latter's callback logic is not yet fully reconstructed.

## Recovered original behavior

All three virtuals write an integer decision to advisor `this+0x20`. Prior Phase 76 proves the original server `DDAdvisorScores` writer reads this same member at `0x222BA70E` as a signed 16-bit wire field; the original client reader `0x241FAA60` stores the received code in 32-bit slots. The source is therefore now connected to the existing verified server-to-client contract.

For finite values, the neutral C++ equivalences are:

```cpp
// Slot 9, original VA 0x222C4D00
if (channel2 < channel1) code = score < channel3 ? 4 : 3;
else                     code = score > channel3 ? 4 : 3;
return true;

// Slot 10, original VA 0x222C4D40
if (channel1 > channel2) code = score > channel0 ? 0 : 1;
else                     code = score < channel0 ? 0 : 1;
return true;

// Slot 11, original VA 0x222C4D80
code = 2;
return false;
```

Values read: score float from `this+0x1C`; channel floats from `+0x24,+0x28,+0x2C,+0x30`. The exact original method names and the gameplay meaning of codes 0–4 are **not** established.

The functions use x87 FCOMP/FNSTSW and conditional tests of status-byte AH bits C0/C2/C3 (`0x05` and `0x41`). For unordered float comparisons, the branch equivalence is preserved for quiet NaNs: in the relevant unordered score comparisons slot 9 chooses 3 and slot 10 chooses 1, not a guessed exception path. Original return values and stack cleanup were separately inspected.

The independent `0x222C4D90` helper compares a float at `this+0x34` against a caller float. If the stored value is strictly greater, it returns false without a change. Otherwise it sets `this+0x34 = argument + this+0x38` and returns true, including unordered comparisons. The meaning of the gate and step remains unverified.

At `0x222C4DC0`, original code checks the gate, calls original `0x222C6750`, invokes virtual slot +4, stores returned float at `this+0x1C`, and calls `0x222C4C80`. The callback's wider semantics remain unresolved and are **not** implemented by this phase.

## Implementation and reproducibility

- `include/sin_compat/original_stat_advisor_evaluators.hpp`: independent portable C++17 behavioral reconstruction for three verified virtual evaluator functions and separate gate helper; no guessed original object layout or Source engine ABI.
- `tests/test_original_stat_advisor_evaluators.cpp`: finite threshold matrices, equality, NaN, gate behavior and connection to existing Phase 76 typed client/server advisor wire.
- `research/tools/audit_sin_original_stat_advisor_evaluators.py`: chains earlier SHA-pinned **both-DLL** audits and verifies RTTI, vtable addresses, original opcode/call witnesses and five whole function SHA-256s.
- `research/tests/test_sin_original_stat_advisor_evaluators.py`: original-free synthetic x86 tamper detection and an independent x87 flag-emulation truth table, including infinity/NaN.
- Root CMake/CTest includes the new C++ regression.

Original files never enter GitHub or CI. The test harness and synthetic fixtures do not prove game execution. This is a narrow, evidence-backed **server implementation recovery with client message integration**, not full DLL source recovery or playable PC/Android/Quest modules.

Owner-only verifier:
```sh
python3 research/tools/audit_sin_original_stat_advisor_evaluators.py \
  --server /private/server.dll --client /private/client.dll \
  --output /private/stat-advisor-report.json
```
Original-free tests:
```sh
python3 -m unittest discover -s research/tests -p 'test_sin_original_stat_advisor_evaluators.py' -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target sin_original_stat_advisor_evaluators_tests
ctest --test-dir build -R '^sin_original_stat_advisor_evaluators_tests$' --output-on-failure
```

**Next high-value decomp**: server `0x222C4C80` selection and `0x222C4DC0` callback schedule, plus original `C_DynamicDifficulty` client presentation consumers. Continue using Phase 76/80/81 implementations rather than duplicating them.
