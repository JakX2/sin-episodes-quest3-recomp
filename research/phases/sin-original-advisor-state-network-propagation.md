# Phase 81: original CStatAdvisor state, listener notifications and message bridge

## Original binary basis

The owner-held original 2006 SiN Episodes PE32 binaries passed their known SHA-256 checks:
- client.dll: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- server.dll: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

The Phase 81 auditor chains Phase 80's original dual-DLL verification and adds **75 exact instruction anchors, four relative CALLs, one relative JMP, five RTTI/vtable addresses, original 0.5f constant and eight whole code/data SHA-256 fingerprints**. No proprietary original game DLL bytes are present in the repository.

## Exact original class and initialization path

At server address `0x222B9BB0`, the difficulty manager calls advisor builder `0x222BC520` and stores **six resulting pointers** at manager offsets `+0x48,+0x4C,+0x50,+0x54,+0x58,+0x5C`. The builder allocates a 64-byte advisor instance and stores vtable `0x2245CBD4`.

The vtable's original MSVC RTTI complete-object-locator address at `0x2245CBD0` is `0x224CA970`. Its type descriptor pointer at `0x224CA97C` is `0x2252191C`, whose name at `0x22521924` is **`.?AVCStatAdvisor@@`**. This proves the original type name `CStatAdvisor`, rather than merely guessing from static strings.

Phase 80's preset dispatcher `0x222C0B90` calls `0x222B9450` (site `0x222C0BD9`) once for each of six advisors. Tracing the original stack PUSH instructions and call `0x222B9470 -> 0x222C4AC0` resolves the four output floats into ascending advisor member offsets:

| Phase 80 computed output | Original advisor member |
|---|---|
| output 0 | +0x24 |
| output 1 | +0x28 |
| output 2 | +0x2C |
| output 3 | +0x30 |

The compiled setter writes in reverse instruction order but these are the **resulting memory offsets**.

## Derived scalar and fanout

Original instructions `0x222B9475/0x222B9479` sum the two middle outputs, `0x222B9484` computes x87 `FABS`, `0x222B9486` multiplies by **0.5f** (verified at `0x223D9778`), then `0x222B948F` calls the advisor virtual slot `+8`.

For finite values the score is:

```cpp
score = 0.5f * fabs(output_1 + output_2);
```

The original `CStatAdvisor` vtable slot `+8` at `0x2245CBDC` points to `0x222C4AB0`, which stores the scalar into advisor `+0x1C` and jumps to `0x222C6720`. That routine forwards it to **each registered listener** via pointers at advisor `+0x08`, count `+0x14`, listener virtual slot `+8`. Another method `0x222C4A90` reloads the already stored `+0x1C` scalar and calls virtual slot `+8`.

A separate manager routine `0x222B9420` iterates all six advisors and calls `0x222C4AA0` to store an independent 32-bit scalar at advisor member `+0x34`. Its game meaning and invocation timing remain **unknown**, so the code uses the neutral name *auxiliary*.

## Bridge to independently recovered user messages

Phase 76 original server message writers read precisely these same values: `DDAdvisorScores` reads float `+0x1C` at `0x222BA703`, plus the separate signed-16 code from integer member `+0x20` at `0x222BA70E`; `DDAdvisorChannels` reads `+0x24/+0x28/+0x2C/+0x30` at `0x222BA7E5/F0/FB` and `0x222BA806`.

The proven initialization path is therefore:

`dd_challenge -> six original advisor presets -> four calculated float outputs -> CStatAdvisor fields -> 0.5*abs(middle-sum) + listener callbacks -> DDAdvisorScores/Channels wire fields`.

With default `dd_challenge=0.2f`, finite C++ reference results for the six initial float scores are approximately `[62, 31, 54, 20.5, 31, 0]`. These are **computed reference values**; the exact original x87 intermediate bit patterns and a live runtime capture have not been demonstrated.

## Implementation and verification

- `include/sin_compat/difficulty_advisor_state_flow.hpp`: host-neutral C++17 state fields, six derived advisor outputs, score calculation, listener notification adapter, independent auxiliary scalar broadcast and Phase 76 score/channel packet assembly. Signed-16 status codes are explicit inputs, never invented from presets.
- `research/tests/sin_difficulty_advisor_state_flow_reference.cpp`: verifies six configurations, absolute-value handling, listener call order, zero listeners, auxiliary broadcast, all four channel offsets, independent status codes and the Phase 76 client layout.
- `research/tools/audit_sin_advisor_state_flow.py`: exact original x86/RTTI/vtable proof and private DLL hash checks.
- `research/tests/test_sin_advisor_state_flow.py`: eight synthetic mutation suites covering each listed code anchor, call, JMP, class descriptor, constant and SHA guard.
- `CMakeLists.txt`: C++17 CTest target under `BUILD_TESTING`.

Local SHA-verified original DLL audit and eight Python mutation suites passed; C++17 `-Wall -Wextra -Wpedantic -Werror` and detached CTest passed. GitHub CI is checked independently.

## Remaining reverse-engineering and integration work

The original runtime evaluation methods around `0x222C4D00/0x222C4D40`, listener registration lifecycle, precise advisor semantics, difficulty decision/update schedule, x87-to-ARM64 bitwise parity, Source engine callback/ABI integration and working VR gameplay are **not yet recovered**. The current work is a reference contract and evidence-backed data path, **not a complete server.dll reconstruction or playable Quest 3 build**.
