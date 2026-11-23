# Gameplay Phase636: complete original Elite mapping node initialization and three-node owner registration

## Retail binary basis

Original **SiN Episodes: Emergence** 2006 PE32 x86 `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Companion `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`. All original method hashes, exact rel32 edges, original instructions and section boundaries below were verified directly against these owner-supplied original binaries. No retail executable bytes are copied into the source repo.

The prior Phase635 authenticated the complete `0x22073700` range-ID converter and Elite vtable `+0x760` getter returning mapping owner `0x2259C2F8`; the first embedded mapping node is `owner+0x34=0x2259C32C`. This phase reconstructs original *initialization and registration* methods, not the still-unknown later runtime range population.

| Complete original executable method | Size | Whole source SHA256 |
| --- | ---: | --- |
| `0x22073610` 24-byte node constructor | **58 bytes** | `9c393a56c72a4cb224bebc3f12e5e0ffa40c37e249234c6a1a7a473288c2f4bc` |
| `0x22073650` conditional node linker | **55 bytes** | `8a0c782739b72f07fa9c673161442cc592b5561e12fda400cbcc40a5e6786d4a` |
| `0x22088F10` three-node owner registrar | **119 bytes** | `fee95bd5819a59e5d5641dca6fa2e55daed2e3503a23e01bc142c2c9f253e9c3` |

**Net distinct complete original executable code:** **232 bytes, three methods**. The corresponding new portable source is a host-owned reference, not proprietary decompiled game code or a real Source game engine ABI.

## Complete source-backed behavior

### `0x22073610`: initialize six DWORDs from a BYTE

The original `__thiscall` takes an `ECX` mapping-node pointer and reads the low byte of `[ESP+4]`. It does a `NEG/SBB` flag conversion before writing the same six DWORD fields **in the order below**:

| Node field | Input byte = 0 | Input byte nonzero |
| --- | ---: | ---: |
| `+0x00` | `0xFFFFFFFF` | `0` |
| `+0x04` | `9999` | `0` |
| `+0x08` | `0xFFFFFFFF` | `0xFFFFFFFF` |
| `+0x0C` | `0xFFFFFFFF` | `0xFFFFFFFF` |
| `+0x10` | `0` | `0` |
| `+0x14` | `0` | `0` |

A zero-mode node initially has original `source_start=9999`, the **explicit skip sentinel** identified in Phase635; this is *not* a registered range. Exact original startup code separately calls this constructor with `push 0` at `0x223C6A57`, `0x223C6A63`, `0x223C6A6F` for the three embedded nodes at `0x2259C2FC`, `0x2259C314`, `0x2259C32C` and at `0x223C6A87` for the neighboring node `0x2259C344`.

### `0x22073650`: original node link and copy

The original receives `ECX=node`, `[ESP+4]=source-record pointer`, `[ESP+8]=successor pointer`.

1. Read node DWORD `+0x0C`. When **not `0xFFFFFFFF`**, set `node+4` to **9999** if successor is nonnull, otherwise **0**, then set `node+8` and `node+0xC` to `0xFFFFFFFF`. When `node+0x0C == -1`, preserve all these fields.
2. Write `node+0x10=successor`, then `node+0x14=source-record pointer`.
3. **Only after both pointer writes**, read DWORD `[source-record+4]` and write the result to `node+0`.
4. Return original boolean **1** (`ret 8`) in all normal, valid-memory paths.

There is **no original null check** for the source pointer. If source is null, the original has already written successor/source fields before it would attempt `[source+4]`; the reconstructed reference reports an original null-pointer hazard without dereferencing it, and keeps the earlier changes. It deliberately retains original partial-write behavior on host failure, with no fabricated rollback.

### `0x22088F10`: register an owner with three embedded nodes

Original `__thiscall` receives `ECX=owner` and exactly **three stack arguments**: `name/classname`, `source-record base`, `linked owner or null`. Native code writes `owner+0=name` **first**, then invokes `0x22073650` three times in order:

| Callsite | Receiver node | Source record pointer | Successor pointer |
| --- | --- | --- | --- |
| `0x22088F33` | `owner+0x04` | `source+0x00` | `linked_owner+0x04` or null |
| `0x22088F4F` | `owner+0x1C` | `source+0x08` | `linked_owner+0x1C` or null |
| `0x22088F6B` | `owner+0x34` | `source+0x10` | `linked_owner+0x34` or null |

After each call, it checks the returned `AL` and stops on zero; after all three it returns **1**, otherwise **0**. The current recovered link helper returns 1 on valid guest memory. The portable host can reject invalid memory or callbacks, reports the precise failed stage and retains source-ordered partial effects.

**Concrete original Elite registration evidence:** at the original callsite `0x22355AD9`, the preceding native `PUSH` instructions appear in C-style reverse argument order:

- `0x22355AC5`: `push 0x2259C658` (third, linked owner, uninitialized .data).
- `0x22355ACA`: `push 0x22547624` (second, source-record base, uninitialized .data).
- `0x22355ACF`: `push 0x2248DA68` (first, exact ASCII classname `CNPC_EliteGrunt` in .rdata).
- `0x22355AD4`: `mov ECX,0x2259C2F8` (this = the original Elite mapping owner).
- `0x22355AD9`: `call 0x22088F10` (this complete registered method).

The original `.data` starts at RVA `0x4D6000`, has **`VirtualSize=0xCAB24`**, and only **`SizeOfRawData=0x68000`**. All of `0x2259C2F8`, `0x2259C32C`, `0x22547624`, and `0x2259C658` lie in the *virtual uninitialized tail*. Their actual range records, despite being referenced by fixed original instructions, are **not physically stored in the DLL**. Using the flat file offset `VA-base` to read data beyond the end of the `.data` raw section would accidentally read `.reloc` bytes and fabricate records.

## Native reconstruction and test proof

- `include/sin_compat/original_elite_mapping_init_phase636.hpp` and `src/original_elite_mapping_init_phase636.cpp` implement distinct typed references for the complete constructor, conditional linker and three-stage owner registrar. The host owns all checked 32-bit guest-address reads and writes; no real original addresses are dereferenced by the C++ reference. Reentrancy, host errors and invalid pointers are reported separately from authentic normal branches.
- `tests/test_original_elite_mapping_init_phase636.cpp` checks both constructor modes and byte truncation, six source-ordered writes, original +C sentinel branches, successor null/non-null behavior, original node+0x10 and +0x14 pointer write order, source-record+4 copying, partial effects on failed source reads, partial owner-registration refusal, linked and unlinked owner cases, overflows and callback reentrancy.
- That same compiled C++ test composes the real Phase636 constructor and registrar with the **actual reconstructed Phase635 source-ID linked-range converter**. Immediately after the original constructor/registration, the source-low remains the original sentinel **9999**, so the converter cannot resolve an event ID, even though the node links and destination metadata exist. For a separate clearly **invented test-only later range population**, the test host fills source low/high and proves that Phase635 then resolves an ID. There is **no claim** that this invented write is the missing original range-registration function.
- `research/tools/audit_original_elite_mapping_init_phase636.py` checks both exact retail original DLL SHA256s, **three complete method SHA256s**, the **eight** direct original rel32 calls (four constructors, three link calls, one owner registration), 47 isolated source/caller instruction witnesses, exact original Elite classname, and all four important guest addresses inside the real .data BSS region. Public CI uses only synthesized SHA/call/opcode mutation cases, never the copyrighted game DLL.
- Root and native MSVC Win32 x86 CMake targets compile/test the same portable host references.

## Remaining original-specific work

**Registration and range population are separate operations.** Original constructor + registrar **do not** establish actual event-to-flag ID ranges. The exact runtime records need either the original functions that populate those source ranges or an owner-run authenticated, read-only runtime snapshot. The known next reverse-engineering targets are the surrounding original range operations at `0x22073690` and `0x22073760`, and the initializer arguments whose BSS contents are set during startup.

Public CI and the new C++ host model do **not** mean original game code has run inside the reconstructed DLL. There is no observed replacement-engine callback in a retail Windows game and no in-level Android or Quest 3 gameplay claim. No unrelated HL2 model/prop, physics, surface, render or material code was touched.
