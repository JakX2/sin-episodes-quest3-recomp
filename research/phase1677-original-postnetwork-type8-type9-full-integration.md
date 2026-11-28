# Phase 1677: native type-8/type-9 PostNetwork reconciliation integration

## Original provenance and scope

This phase **does not claim newly recovered retail x86 method bytes**. It integrates previously SHA-authenticated original type-8 and type-9 call sequences into one original-free MSVC x86 runtime test. Original 2006 PE32 client.dll complete image SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. Original dispatcher `0x241996F0`, 1036 bytes and exact 27-to-13 type table already authenticated in Phase1672. Previous Phase217 `research/tools/audit_sin_original_client_binary_integer_specialists.py` and Phase212 `research/tools/audit_sin_original_client_snapshot_record_router.py` authenticate comparison, diagnostics, and preview calls from the parent original retail typed router.

Original type-9 binary branch calls the shared source compare `0x24197A90` at `0x2419992E`, applies **inline** conditional byte correction, calls binary diagnostics `0x24197AD0` at `0x24199964`, and calls binary preview `0x24198360` at `0x24199973`. Its signed count from the record is multiplied by **four** for both byte-compare/correction and preview. Type-8 integer branch calls shared compare `0x24197A90` at `0x24199A2E`, applies its own inline conditional byte correction, calls integer diagnostics `0x24197B60` at `0x24199A78`, and calls the same binary preview `0x24198360` at `0x24199A87`. Type 8 uses the original signed record count as a **byte count** and its integer diagnostics interpret the **first** post-correction bytes as signed 8-bit values. The conditional correction source regions and source this+0x2C gate are authenticated in Phase1674, and complete byte comparator 56B/RET12 in Phase1673. Phase1675 pinned the 30B binary preview gate with authentic native four-DWORD RET16 and `this+0x34 == this+0x18`.

## New integration

`pc/independent_slot0/native_original_postnetwork_case8_case9_bytecmp.h` now derives its safe comparator fixture from the existing nonvirtual Phase1675 scalar-format fixture instead of directly deriving from Phase1672's typed dispatcher. No new virtual pointer is introduced. This preserves the source-pinned original layout `this+0x14`, `+0x18`, `+0x2C`, `+0x34`, and makes **the same native MSVC x86 object** handle compare RET12, conditional correction, and binary preview RET16 after Phase1672 native dispatcher RET16.

`pc/independent_slot0/test_original_postnetwork_case8_case9_integrated.cpp` uses two synthetic records and invented, separately owned 40-byte buffers to verify:

- Type-8 **N=5** and type-9 **4×N=12** compare and correction lengths, with source-right-to-destination-left writes.
- Native x86 ESP preservation around `CompareBytes`, `FormatBinaryIfSelected`, and `Dispatch`.
- Comparison result 0 surviving the correction, with integer diagnostics reading the first post-copy signed byte (`-1`), and type9 binary status reporting 12 bytes.
- Comparison enabled but copy disabled: original mismatch still reported and integer difference uses uncorrected `-128` vs `-1`, giving `-127`.
- Source record +0x13 bit `0x04` forced comparison equality blocking correction; source +0x13 bit `0x08` using an **injected synthetic symbol host**, never claiming to reproduce original engine symbol registry or `Q_FileBase`.
- Comparison/diagnostic enabled independently from correction; disabled comparison produces status zero but still permits original `this+0x2C` copy, and selected-record preview can still run.
- Outer dispatcher flag `0x0200` prefilter suppressing case execution even if the skipped record was selected for preview.
- Original zero-count native comparator returning equality and selected preview receiving count zero.
- Negative signed record count producing native 32-bit wrapped byte-count `0xFFFFFFFC` for type9, host-authorized refusal of unsafe compare/copy, and selected preview forwarding signed `-4`. Unlike original retail unguarded memory access, this is a **safe test-only divergence**.

The native comparator grants in Phase1673 are general and only enforce their 4096-byte synthetic ceiling. To prevent a valid ceiling but out-of-range invented buffer access, the new integration refuses per-record counts above its owned 40 bytes before using the comparator, while still testing the established >4096 safe refusal. This is not a new game behavior claim.

**Scope:** the diagnostic helpers use Phase217's portable C++ reference model. The real original engine logger, symbol registry, and actual engine client-module initialization remain opaque. Public CI never loads, executes, or redistributes retail client/server DLLs. No physics, surfaces, props, models, materials, or rendering are implemented.

## Verification

Stacked on fully Windows-verified Phase1676 **299/299** native MSVC x86 tests (five workflows green, exact commit `37bee90b44b9f97c8a0e8c1de1885b0bf8d61b65`). This integration adds **one** original-free native CTest: target **300**, to be claimed only after the final exact-head Windows run actually passes. Retail Windows executable gameplay startup and Android/Quest 3 gameplay remain unverified.
