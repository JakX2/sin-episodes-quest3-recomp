# SiN 2006 dynamic-difficulty typed-call contract: first implementation bridge

**Status:** clean-room host-agnostic reference **implementation**, NOT recovered Ritual C++, byte-compatible Source network transport, native Android/SourceVR gameplay integration, or difficulty formulas.

**Evidence basis:** [original binary-verified protocol investigation in PR #72](https://github.com/JakX2/sin-episodes-quest3-recomp/pull/72). The original 2006 client.dll and server.dll remain privately held and SHA-256-pinned. This commit does **not** perform a new original-binary inspection.

## What this adds

- `include/sin_compat/difficulty_message_contract.hpp` models the exact nine original cross-DLL message registrations and their declared lengths, including `-1` as variable length. A flag means **callback observed in the inspected `C_DynamicDifficulty` client initialization block**, not a claim about all possible game handlers.
- A `TypedReader`/`TypedWriter` interface models only the **order of typed original read/write calls**. It assumes no Source-specific packet bit sizes, channel framing, byte order, endian conversion, engine interfaces, or scheduler.
- A normal `DDDecisionMade` record is **f32, signed-16, signed-16, signed-16, f32, f32, f32, f32** (26 bytes of field data). The client branches on **thresholds** at 0.0f and -1.0f, not equality: nonnegative first values take the ordinary path, [-1,0) resets two client fields, and first values <-1 invoke a different list-clear operation. The adapter **classifies those prefixes and stops**, deliberately leaving their remaining branch and payload behavior unknown. It does not invent sentinel gameplay meanings or create a fabricated complete packet.
- `DDSuccessModifiers` is ten ordered pairs of f32 values, without inventing advisor/index names or difficulty formulas. Two selectors 1 and 3 were present in original callsites, but their meaning is not established.
- C++17 test fixtures check the exact ordinary-record typed operation sequence, both negative-threshold branches, 20-float pair order, mutated types, truncation/error propagation, writer failure, and no output mutation on partial reads.

## Build the independent regression

Run with CMake and CTest on desktop or compile `tests/test_difficulty_message_contract.cpp` with a C++17 compiler and include directory `include/`. This is suitable for cross-compilation, but **does not link an ARM64 Source engine DLL**. Tests use synthetic values only. No proprietary original executable bytes are committed.

## Evidence still needed before actual game-network implementation

1. Complete the original server control-message call graph and determine why the client branches when the first float is negative. The branch instructions and client-side effects are now verified; their gameplay meanings are not.
2. Determine actual Source engine read/write packing and bit widths, packet boundaries, and message end handling for both the normal and special paths.
3. Decode the remaining dynamic-difficulty messages and class state mutations (especially `DDInfo`, advisor channels and decisions/performance).
4. Integrate with a verified SourceVR ARM64 client/server loader and gameplay hooks; compare actual 2006 game behavior before treating it as compatible.

No new whole-binary recovery counts or playable Quest build are claimed by this adapter.

## 2026-11-03 correction from original binaries

Direct inspection of exact SHA-pinned DLLs resolves a PR #72 oversight: integer writer helper 0x221E87A0 calls 0x2225A160, which explicitly pushes bit width 0x10 before its signed-bit writer. Client reader 0x240A5600 likewise passes 0x10 to the signed-bit reader. The source struct has 32-bit member loads but the **wire transport is signed 16-bit**, so an ordinary message transmits 32 + 16 + 16 + 16 + 4*32 = 208 bits. The client x87 FCOMP / TEST AH,0x05 / JP branches check negative value ranges, not equality against sentinels. Earlier PR #72 type labels are corrected by this subsequent primary-evidence analysis.
