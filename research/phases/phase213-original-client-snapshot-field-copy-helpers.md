# Phase 213: original client prediction snapshot field-copy helpers

13 November 2026. **Stacked DRAFT on Phase212 PR #266**. This phase follows the original 2006 SiN Episodes: Emergence `client.dll` 27-type record router into the **first four concrete native helper implementations**, rather than counting its 36 outbound targets as if they were decompiled. All native evidence was inspected in the owner-provided original 32-bit game DLLs. Original binary files and complete proprietary native method bodies remain private.

## Four fully SHA-authenticated original native entrypoints

Both entire original game DLLs are SHA256-gated: server SHA `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` and client SHA `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. The client's original PE32 image base is `0x24000000`.

| Native 2006 client VA | Complete function length | Whole original native SHA256 | Phase212 type and direct caller |
| --- | ---: | --- | --- |
| `0x24198A10` | 34 bytes | `8c6cd7b6e20fc8e508282f71240c46268eda44d67f968d00b699b197caaeade8` | Type **5**, `CALL 0x241999CB` |
| `0x24198A40` | 46 bytes | `61e309166f7591611a783cd90c9c749f686d2c202ad3280fe87d821d37fb6f02` | Type **6**, `CALL 0x24199993` |
| `0x24198A70` | 34 bytes | `8c6cd7b6e20fc8e508282f71240c46268eda44d67f968d00b699b197caaeade8` | Type **1**, `CALL 0x2419985B` |
| `0x24198B40` | 61 bytes | `98a4f883c300f0aeec3998db6434c5b7b7068552a6957140e352d64262af2e67` | Type **2**, `CALL 0x2419988F` |

**Four new distinct native method entrypoints, 175 complete original x86 bytes.** The two 34-byte copy functions at `0x24198A10` and `0x24198A70` are **byte-for-byte identical across all 34 bytes**, but remain separately addressable methods reached from different Phase212 record cases. Both use original `ret 0x10`. All three four-argument helpers use `ret 0x10`; the original three-argument C-string helper uses `ret 0x0C`. Their exact original `INT3` padding lengths following the final RET are, in native address order, **14, 2, 14, 3 bytes**.

The audit checks all four direct original x86 `CALL rel32` destinations, the corresponding native code segments, exact full-body original method SHA256s, **35 sparse opcode witnesses**, all four original RET/INT3 boundaries, complete raw 34-byte duplicate identity, and the original 13-pointer jump table / 27-selector array from Phase212. The original-only audit **does not** claim the other 32 native specialist targets recovered.

## Original complete helper behavior

The four native functions share a simple initial condition. They read the byte at their `this+0x2C` and the **FIRST original stack DWORD**:

- If the byte equals **zero**, return immediately, without reading or modifying source/destination memory.
- If the first stack DWORD equals **1**, return immediately. Other values do not affect the copy.
- Otherwise perform the exact helper-specific forward copy. Original x86 does not check pointer validity, source terminators or available destination length.

The remaining native stack fields are interpreted without inventing original class/source member names:

| Type case | Remaining native stack DWORDs | Native behavior |
| --- | --- | --- |
| Type 1, `0x24198A70` | destination, source, **DWORD count** | `REP MOVSD`, copying exactly `4 × count` bytes |
| Type 5, `0x24198A10` | destination, source, **DWORD count** | Same complete native code as type 1 |
| Type 6, `0x24198A40` | destination, source, **BYTE count** | `REP MOVSD(count>>2)`, then `REP MOVSB(count&3)` |
| Type 2, `0x24198B40` | destination, NUL-terminated source | Scans source until the first NUL and copies **strlen(source)+1** bytes, including NUL, using `REP MOVSD` then `REP MOVSB` |

**Overlap semantics matter.** The original `REP MOVSD` transfers a complete four-byte word per instruction repetition. When source and destination overlap, it reads the complete source DWORD before writing that DWORD and continues forward (x86 direction flag expected clear). Naively replacing the original with `memcpy`, `memmove`, or a one-byte-at-a-time forward loop can change the results. The new C++17 reference copies four-byte snapshots per original DWORD and then handles the final 1–3 bytes one at a time. Overlapping-buffer regression tests distinguish these behaviors.

**Native safety differences:** The original has no allocation/protection/buffer-length checks. The host model checks source/destination bounds and refuses missing C-string terminators or overflow of a native 32-bit count-to-byte conversion. These are **host-only refusal conditions**, not claims of matching original x86 fault behavior. The source-level field names, object ownership and complete engine-side record case effects remain unknown.

Each of these functions is **one step within a larger Phase212 type case**. For example, the type 1 case additionally invokes original `0x24198AA0`, `0x24197CA0` and `0x24198470`; those bodies are not recovered in this phase. The type 2 case also invokes `0x24198B80` and other helper functions. Therefore the four host methods are not, by themselves, faithful implementations of an entire type case or entire SaveData serializer.

## Original-independent implementation and regression

- `include/sin_compat/original_client_snapshot_field_copy.hpp`: independently written C++17 original helper reference. Exact gating, original four stack argument positions for DWORD/BYTE copies, original three stack arguments for strings, complete-DWORD + trailing-byte forward-copy semantics, original method and caller VA metadata. Uses synthetic `std::vector<uint8_t>` buffers rather than native original game pointers.
- `tests/test_original_client_snapshot_field_copy.cpp`: exact four new native entrypoint/callsite assertions; type 1 and type 5 full behavioral parity on test inputs; DWORD versus byte counts and 1–3 trailing bytes; no-copy early returns for every relevant flag/argument combination; NUL-inclusive source strings; empty string; deliberate overlapping-copy patterns that distinguish `REP MOVSD` from naïve forward-byte copy; synthetic-only bounds/NUL/overflow refusal.
- `research/tools/audit_sin_original_client_snapshot_field_copy.py`: read-only complete original server/client SHA256 gates, all four original whole-function SHA256s, exact original `CALL rel32` edges, 35 short x86 witnesses, four function boundaries, explicit raw duplicate 34-byte equality, original Phase212 switch tables, native method PE section checks.
- `research/tests/test_sin_original_client_snapshot_field_copy.py`: 11 original-free mutation groups, including corruption of every one of 35 x86 opcode witnesses, all four exact original calls, four RET/INT3 boundaries, all 13 original jump-table destinations and all 27 type selectors. Synthetic bytes must never be counted as original SHA evidence.
- Portable root Linux CTest and actual 32-bit MSVC Windows x86 CTest registered. **CI on the exact Phase213 head is checked separately**.

## Status and next dependency

Unique reviewed original function catalog **239 → 243** (**128 server / 115 client**). These numbers count SHA-reviewed native functions, **not percentage of playable game**. The highest-value next source code is the three OTHER original type 1 and type 2 specialist helpers, and the original prediction record data layout. A real original Windows Source engine process executing reconstructed gameplay is **still not demonstrated**. The port order remains **Windows PC → Android ARM64 → Quest 3**.
