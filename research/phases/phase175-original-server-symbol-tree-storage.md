# Phase 175: Native 2006 server symbol-tree storage and allocation paths

11 November 2026. Continues Phase174 draft PR #227 in the independent *SiN Episodes: Emergence* server/client DLL decompilation. These routines were disassembled from the **actual owner-held 2006 Windows PE32** `server.dll`. The original DLL bytes, original executable bodies, and assets are **not committed**.

## Original executable evidence

- Original `server.dll` SHA256: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Original `client.dll` SHA256: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

| Original server VA | Complete native body | Complete-region SHA256 | Verified role |
|---|---:|---|---|
| `0x2228E0E0` | 17 bytes | `9047b1c9eb80f184fa071a6108f8601051de53ea7ff23b549b7ea28a6233fc18` | 16-bit index to address of 12-byte record |
| `0x2228E450` | 24 bytes | `3a19f97ad1b4b9d5abf34e4bbd2a7fff9dd645dee2b2aa3519ac2cca35cded05` | 16-bit right-child pointer setter at record `+2` |
| `0x2228E620` | 86 bytes | `9b74307d379f272da366ba0b6d6ce0c8f486dbd23744783f101446acd14cb14b` | 16-bit free-list or fresh slot allocator |
| `0x2228E680` | 46 bytes | `4723bd92f6896d1f8887b2652295669e8280e5d4373b6cf2947b7947ed0927a4` | DWORD-sized reserve-and-append counter |
| `0x2228F460` | 109 bytes | `9da5ebfd31dde84f40dbb9c53852824e085b2b2ad5958e5b908d1dca83a03a8a` | 12-byte tree-node left/right/root wiring followed by balancing |

**Total: 282 complete native function bytes in five SHA256-pinned original x86 regions.** The owner-only auditor checks **56 deliberately sparse native opcode witnesses**, all **six native direct CALL rel32 instructions within the five original bodies**, and both original incoming calls at `0x2228F732 -> 0x2228E620` and `0x2228F746 -> 0x2228F460`. It chains the Phase174 auditor and full original dual-DLL fingerprints. Full original executable bodies are not copied into this repository.

## 12-byte original indexed record

Original `0x2228E0E0` zero-extends a WORD index, multiplies it by 12, adds the original backing pointer at object `+0x04`, and returns the **address**. It performs no bounds or sentinel check. Original `0x2228E450` takes a WORD index and WORD child identifier, computes the same stride, and writes the child identifier at record `+0x02`. This matches the native structure used by the record installation and symbol lookup phases.

## Slot allocation at `0x2228E620`

1. Read the 16-bit free-list head at object `+0x14`. Compare against `0xFFFF`.
2. **Fresh slot** when the head is invalid: zero-extend used-count WORD at `+0x16`, compare it with DWORD capacity at `+0x08`. When numerically equal, call original storage growth `0x2228E200` with integer 1 and `ECX=this+0x04`. Read current buffer pointer at `+0x04`, copy it to cached buffer `+0x18`, return the previous 16-bit used count, and increment it with WORD truncation.
3. **Recycled slot** when head valid: call original record-address accessor `0x2228E100` using the old WORD head. Read that old record's WORD at `+0x02` and put it in the new free-list head at `+0x14`. Copy current buffer pointer at `+0x04` to cache `+0x18`, return old head. Do **not** increment used count.
4. The native routine does **not** reset a recycled record's fields. Resetting happens later when linking it. No overflow prevention is inferred for 16-bit counter wrap.

## DWORD append/reserve at `0x2228E680`

This routine loads DWORD used count at `this+0x0C`, DWORD capacity at `this+0x04`, and requested DWORD argument. It computes `future = (used + requested) mod 2^32` and performs a **signed** x86 `JLE` comparison against capacity. If `future > capacity` signed, it calls `0x2228E280` with `(used - capacity + requested) mod 2^32`; otherwise it skips the call. In either case it adds requested to used and copies the latest buffer pointer at `this+0x00` to cache `this+0x10`. The C++ reference reproduces signed comparison and unsigned wrapping exactly without claiming original overflow behavior is safe.

## Symbol tree link at `0x2228F460`

Node record has WORD fields at `+0` (left), `+2` (right), `+4` (parent), `+6` (metadata, cleared) and DWORD data `+8` (not modified). The native function writes child-parent, clears left and right to `0xFFFF`, metadata to zero, and chooses one of three paths:

- Parent WORD is `0xFFFF`: store child in tree root at object `+0x10`.
- Parent valid and direction byte nonzero: write child to the parent's **left** WORD at `+0`.
- Parent valid and direction byte zero: write child to the parent's **right** WORD at `+2`.

All three branches then call original native `0x2228EDF0` to rebalance, after wiring the parent/child links, returning via `RET 0x0C`. The rebalancing algorithm and original entity object ABI remain unrecovered and host-owned. The model does not invent rotations.

## Source, tests, constraints

- `include/sin_compat/original_server_symbol_tree_storage.hpp`: host-neutral C++17 models of all five operations, with native layout and exact branch decisions. Engine allocation/balancing are external callbacks.
- `tests/test_original_server_symbol_tree_storage.cpp`: 12-byte stride, child writes, fresh/recycle slot, 16-bit wrap, left/right/root wiring, signed capacity edge cases, callback ordering and slot-to-node integration. Deliberately out-of-bounds host-index tests use a runtime index to avoid GCC `-Werror=array-bounds` false-positive about the deliberate exception test.
- `research/tools/audit_sin_original_server_symbol_tree_storage.py`: owner-only SHA checks, sparse opcode and original CALL rel32 targets, chained through Phase174.
- `research/tests/test_sin_original_server_symbol_tree_storage.py`: 9 original-free unittest cases, opcode/call mutation checks, 512-case independent signed-capacity matrix and branch reasoning.
- CMake test target `sin_original_server_symbol_tree_storage_tests`.

Local focused checks passed using the actual original binary and host GCC `-O2 -Wall -Wextra -Wpedantic -Werror`. The original-free 9-test suite passed independently. These are **not yet** a functional original Windows game DLL pair, nor an Android/Quest3 gameplay port. No original complete executable functions, resources or game files enter version control.

## Next native targets

Original balancing callback `0x2228EDF0`, native slot-record accessor `0x2228E100`, growth `0x2228E200`, capacity reserve `0x2228E280`, original factory class virtual callbacks and actual gameplay entity construction. Maintain **Windows game reconstruction first**, then Android/Quest3.