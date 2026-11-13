# Phase 197: original server cache-tree insertion balancing, evidence and safe-prefix reconstruction

13 November 2026. **Draft stacked on Phase196 PR #249**, with original-binary research sourced from the owner's privately held, SHA256-matched *SiN Episodes: Emergence* (2006) 32-bit Windows DLLs. The original DLLs, extracted commercial assets, and complete native disassemblies are not committed.

## Two complete native code regions identified and authenticated

| Original server.dll function | Native entry | Complete byte count | Complete original SHA256 |
| --- | --- | ---: | --- |
| Scene-cache balancing/fixup | `0x221A8F20` | 1,387 | `d283bb9cfb88ea76182c67edf5e9a4e747e3f1005f7bd6f03af9a0d9b7707132` |
| Model-sounds balancing/fixup | `0x221429D0` | 1,387 | `3398245617c372d63f037cb7496f1308e298867ab501c982a7103b63741101aa` |

Original `server.dll`: SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`; original `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Both 1,387-byte functions end in an x86 `ret 4`, followed by five `int3` alignment bytes. **Both complete bodies are byte-for-byte identical after normalizing precisely 85 four-byte references to each function's distinct sentinel state and eight repository-specific CALL rel32 operands per method**. The full normalized SHA256 is `2883dbef8df10e4d3cdf27e81d0b513c2b4c5d71467fc24618c880a0bce2c683`. This covers **2,774 additional complete original x86 bytes** and **two distinct original native entrypoints**. The original audit pins all original full hashes, 170 absolute address operands and 16 outgoing plus six incoming direct calls.

## Concrete flow facts from the original machine code

1. Input comes through `ECX` (tree object) and one 32-bit stack index argument, with the new index inspected through its low 16 bits.
2. The first comparison checks the low-16 inserted index against the tree's root WORD at object `+0x10`. If equal, it skips rotations and ultimately writes color WORD **1** (black) into the root node at slot offset `+0x06`.
3. On a non-root insertion, the routine resolves the new node's parent WORD at slot `+0x04` (or the module-local static sentinel for `0xFFFF`). It checks the parent's color WORD at node `+0x06`; when nonzero, the native code goes directly to its common root-blackening exit.
4. Otherwise, the routine follows repeated parent/grandparent/uncle resolution, tests parent and uncle color flags, conditionally recolors nodes and invokes one of **two repository-specific rotation-like helpers**. The body is consistent with red-black tree insertion fixup, but the full algorithm and exact rotation helper effects are **not yet decompiled**.
5. The final loop exit again writes WORD **1** to root node `+0x06`. The native code has **no equivalent of the host-only safe vector index validation**.

Each method uses its own 8-byte static sentinel: scene `0x2256E1F0` with initialization flag at `0x2256E1F8`, model-sounds `0x22558D50` and `0x22558D58`. These are in original virtual `.data` and need not have raw file-backed values.

### Actual original helper call targets

| Role (native callsites in each 1,387-byte routine) | Scene original VA | Model-sounds original VA |
| --- | --- | --- |
| Rotation-like helper A at `+0x34C` and `+0x541` | `0x221A6BB0` | `0x221409B0` |
| Rotation-like helper B at `+0x38A` and `+0x503` | `0x221A6CA0` | `0x22140AA0` |
| Existing Phase195 node resolver at `+0x354`, `+0x360`, `+0x50B`, `+0x517` | `0x221A4D90` | `0x2213E990` |

Phase196 link-wrapper callsites at `0x221AA8D7`, `0x221AA8E6`, `0x221AA8F4` target the scene function; `0x221441F7`, `0x22144206`, `0x22144214` target the model-sounds function. All 22 total CALL edges are verified as actual original `CALL rel32` instructions. The labels A/B are intentional: **do not infer left/right semantics before auditing both helper bodies**.

## Implemented conservative host behavior

`include/sin_compat/original_server_cache_balance_preflight.hpp` provides a *narrow* C++17 behavioral reference for only two x86-proven fast paths: inserted index is root, or inserted node's parent is already non-red. It models parent and color as the LOW and HIGH 16-bit halves of Phase191's placeholder `unknown_04` node DWORD, blackens the root on the accepted paths, and **returns `requires_unimplemented_rotation_or_recolor` for red-parent input with no mutation**. Invalid vector indices return a distinct host-only invalid outcome. Do not use this method to claim a fully balanced insertion or a playable game.

`tests/test_original_server_cache_balance_preflight.cpp` exercises both original configurations through Phase196's link callback, checks root color and left/right child insertion, demonstrates the red-parent fail-closed boundary, checks unchanged data on refusal, and verifies low-16 index truncation.

`research/tools/audit_sin_original_server_cache_balancers.py` authenticates original whole-file and full-function SHA256s, 30 sparse opcode witnesses, **85 distinct absolute operands per method**, eight direct internal calls per method, six Phase196 incoming CALLs, exact return/boundary and completely normalized function identity. `research/tests/test_sin_original_server_cache_balancers.py` supplies original-free synthetic mutation groups, rejecting corrupt witnesses, original globals, calls, boundaries, wrong sections, synthetic hashes and unwitnessed differences.

Root CTest and MSVC Win32 x86 CTest targets are registered. Native original research used the owner-owned `bin.zip`; GitHub CI uses synthetic evidence only and must be checked on this branch.

## Delivery status and next target

Reviewed native methods **189 -> 191 (server 97 -> 99; client stays 92)**. This is research coverage and **not** completion percentage. The next decompilation dependency is the two pairs of rotation-like helper functions at the VAs above, followed by a fully verified behavioral fixup and end-to-end synthetic insert/find validation. An **owner-only retail Windows startup trace** using the already implemented proxy remains a separate high-priority integration gate. Neither real original-engine gameplay, original Windows replacement modules, Android gameplay, nor Meta Quest 3 gameplay has been demonstrated.

The dependency strategy remains **Windows PC first, Android second, Quest 3 third**, with SourceVR/screng compatibility evaluated separately from SiN-specific game-code reconstruction.
