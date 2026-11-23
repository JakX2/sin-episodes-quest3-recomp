# Phase 643: Original SiN symbol-tree enumeration and caller argument mutation

## Source authentication

This phase continues the 2006 SiN Episodes: Emergence original gameplay DLL project, focused exclusively on original SiN-specific symbol/NPC state rather than already-working Half-Life 2 physics, props, models, surfaces, materials or rendering.

The source is the unchanged **owner-provided retail** binary:

- server.dll SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client.dll SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The exact native method is **0x22280FB0 through 0x222810AE inclusive, all 255 executable x86 bytes**, entire SHA256:

`3dcd2d8a66908958c0df0ccb5be7cdc3130a06b526ec4df58b9ee18f52e0560f`

Original `RET 0x10` at 0x222810AC; following byte 0x222810AF is INT3 padding. The existing Phase639 registrar has an authenticated rel32 call at **0x2228228B → 0x22280FB0**.

Owner-only auditor verifies both complete original DLL identities, the entire 255-byte method digest, **11 exact jump/conditional edges**, **51 original x86 opcode witnesses**, this source caller edge and end-of-method boundary. The public mutation self-test uses only **invented** bytes and does not ship original game binaries.

## Verified original ABI and control flow

The routine is a four-argument original `__thiscall`: ECX=table, then 16 bytes of stack args; only the first three are read by the method: (1) key/record pointer, (2) output WORD parent pointer, (3) output BYTE branch pointer. The fourth stack DWORD is unused by the observed code and nevertheless cleaned by `RET 16`.

The Phase639 caller `0x22282270` passes the **addresses of its own incoming stack argument words** as outputs, so source writes are partial-width, not whole-word replacement:

- At 0x22280FC9, set **WORD** parent output to FFFF.
- At 0x22280FCC, set **BYTE** side output to zero.
- If root WORD at table+0x10 equals FFFF, return immediately. No comparator call and **no sentinel initialization**.
- For each visited index, write WORD index into parent output at 0x22280FEB **before calling the comparator**.
- Load array pointer from table+4; form the existing slot's symbol record address **array+16*index+8**.
- Push existing record pointer and original input key pointer and invoke **indirect function pointer [table+0]** at **0x22280FFA** with cdecl stack cleanup (`ADD ESP,8`).
- Test raw returned AL at 0x22281000. Any nonzero AL writes **BYTE side=1** and selects the left child at slot+0. Zero writes **BYTE side=0** and selects right child at slot+2.
- After callback and side output, each branch checks sentinel-global **byte bit 0 at 0x22589140**. If clear, the original ORs bit 0 into the **full DWORD** at this address first, then writes three FFFF WORDs and one WORD=1 into absolute 0x22589138..0x2258913E. This can leave a partially initialized sentinel if a later operation faults.
- Re-read **[table+4] after the comparator** before reading the selected child. This matters if comparator side effects relocate the backing array.
- Iterate until the child index is FFFF, preserving the last parent and last direction output.

The original comparator's actual function-pointer target is **not yet recovered**. The portable C++ reference requires a typed host `compare_original_22280fb0` callback that supplies returned AL. Synthetic tests implement an invented token comparison solely to verify tree traversal and subsequent link/rebalance.

### Crucial Phase639 ABI correction

Before Phase643, Phase639's host API modeled the enumeration dependency as mutable 32-bit parent and side arguments, and the test mock assigned complete DWORDs. The real native method **only overwrites low 16 bits of the first caller word and low 8 bits of the second caller word**. Phase643 models exactly this behavior, leaving higher bits unchanged. The Phase640 linker uses parent.low16 and side.low8; full caller-word contents remain available for later debugging.

## Compiled integration and safety

The new C++17 `Enumerate` implementation preserves source ordering, callback return-byte semantics, the one-time sentinel side effects, and the fresh array reread. A portability-only 65536-iteration guard and checks for null/overflow, host failures, exceptions and callback reentrancy avoid reproducing crashes/hangs in corrupted synthetic guest-state fixtures. They are **not** original native error semantics.

Tests cover:
- Empty tree initialization without touching sentinel globals.
- Nonzero AL=0x80 selecting left, AL=0 selecting right; three-node descent with last-parent/side semantics.
- Preservation of **high 16 parent / high 24 side argument bits**.
- Comparator-triggered array relocation; cached pre-comparison record pointer and post-comparison child-pointer distinction.
- Original sentinel DWORD-bit preservation, first-time five writes, later no writes and partial committed writes on injected failure.
- Comparator refusal, exception, reentrancy, null table, null array, null comparator and address overflow.
- **Full source-composed synthetic chain**: compiled Phase638 Forwarder → Phase639 RegisterSlot/AcquireSlot → Phase643 Enumerate → Phase640 LinkNode → Phase642 Rebalance → Phase641 Rotate/Sentinel. Inserts a varied 18-symbol sequence, verifies root black, BST order, parent links, red rule, equal black-height, and mapped IDs after each insertion, then tests a three-symbol zigzag with both rotations.
- Local original retail auditor, portable C++17 and ASan/UBSan integration pass.

No original engine process, actual SiN NPC execution, Android/Quest 3 in-level gameplay or playable DLL replacement is claimed. The test comparator and symbol values are invented fixtures. No original proprietary DLL binaries are committed.

### Owner verification

`python research/tools/audit_original_symbol_enumeration_phase643.py --server ORIGINAL_SERVER_DLL --client ORIGINAL_CLIENT_DLL`

Public original-free mutation test: `python research/tools/audit_original_symbol_enumeration_phase643.py --self-test`.

## Next priority

**0x22280DD0 (110 complete original bytes)** is the slot growth/capacity owner, with full original SHA256 `0b4c3d93fa9aef438bff1693d141d16c30ed3301154e5e1cd7fc15ae6732e9f1`. Analyze its subordinate allocation/resize calls, then the **real comparator** target and actual original-engine registration invocation. Continue to exclude generic HL2 systems.
