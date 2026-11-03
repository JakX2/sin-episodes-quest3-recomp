# SiN 2006 dynamic-difficulty typed-call contract: first implementation bridge

**Status:** clean-room host-agnostic reference **implementation**, NOT recovered Ritual C++, byte-compatible Source network transport, native Android/SourceVR gameplay integration, or difficulty formulas.

**Evidence basis:** [original binary-verified protocol investigation in PR #72](https://github.com/JakX2/sin-episodes-quest3-recomp/pull/72). The original 2006 client.dll and server.dll remain privately held and SHA-256-pinned. This commit does **not** perform a new original-binary inspection.

## What this adds

- `include/sin_compat/difficulty_message_contract.hpp` models the exact nine original cross-DLL message registrations and their declared lengths, including `-1` as variable length. A flag means **callback observed in the inspected `C_DynamicDifficulty` client initialization block**, not a claim about all possible game handlers.
- A `TypedReader`/`TypedWriter` interface models only the **order of typed original read/write calls**. It assumes no Source-specific packet bit sizes, channel framing, byte order, endian conversion, engine interfaces, or scheduler.
- A normal `DDDecisionMade` record is **f32, i32, i32, i32, f32, f32, f32, f32**. The first float equals `0.0f` or `-1.0f` on two observed client special paths. The adapter **classifies those prefixes and stops**, deliberately leaving their remaining branch and payload behavior unknown. It does not invent sentinel gameplay meanings or create a fabricated complete packet.
- `DDSuccessModifiers` is ten ordered pairs of f32 values, without inventing advisor/index names or difficulty formulas. Two selectors 1 and 3 were present in original callsites, but their meaning is not established.
- C++17 test fixtures check the exact ordinary-record typed operation sequence, both sentinel prefixes, 20-float pair order, mutated types, truncation/error propagation, writer failure, and no output mutation on partial reads.

## Build the independent regression

Run with CMake and CTest on desktop or compile `tests/test_difficulty_message_contract.cpp` with a C++17 compiler and include directory `include/`. This is suitable for cross-compilation, but **does not link an ARM64 Source engine DLL**. Tests use synthetic values only. No proprietary original executable bytes are committed.

## Evidence still needed before actual game-network implementation

1. Disassemble complete server sentinel message paths and both client sentinel branch tails; resolve the meaning, branch lengths and handling of `0.0f` and `-1.0f`.
2. Determine actual Source engine read/write packing and bit widths, packet boundaries, and message end handling for both the normal and special paths.
3. Decode the remaining dynamic-difficulty messages and class state mutations (especially `DDInfo`, advisor channels and decisions/performance).
4. Integrate with a verified SourceVR ARM64 client/server loader and gameplay hooks; compare actual 2006 game behavior before treating it as compatible.

No new whole-binary recovery counts or playable Quest build are claimed by this adapter.
