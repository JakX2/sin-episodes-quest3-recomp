# Phase 646: Original 2006 SiN symbol comparator and table initialization

## Objective and provenance

Continue recovering *SiN Episodes: Emergence* original-specific entity/NPC symbol registration, rather than revisiting physics, materials/surface properties, models, props or rendering already working in Half-Life 2.

The **previously opaque function pointer at [table+0] is now exactly resolved**. Both original retail table-construction paths hardcode original function VA **0x22280B50**, and the original Phase643 enumerator indirectly calls this exact pointer. Research was performed on the owner's unchanged genuine 2006 PE32 DLLs:

- server.dll SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client.dll SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

**251 complete original executable x86 bytes** authenticated by method SHA256, original .text ownership, and complete-method ending INT3 padding:

| Original method | Exact length | Original complete-method SHA256 |
|---|---:|---|
| `0x22280B50` 16-bit symbol comparator | **19B** | `b0521eacdb9687665838d9a4df7374f3b2665f842939aea4d2d4c550dc97d413` |
| `0x22282130` table direct constructor | **63B** | `93ae1a19f1ec9f63c8a36dd04b08c748d163715e5ab71639a7669c6108fa2d6f` |
| `0x22282210` table heap factory | **90B** | `75ce836b6fffec4fd84da9ccd39ff82187bc3a767bade60c10fd610d2efe22aa` |
| `0x2226E790` factory's original allocation wrapper | **79B** | `1ba57fe89ebae2110c154315e1371c8481cc0bd1cf8363a4c83689219ff9714f` |

The original-only audit verifies **five native rel32 call edges**, **four native short branches**, **two exact comparator function-pointer immediate assignments**, and **93 address-pinned original machine-instruction witnesses** in addition to whole method hashes. Only derived C++/test/audit source is uploaded; original game binaries remain owner-private.

## Comparator semantics directly from x86

Full 19-byte method `0x22280B50`:

```asm
mov eax, [esp+4]      ; pointer to new key record
mov cx, word [eax]
mov edx, [esp+8]      ; pointer to existing key record
cmp cx, word [edx]
sbb eax, eax
neg eax
ret
```

This is a **cdecl** callback accepting two **DWORD guest pointers** and returning **EAX = 1** exactly when `uint16(new_record[0]) < uint16(existing_record[0])`, otherwise **EAX = 0**. The native compare is **unsigned**, not signed: `0x7FFF < 0x8000` is true, and equality returns zero. Phase643 enumeration chooses the **left child** if comparator `AL` is nonzero, the **right child** otherwise. Duplicate values therefore also descend right, subject to the remaining tree-registration semantics.

## Table constructor and factory semantics

Original `0x22282130` initializes an existing 0x50-byte table object at `ECX`:

1. **DWORD [table+0] = 0x22280B50** (original native comparator).
2. **DWORD [table+4/+8/+0xC] = 0** for array pointer, capacity, and growth-step policy.
3. **WORD [table+0x10] = 0xFFFF** root; **WORD [table+0x14] = 0xFFFF** free-list head.
4. Read **DWORD [table+4]** again, store to **DWORD [table+0x18]** cached array pointer.
5. **WORD [table+0x12] = 0**, **WORD [table+0x16] = 0** for active slot count and next unused slot.
6. Call original **`0x2228F360`** with `ECX=table+0x1C` and the three pushed arguments resulting in **(0,0,1)** at the callee.
7. Return the original `table` pointer in EAX.

Original `0x22282210` is a factory with `ECX` pointing to an owner DWORD pointer field:

- Calls original `0x2226E790(0x50)` to allocate 80 bytes.
- If returned address is nonnull, inlines **the exact same table field initialization**, including the comparator pointer, and calls `0x2228F360`. **After** nested initialization, stores new table pointer into owner `[ECX]`.
- If returned address is null, **skips the initialization and stores zero** into owner instead. Returns the owner pointer.
- The 79B original `0x2226E790` wrapper has the same memory-manager virtual dispatch shape as Phase645 allocate, using the same original static tag/flag globals; the compiled test's factory allocation callback routes through the **compiled Phase645 source reference**. The underlying engine memory-manager virtual methods remain host-supplied.

### Scope of reconstruction

New `CompareKey` and `InitializeTable` C++17 references retain the original key width and unsigned comparison, exact original comparator pointer installation, width-sensitive field initialization and sequence, and optional factory allocation plus null-return path.

**Original nested map initializer `0x2228F360` is NOT yet recovered.** Its invocation and exact arguments are verified, but its internal string table and hash setup remain a required typed host callback. Defensive callback failure handling, guest-address overflow checks and reentrancy refusal are portable safety measures, not native original x86 failure semantics.

## Verification and full cross-phase composition

- The owner-only Python auditor succeeds against the exact two unmodified original retail DLLs, authenticating 251 native bytes, method boundaries, calls/branches/pointer stores/witnesses.
- The public `--self-test` checks only invented bytes and mutates every byte of each invented whole method, rel32 callsites, branch opcodes, exact original constructor pointer assignments and comparator instructions.
- Standalone C++17 `-Wall -Wextra -Wpedantic -Werror` synthetic tests pass. ASan+UBSan also pass locally.
- Tests exercise unsigned 16-bit comparisons across zero, equality, FFFF and 0x7FFF/0x8000 boundary; direct constructor field order and exact 16-bit sentinels; factory allocation success, null result, nested callback failure, committed write refusal, allocator failure and reentrancy.
- The extended actual compiled **Phase638 → Phase639 → Phase643 → Phase640 → Phase642 → Phase641 → Phase644 → Phase645 → Phase646** synthetic registration chain now starts by **running the original Phase646 direct table constructor** and routes **all Phase643 comparator callbacks through the compiled original Phase646 comparator**, replacing the former invented comparison. It inserts 39 invented symbol keys with six capacities (2,4,8,16,32,64) and five guest allocation relocations, validates mapping and red-black invariants after each insertion, then releases storage through the reconstructed original free wrapper. The original `0x2228F7A0` lookup and `0x2228F360` nested init remain separate, explicit callbacks, not invented algorithmic implementations.
- The standalone *local link-only* placeholder for the unrelated Phase637 `RegisterRange::add` method was solely needed because the local Phase638 source distribution lacked Phase637 implementation. GitHub CMake links the **actual existing compiled Phase637 library**, not this local placeholder. No placeholder is uploaded.

The synthetic integration tests and Android Bionic/PE32 factory checks **do not constitute** execution of a playable original retail SiN DLL, actual in-level NPC behavior, or Quest 3 gameplay.

### Owner-only original audit

`python research/tools/audit_original_symbol_comparator_phase646.py --server ORIGINAL_SERVER_DLL --client ORIGINAL_CLIENT_DLL`

### Public CI original-free mutation test

`python research/tools/audit_original_symbol_comparator_phase646.py --self-test`

## Next original SiN-specific gaps

1. **`0x2228F360`**, the nested table/subobject initializer (owner +0x1C) called by both original constructors.
2. **`0x2228F7A0`**, original tree symbol lookup used by the Phase638 forwarding path, with dependencies `0x2228EB40` and map/hash operations.
3. Underlying memory-manager vtable implementations and actual in-process Windows retail-game / Android/VR engine integration.

Maintain strict priority on missing SiN gameplay/AI logic rather than redoing already-working HL2 physics/material/prop systems.
