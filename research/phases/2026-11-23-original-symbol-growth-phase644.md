# Phase644: owner-authenticated original symbol slot growth and release

## Context and boundaries

Continuing the SiN Episodes: Emergence (2006) retail original gameplay DLL recovery. The owner has already confirmed that HL2-equivalent physics, materials, surfaces, model/prop loading and rendering are functioning in ported levels, so **none of those systems was worked on here**.

The exact unchanged owner-original DLLs used for this research have SHA256s:

- server.dll `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client.dll `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

Two complete source methods were authenticated, **149 original x86 executable bytes**:

| Function | Length | Whole native method SHA256 |
|---|---:|---|
| 0x22280DD0 original dynamic slot-array grow | 110 bytes | `0b4c3d93fa9aef438bff1693d141d16c30ed3301154e5e1cd7fc15ae6732e9f1` |
| 0x22280DA0 original array release/cleanup | 39 bytes | `7aa2cb04909c586cb3f63da1279b2fa7e924d84e81583b465f0339775879e2af` |

Both methods' exact original post-RET INT3 boundary bytes were confirmed. The owner-only audit verifies the **full SHA256 of both original methods**, 4 direct rel32 callsites, 10 control-flow branches and 55 address-specific x86 opcode witnesses; both retail whole DLLs' hashes must match. No original DLL or proprietary binary content is committed.

## Original growth owner structure

Native `0x222810FC` calls 0x22280DD0 with **ECX=table+4, push 1**. This means the owner points into the table:

- `[owner+0]` = `[table+4]` = current 32-bit slot-array pointer
- `[owner+4]` = `[table+8]` = capacity (32-bit slot count)
- `[owner+8]` = `[table+0xC]` = growth-step policy (32-bit)

At `0x22280DD6`, if growth-step is DWORD `0xFFFFFFFF`, original returns immediately without any allocator call or memory write.

Otherwise, the function computes `target = (old_capacity + requested_increment) mod 2^32` once. Native signed `JGE`/`JL` comparisons determine whether the capacity is insufficient, including cases with wrapped/negative 32-bit values.

The original loop reads the current capacity and:

1. If capacity is **zero**, writes capacity 2.
2. Otherwise, if growth-step is **zero**, doubles capacity (32-bit wrapping).
3. Otherwise, adds growth-step (32-bit wrapping).
4. After **each capacity write**, checks the newly stored capacity against the original target with **signed** comparison, repeating until sufficient.

**Subtle but essential:** even when the original comparison decides capacity is already sufficient, this function **still calls the allocation/reallocation helper** and overwrites the pointer. There is no early return except disabled growth. A request with increment 0 can therefore reallocate an unchanged-size array.

If `[owner+0]` is nonzero, it calls original **`0x2226E400`** with `old_pointer` and `capacity << 4`. If it is zero, it calls original **`0x2226E390`** with `capacity << 4`. Both byte-size calculations wrap at 32 bits (x86 SHL 4). The returned pointer from the original allocator is then written directly to `[owner]`, **even if NULL**; there is no native allocator-result check. Capacity writes happen **before** the allocator is called; rollback is not provided.

The allocator wrappers at 0x2226E390/0x2226E400 are **not yet reconstructed**. They branch through the engine's allocator interface and one-time global allocator tag setup. Portable C++ exposes them as required typed callbacks, not fabricated in-engine allocators or unconditional no-ops.

## Original release helper

At original `0x22280DA0`:
1. If owner growth-step `[owner+8]` is `0xFFFFFFFF`, return immediately.
2. If `[owner]` is nonzero, pass the pointer to original **`0x2226E3E0`** and subsequently clear `[owner]` to zero.
3. Regardless of whether the array pointer was null, clear capacity `[owner+4]` to zero.
4. The original leaves growth-step `[owner+8]` unchanged.

The original free wrapper `0x2226E3E0` remains host-owned. No Source engine memory manager has been invented.

## Portable model and tests

Added `GrowArray` and `ReleaseArray` C++17 source models through an `IHost` interface for guest DWORD reads/writes and allocator/free operations. C++ portability safeguards explicitly refuse null and overflowing guest addresses, host callback failures/exceptions, callback reentrancy and growth loops beyond 65,536 steps. These safeguards are **not native SiN error semantics**. The model preserves successfully committed earlier guest writes when a later host operation fails.

Compiled regression tests cover:

- Initial capacity 0 → 2 slots, the transition to 4, and `increment=0` same-size reallocations.
- A positive, configured growth-step of 3, starting at 2 and reaching 8 after two additive updates.
- `0xFFFFFFFF` disabled policy, including release skipping.
- Native signed 32-bit comparison under wrapping addition; byte-size SHL overflow; null allocator results stored.
- Release with null and non-null pointers; allocator/refusal exceptions and failures; memory writes that fail after earlier capacity writes; reentrancy; deliberately malformed cyclic capacity growth.
- Full **actual compiled Phase638 Forwarder → Phase639 RegisterSlot/AcquireSlot → Phase643 Enumerator → Phase640 LinkNode → Phase642 Rebalance → Phase641 Rotations/Sentinel**, with **Phase644 GrowArray replacing the former invented successful growth callback**, and final Phase644 ReleaseArray. The independent synthetic guest allocator copies old guest memory to a newly allocated guest VA on each reallocation, letting tests confirm all previous slot records and balanced search-tree links survive **five capacity-doubling reallocations**, reaching 64 slots with **39 synthetic symbols**. Root-black, red child rule, equal black heights, parent backlinks, BST order, symbol token and symbol mapping are checked after *every* inserted symbol.

The standalone C++17 compiled suite and ASan+UBSan suite pass. The Python auditor passes against the unchanged original DLLs. GitHub Actions tests are separately recorded on the PR after its exact commit completes.

### Owner-only audit

`python research/tools/audit_original_symbol_growth_phase644.py --server ORIGINAL_SERVER_DLL --client ORIGINAL_CLIENT_DLL`

The public CI-compatible `--self-test` uses invented original-free bytes only.

## Remaining original runtime gaps

The actual engine allocator wrappers `0x2226E390`, `0x2226E400`, `0x2226E3E0` and their memory-manager interface remain unreconstructed. Another important missing dependency is the original symbol-comparison callback resolved through `[table+0]`; the 2006 retail comparator address must be found from actual table construction/caller sites. The Phase638 original tree symbol lookup `0x2228F7A0` is still opaque. No in-process original SiN DLL replacement or playable Android/Quest 3 engine execution has been demonstrated.

The focus remains recovering original SiN-specific game behavior rather than reimplementing functioning HL2-equivalent systems.
