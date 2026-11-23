# Phase638: authenticated condition registration and symbol forwarding

Original owner-supplied retail DLLs match the Phase637 SHA256s:

- server.dll: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
- client.dll: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Two **complete original x86 methods totaling 108 bytes**:

| Address | Original purpose | Length | Whole-method SHA256 |
| --- | --- | ---: | --- |
| 0x2205A760 | owner+0x34 condition LOCALID registration wrapper | 31 | f4834824b68fcbd0c4b0ca540b8b4b354349a3d00404d952b76d66ed47dd3b28 |
| 0x22282370 | original symbol/index slot forwarding | 77 | 1ee09397a760fe3547a48936da90af271eada973a8d2d9d97fa29b89b24769be |

## Important correction: third node is "condition"

Retail 0x2205A76D pushes category address 0x223E05C8, whose exact NUL-terminated value is **condition**. Original 0x2205A774 adds **0x34** to the owner in ECX, then 0x2205A777 calls the original Phase637 registrar 0x22073760. Wrapper returns RET 0x0C. The previously known categories are +0x04 schedule and +0x1C task. Therefore the 3rd map's label should be condition/flag, not an unverified separate animation-event registration category. This wrapper is generic; a specific Elite condition caller and real runtime IDs are not newly claimed.

## Source-ordered original forwarding behavior

Native 0x22282370 uses __thiscall: ECX=context, stack args (symbol argument, mapped index), RET 8 with no success code.

1. Read [context] as underlying table.
2. Call 0x2228F7A0 with ECX=table+0x1C to locate or insert a symbol; the original passes the source symbol pointer and address of its first stack-argument word. Read a 16-bit tree token from the returned pointer.
3. Call 0x22282270 with ECX=table, a pointer to token scratch, and constant zero. AX is the 16-bit slot handle. The allocator can mutate the table.
4. **After allocation**, load [table+4] again for the slot-array pointer.
5. Zero extend AX, shift left 4, then store mapped index DWORD at **slot_array + 16*slot_handle + 0x0C**.

The tree lookup and allocation helpers are **not** reconstructed here; both remain explicit, mandatory, host-owned callbacks. The host model refuses invalid accesses and exceptions rather than emulating native faults, with no rollback of completed host mutations. The original has no checked return from these helpers.

The upstream native call 0x22073822 -> 0x22282370 is already present in Phase637. The compiled Phase638 regression composes Phase637 range registration with the Phase638 writer using **fabricated IDs and callbacks**. This is not an original game process test.

## Owner audit

Run: python research/tools/audit_original_symbol_forward_phase638.py --server PATH_TO_SERVER_DLL --client PATH_TO_CLIENT_DLL

The owner audit demands both whole DLL identities, full two-method hashes, four original rel32 edges, **25 opcode witnesses**, and original "condition" .rdata literal. Public --self-test uses **invented** bytes.

Still needed: full 0x2228F7A0 tree and 0x22282270 slot allocator semantics, original runtime table contents, and actual original game DLL interface execution. No playable Windows/Quest or in-level gameplay is implied; unrelated HL2 props/models/materials/physics/rendering are excluded.
