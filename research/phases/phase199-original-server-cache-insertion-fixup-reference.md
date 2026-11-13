# Phase 199: candidate complete cache-tree insertion fixup, cross-phase invariant testing

13 November 2026. New DRAFT stacked on Phase198 PR #251. **This is a behavior/integration phase: zero newly verified original native functions and zero new native bytes.** The reviewed atlas remains **195 unique original functions (103 server, 92 client)**.

## Why a provisional model is justified but not yet byte-for-byte reconstruction

Previous original DLL SHA-pinned evidence establishes the relevant native code and data-field boundaries:

- Phase191 lookup: scene `0x221A7FB0`, model-sounds `0x22142670`, 24-byte indexed nodes and `0xFFFF` missing index.
- Phase192 comparator: scene `0x221A2F40`, model-sounds `0x2213A3F0`, first-DWORD **unsigned** ordering.
- Phase193 insertion wrapper and Phase196 link wrapper: new record key copying, 16-bit slot count, root/parent-child insertion, then balance callback with full 32-bit raw index.
- Phase197 whole original **1,387-byte** scene balancing function `0x221A8F20` and twin model-sounds `0x221429D0`: parent/uncle colors, recoloring, root-blackening, and direct calls into the two rotation directions verified at the machine-code level.
- Phase198 four complete SHA-verified native left/right tree rotations with precise 16-bit parent/child/root mutations, including original scene `0x221A6BB0/0x221A6CA0` and model-sounds `0x221409B0/0x22140AA0`.

The current implementation **does not claim that every instruction and branch of those two 1,387-byte original balancers is understood**. In particular, exact condition evaluation order, indirect node resolver side effects, number of resolver calls, and the native tree sentinel's lazy-initialization timing may differ. A conventional red-black insertion algorithm is consistent with the previously verified structural evidence, but that consistency is **not proof of native behavioral equivalence**.

## New safe reference, without changing the native function atlas

`include/sin_compat/original_server_cache_insert_fixup.hpp` adds a C++17 **candidate** red-parent insertion model using the original node layout:

1. Root is a 16-bit index at tree `+0x10`; node parent is a WORD at `+4`; color is a WORD at `+6`; `0` means red, `1` means black.
2. If the newly linked node is root or has a black parent, only enforce a black root.
3. If the parent is red and uncle is red, recolor parent and uncle black, recolor grandparent red, and continue upward.
4. If the uncle is black/missing, handle left/right **inner triangle** via one preparatory rotation, then recolor the parent and grandparent and perform the **outer rotation**. Both directions use the recovered Phase198 rotation pointer updates.
5. Always enforce black root at completion. Truncate raw 32-bit index to 16 bits only for slot indexing. The original balancer entrypoint addresses and both rotation addresses remain explicit per repository configuration.

To keep these assumptions from contaminating memory-unsafe native execution, the host reference **validates reachable topology, checks parent links and colors, bounds all indexes, limits loop work by the node count, and performs the operation on a copy**. It commits changes only after successful completion, so malformed synthetic trees cannot suffer partial mutation. Those guards and transactional writes are intentionally **not** claims about the retail 2006 x86 function.

**Do not make this candidate the engine's replacement native balancer until the actual original red-parent paths are disassembled and compared against cases, ideally with an owner-only Windows trace.**

## Test coverage actually executed locally

`tests/test_original_server_cache_insert_fixup.cpp` integrates the new candidate with the recovered Phase196 `link` wrapper and Phase198 `rotate`. Under both scene and model-sounds configurations:

- LL, RR, LR, and RL cases, with separately asserted direction-specific rotation counts.
- Red-uncle recoloring case, including root-blackening.
- Keys crossing unsigned bit 31 and the legal `0` key.
- **Every one of the 720 permutations of six distinct keys per repository**, checking after **each insertion**: correct unsigned BST key ranges, parent/child consistency, root black, no red-red edges, matching black height along all root-leaf paths, and preservation of key, payload and opaque node data.
- **12 deterministic 64-key shuffled insertion sequences per repository**, with invariants checked after every insertion.
- Malformed root, orphan inserted slot, and missing root fail closed without modifying native-layout synthetic fields.

Local optimized **GCC and Clang C++17** builds using `-O2 -Wall -Wextra -Wpedantic -Werror -UNDEBUG` completed successfully in a minimal matching-header harness. CI runs the identical source against the repository's actual recovered headers and the native Win32 MSVC x86 build. Check those results before asserting that the integrated GitHub branch passes.

The tests are **synthetic**, not a native original DLL dynamic trace. No copyrighted original DLLs, raw x86 function bodies or proprietary game assets are committed.

## Next milestones

1. Audit the original 1,387-byte balancing functions' entire control-flow graphs and color/uncle/rotation branching in both original `server.dll` twins. Add exact machine-code/edge witnesses for these branch paths rather than extrapolating from standard textbook red-black trees.
2. Instrument owner-only original Windows retail engine startup and original cache insertion in a reversible test installation. Check real `ServerGameDLL003` / `VClient011` startup before replacing game modules.
3. Connect verified game-specific cache record keys, filesystem virtual `+0xD4`, and record payload handling. Do not treat synthetic lookups or Bionic `dlopen` smoke as gameplay.

Deployment order remains **Windows PC**, then **Android**, then **Quest 3**. This phase improves an independently tested portable behavioral reference, **not game playability** and **not the verified-native-function count**.
