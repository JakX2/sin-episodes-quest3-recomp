# Gameplay Phase611: original Assault shutdown and conditional relationship unlink

## Original source and the missing NPC lifecycle connection

This phase directly reads the user's original 2006 SiN Episodes: Emergence retail server.dll/client.dll, SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 / 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d, from the connected Drive SE1/bin.zip. The original commercial binaries and complete disassembly are not committed.

Phase604 recovered the original behavior current-pointer transition system, which calls old behavior virtual byte offset **+0x20** on deactivation. Phase611 identifies this actual slot for original RTTI-backed **CAI_AssaultBehavior** vtable 0x223F5544: **+0x20 -> original 0x220CEBF0**. This method begins exactly one INT3 after the 255-byte Assault eligibility method already recovered in Phase607. It calls genuine original helper **0x220CE4A0** to release a linked-entity owner relationship; the helper is not a generic Source engine model loader or surface/physics method.

## Two newly authenticated complete native functions, 220 bytes

| Original function | Native entry | Real method length | Whole original SHA256 |
| --- | --- | ---: | --- |
| Assault +0x20 deactivation callback | 0x220CEBF0 | **105** | 4c1d6122cb572112702dd69d8775b32d6535e272d024578c0376f4e0225c57cf |
| Linked target/owner relationship conditional release | 0x220CE4A0 | **115** | afebea980deb52065acaf4d1c29e667257f02b4f4307dafa2e1f2ccaa3841082 |

Original method boundaries are authenticated by entire SHA256 digests and genuine following **7** and **13** INT3 bytes; separate native methods follow at 0x220CEC60 and 0x220CE520. Original deactivation calls helper 0x220CE4A0 at **0x220CEC48** and **0x220CEC52**, both native E8 rel32 calls proved by real code bytes. The SHA-locked verifier checks **30 additional original opcode/field witnesses**. All have been independently verified locally on the exact retail PE32.

## Actual original behavior semantics

### Assault vtable+0x20: 0x220CEBF0

1. Unconditionally write zero to Assault behavior bytes **+0x24 and +0x25**.
2. Read encoded target handle at behavior+0x18 and check invalid sentinel 0xFFFFFFFF. If invalid, return.
3. Validate low12-bit entity-list index and upper serial against original entity table global 0x224F5620; if mismatch or null entity, return.
4. **Read behavior+0x18 a second time** and revalidate serial. Pass the resolved target pointer (as implicit this) and behavior owner pointer at +0x04 to helper 0x220CE4A0.
5. If the second handle became invalid/stale after the first passed, original x86 can call the helper with a null pointer. That helper dereferences target+0x3BC immediately, making this an original potential crash path. No engine lifetime guarantees are inferred.

This callback is directly relevant to deactivating Assault when Phase606 selects another registered behavior. It is not an Assault eligibility method despite being located immediately after one.

### Linked-target release helper: 0x220CE4A0

1. Read target entity's **encoded linked-owner handle at +0x3BC**.
2. If the link is invalid, stale or resolves to no entity, write **0xFFFFFFFF** back to target+0x3BC and return true.
3. If it resolves successfully, **read target+0x3BC again** and repeat the original serial resolution, obtaining a linked entity pointer (or null on failure).
4. Compare that pointer to the supplied original owner pointer. **Only when equal** does it write invalid sentinel to target+0x3BC and return true. If the link points to a different valid owner, return false with link unchanged.
5. The deactivation caller does not use the helper's returned boolean. The purpose is restoring relationship consistency, but the historical original field/symbol name is not recovered.

This distinction prevents a subtle port bug: **unconditionally clearing target+0x3BC on Assault shutdown would break a relationship to another owner**.

## Portable implementation, deliberate safe deviations

- include/sin_compat/original_grunt_assault_deactivate_phase611.hpp: C++17 original-free Host interface that maps encoded handles to opaque entity tokens, repeats both target and linked-owner lookups, clears behavior+0x24/+0x25, and only unlinks missing/stale/same-owner relations.
- tests/test_original_grunt_assault_deactivate_phase611.cpp: tests invalid first target, stale first/second target, stale/invalid linked owner, matching owner unlink, other owner link preservation, changed linked handle, changed target between reads, short-circuit and unconditional two-flag clearing.
- research/tools/audit_sin_original_assault_deactivate_unlink_phase611.py: owner-original SHA pair, Assault original vtable+0x20, **two complete original whole method hashes**, precise 7/13-INT3 boundaries, next-method starts, two direct helper E8 calls and 30 source-specific instruction witnesses. Synthetic mutation self-test refuses ten corrupted five-byte E8 encodings.
- CMake registers one C++ host and one Python native provenance regression CTest.

For portability, entity objects remain **engine-owned** and only opaque tokens cross the host. The native potential null call on a failed second serial resolution is instead **safely refused**. The source helper's second link resolution with null owner is not claimed equivalent under every possible input; the independent port explicitly requires a nonnull owning NPC token to unlink on owner match. Original concurrency/lifetime contract remains unknown.

Local standalone C++17 regression passed with -Wall -Wextra -Wpedantic -Werror. The original 2006 retail source check of both method complete hashes, 30 short native witnesses and two E8 calls passed locally. The public CI itself does not receive commercial retail DLLs.

## Milestone

**Phase611 adds 2 complete original native function addresses and 220 original machine-code bytes.** Cumulative Phases601–611: **40 distinct complete original method addresses, 11,224 authenticated original machine-code bytes**. Source-grounded host models do not yet constitute an engine-loaded Windows server.dll or working Android/Quest 3 port. Standard HL2-equivalent surfaces, model rendering, prop loading and physics are still excluded.
