# Phase 149: original CStatAdvisor sample-to-decision pipeline

**9 November 2026.** Read-only direct disassembly of the owner-held original SiN Episodes: Emergence Windows 2006 server.dll, with the previously recovered original client.dll advisor-message contract from Phases 76 and 148.

Exact full original SHA-256: server = 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650; client = 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d. The original binaries remain private and are not uploaded to GitHub.

## Verified original function regions

| VA | Code bytes | Full SHA256 | Function behavior |
|---|---:|---|---|
| 0x222C4DC0 | 66 | 1ae1d1ce3a05553c94fa47aad02c2d8dd4bb0c1082b43d0bdef9f89ec50a4372 | Sample callback, gate, listener fanout, average, classification |
| 0x222C6750 | 42 | 7ccb2679f144b8b485ce2638b524e208aef6ef93c7b35615d935b7eb8071a1ee | Notify listener vtable slot3 with raw input |
| 0x222C6830 | 72 | c89963bb26fa2e9f3a4ca307d25ee916fe9f0750ed132b620948b9165ea72497 | Average floats from listener slot1 |
| 0x222C4C80 | 126 | 0b40f3c8e6aeb436b3dd8cc1bebbe4aebb92f50484ab956a73b3496d38f20c46 | Select original virtual classifier slot9/10/11 |

The original pointer array is at this+0x08 with signed count at +0x14. Verified primary CStatAdvisor vtable entry 0x2245CBD8 points to 0x222C6830. Direct CALL edges at 0x222C4DE0 -> 0x222C6750 and 0x222C4DF1 -> 0x222C4C80 were validated against original instruction bytes.

## Original ordering and gate

A sample is rejected (returns false) if stored float gate +0x34 is strictly greater than the caller float. Otherwise the callback stores gate = caller + member +0x38, notifies **all** registered listeners with the raw sample using their virtual slot3, reads **all** listener floating values using virtual slot1, averages the values through the original CStatAdvisor virtual slot1, writes score at +0x1C, invokes classification, and returns true. The callback return is distinct from its classifier virtual return.

For a signed count of zero, original x87 mean computes 0/0, producing NaN if invalid-operation exceptions are masked; the portable reference explicitly uses quiet NaN. This is not a real-engine test.

## Classifier branch semantics

The dispatcher at 0x222C4C80 reads the mode byte at +0x3C. When zero, it selects virtual slot11 (code 2). Otherwise it compares the current score (+0x1C) against channel1 (+0x28) and channel2 (+0x2C), using x87 FCOMP/FNSTSW/TEST AH status flags. The following rules match the original status-flag branches for tested finite values, boundaries, infinities and quiet NaNs:

- If channel1 > channel2: score above channel1 selects slot10; score below channel2 selects slot9; otherwise slot11.
- Otherwise: score below channel1 selects slot10; score above channel2 selects slot9; otherwise slot11.
- Slot10 receives fabs(score-channel1), slot9 receives fabs(channel2-score), slot11 receives the original score.

Phase148 proved slot9 writes code 3/4, slot10 writes code 0/1, and slot11 writes code 2 to advisor member +0x20. Phase76 proved DDAdvisorScores sends this member as signed16 to client.dll receiver 0x241FAA60. These are verified control-flow and wire relations; their gameplay-facing semantic names are not yet proven.

## Authored reconstruction and verification

- include/sin_compat/original_stat_advisor_sample_runtime.hpp: portable C++17 model with listener callbacks, classifier selection and gate.
- tests/test_original_stat_advisor_sample_runtime.cpp: CTest route matrices, notification ordering, mean, failure and NaN cases.
- research/tools/audit_sin_original_stat_advisor_sample_pipeline.py: chains earlier SHA-pinned *both-DLL* verification; checks 4 full original code hashes, 54 opcode anchors, 4 vtable pointers and 2 direct calls.
- research/tests/test_sin_original_stat_advisor_sample_pipeline.py: original-free synthetic mutation tests and independent x87 flag emulator.
- Root CMakeLists.txt: added CTest target for the new C++ test.

Local original-only full region checks and local C++ test under GCC C++17, -Werror, ASan and UBSan passed. External GitHub CI separately checks this component against synthetic fixtures; original commercial DLL bytes are never in the repo.

Remaining boundaries: no original function names, whole x87 bit-level numeric parity, native game entity lifecycle, source-defined event names, gameplay execution, or downloadable full SiN game rebuild established.

Next: find actual registered listener classes and trace their slot1/slot3 implementations; identify writes to the +0x3C mode flag and gameplay callers of 0x222C4DC0.
